#include "filters.h"
#include "out_stream.h"
#include "testers.h"
#include "value_visitors.h"
#include "value_helpers.h"
#include "generic_adapters.h"

#include <algorithm>
#include <numeric>
#include <sstream>
#include <string>
#include <random>

using namespace std::string_literals;

namespace jinja2
{

template<typename F>
struct FilterFactory
{
    static FilterPtr Create(FilterParams params, InternalValueDataPool* pool)
    {
        return std::make_shared<F>(std::move(params), pool);
    }

    template<typename ... Args>
    static ExpressionFilter::FilterFactoryFn MakeCreator(Args&& ... args)
    {
        return [args...](FilterParams params, InternalValueDataPool* pool) {return std::make_shared<F>(std::move(params), args..., pool);};
    }
};

std::unordered_map<std::string, ExpressionFilter::FilterFactoryFn> s_filters = {
    {"abs", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::AbsMode)},
    {"applymacro", &FilterFactory<filters::ApplyMacro>::Create},
    {"attr", &FilterFactory<filters::Attribute>::Create},
    {"batch", FilterFactory<filters::Slice>::MakeCreator(filters::Slice::BatchMode)},
    {"camelize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::CamelMode)},
    {"capitalize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::CapitalMode)},
    {"default", &FilterFactory<filters::Default>::Create},
    {"d", &FilterFactory<filters::Default>::Create},
    {"dictsort", &FilterFactory<filters::DictSort>::Create},
    {"escape", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::EscapeHtmlMode)},
    {"escapecpp", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::EscapeCppMode)},
    {"first", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::FirstItemMode)},
    {"float", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ToFloatMode)},
    {"format", FilterFactory<filters::StringFormat>::MakeCreator(filters::StringFormat::PythonMode)},
    {"groupby", &FilterFactory<filters::GroupBy>::Create},
    {"int", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ToIntMode)},
    {"join", &FilterFactory<filters::Join>::Create},
    {"last", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::LastItemMode)},
    {"length", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::LengthMode)},
    {"list", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ToListMode)},
    {"lower", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::LowerMode)},
    {"map", &FilterFactory<filters::Map>::Create},
    {"max", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::MaxItemMode)},
    {"min", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::MinItemMode)},
    {"pprint", &FilterFactory<filters::PrettyPrint>::Create},
    {"random", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::RandomMode)},
    {"reject", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::RejectMode)},
    {"rejectattr", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::RejectAttrMode)},
    {"replace", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::ReplaceMode)},
    {"round", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::RoundMode)},
    {"reverse", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::ReverseMode)},
    {"select", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::SelectMode)},
    {"selectattr", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::SelectAttrMode)},
    {"slice", FilterFactory<filters::Slice>::MakeCreator(filters::Slice::SliceMode)},
    {"sort", &FilterFactory<filters::Sort>::Create},
    {"sum", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::SumItemsMode)},
    {"title", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::TitleMode)},
    {"tojson", FilterFactory<filters::Serialize>::MakeCreator(filters::Serialize::JsonMode)},
    {"toxml", FilterFactory<filters::Serialize>::MakeCreator(filters::Serialize::XmlMode)},
    {"toyaml", FilterFactory<filters::Serialize>::MakeCreator(filters::Serialize::YamlMode)},
    {"trim", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::TrimMode)},
    {"truncate", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::TruncateMode)},
    {"unique", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::UniqueItemsMode)},
    {"upper", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UpperMode)},
    {"urlencode", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UrlEncodeMode)},
    {"wordcount", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::WordCountMode)},
    {"wordwrap", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::WordWrapMode)},
    {"underscorize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UnderscoreMode)},};

extern FilterPtr CreateFilter(std::string filterName, CallParams params, InternalValueDataPool* pool)
{
    auto p = s_filters.find(filterName);
    if (p == s_filters.end())
        return std::make_shared<filters::UserDefinedFilter>(std::move(filterName), std::move(params), pool);

    return p->second(std::move(params), pool);
}

namespace filters
{

Join::Join(FilterParams params, InternalValueDataPool* pool)
{
    ParseParams({{"d", false, InternalValue::Create(std::string(), pool)}, {"attribute"}}, params);
}

InternalValue Join::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue attrName = GetArgumentValue("attribute", context);

    bool isConverted = false;
    ListAdapter values = ConvertToList(baseVal, attrName, context, isConverted);

    if (!isConverted)
        return InternalValue();

    bool isFirst = true;
    InternalValue result;
    InternalValue delimiter = m_args["d"]->Evaluate(context);
    for (const InternalValue& val : values)
    {
        if (isFirst)
            isFirst = false;
        else
            Apply2<visitors::StringJoiner>(result, delimiter, &result);

        Apply2<visitors::StringJoiner>(result, val, &result);
    }

    return result;
}

Sort::Sort(FilterParams params, InternalValueDataPool* pool)
{
    ParseParams({{"reverse", false, InternalValue::Create(false, pool)}, {"case_sensitive", false, InternalValue::Create(false, pool)}, {"attribute", false}}, params);
}

InternalValue Sort::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue attrName = GetArgumentValue("attribute", context);
    InternalValue isReverseVal = GetArgumentValue("reverse", context, InternalValue::Create(false, context.GetPool()));
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context, InternalValue::Create(false, context.GetPool()));

    bool isConverted = false;
    ListAdapter origValues = ConvertToList(baseVal, context, isConverted);
    if (!isConverted)
        return InternalValue();
    InternalValueList values = origValues.ToValueList();

    BinaryExpression::Operation oper =
            ConvertToBool(isReverseVal) ? BinaryExpression::LogicalGt : BinaryExpression::LogicalLt;
    BinaryExpression::CompareType compType =
            ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    std::sort(values.begin(), values.end(), [&attrName, oper, compType, &context](auto& val1, auto& val2) {
        InternalValue cmpRes;
        if (IsEmpty(attrName))
            Apply2<visitors::BinaryMathOperation>(val1, val2, &cmpRes, oper, compType);
        else
            Apply2<visitors::BinaryMathOperation>(Subscript(val1, attrName, context), Subscript(val2, attrName, context), &cmpRes, oper, compType);

        return ConvertToBool(cmpRes);
    });

    return CreateListAdapterValue(context.GetPool(), std::move(values));
}

