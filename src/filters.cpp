#include "filters.h"

#include "generic_adapters.h"
#include "out_stream.h"
#include "testers.h"
#include "unicode_tables.h"
#include "value_helpers.h"
#include "value_visitors.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <random>
#include <limits>
#include <locale>
#include <sstream>
#include <string>

using namespace std::string_literals;

namespace jinja2
{

template<typename F>
struct FilterFactory
{
    static FilterPtr Create(FilterParams params) { return std::make_shared<F>(std::move(params)); }

    template<typename... Args>
    static ExpressionFilter::FilterFactoryFn MakeCreator(Args&&... args)
    {
        return [args...](FilterParams params) { return std::make_shared<F>(std::move(params), args...); };
    }
};

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
    { "forceescape", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::EscapeHtmlMode) },
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
        return std::make_shared<filters::UserDefinedFilter>(std::move(filterName), std::move(params));

    return p->second(std::move(params));
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

static OrderKind GetOrderKind(const InternalValue& val)
{
    auto& data = val.GetData();
    if (GetIf<int64_t>(&val) || GetIf<double>(&val) || GetIf<bool>(&val))
        return OrderKind::Number;
    if (nonstd::get_if<std::string>(&data) || nonstd::get_if<TargetString>(&data) || nonstd::get_if<TargetStringView>(&data))
        return OrderKind::String;
    if (GetIf<ListAdapter>(&val) || GetIf<KeyValuePair>(&val))
        return OrderKind::Sequence;
    return OrderKind::Unordered;
}

// Python's `<` (or `>`) as min and max use it: values that have no order between them,
// like 1 and 'a' or two dicts, are an error. Undefined values still compare as false.
// `sort` does not use it: it compares its keys wrapped in lists, testing `==` first.
static bool CompareForOrder(const InternalValue& left,
                            const InternalValue& right,
                            BinaryExpression::Operation oper,
                            BinaryExpression::CompareType compType)
{
    if (!IsEmpty(left) && !IsEmpty(right))
    {
        auto kind = GetOrderKind(left);
        if (kind != GetOrderKind(right) || kind == OrderKind::Unordered)
            throw std::runtime_error("'<' not supported between these values");
    }
    return ConvertToBool(Apply2<visitors::BinaryMathOperation>(left, right, oper, compType));
}

// Jinja2's _prepare_attribute_parts: "a.b.0" is the path a, b, 0, a digit part an index
static InternalValueList AttributePath(const InternalValue& attribute)
{
    auto str = GetAsSameString(std::string(), attribute);
    if (!str)
        return { attribute };

    InternalValueList parts;
    size_t start = 0;
    for (;;)
    {
        auto end = str->find('.', start);
        auto part = str->substr(start, end == std::string::npos ? std::string::npos : end - start);
        bool isIndex = !part.empty() && std::all_of(part.begin(), part.end(), [](char ch) { return ch >= '0' && ch <= '9'; });
        if (isIndex)
            parts.emplace_back(static_cast<int64_t>(std::stoll(part)));
        else
            parts.emplace_back(std::move(part));
        if (end == std::string::npos)
            break;
        start = end + 1;
    }
    return parts;
}

// Jinja2's make_attrgetter: looks the path up item by item and replaces an undefined step
// with `defaultVal` unless it is None or missing
static InternalValue GetAttributeByPath(const InternalValue& item, const InternalValueList& path, const InternalValue& defaultVal, RenderContext& context)
{
    InternalValue result = item;
    for (auto& part : path)
    {
        result = Subscript(result, part, &context);
        if (result.IsUndefined() && !IsEmpty(defaultVal))
            result = defaultVal;
    }
    return result;
}

Join::Join(FilterParams params)
{
    ParseParams({ { "d", false, std::string() }, { "attribute" } }, params);
}

InternalValue Join::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue attrName = GetArgumentValue("attribute", context);

    bool isConverted = false;
    ListAdapter values = ConvertToList(baseVal, attrName, isConverted, false);

    if (!isConverted)
        return InternalValue();

    // Python join converts every item and the delimiter with str()
    auto* renderer = context.GetRendererCallback();
    bool isFirst = true;
    InternalValue result;
    InternalValue delimiter = renderer->GetAsTargetString(m_args["d"]->Evaluate(context));
    for (const InternalValue& val : values)
    {
        if (isFirst)
            isFirst = false;
        else
            result = Apply2<visitors::StringJoiner>(result, delimiter);

        result = Apply2<visitors::StringJoiner>(result, InternalValue(renderer->GetAsTargetString(val)));
    }

    return result;
}

