#include "filters.h"

#include "engine_errors.h"
#include "expression_evaluator.h"
#include "function_base.h"
#include "internal_value.h"
#include "loop_attr.h"
#include "markup.h"
#include "out_stream.h" // IWYU pragma: keep (GetStreamOnString returns an OutStream by value)
#include "render_context.h"
#include "string_builder.h"
#include "template_dict.h"
#include "testers.h"
#include "undefined.h"
#include "unicode_tables.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cwchar>
#include <iterator>
#include <limits>
#include <locale>
#include <numeric>
#include <memory>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{
inline namespace v2
{
namespace filters
{

// Which values Python can order against each other with `<`
enum class OrderKind
{
    Number,
    String,
    Sequence,
    Unordered
};

namespace
{

OrderKind GetOrderKind(const InternalValue& val)
{
    switch (val.Kind())
    {
    case ValueKind::Int:
    case ValueKind::Double:
    case ValueKind::Bool:
        return OrderKind::Number;
    case ValueKind::String:
        return OrderKind::String;
    case ValueKind::List:
    case ValueKind::KeyValuePair:
        return OrderKind::Sequence;
    default:
        return OrderKind::Unordered;
    }
}

// Python's `<` (or `>`) as min and max use it: values that have no order between them,
// like 1 and 'a' or two dicts, are an error. Undefined values still compare as false.
// `sort` does not use it: it compares its keys wrapped in lists, testing `==` first.
bool CompareForOrder(const InternalValue& left,
                     const InternalValue& right,
                     BinaryExpression::Operation oper,
                     BinaryExpression::CompareType compType)
{
    if (!IsEmpty(left) && !IsEmpty(right))
    {
        auto kind = GetOrderKind(left);
        if (kind != GetOrderKind(right) || kind == OrderKind::Unordered)
        {
            throw TemplateRuntimeError("'<' not supported between these values");
        }
    }
    return ConvertToBool(Apply2<visitors::BinaryMathOperation>(left, right, oper, compType));
}

// Jinja2's _prepare_attribute_parts: "a.b.0" is the path a, b, 0, a digit part an index
InternalValueList AttributePath(const InternalValue& attribute)
{
    auto str = GetAsSameString(std::string(), attribute);
    if (!str)
    {
        return { attribute };
    }

    InternalValueList parts;
    size_t start = 0;
    for (;;)
    {
        auto end = str->find('.', start);
        auto part = str->substr(start, end == std::string::npos ? std::string::npos : end - start);
        bool isIndex = !part.empty() && std::all_of(part.begin(), part.end(), [](char ch) { return ch >= '0' && ch <= '9'; });
        std::int64_t index = 0;
        if (isIndex)
        {
            if (std::from_chars(part.data(), part.data() + part.size(), index).ec == std::errc())
            {
                parts.emplace_back(index);
            }
            else
            {
                // An index past int64 is a float, as such an integer literal is (docs/tasks/0178):
                // no item of a sequence, and equal to no key a value of the engine can hold
                parts.emplace_back(std::strtod(part.c_str(), nullptr));
            }
        }
        else
        {
            parts.emplace_back(std::move(part));
        }
        if (end == std::string::npos)
        {
            break;
        }
        start = end + 1;
    }
    return parts;
}

// Jinja2's make_attrgetter: looks the path up item by item and replaces an undefined step
// with `defaultVal` unless it is None or missing
InternalValue GetAttributeByPath(const InternalValue& item, const InternalValueList& path, const InternalValue& defaultVal, RenderContext& context)
{
    InternalValue result = item;
    for (const auto& part : path)
    {
        // An undefined step has no attributes, unless its policy is Chainable
        CheckUndefinedUse(result, UndefinedUse::Attribute);
        auto next = Subscript(result, part, &context);
        if (next.IsUndefined() && !GetUndefinedInfo(next))
        {
            // A missing step knows where it came from, like a missing attribute in an expression
            next = MakeUndefined(&context, result, part);
        }
        result = std::move(next);
        if (result.IsUndefined() && !IsEmpty(defaultVal))
        {
            result = defaultVal;
        }
    }
    return result;
}

// Jinja2's _filter_test_common for a filter or test name that is not a string: no filter
// has that name. StrictUndefined already fails when the name is hashed
[[noreturn]] void ThrowNotACallableName(const char* kind, const InternalValue& name)
{
    CheckUndefinedUse(name, UndefinedUse::Operator);
    std::string message = std::string("No ") + kind + " named ";
    Apply<visitors::ValueRenderer<char>>(name, message, true);
    message += '.';
    if (const auto* info = GetUndefinedInfo(name))
    {
        message += " (" + UndefinedMessage(*info) + "; did you forget to quote the callable name?)";
    }
    throw TemplateRuntimeError(message);
}

// Jinja2's filters iterate their input: None, a bool, a number or a callable is not
// iterable. An undefined is left to the caller, which follows its undefined policy
bool IsNotIterable(const InternalValue& val)
{
    switch (val.Kind())
    {
    case ValueKind::None:
    case ValueKind::Bool:
    case ValueKind::Int:
    case ValueKind::Double:
    case ValueKind::Callable:
        return true;
    case ValueKind::Map:
    {
        // An object such as cycler() is stored as a map but is not iterable
        auto map = AsMap(val);
        return map && map->GetObjectClass() != nullptr;
    }
    default:
        return false;
    }
}

[[noreturn]] void ThrowNotIterable(const InternalValue& val)
{
    throw TemplateRuntimeError("'"s + Apply<visitors::PythonTypeNameGetter>(val) + "' object is not iterable");
}

// Jinja2's make_multi_attrgetter: "a,b.c" sorts by the list [item.a, item.b.c]
std::vector<InternalValueList> SortKeyPaths(const InternalValue& attrName)
{
    std::vector<InternalValueList> paths;
    if (IsEmpty(attrName))
    {
        return paths;
    }
    auto str = GetAsSameString(std::string(), attrName);
    if (!str)
    {
        paths.push_back(AttributePath(attrName));
        return paths;
    }
    size_t start = 0;
    for (;;)
    {
        auto end = str->find(',', start);
        paths.push_back(AttributePath(InternalValue(str->substr(start, end == std::string::npos ? std::string::npos : end - start))));
        if (end == std::string::npos)
        {
            break;
        }
        start = end + 1;
    }
    return paths;
}

} // namespace

Join::Join(const FilterParams& params)
{
    static const auto args = MakeArgumentsTable({ { "d", false, std::string() }, { "attribute" } });
    ParseParams(args, params);
}

InternalValue Join::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue attrName = GetArgumentValue("attribute", context);

    bool isConverted = false;
    ListAdapter values = ConvertToList(baseVal, isConverted, false);

    if (!isConverted)
    {
        // Python's str.join iterates its argument
        if (IsNotIterable(baseVal))
        {
            ThrowNotIterable(baseVal);
        }
        return InternalValue();
    }
    // Jinja2's make_attrgetter: a dotted path, digit parts as indexes
    if (!IsEmpty(attrName))
    {
        auto path = AttributePath(attrName);
        InternalValueList items;
        ReserveHint(items, values.GetSize().value_or(0));
        values.ForEach([&](const InternalValue& item) {
            items.push_back(GetAttributeByPath(item, path, InternalValue(), context));
            return true;
        });
        values = ListAdapter::CreateAdapter(std::move(items));
    }

    // Python join converts every item and the delimiter with str(). Under autoescape a
    // Markup delimiter or item makes it a Markup join, which escapes the rest
    auto* renderer = context.GetRendererCallback();
    InternalValue delimiterVal = GetArgumentValue("d", context);
    bool isMarkup = false;
    if (context.IsAutoescape())
    {
        // Iterate once: the sequence may be a generator
        InternalValueList items;
        isMarkup = delimiterVal.IsMarkup();
        for (const InternalValue& val : values)
        {
            isMarkup = isMarkup || val.IsMarkup();
            items.push_back(val);
        }
        values = ListAdapter::CreateAdapter(std::move(items));
    }
    // Every piece is a string of the template's character type
    auto asText = [renderer, isMarkup](const InternalValue& val) { return isMarkup ? MarkupEscape(val, renderer) : TextValue(val, renderer); };
    auto join = [&](auto builder) {
        InternalValue delimiter = asText(delimiterVal);
        bool isFirst = true;
        for (const InternalValue& val : values)
        {
            if (!isFirst)
            {
                builder.AppendString(delimiter);
            }
            isFirst = false;
            builder.AppendString(asText(val));
        }
        return builder.Build();
    };
    InternalValue result = renderer->IsWideTarget() ? join(StringBuilder<wchar_t>()) : join(StringBuilder<char>());
    result.SetMarkup(isMarkup);
    return result;
}

Sort::Sort(const FilterParams& params)
{
    static const auto args = MakeArgumentsTable({ { "reverse", false, InternalValue(false) }, { "case_sensitive", false, InternalValue(false) }, { "attribute", false } });
    ParseParams(args, params);
}

InternalValue Sort::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue attrName = GetArgumentValue("attribute", context);
    InternalValue isReverseVal = GetArgumentValue("reverse", context, InternalValue(false));
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context, InternalValue(false));

    bool isConverted = false;
    ListAdapter origValues = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
    {
        if (IsNotIterable(baseVal))
        {
            ThrowNotIterable(baseVal);
        }
        return InternalValue();
    }
    InternalValueList values = origValues.ToValueList();

    BinaryExpression::Operation oper = ConvertToBool(isReverseVal) ? BinaryExpression::LogicalGt : BinaryExpression::LogicalLt;
    BinaryExpression::CompareType compType = ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    std::vector<InternalValueList> paths = SortKeyPaths(attrName);

    auto isLess = [oper, compType](const InternalValue& key1, const InternalValue& key2, bool& isEqual) {
        // Equal items never reach `<`, so a list of equal dicts or Nones sorts as in Python
        isEqual = ConvertToBool(Apply2<visitors::BinaryMathOperation>(key1, key2, BinaryExpression::LogicalEq, compType));
        return !isEqual && ConvertToBool(Apply2<visitors::BinaryMathOperation>(key1, key2, oper, compType));
    };
    // Python's sorted() is stable
    if (paths.empty())
    {
        std::stable_sort(values.begin(), values.end(), [&isLess](auto& val1, auto& val2) {
            bool isEqual = false;
            return isLess(val1, val2, isEqual);
        });
        return ListAdapter::CreateAdapter(std::move(values));
    }

    // sorted(key=...) takes each item's key once, so a key that fails fails for one item too.
    // The key lists compare item by item, like Python lists
    const size_t width = paths.size();
    InternalValueList keys;
    keys.reserve(values.size() * width);
    for (const auto& val : values)
    {
        for (const auto& path : paths)
        {
            keys.push_back(GetAttributeByPath(val, path, InternalValue(), context));
        }
    }
    std::vector<size_t> order(values.size());
    std::iota(order.begin(), order.end(), size_t{ 0 });
    std::stable_sort(order.begin(), order.end(), [&](size_t idx1, size_t idx2) {
        for (size_t n = 0; n != width; ++n)
        {
            bool isEqual = false;
            auto result = isLess(keys[(idx1 * width) + n], keys[(idx2 * width) + n], isEqual);
            if (!isEqual)
            {
                return result;
            }
        }
        return false;
    });
    InternalValueList sorted;
    sorted.reserve(values.size());
    for (auto idx : order)
    {
        sorted.push_back(std::move(values[idx]));
    }
    values = std::move(sorted);
    return ListAdapter::CreateAdapter(std::move(values));
}

