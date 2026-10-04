#include "filters.h"

#include "expression_evaluator.h"
#include "internal_value.h"
#include "markup.h"
#include "out_stream.h" // IWYU pragma: keep (GetStreamOnString returns an OutStream by value)
#include "render_context.h"
#include "testers.h"
#include "undefined.h"
#include "unicode_tables.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cwchar>
#include <iterator>
#include <limits>
#include <locale>
#include <memory>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{

template<typename F>
struct FilterFactory
{
    static FilterPtr Create(const FilterParams& params) { return std::make_shared<F>(params); }

    template<typename... Args>
    static ExpressionFilter::FilterFactoryFn MakeCreator(const Args&... args)
    {
        return [args...](const FilterParams& params) { return std::make_shared<F>(params, args...); };
    }
};

// NOLINTNEXTLINE(bugprone-throwing-static-initialization): only allocation can throw here, at load time
std::unordered_map<std::string, ExpressionFilter::FilterFactoryFn> s_filters = {
    { "abs", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::AbsMode) },
    { "applymacro", &FilterFactory<filters::ApplyMacro>::Create },
    { "attr", &FilterFactory<filters::Attribute>::Create },
    { "batch", FilterFactory<filters::Slice>::MakeCreator(filters::Slice::BatchMode) },
    { "camelize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::CamelMode) },
    { "capitalize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::CapitalMode) },
    { "center", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::CenterMode) },
    { "count", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::LengthMode) },
    { "default", &FilterFactory<filters::Default>::Create },
    { "d", &FilterFactory<filters::Default>::Create },
    { "dictsort", &FilterFactory<filters::DictSort>::Create },
    { "e", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::EscapeHtmlMode) },
    { "escape", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::EscapeHtmlMode) },
    { "escapecpp", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::EscapeCppMode) },
    { "filesizeformat", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::FileSizeFormatMode) },
    { "first", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::FirstItemMode) },
    { "float", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ToFloatMode) },
    { "forceescape", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::ForceEscapeMode) },
    { "format", FilterFactory<filters::StringFormat>::Create },
    { "groupby", &FilterFactory<filters::GroupBy>::Create },
    { "indent", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::IndentMode) },
    { "int", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ToIntMode) },
    { "items", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ItemsMode) },
    { "join", &FilterFactory<filters::Join>::Create },
    { "last", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::LastItemMode) },
    { "length", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::LengthMode) },
    { "list", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ToListMode) },
    { "lower", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::LowerMode) },
    { "map", &FilterFactory<filters::Map>::Create },
    { "max", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::MaxItemMode) },
    { "min", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::MinItemMode) },
    { "pprint", &FilterFactory<filters::PrettyPrint>::Create },
    { "random", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::RandomMode) },
    { "reject", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::RejectMode) },
    { "rejectattr", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::RejectAttrMode) },
    { "replace", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::ReplaceMode) },
    { "round", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::RoundMode) },
    { "reverse", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::ReverseMode) },
    { "safe", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::SafeMode) },
    { "select", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::SelectMode) },
    { "selectattr", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::SelectAttrMode) },
    { "slice", FilterFactory<filters::Slice>::MakeCreator(filters::Slice::SliceMode) },
    { "sort", &FilterFactory<filters::Sort>::Create },
    { "string", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::ToStringMode) },
    { "striptags", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::StriptagsMode) },
    { "sum", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::SumItemsMode) },
    { "title", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::TitleMode) },
    { "tojson", FilterFactory<filters::Serialize>::MakeCreator(filters::Serialize::JsonMode) },
    { "toxml", FilterFactory<filters::Serialize>::MakeCreator(filters::Serialize::XmlMode) },
    { "toyaml", FilterFactory<filters::Serialize>::MakeCreator(filters::Serialize::YamlMode) },
    { "trim", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::TrimMode) },
    { "truncate", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::TruncateMode) },
    { "unique", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::UniqueItemsMode) },
    { "upper", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UpperMode) },
    { "urlencode", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UrlEncodeMode) },
    { "urlize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UrlizeMode) },
    { "wordcount", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::WordCountMode) },
    { "wordwrap", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::WordWrapMode) },
    { "underscorize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UnderscoreMode) },
    { "xmlattr", &FilterFactory<filters::XmlAttrFilter>::Create }
};