Sort::Sort(FilterParams params)
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
        return InternalValue();
    InternalValueList values = origValues.ToValueList();

    BinaryExpression::Operation oper = ConvertToBool(isReverseVal) ? BinaryExpression::LogicalGt : BinaryExpression::LogicalLt;
    BinaryExpression::CompareType compType = ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    // Jinja2's make_multi_attrgetter: "a,b.c" sorts by the list [item.a, item.b.c]
    std::vector<InternalValueList> paths;
    if (!IsEmpty(attrName))
    {
        auto str = GetAsSameString(std::string(), attrName);
        if (!str)
            paths.push_back(AttributePath(attrName));
        else
        {
            size_t start = 0;
            for (;;)
            {
                auto end = str->find(',', start);
                paths.push_back(AttributePath(InternalValue(str->substr(start, end == std::string::npos ? std::string::npos : end - start))));
                if (end == std::string::npos)
                    break;
                start = end + 1;
            }
        }
    }

    // Python's sorted() is stable
    std::stable_sort(values.begin(), values.end(), [&paths, oper, compType, &context](auto& val1, auto& val2) {
        auto isLess = [oper, compType](const InternalValue& key1, const InternalValue& key2, bool& isEqual) {
            // Equal items never reach `<`, so a list of equal dicts or Nones sorts as in Python
            isEqual = ConvertToBool(Apply2<visitors::BinaryMathOperation>(key1, key2, BinaryExpression::LogicalEq, compType));
            return !isEqual && ConvertToBool(Apply2<visitors::BinaryMathOperation>(key1, key2, oper, compType));
        };
        bool isEqual = false;
        if (paths.empty())
            return isLess(val1, val2, isEqual);
        // The key lists compare item by item, like Python lists
        for (auto& path : paths)
        {
            auto result = isLess(GetAttributeByPath(val1, path, InternalValue(), context), GetAttributeByPath(val2, path, InternalValue(), context), isEqual);
            if (!isEqual)
                return result;
        }
        return false;
    });

    return ListAdapter::CreateAdapter(std::move(values));
}

Attribute::Attribute(FilterParams params)
{
    ParseParams({ { "name", true }, { "default", false } }, params);
}

InternalValue Attribute::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const auto attrNameVal = GetArgumentValue("name", context);
    // Python's attr reads attributes only: the items of a dict are not attributes, the fields
    // of a reflected object are
    auto* map = GetIf<MapAdapter>(&baseVal);
    if (map != nullptr && !map->HasAttributes())
        return GetArgumentValue("default", context);
    const auto result = Subscript(baseVal, attrNameVal, &context);
    if (result.IsUndefined())
        return GetArgumentValue("default", context);
    return result;
}

Default::Default(FilterParams params)
{
    ParseParams({ { "default_value", false, InternalValue(""s) }, { "boolean", false, InternalValue(false) } }, params);
}

InternalValue Default::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue defaultVal = GetArgumentValue("default_value", context);
    InternalValue conditionResult = GetArgumentValue("boolean", context);

    // None is a defined value
    if (baseVal.IsUndefined())
        return defaultVal;

    if (ConvertToBool(conditionResult) && !ConvertToBool(baseVal))
        return defaultVal;

    return baseVal;
}

DictSort::DictSort(FilterParams params)
{
    ParseParams({ { "case_sensitive", false }, { "by", false, "key"s }, { "reverse", false } }, params);
}

InternalValue DictSort::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const MapAdapter* map = GetIf<MapAdapter>(&baseVal);
    if (map == nullptr)
        return InternalValue();

    InternalValue isReverseVal = GetArgumentValue("reverse", context);
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context);
    InternalValue byVal = GetArgumentValue("by", context);

    bool (*comparator)(const KeyValuePair& left, const KeyValuePair& right);

    if (AsString(byVal) == "key") // Sort by key
    {
        if (ConvertToBool(isCsVal))
        {
            comparator = [](const KeyValuePair& left, const KeyValuePair& right) { return left.key < right.key; };
        }
        else
        {
            comparator = [](const KeyValuePair& left, const KeyValuePair& right) {
                return boost::lexicographical_compare(left.key, right.key, boost::algorithm::is_iless());
            };
        }
    }
    else if (AsString(byVal) == "value")
    {
        if (ConvertToBool(isCsVal))
        {
            comparator = [](const KeyValuePair& left, const KeyValuePair& right) {
                return CompareForOrder(left.value, right.value, BinaryExpression::LogicalLt, BinaryExpression::CaseSensitive);
            };
        }
        else
        {
            comparator = [](const KeyValuePair& left, const KeyValuePair& right) {
                return CompareForOrder(left.value, right.value, BinaryExpression::LogicalLt, BinaryExpression::CaseInsensitive);
            };
        }
    }
    else
        return InternalValue();

    std::vector<KeyValuePair> tempVector;
    tempVector.reserve(map->GetSize());
    for (auto& key : map->GetKeys())
    {
        auto val = map->GetValueByName(key);
        tempVector.push_back(KeyValuePair{ key, val });
    }

    // Python's sorted() is stable, also with reverse=True: ties keep the mapping's order
    if (ConvertToBool(isReverseVal))
        std::stable_sort(tempVector.begin(), tempVector.end(), [comparator](auto& l, auto& r) { return comparator(r, l); });
    else
        std::stable_sort(tempVector.begin(), tempVector.end(), [comparator](auto& l, auto& r) { return comparator(l, r); });

    InternalValueList resultList;
    for (auto& tmpVal : tempVector)
    {
        auto resultVal = InternalValue(std::move(tmpVal));
        if (baseVal.ShouldExtendLifetime())
            resultVal.SetParentData(baseVal);
        resultList.push_back(std::move(resultVal));
    }

    return InternalValue(ListAdapter::CreateAdapter(std::move(resultList)));
}

GroupBy::GroupBy(FilterParams params)
{
    ParseParams({ { "attribute", true }, { "default", false }, { "case_sensitive", false, false } }, params);
}