Attribute::Attribute(const FilterParams& params)
{
    static const auto args = MakeArgumentsTable({ { "name", true }, { "default", false } });
    ParseParams(args, params);
    // default is a Jinja2C++ extension that Python's do_attr(obj, name) lacks: it can only be
    // passed by name
    if (params.posParams.size() > 1)
    {
        SetArgumentsError("got " + std::to_string(params.posParams.size() - 1) + " more positional argument(s) than it takes");
    }
}

InternalValue Attribute::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const auto attrNameVal = GetArgumentValue("name", context);
    if (!IsStringValue(attrNameVal))
    {
        const auto* info = GetUndefinedInfo(attrNameVal);
        throw TemplateRuntimeError("attribute name must be string, not '" + (info ? std::string(UndefinedClassName(*info)) : Apply<visitors::PythonTypeNameGetter>(attrNameVal)) + "'");
    }
    CheckUndefinedUse(baseVal, UndefinedUse::Attribute);
    // Python's attr reads attributes only: the items of a dict are not attributes, the fields
    // of a reflected object and of an object such as namespace() are
    auto map = AsMap(baseVal);
    if (map && !map->HasAttributes() && !map->GetObjectClass())
    {
        return GetArgumentValue("default", context);
    }
    auto result = Subscript(baseVal, attrNameVal, &context);
    if (result.IsUndefined())
    {
        return GetArgumentValue("default", context);
    }
    return result;
}

Default::Default(const FilterParams& params)
{
    static const auto args = MakeArgumentsTable({ { "default_value", false, InternalValue(""s) }, { "boolean", false, InternalValue(false) } });
    ParseParams(args, params);
}

InternalValue Default::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue defaultVal = GetArgumentValue("default_value", context);
    InternalValue conditionResult = GetArgumentValue("boolean", context);

    // None is a defined value
    if (baseVal.IsUndefined())
    {
        return defaultVal;
    }

    if (ConvertToBool(conditionResult) && !ConvertToBool(baseVal))
    {
        return defaultVal;
    }

    return baseVal;
}

DictSort::DictSort(const FilterParams& params)
{
    static const auto args = MakeArgumentsTable({ { "case_sensitive", false }, { "by", false, "key"s }, { "reverse", false } });
    ParseParams(args, params);
}

InternalValue DictSort::Filter(const InternalValue& baseVal, RenderContext& context)
{
    CheckUndefinedUse(baseVal, UndefinedUse::Method);
    auto map = AsMap(baseVal);
    if (!map || map->GetObjectClass())
    {
        // Python calls value.items(), which only a mapping has
        if (!baseVal.IsUndefined())
        {
            throw TemplateRuntimeError("'"s + Apply<visitors::PythonTypeNameGetter>(baseVal) + "' object has no attribute 'items'");
        }
        return InternalValue();
    }

    InternalValue isReverseVal = GetArgumentValue("reverse", context);
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context);
    InternalValue byVal = GetArgumentValue("by", context);

    // Python sorts by key.lower() when case-insensitive: the lowered keys are made once,
    // before sorting, rather than in every comparison. The sort moves pointers to the
    // pairs, not the pairs.
    struct SortItem
    {
        KeyValuePair* pair;
        // The lowered key when case-insensitive and every key is a string
        std::string sortKey;
    };
    bool (*comparator)(const SortItem& left, const SortItem& right) = nullptr;
    bool lowerKeys = false;
    bool sortByKey = false;
    bool keysAreStrings = true;

    auto entries = map->GetEntries();

    if (AsString(byVal) == "key") // Sort by key
    {
        sortByKey = true;
        lowerKeys = !ConvertToBool(isCsVal);
        // Narrow text, as map keys are UTF-8 (a template dict converts wide keys)
        keysAreStrings = std::all_of(entries.begin(), entries.end(), [](const KeyValuePair& entry) { return AsStringView<char>(entry.key).has_value(); });
        if (keysAreStrings && lowerKeys)
        {
            comparator = [](const SortItem& left, const SortItem& right) { return left.sortKey < right.sortKey; };
        }
        else if (keysAreStrings)
        {
            comparator = [](const SortItem& left, const SortItem& right) { return *AsStringView<char>(left.pair->key) < *AsStringView<char>(right.pair->key); };
        }
        else
        {
            // Python lowers only str keys, and a str key next to a key of another type
            // fails the comparison either way, so other keys compare as they are
            comparator = [](const SortItem& left, const SortItem& right) {
                return CompareForOrder(left.pair->key, right.pair->key, BinaryExpression::LogicalLt, BinaryExpression::CaseSensitive);
            };
        }
    }
    else if (AsString(byVal) == "value")
    {
        if (ConvertToBool(isCsVal))
        {
            comparator = [](const SortItem& left, const SortItem& right) {
                return CompareForOrder(left.pair->value, right.pair->value, BinaryExpression::LogicalLt, BinaryExpression::CaseSensitive);
            };
        }
        else
        {
            comparator = [](const SortItem& left, const SortItem& right) {
                return CompareForOrder(left.pair->value, right.pair->value, BinaryExpression::LogicalLt, BinaryExpression::CaseInsensitive);
            };
        }
    }
    else
    {
        // Jinja2 raises FilterArgumentError
        throw TemplateRuntimeError(R"(You can only sort by either "key" or "value")");
    }

    std::vector<SortItem> tempVector;
    tempVector.reserve(entries.size());
    for (auto& entry : entries)
    {
        SortItem item{ &entry, std::string() };
        if (sortByKey && keysAreStrings && lowerKeys)
        {
            detail::LowerUtf8(*AsStringView<char>(entry.key), item.sortKey);
        }
        tempVector.push_back(std::move(item));
    }

    // Python's sorted() is stable, also with reverse=True: ties keep the mapping's order
    if (ConvertToBool(isReverseVal))
    {
        std::stable_sort(tempVector.begin(), tempVector.end(), [comparator](auto& l, auto& r) { return comparator(r, l); });
    }
    else
    {
        std::stable_sort(tempVector.begin(), tempVector.end(), [comparator](auto& l, auto& r) { return comparator(l, r); });
    }

    InternalValueList resultList;
    resultList.reserve(tempVector.size());
    for (auto& tmpVal : tempVector)
    {
        auto resultVal = InternalValue(std::move(*tmpVal.pair));
        if (baseVal.ShouldExtendLifetime())
        {
            resultVal.SetParentData(baseVal);
        }
        resultList.push_back(std::move(resultVal));
    }

    return InternalValue(ListAdapter::CreateAdapter(std::move(resultList)));
}

GroupBy::GroupBy(const FilterParams& params)
{
    static const auto args = MakeArgumentsTable({ { "attribute", true }, { "default", false }, { "case_sensitive", false, false } });
    ParseParams(args, params);
}

InternalValue GroupBy::Filter(const InternalValue& baseVal, RenderContext& context)
{
    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, isConverted, false);

    if (!isConverted)
    {
        if (IsNotIterable(baseVal))
        {
            ThrowNotIterable(baseVal);
        }
        return InternalValue();
    }

    auto path = AttributePath(GetArgumentValue("attribute", context));
    auto defaultVal = GetArgumentValue("default", context);
    auto compType = ConvertToBool(GetArgumentValue("case_sensitive", context)) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    struct Item
    {
        InternalValue key;
        InternalValue value;
    };
    std::vector<Item> items;
    list.ForEach([&](InternalValue&& item) {
        auto key = GetAttributeByPath(item, path, defaultVal, context);
        items.push_back(Item{ std::move(key), std::move(item) });
        return true;
    });

    // Like Jinja2: sort by the key (stable), then group runs of equal keys. Without
    // case_sensitive strings compare lowercased and a group keeps its first item's key
    std::stable_sort(items.begin(), items.end(), [compType](const Item& left, const Item& right) {
        return CompareForOrder(left.key, right.key, BinaryExpression::LogicalLt, compType);
    });

    static const auto fieldNames = std::make_shared<const std::vector<std::string>>(std::vector<std::string>{ "grouper", "list" });
    InternalValueList result;
    for (auto p = items.begin(); p != items.end();)
    {
        InternalValueList group;
        auto isSameGroup = [&p, compType](const Item& item) {
            return ConvertToBool(Apply2<visitors::BinaryMathOperation>(p->key, item.key, BinaryExpression::LogicalEq, compType));
        };
        auto groupEnd = p;
        for (; groupEnd != items.end() && isSameGroup(*groupEnd); ++groupEnd)
        {
            group.push_back(groupEnd->value);
        }
        result.emplace_back(ListAdapter::CreateAdapter(InternalValueList{ p->key, ListAdapter::CreateAdapter(std::move(group)) }).MarkAsNamedTuple(fieldNames));
        p = groupEnd;
    }

    return ListAdapter::CreateAdapter(std::move(result));
}