extern FilterPtr CreateFilter(std::string filterName, CallParamsInfo params)
{
    auto p = s_filters.find(filterName);
    if (p == s_filters.end())
    {
        return std::make_shared<filters::UserDefinedFilter>(std::move(filterName), std::move(params));
    }

    return p->second(std::move(params));
}

FilterPtr CreateFilter(std::string filterName, CallParamsInfo params, RenderContext& context)
{
    auto* env = context.GetEnv();
    auto registered = env ? env->FindFilter(filterName) : std::optional<UserCallable>();
    if (!registered)
    {
        return CreateFilter(std::move(filterName), std::move(params));
    }
    auto callable = visitors::InputValueConvertor::ConvertUserCallable(*registered);
    return std::make_shared<filters::UserDefinedFilter>(std::move(filterName), std::move(params), std::move(callable));
}

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
    const auto& data = val.GetData();
    if (GetIf<int64_t>(&val) || GetIf<double>(&val) || GetIf<bool>(&val))
    {
        return OrderKind::Number;
    }
    if (std::get_if<std::string>(&data) || std::get_if<TargetString>(&data) || std::get_if<TargetStringView>(&data))
    {
        return OrderKind::String;
    }
    if (GetIf<ListAdapter>(&val) || GetIf<KeyValuePair>(&val))
    {
        return OrderKind::Sequence;
    }
    return OrderKind::Unordered;
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
            throw std::runtime_error("'<' not supported between these values");
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
        if (isIndex)
        {
            parts.emplace_back(static_cast<int64_t>(std::stoll(part)));
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
        result = Subscript(result, part, &context);
        if (result.IsUndefined() && !IsEmpty(defaultVal))
        {
            result = defaultVal;
        }
    }
    return result;
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
    ParseParams({ { "d", false, std::string() }, { "attribute" } }, params);
}

InternalValue Join::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue attrName = GetArgumentValue("attribute", context);

    bool isConverted = false;
    ListAdapter values = ConvertToList(baseVal, attrName, isConverted, false);

    if (!isConverted)
    {
        return InternalValue();
    }

    // Python join converts every item and the delimiter with str(). Under autoescape a
    // Markup delimiter or item makes it a Markup join, which escapes the rest
    auto* renderer = context.GetRendererCallback();
    InternalValue delimiterVal = m_args["d"]->Evaluate(context);
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
    auto asText = [renderer, isMarkup](const InternalValue& val) {
        return isMarkup ? MarkupEscape(val, renderer) : InternalValue(renderer->GetAsTargetString(val));
    };
    bool isFirst = true;
    InternalValue result;
    InternalValue delimiter = asText(delimiterVal);
    for (const InternalValue& val : values)
    {
        if (isFirst)
        {
            isFirst = false;
        }
        else
        {
            result = Apply2<visitors::StringJoiner>(result, delimiter);
        }

        result = Apply2<visitors::StringJoiner>(result, asText(val));
    }

    result.SetMarkup(isMarkup);
    return result;
}

Sort::Sort(const FilterParams& params)
{
    ParseParams({ { "reverse", false, InternalValue(false) }, { "case_sensitive", false, InternalValue(false) }, { "attribute", false } }, params);
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
        return InternalValue();
    }
    InternalValueList values = origValues.ToValueList();

    BinaryExpression::Operation oper = ConvertToBool(isReverseVal) ? BinaryExpression::LogicalGt : BinaryExpression::LogicalLt;
    BinaryExpression::CompareType compType = ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    std::vector<InternalValueList> paths = SortKeyPaths(attrName);

    // Python's sorted() is stable
    std::stable_sort(values.begin(), values.end(), [&paths, oper, compType, &context](auto& val1, auto& val2) {
        auto isLess = [oper, compType](const InternalValue& key1, const InternalValue& key2, bool& isEqual) {
            // Equal items never reach `<`, so a list of equal dicts or Nones sorts as in Python
            isEqual = ConvertToBool(Apply2<visitors::BinaryMathOperation>(key1, key2, BinaryExpression::LogicalEq, compType));
            return !isEqual && ConvertToBool(Apply2<visitors::BinaryMathOperation>(key1, key2, oper, compType));
        };
        bool isEqual = false;
        if (paths.empty())
        {
            return isLess(val1, val2, isEqual);
        }
        // The key lists compare item by item, like Python lists
        for (auto& path : paths)
        {
            auto result = isLess(GetAttributeByPath(val1, path, InternalValue(), context), GetAttributeByPath(val2, path, InternalValue(), context), isEqual);
            if (!isEqual)
            {
                return result;
            }
        }
        return false;
    });

    return ListAdapter::CreateAdapter(std::move(values));
}