InternalValue GroupBy::Filter(const InternalValue& baseVal, RenderContext& context)
{
    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, isConverted, false);

    if (!isConverted)
        return InternalValue();

    auto path = AttributePath(GetArgumentValue("attribute", context));
    auto defaultVal = GetArgumentValue("default", context);
    auto compType = ConvertToBool(GetArgumentValue("case_sensitive", context)) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    struct Item
    {
        InternalValue key;
        InternalValue value;
    };
    std::vector<Item> items;
    for (auto& item : list)
        items.push_back(Item{ GetAttributeByPath(item, path, defaultVal, context), item });

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
            group.push_back(groupEnd->value);
        result.push_back(ListAdapter::CreateAdapter(InternalValueList{ p->key, ListAdapter::CreateAdapter(std::move(group)) }).MarkAsNamedTuple(fieldNames));
        p = groupEnd;
    }

    return ListAdapter::CreateAdapter(std::move(result));
}

ApplyMacro::ApplyMacro(FilterParams params)
{
    ParseParams({ { "macro", true } }, params, ExtraArgs::Accept);
    m_mappingParams.kwParams = m_args.extraKwArgs;
    m_mappingParams.posParams = m_args.extraPosArgs;
}

InternalValue ApplyMacro::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue macroName = GetArgumentValue("macro", context);
    if (IsEmpty(macroName))
        return InternalValue();

    bool macroFound = false;
    auto macroValPtr = context.FindValue(AsString(macroName), macroFound);
    if (!macroFound)
        return InternalValue();

    const Callable* callable = GetIf<Callable>(&macroValPtr->second);
    if (callable == nullptr || callable->GetKind() != Callable::Macro)
        return InternalValue();

    CallParams tmpCallParams = helpers::EvaluateCallParams(m_mappingParams, context);
    CallParams callParams;
    callParams.kwParams = std::move(tmpCallParams.kwParams);
    callParams.posParams.reserve(tmpCallParams.posParams.size() + 1);
    callParams.posParams.push_back(baseVal);
    for (auto& p : tmpCallParams.posParams)
        callParams.posParams.push_back(std::move(p));

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
        result.kwParams["default"] = defaultIt->second;

    return result;
}

InternalValue Map::Filter(const InternalValue& baseVal, RenderContext& context)
{
    if (m_byAttribute)
    {
        bool isConverted = false;
        auto list = ConvertToList(baseVal, isConverted, false);
        if (!isConverted)
            return InternalValue();

        auto params = helpers::EvaluateCallParams(m_mappingParams, context);
        auto path = AttributePath(params.kwParams["name"]);
        auto defaultVal = params.kwParams["default"];
        InternalValueList resultList;
        for (auto& item : list)
            resultList.push_back(GetAttributeByPath(item, path, defaultVal, context));
        return ListAdapter::CreateAdapter(std::move(resultList));
    }

    InternalValue filterName = GetArgumentValue("filter", context);
    if (IsEmpty(filterName))
        return InternalValue();

    auto filter = CreateFilter(AsString(filterName), m_mappingParams);
    if (!filter)
        return InternalValue();

    bool isConverted = false;
    auto list = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
        return InternalValue();

    InternalValueList resultList;
    resultList.reserve(list.GetSize().value_or(0));
    std::transform(list.begin(), list.end(), std::back_inserter(resultList), [filter, &context](auto& val) { return filter->Filter(val, context); });

    return ListAdapter::CreateAdapter(std::move(resultList));
}
Random::Random(FilterParams params) {}

InternalValue Random::Filter(const InternalValue&, RenderContext&)
{
    return InternalValue();
}

SequenceAccessor::SequenceAccessor(FilterParams params, SequenceAccessor::Mode mode)
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