ApplyMacro::ApplyMacro(const FilterParams& params)
{
    static const auto args = MakeArgumentsTable({ { "macro", true } });
    ParseParams(args, params, ExtraArgs::Accept, &m_mappingParams);
}

InternalValue ApplyMacro::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue macroName = GetArgumentValue("macro", context);
    if (IsEmpty(macroName))
    {
        return InternalValue();
    }

    const auto macroVal = context.FindValue(AsString(macroName));
    if (!macroVal)
    {
        return InternalValue();
    }

    const auto* callable = GetIf<Callable>(&*macroVal);
    if (!callable || callable->GetKind() != Callable::Macro)
    {
        return InternalValue();
    }

    CallParams tmpCallParams = helpers::EvaluateCallParams(m_mappingParams, context);
    CallParams callParams;
    callParams.kwParams = std::move(tmpCallParams.kwParams);
    callParams.posParams.reserve(tmpCallParams.posParams.size() + 1);
    callParams.posParams.push_back(baseVal);
    for (auto& p : tmpCallParams.posParams)
    {
        callParams.posParams.push_back(std::move(p));
    }

    InternalValue result;
    if (callable->GetType() == Callable::Type::Expression)
    {
        result = callable->GetExpressionCallable()(callParams, context);
    }
    else
    {
        result = RenderToString(context.GetRendererCallback(),
                                [&](OutStream& stream) { callable->GetStatementCallable()(callParams, stream, context); });
        result.SetMarkup(context.IsAutoescape());
    }

    return result;
}

Map::Map(const FilterParams& params)
{
    // map(attribute='x', default=d) reads the attribute of each item; it binds no filter
    const auto attributeIt = params.kwParams.find("attribute");
    if (params.posParams.empty() && attributeIt != params.kwParams.cend())
    {
        m_byAttribute = true;
        m_mappingParams.kwParams["name"] = attributeIt->second;
        for (const auto& [name, value] : params.kwParams)
        {
            if (name == "default")
            {
                m_mappingParams.kwParams["default"] = value;
            }
            else if (name != "attribute")
            {
                // Jinja2's prepare_map raises FilterArgumentError
                SetArgumentsError("got an unexpected keyword argument '" + name + "'");
                break;
            }
        }
        return;
    }

    static const auto args = MakeArgumentsTable({ { "filter", true } });
    ParseParams(args, params, ExtraArgs::Accept, &m_mappingParams);
}

InternalValue Map::Filter(const InternalValue& baseVal, RenderContext& context)
{
    if (m_byAttribute)
    {
        bool isConverted = false;
        auto list = ConvertToList(baseVal, isConverted, false);
        if (!isConverted)
        {
            if (IsNotIterable(baseVal))
            {
                // Jinja2 starts with `if value:`, so None, False and 0 give nothing
                if (!ConvertToBool(baseVal))
                {
                    return ListAdapter::CreateAdapter(InternalValueList());
                }
                ThrowNotIterable(baseVal);
            }
            return InternalValue();
        }

        auto params = helpers::EvaluateCallParams(m_mappingParams, context);
        auto path = AttributePath(params.kwParams["name"]);
        auto defaultVal = params.kwParams["default"];
        InternalValueList resultList;
        ReserveHint(resultList, list.GetSize().value_or(0));
        list.ForEach([&](const InternalValue& item) {
            resultList.push_back(GetAttributeByPath(item, path, defaultVal, context));
            return true;
        });
        return ListAdapter::CreateAdapter(std::move(resultList));
    }

    InternalValue filterName = GetArgumentValue("filter", context);
    // A plain undefined (an error path's result) stays lenient
    if (filterName.IsUndefined() && !GetUndefinedInfo(filterName))
    {
        return InternalValue();
    }

    FilterPtr filter;
    if (IsStringValue(filterName))
    {
        filter = CreateFilter(AsString(filterName), m_mappingParams, context);
        if (!filter)
        {
            return InternalValue();
        }
    }

    bool isConverted = false;
    auto list = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
    {
        // Jinja2 starts with `if value:`, so None, False and 0 give nothing
        if (!ConvertToBool(baseVal))
        {
            return ListAdapter::CreateAdapter(InternalValueList());
        }
        ThrowNotIterable(baseVal);
    }

    // The extra arguments go to the filter, which checks them when it is called
    const auto argsError = filter ? filter->GetArgumentsError() : std::string();
    InternalValueList resultList;
    ReserveHint(resultList, list.GetSize().value_or(0));
    list.ForEach([&](const InternalValue& val) {
        // Python's map is lazy: a name that is no filter, or arguments that do not fit it,
        // fail at the first item
        if (!filter)
        {
            ThrowNotACallableName("filter", filterName);
        }
        if (!argsError.empty())
        {
            throw TemplateRuntimeError(AsString(filterName) + "() " + argsError);
        }
        resultList.push_back(filter->Filter(val, context));
        return true;
    });

    return ListAdapter::CreateAdapter(std::move(resultList));
}
Random::Random(const FilterParams& /*params*/) {}

InternalValue Random::Filter(const InternalValue&, RenderContext&)
{
    return InternalValue();
}

SequenceAccessor::SequenceAccessor(const FilterParams& params, SequenceAccessor::Mode mode)
    : m_mode(mode)
{
    switch (mode)
    {
    case FirstItemMode:
    case LastItemMode:
    case LengthMode:
    case RandomMode:
    case ReverseMode:
        ParseParams(NoArguments(), params);
        break;
    case MaxItemMode:
    case MinItemMode:
    {
        static const auto args = MakeArgumentsTable({ { "case_sensitive", false, InternalValue(false) }, { "attribute", false } });
        ParseParams(args, params);
        break;
    }
    case SumItemsMode:
    {
        static const auto args = MakeArgumentsTable({ { "attribute", false }, { "start", false } });
        ParseParams(args, params);
        break;
    }
    case UniqueItemsMode:
    {
        static const auto args = MakeArgumentsTable({ { "case_sensitive", false, InternalValue(false) }, { "attribute", false } });
        ParseParams(args, params);
        break;
    }
    }
}

namespace
{
// Keeps an item borrowed from baseVal alive as long as the item itself
InternalValue WithParent(const InternalValue& baseVal, InternalValue value)
{
    if (baseVal.ShouldExtendLifetime())
    {
        value.SetParentData(baseVal);
    }
    return value;
}

// Jinja2's first and last of an empty sequence: an undefined that says so when used
InternalValue NoItem(RenderContext& context, const char* which)
{
    return MakeUndefinedWithHint(context, std::string("No ") + which + " item, sequence was empty.");
}

InternalValue FirstItem(const ListAdapter& list, const InternalValue& baseVal, RenderContext& context)
{
    const auto& listSize = list.GetSize();
    if (listSize && *listSize > 0)
    {
        return WithParent(baseVal, list.GetValueByIndex(0));
    }
    auto it = list.begin();
    if (it != list.end())
    {
        return WithParent(baseVal, *it);
    }
    return NoItem(context, "first");
}

InternalValue LastItem(const ListAdapter& list, const InternalValue& baseVal, RenderContext& context)
{
    const auto& listSize = list.GetSize();
    if (listSize && *listSize > 0)
    {
        return WithParent(baseVal, list.GetValueByIndex(static_cast<int64_t>(listSize.value() - 1)));
    }
    std::optional<InternalValue> result;
    list.ForEach([&result](InternalValue&& item) {
        result = std::move(item);
        return true;
    });
    return result ? WithParent(baseVal, std::move(*result)) : NoItem(context, "last");
}

InternalValue Length(const ListAdapter& list)
{
    const auto& listSize = list.GetSize();
    if (listSize && *listSize > 0)
    {
        return static_cast<int64_t>(listSize.value());
    }
    int64_t count = 0;
    list.ForEach([&count](const InternalValue&) {
        ++count;
        return true;
    });
    return count;
}

InternalValue RandomItem(const ListAdapter& list, const InternalValue& baseVal)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    const auto& listSize = list.GetSize();
    if (listSize && *listSize > 0)
    {
        std::uniform_int_distribution<size_t> dis(0, listSize.value() - 1);
        return WithParent(baseVal, list.GetValueByIndex(static_cast<int64_t>(dis(gen))));
    }
    // Reservoir sampling over a sequence of unknown size
    InternalValue result;
    size_t count = 0;
    for (auto it = list.begin(), end = list.end(); it != end; ++it, ++count)
    {
        bool doCopy = count == 0 || std::uniform_int_distribution<size_t>(0, count)(gen) == 0;
        if (doCopy)
        {
            result = WithParent(baseVal, *it);
        }
    }
    return result;
}

InternalValue Reverse(const ListAdapter& list, const InternalValue& baseVal)
{
    if (IsStringValue(baseVal))
    {
        // Python reverses a string into a string
        return ApplyStringConverter(baseVal, [](auto strView) -> TargetString {
            // Walk back over the characters; a multi-unit one keeps its unit order
            std::basic_string<typename decltype(strView)::value_type> reversed;
            reversed.reserve(strView.size());
            for (size_t end = strView.size(); end != 0;)
            {
                size_t start = end - 1;
                while (start != 0 && IsCodePointTail(strView[start]))
                {
                    --start;
                }
                reversed.append(strView.substr(start, end - start));
                end = start;
            }
            return TargetString(std::move(reversed));
        });
    }

    const auto& listSize = list.GetSize();
    if (listSize)
    {
        auto size = listSize.value();
        CheckSequenceSize(size, "a list");
        InternalValueList resultList(size);
        for (std::size_t n = 0; n < size; ++n)
        {
            resultList[size - n - 1] = WithParent(baseVal, list.GetValueByIndex(static_cast<int64_t>(n)));
        }
        return ListAdapter::CreateAdapter(std::move(resultList));
    }

    InternalValueList resultList;
    list.ForEach([&](InternalValue&& item) {
        resultList.push_back(WithParent(baseVal, std::move(item)));
        return true;
    });
    std::reverse(resultList.begin(), resultList.end());
    return ListAdapter::CreateAdapter(std::move(resultList));
}