Attribute::Attribute(const FilterParams& params)
{
    ParseParams({ { "name", true }, { "default", false } }, params);
}

InternalValue Attribute::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const auto attrNameVal = GetArgumentValue("name", context);
    CheckUndefinedUse(baseVal, UndefinedUse::Attribute);
    // Python's attr reads attributes only: the items of a dict are not attributes, the fields
    // of a reflected object are
    const auto* map = GetIf<MapAdapter>(&baseVal);
    if (map && !map->HasAttributes())
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
    ParseParams({ { "default_value", false, InternalValue(""s) }, { "boolean", false, InternalValue(false) } }, params);
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
    ParseParams({ { "case_sensitive", false }, { "by", false, "key"s }, { "reverse", false } }, params);
}

InternalValue DictSort::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const MapAdapter* map = GetIf<MapAdapter>(&baseVal);
    if (!map)
    {
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
        std::string sortKey;
    };
    bool (*comparator)(const SortItem& left, const SortItem& right) = nullptr;
    bool lowerKeys = false;

    if (AsString(byVal) == "key") // Sort by key
    {
        lowerKeys = !ConvertToBool(isCsVal);
        if (lowerKeys)
        {
            comparator = [](const SortItem& left, const SortItem& right) { return left.sortKey < right.sortKey; };
        }
        else
        {
            comparator = [](const SortItem& left, const SortItem& right) { return left.pair->key < right.pair->key; };
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
        return InternalValue();
    }

    auto entries = map->GetEntries();
    std::vector<SortItem> tempVector;
    tempVector.reserve(entries.size());
    for (auto& entry : entries)
    {
        SortItem item{ &entry, std::string() };
        if (lowerKeys)
        {
            const auto& key = entry.key;
            item.sortKey.reserve(key.size());
            std::transform(key.begin(), key.end(), std::back_inserter(item.sortKey), [](char ch) {
                return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch - 'A' + 'a') : ch;
            });
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
    ParseParams({ { "attribute", true }, { "default", false }, { "case_sensitive", false, false } }, params);
}

InternalValue GroupBy::Filter(const InternalValue& baseVal, RenderContext& context)
{
    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, isConverted, false);

    if (!isConverted)
    {
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
    for (const auto& item : list)
    {
        items.push_back(Item{ GetAttributeByPath(item, path, defaultVal, context), item });
    }

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
    ParseParams({ { "macro", true } }, params, ExtraArgs::Accept);
    m_mappingParams.kwParams = m_args.extraKwArgs;
    m_mappingParams.posParams = m_args.extraPosArgs;
}

InternalValue ApplyMacro::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue macroName = GetArgumentValue("macro", context);
    if (IsEmpty(macroName))
    {
        return InternalValue();
    }

    bool macroFound = false;
    auto macroValPtr = context.FindValue(AsString(macroName), macroFound);
    if (!macroFound)
    {
        return InternalValue();
    }

    const auto* callable = GetIf<Callable>(&macroValPtr->second);
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
        TargetString resultStr;
        auto stream = context.GetRendererCallback()->GetStreamOnString(resultStr);
        callable->GetStatementCallable()(callParams, stream, context);
        result = std::move(resultStr);
        result.SetMarkup(context.IsAutoescape());
    }

    return result;
}