Attribute::Attribute(FilterParams params, InternalValueDataPool*)
{
    ParseParams({{"name", true}, {"default", false}}, params);
}

InternalValue Attribute::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const auto attrNameVal = GetArgumentValue("name", context);
    const auto result = Subscript(baseVal, attrNameVal, context);
    if (result.IsEmpty())
      return GetArgumentValue("default", context);
    return result;
}

Default::Default(FilterParams params, InternalValueDataPool* pool)
{
    ParseParams({{"default_value", false, InternalValue::Create(""s, pool)}, {"boolean", false, InternalValue::Create(false, pool)}}, params);
}

InternalValue Default::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue defaultVal = GetArgumentValue("default_value", context);
    InternalValue conditionResult = GetArgumentValue("boolean", context);

    if (IsEmpty(baseVal))
        return defaultVal;

    if (ConvertToBool(conditionResult) && !ConvertToBool(baseVal))
        return defaultVal;

    return baseVal;
}

DictSort::DictSort(FilterParams params, InternalValueDataPool* pool)
{
    ParseParams({{"case_sensitive", false}, {"by", false, InternalValue::Create("key"s, pool)}, {"reverse", false}}, params);
}

InternalValue DictSort::Filter(const InternalValue& baseVal, RenderContext& context)
{
    const MapAdapter* map = GetIf<MapAdapter>(&baseVal);
    if (map == nullptr)
        return InternalValue();

    auto pool = context.GetPool();

    InternalValue isReverseVal = GetArgumentValue("reverse", context);
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context);
    InternalValue byVal = GetArgumentValue("by", context);

    std::function<bool (const KeyValuePair& left, const KeyValuePair& right)> comparator;

    if (AsString(byVal) == "key") // Sort by key
    {
        if (ConvertToBool(isCsVal))
        {
            comparator =  [](const KeyValuePair& left, const KeyValuePair& right)
            {
                return left.key < right.key;
            };
        }
        else
        {
            comparator = [](const KeyValuePair& left, const KeyValuePair& right)
            {
                return boost::lexicographical_compare(left.key, right.key, boost::algorithm::is_iless());
            };
        }
    }
    else if (AsString(byVal) == "value")
    {
        if (ConvertToBool(isCsVal))
        {
            comparator = [result=InternalValue::CreateEmpty(pool)](const KeyValuePair& left, const KeyValuePair& right) mutable -> bool
            {
                Apply2<visitors::BinaryMathOperation>(left.value, right.value, &result, BinaryExpression::LogicalLt, BinaryExpression::CaseSensitive);
                return ConvertToBool(result);
            };
        }
        else
        {
            comparator = [result=InternalValue::CreateEmpty(pool)](const KeyValuePair& left, const KeyValuePair& right) mutable -> bool
            {
                Apply2<visitors::BinaryMathOperation>(left.value, right.value, &result, BinaryExpression::LogicalLt, BinaryExpression::CaseInsensitive);
                return ConvertToBool(result);
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
        tempVector.push_back(KeyValuePair{key, val});
    }

    if (ConvertToBool(isReverseVal))
        std::sort(tempVector.begin(), tempVector.end(), [comparator](auto& l, auto& r) {return comparator(r, l);});
    else
        std::sort(tempVector.begin(), tempVector.end(), [comparator](auto& l, auto& r) {return comparator(l, r);});

    InternalValueList resultList;
    for (auto& tmpVal : tempVector)
    {
        auto resultVal = InternalValue::Create(std::move(tmpVal), pool);
        if (baseVal.ShouldExtendLifetime())
            resultVal.SetParentData(baseVal);
        resultList.push_back(std::move(resultVal));
    }

    return CreateListAdapterValue(pool, std::move(resultList));
}

GroupBy::GroupBy(FilterParams params, InternalValueDataPool*)
{
    ParseParams({{"attribute", true}}, params);
}

InternalValue GroupBy::Filter(const InternalValue& baseVal, RenderContext& context)
{
    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, context, isConverted);

    if (!isConverted)
        return InternalValue();

    InternalValue attrName = GetArgumentValue("attribute", context);

    auto equalComparator = [cmpRes = InternalValue::CreateEmpty(context.GetPool())](auto& val1, auto& val2) mutable {
        Apply2<visitors::BinaryMathOperation>(val1, val2, &cmpRes, BinaryExpression::LogicalEq, BinaryExpression::CaseSensitive);

        return ConvertToBool(cmpRes);
    };

    struct GroupInfo
    {
        InternalValue grouper;
        InternalValueList items;
    };

    std::vector<GroupInfo> groups;

    for (auto& item : list)
    {
        auto attr = Subscript(item, attrName, context);
        auto p = std::find_if(groups.begin(), groups.end(), [&equalComparator, &attr](auto& i) {return equalComparator(i.grouper, attr);});
        if (p == groups.end())
            groups.push_back(GroupInfo{attr, {item}});
        else
            p->items.push_back(item);
    }

    InternalValueList result;
    auto pool = context.GetPool();
    for (auto& g : groups)
    {
        InternalValueMap groupItem{{"grouper", std::move(g.grouper)}, {"list", CreateListAdapterValue(pool, std::move(g.items))}};
        result.push_back(CreateMapAdapterValue(pool, std::move(groupItem)));
    }

    return CreateListAdapterValue(pool, std::move(result));
}