InternalValue SequenceAccessor::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue result;

    // Like Python, a string is a sequence of characters and a mapping one of its keys
    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, isConverted, false);

    if (!isConverted)
        return result;

    auto ProtectedValue = [&baseVal](InternalValue value) {
        if (baseVal.ShouldExtendLifetime())
            value.SetParentData(baseVal);
        return value;
    };

    InternalValue attrName = GetArgumentValue("attribute", context);
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context, InternalValue(false));

    BinaryExpression::CompareType compType = ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    auto lessComparator = [&attrName, &compType, &context](auto& val1, auto& val2) {
        if (IsEmpty(attrName))
            return CompareForOrder(val1, val2, BinaryExpression::LogicalLt, compType);
        return CompareForOrder(Subscript(val1, attrName, &context), Subscript(val2, attrName, &context), BinaryExpression::LogicalLt, compType);
    };

    const auto& listSize = list.GetSize();

    switch (m_mode)
    {
    case FirstItemMode:
        if (listSize && *listSize > 0)
            result = ProtectedValue(list.GetValueByIndex(0));
        else
        {
            auto it = list.begin();
            if (it != list.end())
                result = ProtectedValue(*it);
        }
        break;
    case LastItemMode:
        if (listSize && *listSize > 0)
            result = ProtectedValue(list.GetValueByIndex(listSize.value() - 1));
        else
        {
            auto it = list.begin();
            auto end = list.end();
            for (; it != end; ++it)
                result = ProtectedValue(*it);
        }
        break;
    case LengthMode:
        if (listSize && *listSize > 0)
            result = static_cast<int64_t>(listSize.value());
        else
            result = static_cast<int64_t>(std::distance(list.begin(), list.end()));
        break;
    case RandomMode:
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        if (listSize && *listSize > 0)
        {
            std::uniform_int_distribution<> dis(0, static_cast<int>(listSize.value()) - 1);
            result = ProtectedValue(list.GetValueByIndex(dis(gen)));
        }
        else
        {
            auto it = list.begin();
            auto end = list.end();
            size_t count = 0;
            for (; it != end; ++it, ++count)
            {
                bool doCopy = count == 0 || std::uniform_int_distribution<size_t>(0, count)(gen) == 0;
                if (doCopy)
                    result = ProtectedValue(*it);
            }
        }
        break;
    }
    case MaxItemMode:
    {
        auto b = list.begin();
        auto e = list.end();
        auto p = std::max_element(list.begin(), list.end(), lessComparator);
        result = p != e ? ProtectedValue(*p) : InternalValue();
        break;
    }
    case MinItemMode:
    {
        auto b = list.begin();
        auto e = list.end();
        auto p = std::min_element(b, e, lessComparator);
        result = p != e ? ProtectedValue(*p) : InternalValue();
        break;
    }
    case ReverseMode:
    {
        if (GetIf<TargetString>(&baseVal) || GetIf<TargetStringView>(&baseVal))
        {
            // Python reverses a string into a string
            result = ApplyStringConverter(baseVal, [](auto strView) -> TargetString {
                auto chars = SplitCodePoints(strView);
                std::basic_string<typename decltype(strView)::value_type> reversed;
                reversed.reserve(strView.size());
                for (auto ch = chars.rbegin(); ch != chars.rend(); ++ch)
                    reversed.append(ch->begin(), ch->end());
                return TargetString(std::move(reversed));
            });
        }
        else if (listSize)
        {
            auto size = listSize.value();
            InternalValueList resultList(size);
            for (std::size_t n = 0; n < size; ++n)
                resultList[size - n - 1] = ProtectedValue(list.GetValueByIndex(n));
            result = ListAdapter::CreateAdapter(std::move(resultList));
        }
        else
        {
            InternalValueList resultList;
            auto it = list.begin();
            auto end = list.end();
            for (; it != end; ++it)
                resultList.push_back(ProtectedValue(*it));

            std::reverse(resultList.begin(), resultList.end());
            result = ListAdapter::CreateAdapter(std::move(resultList));
        }

        break;
    }
    case SumItemsMode:
    {
        ListAdapter l1;
        ListAdapter* actualList;
        if (IsEmpty(attrName))
        {
            actualList = &list;
        }
        else
        {
            l1 = list.ToSubscriptedList(attrName, true);
            actualList = &l1;
        }
        InternalValue start = GetArgumentValue("start", context);
        InternalValue resultVal = std::accumulate(actualList->begin(), actualList->end(), start, [](const InternalValue& cur, const InternalValue& val) {
            if (IsEmpty(cur))
                return val;

            return Apply2<visitors::BinaryMathOperation>(cur, val, BinaryExpression::Plus);
        });

        result = std::move(resultVal);
        break;
    }
    case UniqueItemsMode:
    {
        InternalValueList resultList;

        struct Item
        {
            InternalValue val;
            int64_t idx;
        };
        std::vector<Item> items;

        int idx = 0;
        for (auto& v : list)
            items.push_back(Item{ IsEmpty(attrName) ? v : Subscript(v, attrName, &context), idx++ });

        auto isEqual = [&compType](auto& i1, auto& i2) {
            return ConvertToBool(Apply2<visitors::BinaryMathOperation>(i1.val, i2.val, BinaryExpression::LogicalEq, compType));
        };
        try
        {
            std::stable_sort(items.begin(), items.end(), [&compType](auto& i1, auto& i2) {
                return ConvertToBool(Apply2<visitors::BinaryMathOperation>(i1.val, i2.val, BinaryExpression::LogicalLt, compType));
            });
            items.erase(std::unique(items.begin(), items.end(), isEqual), items.end());
        }
        catch (const std::runtime_error&)
        {
            // Unorderable items (mixed types, None, dicts): Python hashes them, so keep the
            // first of each run of equal items in a quadratic pass instead
            std::stable_sort(items.begin(), items.end(), [](auto& i1, auto& i2) { return i1.idx < i2.idx; });
            std::vector<Item> uniqueItems;
            for (auto& item : items)
            {
                if (std::none_of(uniqueItems.begin(), uniqueItems.end(), [&](auto& u) { return isEqual(u, item); }))
                    uniqueItems.push_back(item);
            }
            items = std::move(uniqueItems);
        }

        std::stable_sort(items.begin(), items.end(), [](auto& i1, auto& i2) { return i1.idx < i2.idx; });

        for (auto& i : items)
            resultList.push_back(ProtectedValue(list.GetValueByIndex(i.idx)));

        result = ListAdapter::CreateAdapter(std::move(resultList));
        break;
    }
    }

    return result;
}
Slice::Slice(FilterParams params, Slice::Mode mode)
    : m_mode{ mode }
{
    if (m_mode == BatchMode)
        ParseParams({ { "linecount"s, true }, { "fill_with"s, false } }, params);
    else
        ParseParams({ { "slices"s, true }, { "fill_with"s, false } }, params);
}