Map::Map(FilterParams params)
{
    ParseParams({ { "filter", true } }, MakeParams(std::move(params)), ExtraArgs::Accept);
    m_mappingParams.kwParams = m_args.extraKwArgs;
    m_mappingParams.posParams = m_args.extraPosArgs;
}

FilterParams Map::MakeParams(FilterParams params)
{
    if (!params.posParams.empty() || params.kwParams.empty() || params.kwParams.size() > 2)
    {
        return params;
    }

    const auto attributeIt = params.kwParams.find("attribute");
    if (attributeIt == params.kwParams.cend())
    {
        return params;
    }

    FilterParams result;
    m_byAttribute = true;
    result.kwParams["name"] = attributeIt->second;
    result.kwParams["filter"] = std::make_shared<ConstantExpression>("attr"s);

    const auto defaultIt = params.kwParams.find("default");
    if (defaultIt != params.kwParams.cend())
    {
        result.kwParams["default"] = defaultIt->second;
    }

    return result;
}

InternalValue Map::Filter(const InternalValue& baseVal, RenderContext& context)
{
    if (m_byAttribute)
    {
        bool isConverted = false;
        auto list = ConvertToList(baseVal, isConverted, false);
        if (!isConverted)
        {
            return InternalValue();
        }

        auto params = helpers::EvaluateCallParams(m_mappingParams, context);
        auto path = AttributePath(params.kwParams["name"]);
        auto defaultVal = params.kwParams["default"];
        InternalValueList resultList;
        for (const auto& item : list)
        {
            resultList.push_back(GetAttributeByPath(item, path, defaultVal, context));
        }
        return ListAdapter::CreateAdapter(std::move(resultList));
    }

    InternalValue filterName = GetArgumentValue("filter", context);
    if (IsEmpty(filterName))
    {
        return InternalValue();
    }

    auto filter = CreateFilter(AsString(filterName), m_mappingParams, context);
    if (!filter)
    {
        return InternalValue();
    }

    bool isConverted = false;
    auto list = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
    {
        return InternalValue();
    }

    InternalValueList resultList;
    resultList.reserve(list.GetSize().value_or(0));
    std::transform(list.begin(), list.end(), std::back_inserter(resultList), [filter, &context](auto& val) { return filter->Filter(val, context); });

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
        ParseParams({}, params);
        break;
    case MaxItemMode:
    case MinItemMode:
        ParseParams({ { "case_sensitive", false, InternalValue(false) }, { "attribute", false } }, params);
        break;
    case SumItemsMode:
        ParseParams({ { "attribute", false }, { "start", false } }, params);
        break;
    case UniqueItemsMode:
        ParseParams({ { "case_sensitive", false, InternalValue(false) }, { "attribute", false } }, params);
        break;
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

InternalValue FirstItem(const ListAdapter& list, const InternalValue& baseVal)
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
    return {};
}

InternalValue LastItem(const ListAdapter& list, const InternalValue& baseVal)
{
    const auto& listSize = list.GetSize();
    if (listSize && *listSize > 0)
    {
        return WithParent(baseVal, list.GetValueByIndex(static_cast<int64_t>(listSize.value() - 1)));
    }
    InternalValue result;
    for (auto it = list.begin(), end = list.end(); it != end; ++it)
    {
        result = WithParent(baseVal, *it);
    }
    return result;
}

InternalValue Length(const ListAdapter& list)
{
    const auto& listSize = list.GetSize();
    if (listSize && *listSize > 0)
    {
        return static_cast<int64_t>(listSize.value());
    }
    return static_cast<int64_t>(std::distance(list.begin(), list.end()));
}