InternalValue Sum(const ListAdapter& list, const InternalValue& attrName, const InternalValue& start, RenderContext& context)
{
    // Python's sum starts from 0, so a string item raises like `0 + 'h'`
    InternalValue resultVal = IsEmpty(start) ? InternalValue(static_cast<int64_t>(0)) : start;
    auto path = IsEmpty(attrName) ? InternalValueList() : AttributePath(attrName);
    list.ForEach([&](InternalValue&& val) {
        auto item = path.empty() ? std::move(val) : GetAttributeByPath(val, path, InternalValue(), context);
        resultVal = Apply2<visitors::BinaryMathOperation>(resultVal, item, BinaryExpression::Plus);
        return true;
    });
    return resultVal;
}

struct UniqueItem
{
    InternalValue val;
    int64_t idx;
};

// Drops repeated keys, keeping the first of each in its original position
void DropDuplicates(std::vector<UniqueItem>& items, BinaryExpression::CompareType compType)
{
    auto isEqual = [compType](const UniqueItem& i1, const UniqueItem& i2) {
        return ConvertToBool(Apply2<visitors::BinaryMathOperation>(i1.val, i2.val, BinaryExpression::LogicalEq, compType));
    };
    // The positions are sorted, not the items: a comparison that throws leaves a sort's
    // range with moved-from (undefined) elements, and the fallback still needs every item
    std::vector<size_t> order(items.size());
    std::iota(order.begin(), order.end(), size_t{ 0 });
    std::vector<UniqueItem> uniqueItems;
    try
    {
        std::stable_sort(order.begin(), order.end(), [&items, compType](size_t i1, size_t i2) {
            return ConvertToBool(Apply2<visitors::BinaryMathOperation>(items[i1].val, items[i2].val, BinaryExpression::LogicalLt, compType));
        });
        order.erase(std::unique(order.begin(), order.end(), [&](size_t i1, size_t i2) { return isEqual(items[i1], items[i2]); }), order.end());
        std::sort(order.begin(), order.end());
        uniqueItems.reserve(order.size());
        for (auto idx : order)
        {
            uniqueItems.push_back(std::move(items[idx]));
        }
    }
    catch (const TemplateRuntimeError&)
    {
        // Unorderable items (mixed types, None, dicts): Python hashes them, so keep the
        // first of each run of equal items in a quadratic pass instead
        uniqueItems.clear();
        for (auto& item : items)
        {
            if (std::none_of(uniqueItems.begin(), uniqueItems.end(), [&](const UniqueItem& u) { return isEqual(u, item); }))
            {
                uniqueItems.push_back(item);
            }
        }
    }
    items = std::move(uniqueItems);
}

InternalValue Unique(const ListAdapter& list,
                     const InternalValue& baseVal,
                     const InternalValue& attrName,
                     BinaryExpression::CompareType compType,
                     RenderContext& context)
{
    // The items are kept as read: a single-pass list cannot be indexed afterwards
    InternalValueList values;
    std::vector<UniqueItem> items;
    int64_t idx = 0;
    const auto path = IsEmpty(attrName) ? InternalValueList() : AttributePath(attrName);
    list.ForEach([&](InternalValue&& v) {
        items.push_back(UniqueItem{ IsEmpty(attrName) ? v : GetAttributeByPath(v, path, InternalValue(), context), idx++ });
        values.push_back(std::move(v));
        return true;
    });

    // Jinja2 keeps a set of the keys it saw
    for (const auto& item : items)
    {
        if (const char* type = UnhashableType(item.val))
        {
            throw TemplateRuntimeError("unhashable type: '"s + type + "'");
        }
    }
    DropDuplicates(items, compType);

    InternalValueList resultList;
    resultList.reserve(items.size());
    for (auto& i : items)
    {
        resultList.push_back(WithParent(baseVal, std::move(values[static_cast<size_t>(i.idx)])));
    }
    return ListAdapter::CreateAdapter(std::move(resultList));
}
} // namespace

namespace
{
// last, random and reverse index a string, and an item or slice of Markup is Markup (first
// iterates it, which gives plain characters)
InternalValue KeepMarkup(const InternalValue& baseVal, InternalValue result)
{
    if (baseVal.IsMarkup() && IsStringValue(result))
    {
        result.SetMarkup();
    }
    return result;
}
} // namespace

InternalValue SequenceAccessor::Filter(const InternalValue& baseVal, RenderContext& context)
{
    // len(loop) is the length of the loop, as Jinja2's LoopContext.__len__, not the
    // number of its attributes
    InternalValue loopLength;
    if (const auto loop = m_mode == LengthMode ? AsMap(baseVal) : std::nullopt; loop && loop->GetLoopAttr(LoopAttr::Length, loopLength))
    {
        return loopLength;
    }
    // Like Python, a string is a sequence of characters and a mapping one of its keys
    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, isConverted, false);

    if (!isConverted)
    {
        // Python iterates an undefined as an empty sequence
        if (baseVal.IsUndefined() && (m_mode == FirstItemMode || m_mode == LastItemMode))
        {
            return NoItem(context, m_mode == FirstItemMode ? "first" : "last");
        }
        if (baseVal.IsUndefined() && (m_mode == MaxItemMode || m_mode == MinItemMode))
        {
            return MakeUndefinedWithHint(context, "No aggregated item, sequence was empty.");
        }
        if (IsNotIterable(baseVal))
        {
            // length and random take len(), last reverses and reverse falls back to list()
            const std::string typeName = Apply<visitors::PythonTypeNameGetter>(baseVal);
            switch (m_mode)
            {
            case LengthMode:
            case RandomMode:
                throw TemplateRuntimeError("object of type '" + typeName + "' has no len()");
            case LastItemMode:
                throw TemplateRuntimeError("'" + typeName + "' object is not reversible");
            case ReverseMode:
                throw TemplateRuntimeError("argument must be iterable");
            default:
                ThrowNotIterable(baseVal);
            }
        }
        return {};
    }

    InternalValue attrName = GetArgumentValue("attribute", context);
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context, InternalValue(false));

    BinaryExpression::CompareType compType = ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    const auto attrPath = IsEmpty(attrName) ? InternalValueList() : AttributePath(attrName);
    // Python's min and max take each item's key once and keep the first extreme item
    auto extreme = [&](bool isMax) {
        std::optional<InternalValue> best;
        InternalValue bestKey;
        list.ForEach([&](InternalValue&& item) {
            InternalValue key = attrPath.empty() ? item : GetAttributeByPath(item, attrPath, InternalValue(), context);
            if (!best || (isMax ? CompareForOrder(bestKey, key, BinaryExpression::LogicalLt, compType) : CompareForOrder(key, bestKey, BinaryExpression::LogicalLt, compType)))
            {
                bestKey = std::move(key);
                best = std::move(item);
            }
            return true;
        });
        return best ? WithParent(baseVal, std::move(*best)) : MakeUndefinedWithHint(context, "No aggregated item, sequence was empty.");
    };

    switch (m_mode)
    {
    case FirstItemMode:
        return FirstItem(list, baseVal, context);
    case LastItemMode:
        return KeepMarkup(baseVal, LastItem(list, baseVal, context));
    case LengthMode:
        return Length(list);
    case RandomMode:
        return KeepMarkup(baseVal, RandomItem(list, baseVal));
    case MaxItemMode:
        return extreme(true);
    case MinItemMode:
        return extreme(false);
    case ReverseMode:
        return KeepMarkup(baseVal, Reverse(list, baseVal));
    case SumItemsMode:
    {
        auto start = GetArgumentValue("start", context);
        // Python's sum refuses a string start ("sum() can't sum strings")
        if (IsStringValue(start))
        {
            context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
        }
        return Sum(list, attrName, start, context);
    }
    case UniqueItemsMode:
        return Unique(list, baseVal, attrName, compType, context);
    }

    return {};
}
Slice::Slice(const FilterParams& params, Slice::Mode mode)
    : m_mode{ mode }
{
    if (m_mode == BatchMode)
    {
        static const auto args = MakeArgumentsTable({ { "linecount"s, true }, { "fill_with"s, false } });
        ParseParams(args, params);
    }
    else
    {
        static const auto args = MakeArgumentsTable({ { "slices"s, true }, { "fill_with"s, false } });
        ParseParams(args, params);
    }
}

InternalValue Slice::Filter(const InternalValue& baseVal, RenderContext& context)
{
    if (m_mode == BatchMode)
    {
        return Batch(baseVal, context);
    }

    // Like Python, a string is a sequence of characters and a mapping one of its keys
    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
    {
        ThrowNotIterable(baseVal);
    }

    auto protectedValue = [&baseVal](InternalValue value) {
        if (baseVal.ShouldExtendLifetime())
        {
            value.SetParentData(baseVal);
        }
        return value;
    };

    // Port of Jinja2's do_slice: `slices` columns, the first length % slices of them one
    // item longer; with fill_with every shorter column gets one fill item
    int64_t slices = ConvertToInt(GetArgumentValue("slices", context));
    InternalValue fillWith = GetArgumentValue("fill_with", context);
    // Python raises ZeroDivisionError
    if (slices == 0)
    {
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
    }

    auto items = list.ToValueList();
    InternalValueList resultList;
    if (slices > 0)
    {
        auto length = static_cast<int64_t>(items.size());
        auto perSlice = length / slices;
        auto withExtra = length % slices;
        int64_t offset = 0;
        for (int64_t slice = 0; slice < slices; ++slice)
        {
            auto start = offset + (slice * perSlice);
            if (slice < withExtra)
            {
                ++offset;
            }
            auto end = offset + ((slice + 1) * perSlice);
            InternalValueList column;
            for (auto idx = start; idx < end; ++idx)
            {
                column.push_back(protectedValue(items[static_cast<size_t>(idx)]));
            }
            if (!IsEmpty(fillWith) && slice >= withExtra)
            {
                column.push_back(fillWith);
            }
            resultList.emplace_back(ListAdapter::CreateAdapter(std::move(column)));
        }
    }

    return ListAdapter::CreateAdapter(std::move(resultList));
}