InternalValue Slice::Filter(const InternalValue& baseVal, RenderContext& context)
{
    if (m_mode == BatchMode)
        return Batch(baseVal, context);

    // Like Python, a string is a sequence of characters and a mapping one of its keys
    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
        return InternalValue();

    auto ProtectedValue = [&baseVal](InternalValue value) {
        if (baseVal.ShouldExtendLifetime())
            value.SetParentData(baseVal);
        return value;
    };

    // Port of Jinja2's do_slice: `slices` columns, the first length % slices of them one
    // item longer; with fill_with every shorter column gets one fill item
    int64_t slices = ConvertToInt(GetArgumentValue("slices", context));
    InternalValue fillWith = GetArgumentValue("fill_with", context);
    // Python raises ZeroDivisionError
    if (slices == 0)
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});

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
            auto start = offset + slice * perSlice;
            if (slice < withExtra)
                ++offset;
            auto end = offset + (slice + 1) * perSlice;
            InternalValueList column;
            for (auto idx = start; idx < end; ++idx)
                column.push_back(ProtectedValue(items[static_cast<size_t>(idx)]));
            if (!IsEmpty(fillWith) && slice >= withExtra)
                column.push_back(fillWith);
            resultList.push_back(ListAdapter::CreateAdapter(std::move(column)));
        }
    }

    return ListAdapter::CreateAdapter(std::move(resultList));
}

InternalValue Slice::Batch(const InternalValue& baseVal, RenderContext& context)
{
    bool isConverted = false;
    auto list = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
        return InternalValue();

    auto ProtectedValue = [&baseVal](InternalValue value) {
        if (baseVal.ShouldExtendLifetime())
            value.SetParentData(baseVal);
        return value;
    };

    // Port of Jinja2's do_batch: rows of `linecount` items, the last one padded only with fill_with
    auto linecount = ConvertToInt(GetArgumentValue("linecount", context));
    InternalValue fillWith = GetArgumentValue("fill_with", context);

    InternalValueList resultList;
    InternalValueList row;
    for (auto& item : list)
    {
        if (static_cast<int64_t>(row.size()) == linecount)
        {
            resultList.push_back(ListAdapter::CreateAdapter(std::move(row)));
            row = InternalValueList();
        }
        row.push_back(ProtectedValue(item));
    }
    if (!row.empty())
    {
        if (!IsEmpty(fillWith))
        {
            while (static_cast<int64_t>(row.size()) < linecount)
                row.push_back(fillWith);
        }
        resultList.push_back(ListAdapter::CreateAdapter(std::move(row)));
    }
    return ListAdapter::CreateAdapter(std::move(resultList));
}

StringFormat::StringFormat(FilterParams params)
{
    ParseParams({}, params, ExtraArgs::Accept);
    m_params.kwParams = std::move(m_args.extraKwArgs);
    m_params.posParams = std::move(m_args.extraPosArgs);
}

Tester::Tester(FilterParams params, Tester::Mode mode)
    : m_mode(mode)
{
    FilterParams newParams;

    if ((mode == RejectMode || mode == SelectMode) && params.kwParams.empty() && params.posParams.empty())
    {
        m_noParams = true;
        return;
    }

    if (mode == RejectMode || mode == SelectMode)
        ParseParams({ { "tester", false } }, params, ExtraArgs::Accept);
    else
        ParseParams({ { "attribute", true }, { "tester", false } }, params, ExtraArgs::Accept);

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
        tester = CreateTester(AsString(testerName), m_testingParams);

        if (!tester)
            return InternalValue();
    }

    bool isConverted = false;
    auto list = ConvertToList(baseVal, isConverted, false);
    if (!isConverted)
        return InternalValue();

    InternalValueList resultList;
    resultList.reserve(list.GetSize().value_or(0));
    std::copy_if(list.begin(), list.end(), std::back_inserter(resultList), [this, tester, attrName, &context](auto& val) {
        InternalValue attrVal;
        bool isAttr = !IsEmpty(attrName);
        if (isAttr)
            attrVal = GetAttributeByPath(val, AttributePath(attrName), InternalValue(), context);

        bool result = false;
        if (tester)
            result = tester->Test(isAttr ? attrVal : val, context);
        else
            result = ConvertToBool(isAttr ? attrVal : val);

        return (m_mode == SelectMode || m_mode == SelectAttrMode) ? result : !result;
    });

    return ListAdapter::CreateAdapter(std::move(resultList));
}

ValueConverter::ValueConverter(FilterParams params, ValueConverter::Mode mode)
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
    ValueConverter::Mode mode;
    InternalValue defValule;
    InternalValue base;
    InternalValue prec;
    InternalValue roundMethod;
};

struct ValueConverterImpl : visitors::BaseVisitor<>
{
    using BaseVisitor::operator();

    ValueConverterImpl(ConverterParams params)
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
                val = val < 0 ? std::floor(val) : std::ceil(val);
            else if (method == "floor")
                val = val > 0 ? std::floor(val) : std::ceil(val);
            else if (method == "common")
                val = std::round(val);
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
                result = m_params.defValule;
            else
                result = dblVal;
            break;
        }
        case ValueConverter::ToIntMode:
        {
            int base = static_cast<int>(GetAs<int64_t>(m_params.base));
            bool converted = false;
            long long intVal = ConvertToInt(val.c_str(), base, converted);

            if (!converted)
                result = m_params.defValule;
            else
                result = static_cast<int64_t>(intVal);
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
    InternalValue operator()(const nonstd::basic_string_view<CharT>& val) const
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
                result = m_params.defValule;
            else
                result = static_cast<double>(dblVal);
            break;
        }
        case ValueConverter::ToIntMode:
        {
            int base = static_cast<int>(GetAs<int64_t>(m_params.base));
            bool converted = false;
            std::basic_string<CharT> str(val.begin(), val.end());
            long long intVal = ConvertToInt(str.c_str(), base, converted);

            if (!converted)
                result = m_params.defValule;
            else
                result = static_cast<int64_t>(intVal);
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
            return InternalValue();

        // list() of a tuple or a range is a new list that prints as [a, b]
        if (val.IsTuple() || val.GetRangeInfo())
            return ListAdapter::CreateAdapter(val.ToValueList());

        return InternalValue(val);
    }

    InternalValue operator()(const MapAdapter& val) const
    {
        if (m_params.mode != ValueConverter::ToListMode)
            return InternalValue();

        auto keys = val.GetKeys();
        auto num_keys = keys.size();
        return ListAdapter::CreateAdapter(num_keys, [values = std::move(keys)](size_t idx) { return InternalValue(values[idx]); });
    }

    template<typename T>
    static T GetAs(const InternalValue& val, T defValue = 0)
    {
        ConverterParams params;
        params.mode = ValueConverter::ToIntMode;
        params.base = static_cast<int64_t>(10);
        InternalValue intVal = Apply<ValueConverterImpl>(val, params);
        const T* result = GetIf<int64_t>(&intVal);
        if (result == nullptr)
            return defValue;

        return *result;
    }

    ConverterParams m_params;
};