InternalValue RandomItem(const ListAdapter& list, const InternalValue& baseVal)
{
    std::random_device rd;
    std::mt19937 gen(rd());
    const auto& listSize = list.GetSize();
    if (listSize && *listSize > 0)
    {
        std::uniform_int_distribution<> dis(0, static_cast<int>(listSize.value()) - 1);
        return WithParent(baseVal, list.GetValueByIndex(dis(gen)));
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
    if (GetIf<TargetString>(&baseVal) || GetIf<TargetStringView>(&baseVal))
    {
        // Python reverses a string into a string
        return ApplyStringConverter(baseVal, [](auto strView) -> TargetString {
            auto chars = SplitCodePoints(strView);
            std::basic_string<typename decltype(strView)::value_type> reversed;
            reversed.reserve(strView.size());
            for (auto ch = chars.rbegin(); ch != chars.rend(); ++ch)
            {
                reversed.append(ch->begin(), ch->end());
            }
            return TargetString(std::move(reversed));
        });
    }

    const auto& listSize = list.GetSize();
    if (listSize)
    {
        auto size = listSize.value();
        InternalValueList resultList(size);
        for (std::size_t n = 0; n < size; ++n)
        {
            resultList[size - n - 1] = WithParent(baseVal, list.GetValueByIndex(static_cast<int64_t>(n)));
        }
        return ListAdapter::CreateAdapter(std::move(resultList));
    }

    InternalValueList resultList;
    for (auto it = list.begin(), end = list.end(); it != end; ++it)
    {
        resultList.push_back(WithParent(baseVal, *it));
    }
    std::reverse(resultList.begin(), resultList.end());
    return ListAdapter::CreateAdapter(std::move(resultList));
}

InternalValue Sum(const ListAdapter& list, const InternalValue& attrName, const InternalValue& start)
{
    ListAdapter subscripted;
    const ListAdapter* actualList = &list;
    if (!IsEmpty(attrName))
    {
        subscripted = list.ToSubscriptedList(attrName, true);
        actualList = &subscripted;
    }
    InternalValue resultVal = std::accumulate(actualList->begin(), actualList->end(), start, [](const InternalValue& cur, const InternalValue& val) {
        if (IsEmpty(cur))
        {
            return val;
        }

        return Apply2<visitors::BinaryMathOperation>(cur, val, BinaryExpression::Plus);
    });
    // Python's sum starts from 0
    if (resultVal.IsUndefined())
    {
        resultVal = static_cast<int64_t>(0);
    }
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
    auto byIndex = [](const UniqueItem& i1, const UniqueItem& i2) { return i1.idx < i2.idx; };
    try
    {
        std::stable_sort(items.begin(), items.end(), [compType](const UniqueItem& i1, const UniqueItem& i2) {
            return ConvertToBool(Apply2<visitors::BinaryMathOperation>(i1.val, i2.val, BinaryExpression::LogicalLt, compType));
        });
        items.erase(std::unique(items.begin(), items.end(), isEqual), items.end());
    }
    catch (const std::runtime_error&)
    {
        // Unorderable items (mixed types, None, dicts): Python hashes them, so keep the
        // first of each run of equal items in a quadratic pass instead
        std::stable_sort(items.begin(), items.end(), byIndex);
        std::vector<UniqueItem> uniqueItems;
        for (auto& item : items)
        {
            if (std::none_of(uniqueItems.begin(), uniqueItems.end(), [&](const UniqueItem& u) { return isEqual(u, item); }))
            {
                uniqueItems.push_back(item);
            }
        }
        items = std::move(uniqueItems);
    }

    std::stable_sort(items.begin(), items.end(), byIndex);
}

InternalValue Unique(const ListAdapter& list,
                     const InternalValue& baseVal,
                     const InternalValue& attrName,
                     BinaryExpression::CompareType compType,
                     RenderContext& context)
{
    std::vector<UniqueItem> items;
    int idx = 0;
    for (const auto& v : list)
    {
        items.push_back(UniqueItem{ IsEmpty(attrName) ? v : Subscript(v, attrName, &context), idx++ });
    }

    DropDuplicates(items, compType);

    InternalValueList resultList;
    for (auto& i : items)
    {
        resultList.push_back(WithParent(baseVal, list.GetValueByIndex(i.idx)));
    }
    return ListAdapter::CreateAdapter(std::move(resultList));
}
} // namespace