ApplyMacro::ApplyMacro(FilterParams params, InternalValueDataPool*)
{
    ParseParams({{"macro", true}}, params);
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

    CallParams callParams;
    callParams.kwParams = m_mappingParams.kwParams;
    callParams.posParams.reserve(m_mappingParams.posParams.size() + 1);
    callParams.posParams.push_back(std::make_shared<ConstantExpression>(baseVal));
    callParams.posParams.insert(callParams.posParams.end(), m_mappingParams.posParams.begin(), m_mappingParams.posParams.end());

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
        result.SetData(std::move(resultStr));
    }

    return result;
}

Map::Map(FilterParams params, InternalValueDataPool* pool)
{
    ParseParams({{"filter", true}}, MakeParams(std::move(params), pool));
    m_mappingParams.kwParams = m_args.extraKwArgs;
    m_mappingParams.posParams = m_args.extraPosArgs;
}

FilterParams Map::MakeParams(FilterParams params, InternalValueDataPool* pool)
{
    if (!params.posParams.empty() || params.kwParams.empty() || params.kwParams.size() > 2) {
        return params;
    }

    const auto attributeIt = params.kwParams.find("attribute");
    if (attributeIt == params.kwParams.cend())
    {
      return params;
    }

    FilterParams result;
    result.kwParams["name"] = attributeIt->second;
    result.kwParams["filter"] = std::make_shared<ConstantExpression>(InternalValue::Create("attr"s, pool));

    const auto defaultIt = params.kwParams.find("default");
    if (defaultIt != params.kwParams.cend())
        result.kwParams["default"] = defaultIt->second;

    return result;
}