// Python's float() of a string: surrounding whitespace, an optional sign, decimal digits with
// single underscores between them, an optional exponent, or inf/infinity/nan in any case.
// Non-ASCII digits and whitespace are not recognised.
static nonstd::optional<double> ParsePythonFloat(std::string str)
{
    auto isSpace = [](char ch) { return unicode::IsSpace(static_cast<unsigned char>(ch)) && static_cast<unsigned char>(ch) < 0x80; };
    auto first = std::find_if_not(str.begin(), str.end(), isSpace);
    auto last = std::find_if_not(str.rbegin(), std::string::reverse_iterator(first), isSpace).base();
    std::string text(first, last);

    size_t pos = 0;
    bool negative = false;
    if (pos < text.size() && (text[pos] == '+' || text[pos] == '-'))
        negative = text[pos++] == '-';
    std::string word;
    for (auto n = pos; n < text.size(); ++n)
        word.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(text[n]))));
    if (word == "inf" || word == "infinity")
        return negative ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
    if (word == "nan")
        return std::numeric_limits<double>::quiet_NaN();

    std::string digits(text.begin(), text.begin() + static_cast<std::ptrdiff_t>(pos));
    // \d(_?\d)*; returns the number of digits read
    auto readDigits = [&]() -> size_t {
        size_t count = 0;
        while (pos < text.size())
        {
            if (std::isdigit(static_cast<unsigned char>(text[pos])))
            {
                digits.push_back(text[pos++]);
                ++count;
            }
            else if (text[pos] == '_' && count != 0 && pos + 1 < text.size() && std::isdigit(static_cast<unsigned char>(text[pos + 1])))
                ++pos;
            else
                break;
        }
        return count;
    };
    auto mantissa = readDigits();
    if (pos < text.size() && text[pos] == '.')
    {
        digits.push_back(text[pos++]);
        mantissa += readDigits();
    }
    if (mantissa == 0)
        return nonstd::nullopt;
    bool negativeExponent = false;
    if (pos < text.size() && (text[pos] == 'e' || text[pos] == 'E'))
    {
        digits.push_back(text[pos++]);
        if (pos < text.size() && (text[pos] == '+' || text[pos] == '-'))
        {
            negativeExponent = text[pos] == '-';
            digits.push_back(text[pos++]);
        }
        if (readDigits() == 0)
            return nonstd::nullopt;
    }
    if (pos != text.size())
        return nonstd::nullopt;

    std::istringstream is(digits);
    is.imbue(std::locale::classic());
    double result = 0;
    is >> result;
    // A valid literal fails only out of range: overflow is inf, underflow is zero
    if (is.fail())
        result = negativeExponent ? 0.0 : std::numeric_limits<double>::infinity();
    return negative ? -std::abs(result) : std::abs(result);
}

// Python's int() of a string in `base` (0, or 2 to 36): surrounding whitespace, a sign, digits
// with single underscores between them and, for base 0 or a matching base, a 0x/0o/0b prefix.
// Values out of the int64 range are not supported and fail.
static nonstd::optional<int64_t> ParsePythonInt(std::string str, int64_t base)
{
    if (base != 0 && (base < 2 || base > 36))
        return nonstd::nullopt;
    auto isSpace = [](char ch) { return unicode::IsSpace(static_cast<unsigned char>(ch)) && static_cast<unsigned char>(ch) < 0x80; };
    auto first = std::find_if_not(str.begin(), str.end(), isSpace);
    auto last = std::find_if_not(str.rbegin(), std::string::reverse_iterator(first), isSpace).base();
    std::string text(first, last);

    size_t pos = 0;
    bool negative = false;
    if (pos < text.size() && (text[pos] == '+' || text[pos] == '-'))
        negative = text[pos++] == '-';

    auto lower = [](char ch) { return static_cast<char>(std::tolower(static_cast<unsigned char>(ch))); };
    bool hasPrefix = false;
    if (pos + 1 < text.size() && text[pos] == '0')
    {
        int64_t prefixBase = 0;
        switch (lower(text[pos + 1]))
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
            hasPrefix = true;
        }
    }
    // Base 0 without a prefix is decimal, where a leading zero is allowed only in zero itself
    bool decimalGuess = base == 0;
    if (decimalGuess)
        base = 10;

    uint64_t value = 0;
    size_t digits = 0;
    bool nonZero = false;
    bool overflow = false;
    // An underscore must follow a digit or the prefix ("0x_1f") and precede a digit
    bool underscoreAllowed = hasPrefix;
    bool lastUnderscore = false;
    for (; pos < text.size(); ++pos)
    {
        auto ch = lower(text[pos]);
        if (ch == '_')
        {
            if (!underscoreAllowed)
                return nonstd::nullopt;
            underscoreAllowed = false;
            lastUnderscore = true;
            continue;
        }
        int64_t digit = base;
        if (ch >= '0' && ch <= '9')
            digit = ch - '0';
        else if (ch >= 'a' && ch <= 'z')
            digit = ch - 'a' + 10;
        if (digit >= base)
            return nonstd::nullopt;
        underscoreAllowed = true;
        lastUnderscore = false;
        ++digits;
        nonZero = nonZero || digit != 0;
        if (value > (std::numeric_limits<uint64_t>::max() - static_cast<uint64_t>(digit)) / static_cast<uint64_t>(base))
            overflow = true;
        value = value * static_cast<uint64_t>(base) + static_cast<uint64_t>(digit);
    }
    if (digits == 0 || lastUnderscore || overflow)
        return nonstd::nullopt;
    if (decimalGuess && nonZero && text[text.find_first_of("0123456789")] == '0')
        return nonstd::nullopt;
    auto limit = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + (negative ? 1 : 0);
    if (value > limit)
        return nonstd::nullopt;
    return negative ? static_cast<int64_t>(0 - value) : static_cast<int64_t>(value);
}