InternalValue SequenceAccessor::Filter(const InternalValue& baseVal, RenderContext& context)
{
    // Like Python, a string is a sequence of characters and a mapping one of its keys
    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, isConverted, false);

    if (!isConverted)
    {
        return {};
    }

    InternalValue attrName = GetArgumentValue("attribute", context);
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context, InternalValue(false));

    BinaryExpression::CompareType compType = ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    auto lessComparator = [&attrName, &compType, &context](auto& val1, auto& val2) {
        if (IsEmpty(attrName))
        {
            return CompareForOrder(val1, val2, BinaryExpression::LogicalLt, compType);
        }
        return CompareForOrder(Subscript(val1, attrName, &context), Subscript(val2, attrName, &context), BinaryExpression::LogicalLt, compType);
    };

    switch (m_mode)
    {
    case FirstItemMode:
        return FirstItem(list, baseVal);
    case LastItemMode:
        return LastItem(list, baseVal);
    case LengthMode:
        return Length(list);
    case RandomMode:
        return RandomItem(list, baseVal);
    case MaxItemMode:
    {
        auto e = list.end();
        auto p = std::max_element(list.begin(), e, lessComparator);
        return p != e ? WithParent(baseVal, *p) : InternalValue();
    }
    case MinItemMode:
    {
        auto e = list.end();
        auto p = std::min_element(list.begin(), e, lessComparator);
        return p != e ? WithParent(baseVal, *p) : InternalValue();
    }
    case ReverseMode:
        return Reverse(list, baseVal);
    case SumItemsMode:
        return Sum(list, attrName, GetArgumentValue("start", context));
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
        ParseParams({ { "linecount"s, true }, { "fill_with"s, false } }, params);
    }
    else
    {
        ParseParams({ { "slices"s, true }, { "fill_with"s, false } }, params);
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
        return InternalValue();
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
        return InternalValue();
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
    ParseParams({}, params, ExtraArgs::Accept);
    m_params.kwParams = std::move(m_args.extraKwArgs);
    m_params.posParams = std::move(m_args.extraPosArgs);
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
        ParseParams({ { "tester", false } }, params, ExtraArgs::Accept);
    }
    else
    {
        ParseParams({ { "attribute", true }, { "tester", false } }, params, ExtraArgs::Accept);
    }

    m_testingParams.kwParams = std::move(m_args.extraKwArgs);
    m_testingParams.posParams = std::move(m_args.extraPosArgs);
}

InternalValue Tester::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue testerName = GetArgumentValue("tester", context);
    InternalValue attrName = GetArgumentValue("attribute", context);

    TesterPtr tester;

    if (!IsEmpty(testerName))
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
        return InternalValue();
    }

    InternalValueList resultList;
    resultList.reserve(list.GetSize().value_or(0));
    std::copy_if(list.begin(), list.end(), std::back_inserter(resultList), [this, tester, attrName, &context](auto& val) {
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
    });

    return ListAdapter::CreateAdapter(std::move(resultList));
}

