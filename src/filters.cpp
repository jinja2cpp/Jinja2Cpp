#include "filters.h"
#include "value_visitors.h"
#include "value_helpers.h"

#include <algorithm>
#include <numeric>

namespace jinja2
{

template<typename F>
struct FilterFactory
{
    static FilterPtr Create(FilterParams params)
    {
        return std::make_shared<F>(std::move(params));
    }

    template<typename ... Args>
    static ExpressionFilter::FilterFactoryFn MakeCreator(Args&& ... args)
    {
        return [args...](FilterParams params) {return std::make_shared<F>(std::move(params), args...);};
    }
};

std::unordered_map<std::string, ExpressionFilter::FilterFactoryFn> s_filters = {
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
    {"map", &FilterFactory<filters::Map>::Create},
    {"max", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::MaxItemMode)},
    {"min", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::MinItemMode)},
    {"pprint", &FilterFactory<filters::PrettyPrint>::Create},
    {"random", &FilterFactory<filters::Random>::Create},
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
    {"wordcount", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::WordCountMode)},
    {"wordwrap", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::WordWrapMode)},
    {"underscorize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UnderscoreMode)},};

extern FilterPtr CreateFilter(std::string filterName, CallParams params)
{
    auto p = s_filters.find(filterName);
    if (p == s_filters.end())
        return FilterPtr();

    return p->second(std::move(params));
}

namespace filters
{

bool FilterBase::ParseParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParams& params)
{
    bool result = true;
    m_args = helpers::ParseCallParams(argsInfo, params, result);

    return result;
}

InternalValue FilterBase::GetArgumentValue(std::string argName, RenderContext& context, InternalValue defVal)
{
    auto argExpr = m_args[argName];
    return argExpr ? argExpr->Evaluate(context) : std::move(defVal);
}

Join::Join(FilterParams params)
{
    ParseParams({{"d", false, std::string()}, {"attribute"}}, params);
}

InternalValue Join::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue attrName = GetArgumentValue("attribute", context);

    bool isConverted = false;
    ListAdapter values = ConvertToList(baseVal, attrName, isConverted);

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
            result = Apply2<visitors::StringJoiner>(result, delimiter);

        result = Apply2<visitors::StringJoiner>(result, val);
    }

    return result;
}

Sort::Sort(FilterParams params)
{
    ParseParams({{"reverse", false, InternalValue(false)}, {"case_sensitive", false, InternalValue(false)}, {"attribute", false}}, params);
}

InternalValue Sort::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue attrName = GetArgumentValue("attribute", context);
    InternalValue isReverseVal = GetArgumentValue("reverse", context, InternalValue(false));
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context, InternalValue(false));

    bool isConverted = false;
    ListAdapter origValues = ConvertToList(baseVal, isConverted);
    if (!isConverted)
        return InternalValue();
    InternalValueList values = origValues.ToValueList();

    BinaryExpression::Operation oper =
            ConvertToBool(isReverseVal) ? BinaryExpression::LogicalGt : BinaryExpression::LogicalLt;
    BinaryExpression::CompareType compType =
            ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    std::sort(values.begin(), values.end(), [&attrName, oper, compType](auto& val1, auto& val2) {
        InternalValue cmpRes;
        if (IsEmpty(attrName))
            cmpRes = Apply2<visitors::BinaryMathOperation>(val1, val2, oper, compType);
        else
            cmpRes = Apply2<visitors::BinaryMathOperation>(Subscript(val1, attrName), Subscript(val2, attrName), oper, compType);

        return ConvertToBool(cmpRes);
    });

    return ListAdapter::CreateAdapter(std::move(values));
}

Attribute::Attribute(FilterParams params)
{
    ParseParams({{"name", true}}, params);
}

InternalValue Attribute::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue attrNameVal = GetArgumentValue("name", context);
    return Subscript(baseVal, attrNameVal);
}

Default::Default(FilterParams params)
{
    ParseParams({{"default_value", false, InternalValue(std::string(""))}, {"boolean", false, InternalValue(false)}}, params);
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

DictSort::DictSort(FilterParams params)
{

}

InternalValue DictSort::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}

GroupBy::GroupBy(FilterParams params)
{

}

InternalValue GroupBy::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}

Map::Map(FilterParams params)
{
    FilterParams newParams;

    if (params.kwParams.size() == 1 && params.posParams.empty() && params.kwParams.count("attribute") == 1)
    {
        newParams.kwParams["name"] = params.kwParams["attribute"];
        newParams.kwParams["filter"] = std::make_shared<ConstantExpression>(std::string("attr"));
    }
    else
    {
        newParams = std::move(params);
    }

    ParseParams({{"filter", true}}, newParams);
    m_mappingParams.kwParams = m_args.extraKwArgs;
    m_mappingParams.posParams = m_args.extraPosArgs;
}