InternalValue Map::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue filterName = GetArgumentValue("filter", context);
    if (IsEmpty(filterName))
        return InternalValue();

    auto pool = context.GetPool();
    auto filter = CreateFilter(AsString(filterName), m_mappingParams, pool);
    if (!filter)
        return InternalValue::CreateEmpty(pool);

    bool isConverted = false;
    auto list = ConvertToList(baseVal, context, isConverted);
    if (!isConverted)
        return InternalValue();

    InternalValueList resultList;
    resultList.reserve(list.GetSize().value_or(0));
    std::transform(list.begin(), list.end(), std::back_inserter(resultList), [filter, &context](auto& val) {return filter->Filter(val, context);});

    return CreateListAdapterValue(pool, std::move(resultList));
}

struct PrettyPrinter : visitors::BaseVisitor<std::string>
{
    using BaseVisitor::operator();

    PrettyPrinter(const RenderContext* context)
        : m_context(context)
    {}

    std::string operator()(const ListAdapter& list) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "[");
        bool isFirst = true;

        for (auto& v : list)
        {
            if (isFirst)
                isFirst = false;
            else
                fmt::format_to(os, ", ");
            fmt::format_to(os, "{}", Apply<PrettyPrinter>(v, m_context));
        }
        fmt::format_to(os, "]");

        return str;
    }

    std::string operator()(const MapAdapter& map) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "{");

        const auto& keys = map.GetKeys();

        bool isFirst = true;
        for (auto& k : keys)
        {
            if (isFirst)
                isFirst = false;
            else
                fmt::format_to(os, ", ");

            fmt::format_to(os, "'{}':", k);
            fmt::format_to(os, "{}", Apply<PrettyPrinter>(map.GetValueByName(k), m_context));
        }

        fmt::format_to(os, "}");

        return str;
    }

    std::string operator() (const KeyValuePair& kwPair) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "'{}':", kwPair.key);
        fmt::format_to(os, "{}", Apply<PrettyPrinter>(kwPair.value, m_context));

        return str;
    }

    std::string operator()(const std::string& str) const
    {
        return "'"s + str + "'"s;
    }

    std::string operator()(const nonstd::string_view& str) const
    {
        return "'"s + std::string(str.begin(), str.end()) + "'"s;
    }

    std::string operator()(const std::wstring& str) const
    {
        return ConvertString<std::string>(str);
    }

    std::string operator()(const nonstd::wstring_view& str) const
    {
        return ConvertString<std::string>(str);
    }

    std::string operator()(bool val) const
    {
        return val ? "true"s : "false"s;
    }

    std::string operator()(EmptyValue) const
    {
        return "none"s;
    }

    std::string operator()(const Callable&) const
	{
		return "<callable>"s;
	}

    std::string operator()(double val) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "{:.8g}", val);

        return str;
    }

    std::string operator()(int64_t val) const
    {
        std::string str;
        auto os = std::back_inserter(str);

        fmt::format_to(os, "{}", val);

        return str;
    }

    const RenderContext* m_context;
};