InternalValue Slice::Batch(const InternalValue& baseVal, RenderContext& context)
{
    bool isConverted = false;
    auto list = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
    {
        ThrowNotIterable(baseVal);
    }

    auto protectedValue = [&baseVal](InternalValue value) {
        if (baseVal.ShouldExtendLifetime())
        {
            value.SetParentData(baseVal);
        }
        return value;
    };

    // Port of Jinja2's do_batch: rows of `linecount` items, the last one padded only with fill_with
    auto linecount = ConvertToInt(GetArgumentValue("linecount", context));
    InternalValue fillWith = GetArgumentValue("fill_with", context);

    InternalValueList resultList;
    InternalValueList row;
    for (const auto& item : list)
    {
        if (static_cast<int64_t>(row.size()) == linecount)
        {
            resultList.emplace_back(ListAdapter::CreateAdapter(std::move(row)));
            row = InternalValueList();
        }
        row.push_back(protectedValue(item));
    }
    if (!row.empty())
    {
        if (!IsEmpty(fillWith))
        {
            while (static_cast<int64_t>(row.size()) < linecount)
            {
                row.push_back(fillWith);
            }
        }
        resultList.emplace_back(ListAdapter::CreateAdapter(std::move(row)));
    }
    return ListAdapter::CreateAdapter(std::move(resultList));
}

StringFormat::StringFormat(const FilterParams& params)
{
    ParseParams(NoArguments(), params, ExtraArgs::Accept, &m_params);
}

Tester::Tester(const FilterParams& params, Tester::Mode mode)
    : m_mode(mode)
{
    FilterParams newParams;

    if ((mode == RejectMode || mode == SelectMode) && params.kwParams.empty() && params.posParams.empty())
    {
        m_noParams = true;
        return;
    }

    if (mode == RejectMode || mode == SelectMode)
    {
        static const auto args = MakeArgumentsTable({ { "tester", false } });
        ParseParams(args, params, ExtraArgs::Accept, &m_testingParams);
    }
    else
    {
        static const auto args = MakeArgumentsTable({ { "attribute", true }, { "tester", false } });
        ParseParams(args, params, ExtraArgs::Accept, &m_testingParams);
    }
}

InternalValue Tester::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue testerName = GetArgumentValue("tester", context);
    InternalValue attrName = GetArgumentValue("attribute", context);

    TesterPtr tester;

    // No test was named: a plain undefined is the missing argument. A name that is no test
    // fails at the first item, as Python's lazy select does
    const bool isNoName = testerName.IsUndefined() && GetUndefinedInfo(testerName) == nullptr;
    const bool isBadName = !isNoName && !IsStringValue(testerName);
    if (IsStringValue(testerName))
    {
        tester = CreateTester(AsString(testerName), m_testingParams, context);

        if (!tester)
        {
            return InternalValue();
        }
    }

    bool isConverted = false;
    auto list = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
    {
        // Jinja2 starts with `if value:`, so None, False and 0 give nothing
        if (!ConvertToBool(baseVal))
        {
            return ListAdapter::CreateAdapter(InternalValueList());
        }
        ThrowNotIterable(baseVal);
    }

    InternalValueList resultList;
    ReserveHint(resultList, list.GetSize().value_or(0));
    auto isSelected = [this, tester, attrName, isBadName, &testerName, &context](const InternalValue& val) {
        if (isBadName)
        {
            ThrowNotACallableName("test", testerName);
        }
        InternalValue attrVal;
        bool isAttr = !IsEmpty(attrName);
        if (isAttr)
        {
            attrVal = GetAttributeByPath(val, AttributePath(attrName), InternalValue(), context);
        }

        bool result = false;
        if (tester)
        {
            result = tester->Test(isAttr ? attrVal : val, context);
        }
        else
        {
            result = ConvertToBool(isAttr ? attrVal : val);
        }

        return (m_mode == SelectMode || m_mode == SelectAttrMode) ? result : !result;
    };
    list.ForEach([&](InternalValue&& val) {
        if (isSelected(val))
        {
            resultList.push_back(std::move(val));
        }
        return true;
    });

    return ListAdapter::CreateAdapter(std::move(resultList));
}

ValueConverter::ValueConverter(const FilterParams& params, ValueConverter::Mode mode)
    : m_mode(mode)
{
    switch (mode)
    {
    case ToFloatMode:
    {
        static const auto args = MakeArgumentsTable({ { "default"s, false, 0.0 } });
        ParseParams(args, params);
        break;
    }
    case ToIntMode:
    {
        static const auto args = MakeArgumentsTable({ { "default"s, false, static_cast<int64_t>(0) }, { "base"s, false, static_cast<int64_t>(10) } });
        ParseParams(args, params);
        break;
    }
    case ToListMode:
    case AbsMode:
    case ItemsMode:
        ParseParams(NoArguments(), params);
        break;
    case FileSizeFormatMode:
    {
        static const auto args = MakeArgumentsTable({ { "binary"s, false, false } });
        ParseParams(args, params);
        break;
    }
    case RoundMode:
    {
        static const auto args = MakeArgumentsTable({ { "precision"s, false, static_cast<int64_t>(0) }, { "method"s, false, "common"s } });
        ParseParams(args, params);
        break;
    }
    }
}

struct ConverterParams
{
    ValueConverter::Mode mode{};
    InternalValue defValule;
    InternalValue base;
    InternalValue prec;
    InternalValue roundMethod;
};

struct ValueConverterImpl : visitors::BaseVisitor<>
{
    using BaseVisitor::operator();

    explicit ValueConverterImpl(ConverterParams params)
        : m_params(std::move(params))
    {
    }

    InternalValue operator()(int64_t val) const
    {
        InternalValue result;
        switch (m_params.mode)
        {
        case ValueConverter::ToFloatMode:
            result = InternalValue(static_cast<double>(val));
            break;
        case ValueConverter::AbsMode:
            result = InternalValue(static_cast<int64_t>(std::abs(val)));
            break;
        case ValueConverter::ToIntMode:
        case ValueConverter::RoundMode:
            result = InternalValue(static_cast<int64_t>(val));
            break;
        default:
            break;
        }

        return result;
    }

    InternalValue operator()(double val) const
    {
        InternalValue result;
        switch (m_params.mode)
        {
        case ValueConverter::ToFloatMode:
            result = static_cast<double>(val);
            break;
        case ValueConverter::ToIntMode:
            result = static_cast<int64_t>(val);
            break;
        case ValueConverter::AbsMode:
            result = InternalValue(fabs(val));
            break;
        case ValueConverter::RoundMode:
        {
            auto method = AsString(m_params.roundMethod);
            auto prec = GetAs<int64_t>(m_params.prec);
            double pow10 = std::pow(10, static_cast<int>(prec));
            val *= pow10;
            if (method == "ceil")
            {
                val = val < 0 ? std::floor(val) : std::ceil(val);
            }
            else if (method == "floor")
            {
                val = val > 0 ? std::floor(val) : std::ceil(val);
            }
            else if (method == "common")
            {
                val = std::round(val);
            }
            result = InternalValue(val / pow10);
            break;
        }
        default:
            break;
        }

        return result;
    }

    static double ConvertToDouble(const char* buff, bool& isConverted)
    {
        char* endBuff = nullptr;
        double dblVal = strtod(buff, &endBuff);
        isConverted = *endBuff == 0;
        return dblVal;
    }

    static double ConvertToDouble(const wchar_t* buff, bool& isConverted)
    {
        wchar_t* endBuff = nullptr;
        double dblVal = wcstod(buff, &endBuff);
        isConverted = *endBuff == 0;
        return dblVal;
    }

    static long long ConvertToInt(const char* buff, int base, bool& isConverted)
    {
        char* endBuff = nullptr;
        long long intVal = strtoll(buff, &endBuff, base);
        isConverted = *endBuff == 0;
        return intVal;
    }

    static long long ConvertToInt(const wchar_t* buff, int base, bool& isConverted)
    {
        wchar_t* endBuff = nullptr;
        long long intVal = wcstoll(buff, &endBuff, base);
        isConverted = *endBuff == 0;
        return intVal;
    }

    template<typename CharT>
    InternalValue operator()(const std::basic_string<CharT>& val) const
    {
        InternalValue result;
        switch (m_params.mode)
        {
        case ValueConverter::ToFloatMode:
        {
            bool converted = false;
            double dblVal = ConvertToDouble(val.c_str(), converted);

            if (!converted)
            {
                result = m_params.defValule;
            }
            else
            {
                result = dblVal;
            }
            break;
        }
        case ValueConverter::ToIntMode:
        {
            int base = static_cast<int>(GetAs<int64_t>(m_params.base));
            bool converted = false;
            long long intVal = ConvertToInt(val.c_str(), base, converted);

            if (!converted)
            {
                result = m_params.defValule;
            }
            else
            {
                result = static_cast<int64_t>(intVal);
            }
            break;
        }
        case ValueConverter::ToListMode:
        {
            // Code points, the same split as everywhere a string is iterated
            bool isConverted = false;
            result = ConvertToList(InternalValue(TargetString(std::basic_string<CharT>(val.begin(), val.end()))), isConverted, false);
            break;
        }
        default:
            break;
        }

        return result;
    }

    template<typename CharT>
    InternalValue operator()(const std::basic_string_view<CharT>& val) const
    {
        InternalValue result;
        switch (m_params.mode)
        {
        case ValueConverter::ToFloatMode:
        {
            bool converted = false;
            std::basic_string<CharT> str(val.begin(), val.end());
            double dblVal = ConvertToDouble(str.c_str(), converted);

            if (!converted)
            {
                result = m_params.defValule;
            }
            else
            {
                result = static_cast<double>(dblVal);
            }
            break;
        }
        case ValueConverter::ToIntMode:
        {
            int base = static_cast<int>(GetAs<int64_t>(m_params.base));
            bool converted = false;
            std::basic_string<CharT> str(val.begin(), val.end());
            long long intVal = ConvertToInt(str.c_str(), base, converted);

            if (!converted)
            {
                result = m_params.defValule;
            }
            else
            {
                result = static_cast<int64_t>(intVal);
            }
            break;
        }
        case ValueConverter::ToListMode:
        {
            // Code points, the same split as everywhere a string is iterated
            bool isConverted = false;
            result = ConvertToList(InternalValue(TargetString(std::basic_string<CharT>(val.begin(), val.end()))), isConverted, false);
            break;
        }
        default:
            break;
        }

        return result;
    }