// Python's round(x, ndigits) for a float: the exact binary value rounded half to even at
// that decimal position
static double PythonRound(double val, int64_t ndigits)
{
    if (!std::isfinite(val) || val == 0 || ndigits > 323)
        return val;
    if (ndigits < -308)
        return std::copysign(0.0, val);

    // The exact decimal expansion of val: a double has at most 53 - exponent fractional digits
    int exponent = 0;
    std::frexp(val, &exponent);
    auto exact = fmt::format("{:.{}f}", std::fabs(val), std::max(0, 53 - exponent));
    auto point = exact.find('.');
    auto intDigits = static_cast<int64_t>(point == std::string::npos ? exact.size() : point);
    std::string digits = exact;
    if (point != std::string::npos)
        digits.erase(point, 1);

    // Keep `keep` digits, round on the rest
    auto keep = intDigits + ndigits;
    if (keep < 0)
        return std::copysign(0.0, val);
    if (keep >= static_cast<int64_t>(digits.size()))
        return val;
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
                kept[idx] = '0';
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
static int64_t PythonRoundInt(int64_t val, int64_t ndigits)
{
    if (ndigits >= 0)
        return val;
    if (ndigits < -18)
        return 0;
    uint64_t unit = 1;
    for (int64_t n = 0; n != -ndigits; ++n)
        unit *= 10;
    auto magnitude = val < 0 ? 0 - static_cast<uint64_t>(val) : static_cast<uint64_t>(val);
    auto quotient = magnitude / unit;
    auto remainder = magnitude % unit;
    if (remainder * 2 > unit || (remainder * 2 == unit && quotient % 2 == 1))
        ++quotient;
    auto result = static_cast<int64_t>(quotient * unit);
    return val < 0 ? -result : result;
}

// Port of Jinja2's do_filesizeformat
static std::string FormatFileSize(double bytes, bool binary)
{
    const double base = binary ? 1024 : 1000;
    static const char* const decimalPrefixes[] = { "kB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };
    static const char* const binaryPrefixes[] = { "KiB", "MiB", "GiB", "TiB", "PiB", "EiB", "ZiB", "YiB" };
    const auto& prefixes = binary ? binaryPrefixes : decimalPrefixes;

    if (bytes == 1)
        return "1 Byte";
    // int(bytes) in Python: truncated, exact for any finite value, never "-0"
    if (bytes < base)
        return fmt::format("{:.0f} Bytes", std::trunc(bytes) + 0.0);

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
            break;
    }
    return fmt::format("{:.1f} {}", base * bytes / unit, prefix);
}

InternalValue ValueConverter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    if (m_mode == FileSizeFormatMode)
    {
        nonstd::optional<double> bytes;
        if (auto* intVal = GetIf<int64_t>(&baseVal))
            bytes = static_cast<double>(*intVal);
        else if (auto* dblVal = GetIf<double>(&baseVal))
            bytes = *dblVal;
        else if (auto* boolVal = GetIf<bool>(&baseVal))
            bytes = *boolVal ? 1.0 : 0.0;
        else if (auto str = GetAsSameString(std::string(), baseVal))
            bytes = ParsePythonFloat(*str);
        // Python raises on what float() rejects, and int(-inf) overflows
        if (!bytes || (std::isinf(*bytes) && *bytes < 0))
            context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
        return InternalValue(FormatFileSize(*bytes, ConvertToBool(GetArgumentValue("binary", context))));
    }

    if (m_mode == ItemsMode)
    {
        // An undefined value yields no items, anything but a mapping is a TypeError
        if (baseVal.IsUndefined())
            return ListAdapter::CreateAdapter(InternalValueList());
        auto* map = GetIf<MapAdapter>(&baseVal);
        if (map == nullptr)
            context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
        InternalValueList items;
        for (auto& key : map->GetKeys())
            items.push_back(ListAdapter::CreateAdapter(InternalValueList{ InternalValue(key), map->GetValueByName(key) }).MarkAsTuple());
        InternalValue result = ListAdapter::CreateAdapter(std::move(items));
        if (baseVal.ShouldExtendLifetime())
            result.SetParentData(baseVal);
        return result;
    }

    auto* intVal = GetIf<int64_t>(&baseVal);
    auto* dblVal = GetIf<double>(&baseVal);
    auto* boolVal = GetIf<bool>(&baseVal);
    // bool is an int in Python
    nonstd::optional<int64_t> asInt;
    if (intVal != nullptr)
        asInt = *intVal;
    else if (boolVal != nullptr)
        asInt = *boolVal ? 1 : 0;

    switch (m_mode)
    {
    case ToIntMode:
    {
        // Jinja2's do_int: int(value[, base]), then int(float(value)), then the default
        if (asInt)
            return *asInt;
        auto toInt = [](double val) -> nonstd::optional<int64_t> {
            // int() of inf or nan fails; larger values are not supported
            if (!std::isfinite(val) || std::fabs(val) >= 9223372036854775808.0)
                return nonstd::nullopt;
            return static_cast<int64_t>(val);
        };
        nonstd::optional<int64_t> result;
        if (dblVal != nullptr)
            result = toInt(*dblVal);
        else if (auto str = GetAsSameString(std::string(), baseVal))
        {
            result = ParsePythonInt(*str, ConvertToInt(GetArgumentValue("base", context)));
            if (!result)
            {
                auto asFloat = ParsePythonFloat(*str);
                if (asFloat)
                    result = toInt(*asFloat);
            }
        }
        if (result)
            return *result;
        return GetArgumentValue("default", context);
    }
    case ToFloatMode:
    {
        // Jinja2's do_float: float(value), else the default
        if (asInt)
            return static_cast<double>(*asInt);
        if (dblVal != nullptr)
            return *dblVal;
        if (auto str = GetAsSameString(std::string(), baseVal))
        {
            if (auto result = ParsePythonFloat(*str))
                return *result;
        }
        return GetArgumentValue("default", context);
    }
    case AbsMode:
        if (asInt)
            return static_cast<int64_t>(*asInt < 0 ? 0 - static_cast<uint64_t>(*asInt) : static_cast<uint64_t>(*asInt));
        if (dblVal != nullptr)
            return std::fabs(*dblVal);
        // Python's abs() of anything else is a TypeError
        context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
        return InternalValue();
    case RoundMode:
    {
        // Jinja2's do_round: round(value, precision), or math.ceil/floor at that precision
        auto method = AsString(GetArgumentValue("method", context));
        if (method != "common" && method != "ceil" && method != "floor")
            throw std::runtime_error("round(): method must be common, ceil or floor");
        auto precVal = GetArgumentValue("precision", context);
        if (!IsEmpty(precVal) && GetIf<int64_t>(&precVal) == nullptr && GetIf<bool>(&precVal) == nullptr)
            throw std::runtime_error("round(): precision must be an integer");
        auto precision = IsEmpty(precVal) ? 0 : ConvertToInt(precVal);
        if (!asInt && dblVal == nullptr)
            context.GetRendererCallback()->ThrowRuntimeError(ErrorCode::InvalidValueType, ValuesList{});
        if (method == "common")
        {
            // round() of an int is an int
            if (asInt)
                return PythonRoundInt(*asInt, precision);
            return PythonRound(*dblVal, precision);
        }
        double value = asInt ? static_cast<double>(*asInt) : *dblVal;
        double scale = std::pow(10.0, static_cast<double>(precision));
        double scaled = value * scale;
        // Python raises here: OverflowError for 10**precision or ceil(inf), ZeroDivisionError for a zero scale
        if (!std::isfinite(scale) || scale == 0.0 || !std::isfinite(scaled))
            throw std::runtime_error("round(): value or precision out of range");
        // math.ceil/floor return an int, so a negative zero comes back as 0.0
        return (method == "ceil" ? std::ceil(scaled) : std::floor(scaled)) / scale + 0.0;
    }
    default:
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
        result.SetParentData(baseVal);

    return result;
}

UserDefinedFilter::UserDefinedFilter(std::string filterName, FilterParams params)
    : m_filterName(std::move(filterName))
{
    ParseParams({ { "*args" }, { "**kwargs" } }, params);
    m_callParams.kwParams = m_args.extraKwArgs;
    m_callParams.posParams = m_args.extraPosArgs;
}

InternalValue UserDefinedFilter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    bool filterFound = false;
    auto filterValPtr = context.FindValue(m_filterName, filterFound);
    if (!filterFound)
        throw std::runtime_error("Can't find filter '" + m_filterName + "'");

    const Callable* callable = GetIf<Callable>(&filterValPtr->second);
    if (callable == nullptr || callable->GetKind() != Callable::UserCallable)
        return InternalValue();

    CallParams tmpCallParams = helpers::EvaluateCallParams(m_callParams, context);
    CallParams callParams;
    callParams.kwParams = std::move(tmpCallParams.kwParams);
    callParams.posParams.reserve(tmpCallParams.posParams.size() + 1);
    callParams.posParams.push_back(baseVal);
    for (auto& p : tmpCallParams.posParams)
        callParams.posParams.push_back(std::move(p));

    InternalValue result;
    if (callable->GetType() != Callable::Type::Expression)
        return InternalValue();

    return callable->GetExpressionCallable()(callParams, context);
}

} // namespace filters
} // namespace jinja2