ValueConverter::ValueConverter(const FilterParams& params, ValueConverter::Mode mode)
    : m_mode(mode)
{
    switch (mode)
    {
    case ToFloatMode:
        ParseParams({ { "default"s, false, 0.0 } }, params);
        break;
    case ToIntMode:
        ParseParams({ { "default"s, false, static_cast<int64_t>(0) }, { "base"s, false, static_cast<int64_t>(10) } }, params);
        break;
    case ToListMode:
    case AbsMode:
    case ItemsMode:
        ParseParams({}, params);
        break;
    case FileSizeFormatMode:
        ParseParams({ { "binary"s, false, false } }, params);
        break;
    case RoundMode:
        ParseParams({ { "precision"s, false }, { "method"s, false, "common"s } }, params);
        break;
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

    InternalValue operator()(const ListAdapter& val) const
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

        return InternalValue(val);
    }

    InternalValue operator()(const MapAdapter& val) const
    {
        if (m_params.mode != ValueConverter::ToListMode)
        {
            return InternalValue();
        }

        auto keys = val.GetKeys();
        auto numKeys = keys.size();
        return ListAdapter::CreateAdapter(numKeys, [values = std::move(keys)](size_t idx) { return InternalValue(values[idx]); });
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

// The text of `str` without surrounding ASCII whitespace
std::string StripAsciiSpace(const std::string& str)
{
    auto isSpace = [](char ch) { return unicode::IsSpace(static_cast<unsigned char>(ch)) && static_cast<unsigned char>(ch) < 0x80; };
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

// Python's float() of a string: surrounding whitespace, an optional sign, decimal digits with
// single underscores between them, an optional exponent, or inf/infinity/nan in any case.
// Non-ASCII digits and whitespace are not recognised.
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
    return std::copysign(std::strtod(text.c_str(), nullptr), val);
}

// Python's round(n, ndigits) for an integer: unchanged for ndigits >= 0, else half to even
int64_t PythonRoundInt(int64_t val, int64_t ndigits)
{
    if (ndigits >= 0)
    {
        return val;
    }
    if (ndigits < -18)
    {
        return 0;
    }
    uint64_t unit = 1;
    for (int64_t n = 0; n != -ndigits; ++n)
    {
        unit *= 10;
    }
    auto magnitude = val < 0 ? 0 - static_cast<uint64_t>(val) : static_cast<uint64_t>(val);
    auto quotient = magnitude / unit;
    auto remainder = magnitude % unit;
    if (remainder * 2 > unit || (remainder * 2 == unit && quotient % 2 == 1))
    {
        ++quotient;
    }
    auto result = static_cast<int64_t>(quotient * unit);
    return val < 0 ? -result : result;
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
    else if (auto str = GetAsSameString(std::string(), baseVal))
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
    const auto* map = GetIf<MapAdapter>(&baseVal);
    if (!map)
    {
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
    }
    InternalValueList items;
    for (auto& [key, value] : map->GetEntries())
    {
        items.emplace_back(ListAdapter::CreateAdapter(InternalValueList{ InternalValue(std::move(key)), std::move(value) }).MarkAsTuple());
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
    else if (auto str = GetAsSameString(std::string(), baseVal))
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
    if (auto str = GetAsSameString(std::string(), baseVal))
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

InternalValue ValueConverter::Round(const InternalValue& baseVal, RenderContext& context)
{
    // Jinja2's do_round: round(value, precision), or math.ceil/floor at that precision
    auto method = AsString(GetArgumentValue("method", context));
    if (method != "common" && method != "ceil" && method != "floor")
    {
        throw std::runtime_error("round(): method must be common, ceil or floor");
    }
    auto precVal = GetArgumentValue("precision", context);
    if (!IsEmpty(precVal) && !GetIf<int64_t>(&precVal) && !GetIf<bool>(&precVal))
    {
        throw std::runtime_error("round(): precision must be an integer");
    }
    auto precision = IsEmpty(precVal) ? 0 : ConvertToInt(precVal);
    auto asInt = PythonIntOf(baseVal);
    const auto* dblVal = GetIf<double>(&baseVal);
    if (!asInt && !dblVal)
    {
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
    }
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
    double scale = std::pow(10.0, static_cast<double>(precision));
    double scaled = value * scale;
    // Python raises here: OverflowError for 10**precision or ceil(inf), ZeroDivisionError for a zero scale
    if (!std::isfinite(scale) || scale == 0.0 || !std::isfinite(scaled))
    {
        throw std::runtime_error("round(): value or precision out of range");
    }
    // math.ceil/floor return an int, so a negative zero comes back as 0.0
    return ((method == "ceil" ? std::ceil(scaled) : std::floor(scaled)) / scale) + 0.0;
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
    ParseParams({ { "*args" }, { "**kwargs" } }, params);
    m_callParams.kwParams = m_args.extraKwArgs;
    m_callParams.posParams = m_args.extraPosArgs;
}

InternalValue UserDefinedFilter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const Callable* callable = GetIf<Callable>(&m_callable);
    if (!callable)
    {
        bool filterFound = false;
        auto filterValPtr = context.FindValue(m_filterName, filterFound);
        if (!filterFound)
        {
            throw std::runtime_error("Can't find filter '" + m_filterName + "'");
        }
        callable = GetIf<Callable>(&filterValPtr->second);
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
} // namespace jinja2