InternalValue Map::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue filterName = GetArgumentValue("filter", context);
    if (IsEmpty(filterName))
        return InternalValue();

    auto filter = CreateFilter(AsString(filterName), m_mappingParams);
    if (!filter)
        return InternalValue();

    bool isConverted = false;
    auto list = ConvertToList(baseVal, isConverted);
    if (!isConverted)
        return InternalValue();

    InternalValueList resultList;
    resultList.reserve(list.GetSize());
    std::transform(list.begin(), list.end(), std::back_inserter(resultList), [filter, &context](auto& val) {return filter->Filter(val, context);});

    return ListAdapter::CreateAdapter(std::move(resultList));
}

PrettyPrint::PrettyPrint(FilterParams params)
{

}

InternalValue PrettyPrint::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}

Random::Random(FilterParams params)
{

}

InternalValue Random::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}

SequenceAccessor::SequenceAccessor(FilterParams params, SequenceAccessor::Mode mode)
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
        ParseParams({{"case_sensitive", false, InternalValue(false)}, {"attribute", false}}, params);
        break;
    case MinItemMode:
        ParseParams({{"case_sensitive", false, InternalValue(false)}, {"attribute", false}}, params);
        break;
    case ReverseMode:
        break;
    case SumItemsMode:
        ParseParams({{"attribute", false}, {"start", false}}, params);
        break;
    case UniqueItemsMode:
        break;
    }
}

InternalValue SequenceAccessor::Filter(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue result;

    bool isConverted = false;
    ListAdapter list = ConvertToList(baseVal, isConverted);

    if (!isConverted)
        return result;

    InternalValue attrName = GetArgumentValue("attribute", context);
    InternalValue isCsVal = GetArgumentValue("case_sensitive", context, InternalValue(false));

    BinaryExpression::CompareType compType =
            ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    auto lessComparator = [&attrName, &compType](auto& val1, auto& val2) {
        InternalValue cmpRes;

        if (IsEmpty(attrName))
            cmpRes = Apply2<visitors::BinaryMathOperation>(val1, val2, BinaryExpression::LogicalLt, compType);
        else
            cmpRes = Apply2<visitors::BinaryMathOperation>(Subscript(val1, attrName), Subscript(val2, attrName), BinaryExpression::LogicalLt, compType);

        return ConvertToBool(cmpRes);
    };

    auto equalComparator = [&attrName, &compType](auto& val1, auto& val2) {
        InternalValue cmpRes;

        if (IsEmpty(attrName))
            cmpRes = Apply2<visitors::BinaryMathOperation>(val1, val2, BinaryExpression::LogicalEq, compType);
        else
            cmpRes = Apply2<visitors::BinaryMathOperation>(Subscript(val1, attrName), Subscript(val2, attrName), BinaryExpression::LogicalEq, compType);

        return ConvertToBool(cmpRes);
    };

    switch (m_mode)
    {
    case FirstItemMode:
        result = list.GetSize() == 0 ? InternalValue() : list.GetValueByIndex(0);
        break;
    case LastItemMode:
        result = list.GetSize() == 0 ? InternalValue() : list.GetValueByIndex(list.GetSize() - 1);
        break;
    case LengthMode:
        result = static_cast<int64_t>(list.GetSize());
        break;
    case MaxItemMode:
    {
        auto b = list.begin();
        auto e = list.end();
        auto p = std::max_element(b, e, lessComparator);
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
        InternalValueList resultList(list.GetSize());
        for (int n = 0; n < list.GetSize(); ++ n)
            resultList[list.GetSize() - n - 1] = list.GetValueByIndex(n);

        result = ListAdapter::CreateAdapter(std::move(resultList));
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
        std::unique_copy(list.begin(), list.end(), std::back_inserter(resultList), equalComparator);
        result = ListAdapter::CreateAdapter(std::move(resultList));
        break;
    }
    }

    return result;
}

Serialize::Serialize(FilterParams params, Serialize::Mode mode)
{

}

InternalValue Serialize::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}

Slice::Slice(FilterParams params, Slice::Mode mode)
{

}

InternalValue Slice::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}

StringConverter::StringConverter(FilterParams params, StringConverter::Mode mode)
{

}

InternalValue StringConverter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}

StringFormat::StringFormat(FilterParams params, StringFormat::Mode mode)
{

}

InternalValue StringFormat::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}

Tester::Tester(FilterParams params, Tester::Mode mode)
{

}

InternalValue Tester::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}

ValueConverter::ValueConverter(FilterParams params, ValueConverter::Mode mode)
{

}

InternalValue ValueConverter::Filter(const InternalValue& baseVal, RenderContext& context)
{
    return InternalValue();
}


} // filters
} // jinja2