    InternalValue operator()(const ListRef& val) const
    {
        if (m_params.mode != ValueConverter::ToListMode)
        {
            return InternalValue();
        }

        // list() is always a new list: of a tuple or a range it prints as [a, b], and of a
        // list the template owns an append() to it must not change the original
        if (val.IsTuple() || val.GetRangeInfo() || val.GetMutableItems())
        {
            return ListAdapter::CreateAdapter(val.ToValueList());
        }

        return InternalValue(val.ToAdapter());
    }

    // list() of a dictsort pair is the list [key, value]
    InternalValue operator()(const KeyValuePair& val) const
    {
        if (m_params.mode != ValueConverter::ToListMode)
        {
            return InternalValue();
        }

        return ListAdapter::CreateAdapter(InternalValueList{ val.key, val.value });
    }

    InternalValue operator()(const MapRef& val) const
    {
        if (m_params.mode != ValueConverter::ToListMode)
        {
            return InternalValue();
        }
        if (const char* objectClass = val.GetObjectClass())
        {
            throw TemplateRuntimeError("'"s + objectClass + "' object is not iterable");
        }

        auto keys = val.GetKeyValues();
        auto numKeys = keys.size();
        return ListAdapter::CreateAdapter(numKeys, [values = std::move(keys)](size_t idx) { return values[idx]; });
    }

    template<typename T>
    static T GetAs(const InternalValue& val, T defValue = 0)
    {
        ConverterParams params;
        params.mode = ValueConverter::ToIntMode;
        params.base = static_cast<int64_t>(10);
        InternalValue intVal = Apply<ValueConverterImpl>(val, params);
        const T* result = GetIf<int64_t>(&intVal);
        if (!result)
        {
            return defValue;
        }

        return *result;
    }

    ConverterParams m_params;
};

namespace
{

// The text of `str` without surrounding ASCII whitespace as CPython's Py_ISSPACE knows it: not
// the \x1c-\x1f separators, which str.isspace() accepts but int() and float() do not
std::string StripAsciiSpace(const std::string& str)
{
    auto isSpace = [](char ch) { return ch == ' ' || (ch >= '\t' && ch <= '\r'); };
    auto first = std::find_if_not(str.begin(), str.end(), isSpace);
    auto last = std::find_if_not(str.rbegin(), std::string::const_reverse_iterator(first), isSpace).base();
    return std::string(first, last);
}

// An optional leading sign at `pos`, consumed; returns whether it is '-'
bool ReadNumberSign(const std::string& text, size_t& pos)
{
    bool negative = false;
    if (pos < text.size() && (text[pos] == '+' || text[pos] == '-'))
    {
        negative = text[pos++] == '-';
    }
    return negative;
}

// inf/infinity/nan in any case after the sign at `pos`
std::optional<double> ParseFloatSpecial(const std::string& text, size_t pos, bool negative)
{
    std::string word;
    for (auto n = pos; n < text.size(); ++n)
    {
        word.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(text[n]))));
    }
    if (word == "inf" || word == "infinity")
    {
        return negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    }
    if (word == "nan")
    {
        return std::numeric_limits<double>::quiet_NaN();
    }
    return std::nullopt;
}

// \d(_?\d)*, appended to `digits` without the underscores; returns the number of digits read
size_t ReadFloatDigits(const std::string& text, size_t& pos, std::string& digits)
{
    size_t count = 0;
    while (pos < text.size())
    {
        if (std::isdigit(static_cast<unsigned char>(text[pos])))
        {
            digits.push_back(text[pos++]);
            ++count;
        }
        else if (text[pos] == '_' && count != 0 && pos + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[pos + 1])))
        {
            ++pos;
        }
        else
        {
            break;
        }
    }
    return count;
}

// The mantissa, an optional fraction and an optional exponent from `pos` to the end of `text`,
// appended to `digits`; false unless all of the text is a decimal float literal
bool ReadFloatLiteral(const std::string& text, size_t pos, std::string& digits, bool& negativeExponent)
{
    auto mantissa = ReadFloatDigits(text, pos, digits);
    if (pos < text.size() && text[pos] == '.')
    {
        digits.push_back(text[pos++]);
        mantissa += ReadFloatDigits(text, pos, digits);
    }
    if (mantissa == 0)
    {
        return false;
    }
    if (pos < text.size() && (text[pos] == 'e' || text[pos] == 'E'))
    {
        digits.push_back(text[pos++]);
        if (pos < text.size() && (text[pos] == '+' || text[pos] == '-'))
        {
            negativeExponent = text[pos] == '-';
            digits.push_back(text[pos++]);
        }
        if (ReadFloatDigits(text, pos, digits) == 0)
        {
            return false;
        }
    }
    return pos == text.size();
}

// One code point of CPython's _PyUnicode_TransformDecimalAndSpaceToASCII: a Unicode decimal
// digit becomes its ASCII digit, a Unicode space a blank, and any other non-ASCII character
// fails the parse
bool AppendTransformed(std::string& result, uint32_t cp)
{
    if (cp < 0x80)
    {
        result.push_back(static_cast<char>(cp));
        return true;
    }
    if (unicode::IsSpace(cp))
    {
        result.push_back(' ');
        return true;
    }
    // Decimal digits come in runs of ten from the digit zero
    const auto* it = std::upper_bound(std::begin(unicode::DecimalRanges), std::end(unicode::DecimalRanges), cp, [](uint32_t value, const unicode::CodePointRange& r) { return value < r.first; });
    if (it == std::begin(unicode::DecimalRanges) || cp > std::prev(it)->last)
    {
        return false;
    }
    result.push_back(static_cast<char>('0' + ((cp - std::prev(it)->first) % 10)));
    return true;
}

// The transform of a UTF-8 text; false when it holds a character int() and float() reject
bool TransformUtf8(std::string_view str, std::string& result)
{
    result.reserve(str.size());
    for (size_t pos = 0; pos < str.size();)
    {
        auto lead = static_cast<unsigned char>(str[pos]);
        if (lead < 0x80)
        {
            result.push_back(str[pos++]);
            continue;
        }
        size_t length = lead >= 0xF0 ? 4 : (lead >= 0xE0 ? 3 : 2);
        if (lead < 0xC0 || pos + length > str.size())
        {
            return false;
        }
        uint32_t cp = lead & (0x3FU >> (length - 1));
        for (size_t n = 1; n < length; ++n)
        {
            auto tail = static_cast<unsigned char>(str[pos + n]);
            if ((tail & 0xC0) != 0x80)
            {
                return false;
            }
            cp = (cp << 6) | (tail & 0x3F);
        }
        pos += length;
        if (!AppendTransformed(result, cp))
        {
            return false;
        }
    }
    return true;
}

// The same for wchar_t text, UTF-16 where wchar_t has 16 bits
bool TransformWide(std::wstring_view str, std::string& result)
{
    for (size_t pos = 0; pos < str.size(); ++pos)
    {
        auto cp = static_cast<uint32_t>(static_cast<std::make_unsigned_t<wchar_t>>(str[pos]));
        if (sizeof(wchar_t) == 2 && cp >= 0xD800 && cp < 0xDC00 && pos + 1 < str.size())
        {
            auto low = static_cast<uint32_t>(static_cast<std::make_unsigned_t<wchar_t>>(str[pos + 1]));
            if (low >= 0xDC00 && low < 0xE000)
            {
                cp = 0x10000 + ((cp - 0xD800) << 10) + (low - 0xDC00);
                ++pos;
            }
        }
        if (!AppendTransformed(result, cp))
        {
            return false;
        }
    }
    return true;
}

// The text int() and float() read for a string value, whatever its width: the ASCII text
// they parse, or nullopt for a value that is no string or has characters they reject
std::optional<std::string> NumberTextOf(const InternalValue& val)
{
    std::string result;
    if (auto narrow = AsStringView<char>(val))
    {
        return TransformUtf8(*narrow, result) ? std::optional<std::string>(std::move(result)) : std::nullopt;
    }
    if (auto wide = AsStringView<wchar_t>(val))
    {
        return TransformWide(*wide, result) ? std::optional<std::string>(std::move(result)) : std::nullopt;
    }
    // A host string reference
    auto text = GetAsSameString(std::string(), val);
    return text && TransformUtf8(*text, result) ? std::optional<std::string>(std::move(result)) : std::nullopt;
}

// Python's float() of a string: surrounding whitespace, an optional sign, decimal digits with
// single underscores between them, an optional exponent, or inf/infinity/nan in any case.
// The text is ASCII already (NumberTextOf).
std::optional<double> ParsePythonFloat(const std::string& str)
{
    std::string text = StripAsciiSpace(str);

    size_t pos = 0;
    bool negative = ReadNumberSign(text, pos);
    if (auto special = ParseFloatSpecial(text, pos, negative))
    {
        return special;
    }

    std::string digits(text.begin(), text.begin() + static_cast<std::ptrdiff_t>(pos));
    bool negativeExponent = false;
    if (!ReadFloatLiteral(text, pos, digits, negativeExponent))
    {
        return std::nullopt;
    }

    std::istringstream is(digits);
    is.imbue(std::locale::classic());
    double result = 0;
    is >> result;
    // A valid literal fails only out of range: overflow is inf, underflow is zero
    if (is.fail())
    {
        result = negativeExponent ? 0.0 : std::numeric_limits<double>::infinity();
    }
    return negative ? -std::abs(result) : std::abs(result);
}

char LowerAscii(char ch)
{
    return static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
}

// A 0x/0o/0b prefix at `pos` that `base` (0 or the prefix's base) accepts: consumed, and
// `base` set to the prefix's base; returns whether there was one
bool ReadIntPrefix(const std::string& text, size_t& pos, int64_t& base)
{
    if (pos + 1 >= text.size() || text[pos] != '0')
    {
        return false;
    }
    int64_t prefixBase = 0;
    switch (LowerAscii(text[pos + 1]))
    {
    case 'x':
        prefixBase = 16;
        break;
    case 'o':
        prefixBase = 8;
        break;
    case 'b':
        prefixBase = 2;
        break;
    default:
        break;
    }
    if (prefixBase != 0 && (base == 0 || base == prefixBase))
    {
        base = prefixBase;
        pos += 2;
        return true;
    }
    return false;
}