PrettyPrint::PrettyPrint(FilterParams params, InternalValueDataPool*)
{
}

InternalValue PrettyPrint::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue::Create(Apply<PrettyPrinter>(baseVal, &context), context.GetPool());
}

Random::Random(FilterParams params, InternalValueDataPool*)
{

}

InternalValue Random::Filter(const InternalValue&, RenderContext&)
{
    return InternalValue();
}

SequenceAccessor::SequenceAccessor(FilterParams params, SequenceAccessor::Mode mode, InternalValueDataPool* pool)
    : m_mode(mode)
{
    switch (mode)
    {
    case FirstItemMode:
        break;
    case LastItemMode:
        break;
    case LengthMode:
        break;
    case MaxItemMode:
	case MinItemMode:
		ParseParams({{"case_sensitive", false, InternalValue::Create(false, pool)}, {"attribute", false}}, params);
        break;
    case RandomMode:
    case ReverseMode:
        break;
    case SumItemsMode:
        ParseParams({{"attribute", false}, {"start", false}}, params);
        break;
    case UniqueItemsMode:
        ParseParams({{"attribute", false}}, params);
        break;
    }
}

InternalValue SequenceAccessor::Filter(const InternalValue& baseVal, RenderContext& context)
{
    auto pool = context.GetPool();
    InternalValue result = InternalValue::CreateEmpty(pool);

    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, context, isConverted);

    if (!isConverted)
        return result;

    InternalValue attrName = GetArgumentValue("attribute", context);
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context, InternalValue::Create(false, pool));

    BinaryExpression::CompareType compType =
            ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    auto lessComparator = [&attrName, &compType, &context, result=InternalValue::CreateEmpty(pool)](auto& val1, auto& val2) mutable {
        InternalValue cmpRes;

        if (IsEmpty(attrName))
            Apply2<visitors::BinaryMathOperation>(val1, val2, &result, BinaryExpression::LogicalLt, compType);
        else
            Apply2<visitors::BinaryMathOperation>(Subscript(val1, attrName, context), Subscript(val2, attrName, context), &result, BinaryExpression::LogicalLt, compType);

        return ConvertToBool(result);
    };

    const auto& listSize = list.GetSize();

    switch (m_mode)
    {
    case FirstItemMode:
        if (listSize)
            result = list.GetValueByIndex(0);
        else
        {
            auto it = list.begin();
            if (it != list.end())
                result = *it;
        }
        break;
    case LastItemMode:
        if (listSize)
            result = list.GetValueByIndex(listSize.value() - 1);
        else
        {
            auto it = list.begin();
            auto end = list.end();
            for (; it != end; ++ it)
                result = *it;
        }
        break;
    case LengthMode:
        if (listSize)
            result.SetData(static_cast<int64_t>(listSize.value()));
        else
            result.SetData(static_cast<int64_t>(std::distance(list.begin(), list.end())));
        break;
    case RandomMode:
    {
        std::random_device rd;
        std::mt19937 gen(rd());
        if (listSize)
        {
            std::uniform_int_distribution<> dis(0, static_cast<int>(listSize.value()) - 1);
            result = list.GetValueByIndex(dis(gen));
        }
        else
        {
            auto it = list.begin();
            auto end = list.end();
            size_t count = 0;
            for (; it != end; ++ it, ++ count)
            {
                bool doCopy = count == 0 || std::uniform_int_distribution<>(0, count)(gen) == 0;
                if (doCopy)
                    result = *it;
            }
        }
        break;
    }
    case MaxItemMode:
    {
        auto b = list.begin();
        auto e = list.end();
        auto p = std::max_element(list.begin(), list.end(), lessComparator);
        result = p != e ? *p : InternalValue();
        break;
    }
    case MinItemMode:
    {
        auto b = list.begin();
        auto e = list.end();
        auto p = std::min_element(b, e, lessComparator);
        result = p != e ? *p : InternalValue();
        break;
    }
    case ReverseMode:
    {
        if (listSize)
        {
            auto size = listSize.value();
            InternalValueList resultList(size);
            for (std::size_t n = 0; n < size; ++ n)
                resultList[size - n - 1] = list.GetValueByIndex(n);
            result = CreateListAdapterValue(pool, std::move(resultList));
        }
        else
        {
            InternalValueList resultList;
            auto it = list.begin();
            auto end = list.end();
            for (; it != end; ++ it)
                resultList.push_back(*it);

            std::reverse(resultList.begin(), resultList.end());
            result = CreateListAdapterValue(pool, std::move(resultList));
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
            l1 = list.ToSubscriptedList(attrName, context, true);
            actualList = &l1;
        }
        InternalValue start = GetArgumentValue("start", context);
        auto tmpResult=InternalValue::CreateEmpty(pool);
        tmpResult.SetTemporary(true);
        InternalValue resultVal = std::accumulate(actualList->begin(), actualList->end(), start, [&tmpResult](const InternalValue& cur, const InternalValue& val) {
            if (IsEmpty(cur))
                return val;

            Apply2<visitors::BinaryMathOperation>(cur, val, &tmpResult, BinaryExpression::Plus);
            return tmpResult;
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
            items.push_back(Item{IsEmpty(attrName) ? v : Subscript(v, attrName, context), idx ++});

        std::stable_sort(items.begin(), items.end(), [&compType, cmpRes = InternalValue::CreateEmpty(pool)](auto& i1, auto& i2) mutable {
            Apply2<visitors::BinaryMathOperation>(i1.val, i2.val, &cmpRes, BinaryExpression::LogicalLt, compType);

            return ConvertToBool(cmpRes);
        });

        auto end = std::unique(items.begin(), items.end(), [&compType, cmpRes = InternalValue::CreateEmpty(pool)](auto& i1, auto& i2) mutable {
            Apply2<visitors::BinaryMathOperation>(i1.val, i2.val, &cmpRes, BinaryExpression::LogicalEq, compType);

            return ConvertToBool(cmpRes);
        });
        items.erase(end, items.end());

        std::stable_sort(items.begin(), items.end(), [](auto& i1, auto& i2) {
            return i1.idx < i2.idx;
        });

        for (auto& i : items)
            resultList.push_back(list.GetValueByIndex(i.idx));

        result = CreateListAdapterValue(pool, std::move(resultList));
        break;
    }
    }

    return result;
}

Serialize::Serialize(FilterParams, Serialize::Mode, InternalValueDataPool*)
{

}

InternalValue Serialize::Filter(const InternalValue&, RenderContext&)
{
    return InternalValue();
}

Slice::Slice(FilterParams, Slice::Mode, InternalValueDataPool*)
{

}

InternalValue Slice::Filter(const InternalValue&, RenderContext&)
{
    return InternalValue();
}

StringFormat::StringFormat(FilterParams, StringFormat::Mode, InternalValueDataPool*)
{

}

InternalValue StringFormat::Filter(const InternalValue&, RenderContext&)
{
    return InternalValue();
}

Tester::Tester(FilterParams params, Tester::Mode mode, InternalValueDataPool*)
    : m_mode(mode)
{
    FilterParams newParams;

    if ((mode == RejectMode || mode == SelectMode) && params.kwParams.empty() && params.posParams.empty())
    {
        m_noParams = true;
        return;
    }

    if (mode == RejectMode || mode == SelectMode)
        ParseParams({{"tester", false}}, params);
    else
        ParseParams({{"attribute", true}, {"tester", false}}, params);

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
    auto list = ConvertToList(baseVal, context, isConverted);
    if (!isConverted)
        return InternalValue();

    InternalValueList resultList;
    resultList.reserve(list.GetSize().value_or(0));
    std::copy_if(list.begin(), list.end(), std::back_inserter(resultList), [this, tester, attrName, &context](auto& val)
    {
        InternalValue attrVal;
        bool isAttr = !IsEmpty(attrName);
        if (isAttr)
            attrVal = Subscript(val, attrName, context);

        bool result = false;
        if (tester)
            result = tester->Test(isAttr ? attrVal : val, context);
        else
            result = ConvertToBool(isAttr ? attrVal : val);

        return (m_mode == SelectMode || m_mode == SelectAttrMode) ? result : !result;
    });

    return CreateListAdapterValue(context.GetPool(), std::move(resultList));
}

ValueConverter::ValueConverter(FilterParams params, ValueConverter::Mode mode, InternalValueDataPool* pool)
    : m_mode(mode)
{
    switch (mode)
    {
    case ToFloatMode:
        ParseParams({{"default"s, false}}, params);
        break;
    case ToIntMode:
        ParseParams({{"default"s, false}, {"base"s, false, InternalValue::Create(static_cast<int64_t>(10), pool)}}, params);
        break;
    case ToListMode:
    case AbsMode:
        break;
    case RoundMode:
        ParseParams({{"precision"s, false}, {"method"s, false, InternalValue::Create("common"s, pool)}}, params);
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

    ValueConverterImpl(ConverterParams params, InternalValueDataPool* pool)
        : m_params(std::move(params))
        , m_pool(pool)
    {
    }

    InternalValue operator()(int64_t val) const
    {
        InternalValue result;
        switch (m_params.mode)
        {
        case ValueConverter::ToFloatMode:
            result = InternalValue::Create(static_cast<double>(val), m_pool);
            break;
        case ValueConverter::AbsMode:
            result = InternalValue::Create(static_cast<int64_t>(std::abs(val)), m_pool);
            break;
        case ValueConverter::ToIntMode:
        case ValueConverter::RoundMode:
            result = InternalValue::Create(static_cast<int64_t>(val), m_pool);
            break;
        default:
            break;
        }

        result.SetTemporary(true);

        return result;
    }

    InternalValue operator()(double val) const
    {
        InternalValue result;
        switch (m_params.mode)
        {
        case ValueConverter::ToFloatMode:
            result = InternalValue::Create(static_cast<double>(val), m_pool);
            break;
        case ValueConverter::ToIntMode:
            result = InternalValue::Create(static_cast<int64_t>(val), m_pool);
            break;
        case ValueConverter::AbsMode:
            result = InternalValue::Create(fabs(val), m_pool);
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
            result = InternalValue::Create(val / pow10, m_pool);
            break;
        }
        default:
            break;
        }

        result.SetTemporary(true);

        return result;
    }

    static double ConvertToDouble(const char* buff, bool& isConverted)
    {
        char* endBuff = nullptr;
        double dblVal = strtold(buff, &endBuff);
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
            {
                result = InternalValue::Create(dblVal, m_pool);
                result.SetTemporary(true);
            }
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
            {
                result = InternalValue::Create(intVal, m_pool);
                result.SetTemporary(true);
            }
            break;
        }
        case ValueConverter::ToListMode:
        {
            result = CreateListAdapterValue(m_pool, val.size(), [str = val, pool = m_pool](size_t idx) {
                return InternalValue::Create(str.substr(idx, 1), pool);
            });
            result.SetTemporary(true);
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
            {
                result = InternalValue::Create(dblVal, m_pool);
                result.SetTemporary(true);
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
                result = m_params.defValule;
            else
            {
                result = InternalValue::Create(intVal, m_pool);
                result.SetTemporary(true);
            }
            break;
        }
        case ValueConverter::ToListMode:
        {
            result = CreateListAdapterValue(m_pool, val.size(), [str = val, pool = m_pool](size_t idx) {
                return InternalValue::Create(str.substr(idx, 1), pool);
            });
            result.SetTemporary(true);
        }
        default:
            break;
        }

        return result;
    }

    InternalValue operator()(const ListAdapter& val) const
    {
        if (m_params.mode == ValueConverter::ToListMode)
            return InternalValue::Create(val, m_pool);

        return InternalValue();
    }

    InternalValue operator()(const MapAdapter& val) const
    {
        if (m_params.mode != ValueConverter::ToListMode)
            return InternalValue();

        auto keys = val.GetKeys();
        auto num_keys = keys.size();
        return CreateListAdapterValue(m_pool, num_keys, [values=std::move(keys), this](size_t idx) {
            auto result = InternalValue::Create(values[idx], m_pool);
            result.SetTemporary(true);
            return result;
        });
    }

    template<typename T>
    T GetAs(const InternalValue& val, T defValue = 0) const
    {
        ConverterParams params;
        params.mode = ValueConverter::ToIntMode;
        params.base = InternalValue::Create(static_cast<int64_t>(10), m_pool);
        const InternalValue& intVal = Apply<ValueConverterImpl>(val, params, m_pool);
        const T* result = GetIf<int64_t>(&intVal);
        if (result == nullptr)
            return defValue;

        return *result;
    }

    ConverterParams m_params;
    InternalValueDataPool* m_pool;
};