// The value of the lowercase digit `ch`, or `base` when it is not a digit at all
int64_t IntDigitValue(char ch, int64_t base)
{
    if (ch >= '0' && ch <= '9')
    {
        return ch - '0';
    }
    if (ch >= 'a' && ch <= 'z')
    {
        return ch - 'a' + 10;
    }
    return base;
}

// The digits from `pos` to the end of `text` in `base`, with single underscores between
// them; nullopt on a bad digit or underscore, no digits or uint64 overflow
std::optional<uint64_t> ReadIntDigits(const std::string& text, size_t pos, int64_t base, bool hasPrefix, bool& nonZero)
{
    uint64_t value = 0;
    size_t digits = 0;
    bool overflow = false;
    // An underscore must follow a digit or the prefix ("0x_1f") and precede a digit
    bool underscoreAllowed = hasPrefix;
    bool lastUnderscore = false;
    for (; pos < text.size(); ++pos)
    {
        auto ch = LowerAscii(text[pos]);
        if (ch == '_')
        {
            if (!underscoreAllowed)
            {
                return std::nullopt;
            }
            underscoreAllowed = false;
            lastUnderscore = true;
            continue;
        }
        int64_t digit = IntDigitValue(ch, base);
        if (digit >= base)
        {
            return std::nullopt;
        }
        underscoreAllowed = true;
        lastUnderscore = false;
        ++digits;
        nonZero = nonZero || digit != 0;
        if (value > (std::numeric_limits<uint64_t>::max() - static_cast<uint64_t>(digit)) / static_cast<uint64_t>(base))
        {
            overflow = true;
        }
        value = (value * static_cast<uint64_t>(base)) + static_cast<uint64_t>(digit);
    }
    if (digits == 0 || lastUnderscore || overflow)
    {
        return std::nullopt;
    }
    return value;
}

// Python's int() of a string in `base` (0, or 2 to 36): surrounding whitespace, a sign, digits
// with single underscores between them and, for base 0 or a matching base, a 0x/0o/0b prefix.
// Values out of the int64 range are not supported and fail.
std::optional<int64_t> ParsePythonInt(const std::string& str, int64_t base)
{
    if (base != 0 && (base < 2 || base > 36))
    {
        return std::nullopt;
    }
    std::string text = StripAsciiSpace(str);

    size_t pos = 0;
    bool negative = ReadNumberSign(text, pos);
    bool hasPrefix = ReadIntPrefix(text, pos, base);
    // Base 0 without a prefix is decimal, where a leading zero is allowed only in zero itself
    bool decimalGuess = base == 0;
    if (decimalGuess)
    {
        base = 10;
    }

    bool nonZero = false;
    auto value = ReadIntDigits(text, pos, base, hasPrefix, nonZero);
    if (!value)
    {
        return std::nullopt;
    }
    if (decimalGuess && nonZero && text[text.find_first_of("0123456789")] == '0')
    {
        return std::nullopt;
    }
    auto limit = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + (negative ? 1 : 0);
    if (*value > limit)
    {
        return std::nullopt;
    }
    return negative ? static_cast<int64_t>(0 - *value) : static_cast<int64_t>(*value);
}

// Python's round(x, ndigits) for a float: the exact binary value rounded half to even at
// that decimal position
double PythonRound(double val, int64_t ndigits)
{
    if (!std::isfinite(val) || val == 0 || ndigits > 323)
    {
        return val;
    }
    if (ndigits < -308)
    {
        return std::copysign(0.0, val);
    }

    // The exact decimal expansion of val: a double has at most 53 - exponent fractional digits
    int exponent = 0;
    std::frexp(val, &exponent);
    auto exact = fmt::format("{:.{}f}", std::fabs(val), std::max(0, 53 - exponent));
    auto point = exact.find('.');
    auto intDigits = static_cast<int64_t>(point == std::string::npos ? exact.size() : point);
    std::string digits = exact;
    if (point != std::string::npos)
    {
        digits.erase(point, 1);
    }

    // Keep `keep` digits, round on the rest
    auto keep = intDigits + ndigits;
    if (keep < 0)
    {
        return std::copysign(0.0, val);
    }
    if (keep >= static_cast<int64_t>(digits.size()))
    {
        return val;
    }
    auto cut = static_cast<size_t>(keep);
    auto restNonZero = digits.find_first_not_of('0', cut + 1) != std::string::npos;
    bool roundUp = digits[cut] > '5' || (digits[cut] == '5' && (restNonZero || (cut > 0 && (digits[cut - 1] - '0') % 2 == 1)));
    std::string kept = "0" + digits.substr(0, cut);
    if (roundUp)
    {
        auto idx = kept.size();
        while (idx-- > 0)
        {
            if (kept[idx] == '9')
            {
                kept[idx] = '0';
            }
            else
            {
                ++kept[idx];
                break;
            }
        }
    }
    // No decimal point, so the locale's radix character does not matter
    auto text = kept + "e" + std::to_string(intDigits - keep);
    auto rounded = std::strtod(text.c_str(), nullptr);
    if (std::isinf(rounded))
    {
        throw TemplateRuntimeError("rounded value too large to represent");
    }
    return std::copysign(rounded, val);
}

// Python's round(n, ndigits) for an integer: unchanged for ndigits >= 0, else half to even.
// A result beyond int64 raises like any integer overflow
int64_t PythonRoundInt(int64_t val, int64_t ndigits)
{
    if (ndigits >= 0)
    {
        return val;
    }
    auto magnitude = val < 0 ? 0 - static_cast<uint64_t>(val) : static_cast<uint64_t>(val);
    // The unit is 10 ** -ndigits; from 10 ** 19 up it exceeds every magnitude doubled, and
    // the result is 0 up to half a unit and 10 ** 19 (not an int64) beyond it
    if (ndigits <= -19)
    {
        if (ndigits == -19 && magnitude > 5000000000000000000ULL)
        {
            visitors::ThrowIntegerOverflow();
        }
        return 0;
    }
    uint64_t unit = 1;
    for (int64_t n = 0; n != -ndigits; ++n)
    {
        unit *= 10;
    }
    auto quotient = magnitude / unit;
    auto remainder = magnitude % unit;
    if (remainder * 2 > unit || (remainder * 2 == unit && quotient % 2 == 1))
    {
        ++quotient;
    }
    // At most magnitude + unit, below 2 ** 64
    auto rounded = quotient * unit;
    if (rounded > static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + (val < 0 ? 1 : 0))
    {
        visitors::ThrowIntegerOverflow();
    }
    return val < 0 ? static_cast<int64_t>(0 - rounded) : static_cast<int64_t>(rounded);
}

// 10 ** precision as the float Python converts it to, correctly rounded; 0 when it underflows
double PowerOfTen(int64_t precision)
{
    auto text = "1e" + std::to_string(std::max<int64_t>(-400, std::min<int64_t>(precision, 400)));
    // No decimal point, so the locale's radix character does not matter
    return std::strtod(text.c_str(), nullptr);
}

// Jinja2's round(value, precision, 'ceil' or 'floor'): func(value * (10 ** precision)) / (10 ** precision)
double PythonRoundByMethod(double value, bool isInt, int64_t precision, bool isCeil)
{
    // An int times 10 ** precision is an int, which the true division brings back to the
    // value as exactly as a float can hold it
    if (isInt && precision >= 0)
    {
        return value;
    }
    if (precision > 308)
    {
        throw TemplateRuntimeError("int too large to convert to float");
    }
    auto scale = PowerOfTen(precision);
    auto scaled = value * scale;
    if (std::isnan(scaled))
    {
        throw TemplateRuntimeError("cannot convert float NaN to integer");
    }
    if (std::isinf(scaled))
    {
        throw TemplateRuntimeError("cannot convert float infinity to integer");
    }
    auto whole = isCeil ? std::ceil(scaled) : std::floor(scaled);
    if (precision < 0)
    {
        // The integer is divided by the float 10 ** precision
        if (scale == 0.0)
        {
            throw TemplateRuntimeError("float division by zero");
        }
        return (whole / scale) + 0.0;
    }
    // Two integers are divided exactly and the quotient rounded once: the digits of the
    // integer scaled down by the power of ten, read back by strtod
    auto text = fmt::format("{:.0f}e-{}", whole, precision);
    return std::strtod(text.c_str(), nullptr) + 0.0;
}

// Port of Jinja2's do_filesizeformat
std::string FormatFileSize(double bytes, bool binary)
{
    const double base = binary ? 1024 : 1000;
    static const char* const decimalPrefixes[] = { "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };
    static const char* const binaryPrefixes[] = { "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB" };
    const auto& prefixes = binary ? binaryPrefixes : decimalPrefixes;

    if (bytes == 1)
    {
        return "1 Byte";
    }
    // int(bytes) in Python: truncated, exact for any finite value, never "-0"
    if (bytes < base)
    {
        return fmt::format("{:.0f} Bytes", std::trunc(bytes) + 0.0);
    }

    // Python compares bytes with the exact integer base ** k and divides by it rounded to a
    // double. 1000 ** k = 5 ** 3k * 2 ** 3k, and 5 ** 3k fits in 64 bits for every prefix.
    double unit = base;
    const char* prefix = prefixes[0];
    uint64_t fives = 15625;
    for (int k = 2; k != 10; ++k, fives *= 125)
    {
        prefix = prefixes[k - 2];
        bool less = false;
        if (binary)
        {
            unit = std::ldexp(1.0, 10 * k);
            less = bytes < unit;
        }
        else
        {
            unit = std::ldexp(static_cast<double>(fives), 3 * k);
            // bytes < 5 ** 3k * 2 ** 3k  <=>  floor(bytes / 2 ** 3k) < 5 ** 3k, exactly
            auto scaled = std::floor(std::ldexp(bytes, -3 * k));
            less = scaled < 0 || (scaled < 18446744073709551616.0 && static_cast<uint64_t>(scaled) < fives);
        }
        if (less)
        {
            break;
        }
    }
    return fmt::format("{:.1f} {}", base * bytes / unit, prefix);
}

// bool is an int in Python
std::optional<int64_t> PythonIntOf(const InternalValue& val)
{
    if (const auto* intVal = GetIf<int64_t>(&val))
    {
        return *intVal;
    }
    if (const auto* boolVal = GetIf<bool>(&val))
    {
        return *boolVal ? 1 : 0;
    }
    return std::nullopt;
}

// int() of inf or nan fails; larger values are not supported
std::optional<int64_t> FloatToInt(double val)
{
    if (!std::isfinite(val) || std::fabs(val) >= 9223372036854775808.0)
    {
        return std::nullopt;
    }
    return static_cast<int64_t>(val);
}

} // namespace