InternalValue ValueConverter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    ConverterParams params;
    params.mode = m_mode;
    params.defValule = GetArgumentValue("default", context);
    params.base = GetArgumentValue("base", context);
    params.prec = GetArgumentValue("precision", context);
    params.roundMethod = GetArgumentValue("method", context);
    auto result = Apply<ValueConverterImpl>(baseVal, params, context.GetPool());
    if (baseVal.ShouldExtendLifetime())
        result.SetParentData(baseVal);

    return result;
}

UserDefinedFilter::UserDefinedFilter(std::string filterName, FilterParams params, InternalValueDataPool*)
    : m_filterName(std::move(filterName))
{
    ParseParams({{"*args"}, {"**kwargs"}}, params);
    m_callParams.kwParams = m_args.extraKwArgs;
    m_callParams.posParams = m_args.extraPosArgs;
}

InternalValue UserDefinedFilter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    bool filterFound = false;
    auto filterValPtr = context.FindValue(m_filterName, filterFound);
    if (!filterFound)
        return InternalValue();

    const Callable* callable = GetIf<Callable>(&filterValPtr->second);
    if (callable == nullptr || callable->GetKind() != Callable::UserCallable)
        return InternalValue();

    CallParams callParams;
    callParams.kwParams = m_callParams.kwParams;
    callParams.posParams.reserve(m_callParams.posParams.size() + 1);
    callParams.posParams.push_back(std::make_shared<ConstantExpression>(baseVal));
    callParams.posParams.insert(callParams.posParams.end(), m_callParams.posParams.begin(), m_callParams.posParams.end());

    InternalValue result;
    if (callable->GetType() != Callable::Type::Expression)
        return InternalValue();

    return callable->GetExpressionCallable()(callParams, context);
}
} // filters
} // jinja2