InternalValue ValueConverter::FileSizeFormat(const InternalValue& baseVal, RenderContext& context)
{
    std::optional<double> bytes;
    if (const auto* intVal = GetIf<int64_t>(&baseVal))
    {
        bytes = static_cast<double>(*intVal);
    }
    else if (const auto* dblVal = GetIf<double>(&baseVal))
    {
        bytes = *dblVal;
    }
    else if (const auto* boolVal = GetIf<bool>(&baseVal))
    {
        bytes = *boolVal ? 1.0 : 0.0;
    }
    else if (auto str = NumberTextOf(baseVal))
    {
        bytes = ParsePythonFloat(*str);
    }
    // Python raises on what float() rejects, and int(-inf) overflows
    if (!bytes || (std::isinf(*bytes) && *bytes < 0))
    {
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
    }
    return InternalValue(FormatFileSize(*bytes, ConvertToBool(GetArgumentValue("binary", context))));
}

InternalValue ValueConverter::Items(const InternalValue& baseVal, RenderContext& context)
{
    // An undefined value yields no items, anything but a mapping is a TypeError
    if (baseVal.IsUndefined())
    {
        return ListAdapter::CreateAdapter(InternalValueList());
    }
    auto map = AsMap(baseVal);
    if (!map || map->GetObjectClass())
    {
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
    }
    InternalValueList items;
    for (auto& [key, value] : map->GetEntries())
    {
        items.emplace_back(ListAdapter::CreateAdapter(InternalValueList{ std::move(key), std::move(value) }).MarkAsTuple());
    }
    InternalValue result = ListAdapter::CreateAdapter(std::move(items));
    if (baseVal.ShouldExtendLifetime())
    {
        result.SetParentData(baseVal);
    }
    return result;
}

InternalValue ValueConverter::ToInt(const InternalValue& baseVal, RenderContext& context)
{
    // Jinja2's do_int: int(value[, base]), then int(float(value)), then the default
    if (auto asInt = PythonIntOf(baseVal))
    {
        return *asInt;
    }
    std::optional<int64_t> result;
    if (const auto* dblVal = GetIf<double>(&baseVal))
    {
        result = FloatToInt(*dblVal);
    }
    else if (auto str = NumberTextOf(baseVal))
    {
        result = ParsePythonInt(*str, ConvertToInt(GetArgumentValue("base", context)));
        if (!result)
        {
            auto asFloat = ParsePythonFloat(*str);
            if (asFloat)
            {
                result = FloatToInt(*asFloat);
            }
        }
    }
    if (result)
    {
        return *result;
    }
    return GetArgumentValue("default", context);
}

InternalValue ValueConverter::ToFloat(const InternalValue& baseVal, RenderContext& context)
{
    // Jinja2's do_float: float(value), else the default
    if (auto asInt = PythonIntOf(baseVal))
    {
        return static_cast<double>(*asInt);
    }
    if (const auto* dblVal = GetIf<double>(&baseVal))
    {
        return *dblVal;
    }
    if (auto str = NumberTextOf(baseVal))
    {
        if (auto result = ParsePythonFloat(*str))
        {
            return *result;
        }
    }
    return GetArgumentValue("default", context);
}

InternalValue ValueConverter::Abs(const InternalValue& baseVal, RenderContext& context)
{
    if (auto asInt = PythonIntOf(baseVal))
    {
        return static_cast<int64_t>(*asInt < 0 ? 0 - static_cast<uint64_t>(*asInt) : static_cast<uint64_t>(*asInt));
    }
    if (const auto* dblVal = GetIf<double>(&baseVal))
    {
        return std::fabs(*dblVal);
    }
    // Python's abs() of anything else is a TypeError
    context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
    return InternalValue();
}

namespace
{
// Python's round(x) of a float: the nearest int, halves to even; integers are int64 (0178)
InternalValue PythonRoundToInt(double value)
{
    if (std::isnan(value))
    {
        throw TemplateRuntimeError("cannot convert float NaN to integer");
    }
    if (std::isinf(value))
    {
        throw TemplateRuntimeError("cannot convert float infinity to integer");
    }
    // The default rounding mode rounds halves to even
    const double rounded = std::nearbyint(value);
    // -2**63 is exact; 2**63 is the first double beyond int64
    if (rounded < -9223372036854775808.0 || rounded >= 9223372036854775808.0)
    {
        visitors::ThrowIntegerOverflow();
    }
    return static_cast<int64_t>(rounded);
}
} // namespace

InternalValue ValueConverter::Round(const InternalValue& baseVal, RenderContext& context)
{
    // Jinja2's do_round: round(value, precision), or math.ceil/floor at that precision
    auto method = AsString(GetArgumentValue("method", context));
    if (method != "common" && method != "ceil" && method != "floor")
    {
        throw TemplateRuntimeError("round(): method must be common, ceil or floor");
    }
    auto precVal = GetArgumentValue("precision", context);
    const bool noPrecision = precVal.Kind() == ValueKind::None;
    if (!noPrecision && !GetIf<int64_t>(&precVal) && !GetIf<bool>(&precVal))
    {
        throw TemplateRuntimeError("round(): precision must be an integer");
    }
    auto asInt = PythonIntOf(baseVal);
    const auto* dblVal = GetIf<double>(&baseVal);
    if (!asInt && !dblVal)
    {
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
    }
    if (noPrecision)
    {
        // round(value, None) is round(value): an int, halves to even
        if (method != "common")
        {
            throw TemplateRuntimeError("unsupported operand type(s) for ** or pow(): 'int' and 'NoneType'");
        }
        if (asInt)
        {
            return *asInt;
        }
        return PythonRoundToInt(*dblVal);
    }
    auto precision = ConvertToInt(precVal);
    if (method == "common")
    {
        // round() of an int is an int
        if (asInt)
        {
            return PythonRoundInt(*asInt, precision);
        }
        return PythonRound(*dblVal, precision);
    }
    double value = asInt ? static_cast<double>(*asInt) : *dblVal;
    return PythonRoundByMethod(value, asInt.has_value(), precision, method == "ceil");
}

InternalValue ValueConverter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    switch (m_mode)
    {
    case FileSizeFormatMode:
        return FileSizeFormat(baseVal, context);
    case ItemsMode:
        return Items(baseVal, context);
    case ToIntMode:
        // int() and float() of a named undefined fail as Python's __int__/__float__ do
        CheckUndefinedUse(baseVal, UndefinedUse::Arithmetic);
        return ToInt(baseVal, context);
    case ToFloatMode:
        CheckUndefinedUse(baseVal, UndefinedUse::Arithmetic);
        return ToFloat(baseVal, context);
    case AbsMode:
        return Abs(baseVal, context);
    case RoundMode:
        return Round(baseVal, context);
    case ToListMode:
        // list() of undefined is empty; StrictUndefined refuses
        if (baseVal.IsUndefined())
        {
            bool isConverted = false;
            return ListAdapter(ConvertToList(baseVal, isConverted));
        }
        if (IsNotIterable(baseVal))
        {
            ThrowNotIterable(baseVal);
        }
        break;
    }

    ConverterParams params;
    params.mode = m_mode;
    params.defValule = GetArgumentValue("default", context);
    params.base = GetArgumentValue("base", context);
    params.prec = GetArgumentValue("precision", context);
    params.roundMethod = GetArgumentValue("method", context);
    auto result = Apply<ValueConverterImpl>(baseVal, params);
    if (baseVal.ShouldExtendLifetime())
    {
        result.SetParentData(baseVal);
    }

    return result;
}

UserDefinedFilter::UserDefinedFilter(std::string filterName, const FilterParams& params, InternalValue callable)
    : m_filterName(std::move(filterName))
    , m_callable(std::move(callable))
{
    static const auto args = MakeArgumentsTable({ { "*args" }, { "**kwargs" } });
    ParseParams(args, params, ExtraArgs::Reject, &m_callParams);
}

namespace
{
[[noreturn]] void ThrowUnknownFilter(const std::string& name)
{
    throw TemplateRuntimeError("No filter named '" + name + "' found.");
}
} // namespace

InternalValue UserDefinedFilter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const Callable* callable = GetIf<Callable>(&m_callable);
    if (!callable)
    {
        // Neither built in nor added to the environment: Jinja2 has checked the name at load
        // unless it is in an `if`. Settings::userCallablesAsFilters looks it up among the values
        if (!context.UserCallablesAsFilters())
        {
            ThrowUnknownFilter(m_filterName);
        }
        const auto filterVal = context.FindValue(m_filterName);
        if (!filterVal)
        {
            ThrowUnknownFilter(m_filterName);
        }
        callable = GetIf<Callable>(&*filterVal);
    }
    if (!callable || callable->GetKind() != Callable::UserCallable)
    {
        return InternalValue();
    }

    CallParams tmpCallParams = helpers::EvaluateCallParams(m_callParams, context);
    CallParams callParams;
    callParams.kwParams = std::move(tmpCallParams.kwParams);
    callParams.posParams.reserve(tmpCallParams.posParams.size() + 1);
    callParams.posParams.push_back(baseVal);
    for (auto& p : tmpCallParams.posParams)
    {
        callParams.posParams.push_back(std::move(p));
    }

    InternalValue result;
    if (callable->GetType() != Callable::Type::Expression)
    {
        return InternalValue();
    }

    return callable->GetExpressionCallable()(callParams, context);
}

} // namespace filters
} // namespace v2
} // namespace jinja2
