#include "filters.h"
#include "value_visitors.h"
#include "value_helpers.h"

#include <algorithm>

namespace jinja2
{
namespace filters
{

bool FilterBase::ParseParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParams& params)
{
    bool result = true;
    m_args = helpers::ParseCallParams(argsInfo, params, result);

    return result;
}

Value FilterBase::GetArgumentValue(std::string argName, RenderContext& context, Value defVal)
{
    auto argExpr = m_args[argName];
    return argExpr ? argExpr->Evaluate(context) : std::move(defVal);
}

Join::Join(FilterParams params)
{
    ParseParams({{"d", false, std::string()}, {"attribute"}}, params);
}

Value Join::Filter(const Value& baseVal, RenderContext& context)
{
    if (!baseVal.isList())
        return Value();

    Value attrName = GetArgumentValue("attribute", context);

    ValuesList values = AsValueList(baseVal, attrName);

    bool isFirst = true;
    Value result;
    Value delimiter = m_args["d"]->Evaluate(context);
    for (const Value& val : values)
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
    ParseParams({{"reverse", false, Value(false)}, {"case_sensitive", false, Value(false)}, {"attribute", false}}, params);
}

Value Sort::Filter(const Value& baseVal, RenderContext& context)
{
    Value attrName = GetArgumentValue("attribute", context);
    Value isReverseVal = GetArgumentValue("reverse", context, Value(false));
    Value isCsVal = GetArgumentValue("case_sensitive", context, Value(false));

    ValuesList values = AsValueList(baseVal);
    BinaryExpression::Operation oper =
            ConvertToBool(isReverseVal) ? BinaryExpression::LogicalGt : BinaryExpression::LogicalLt;
    BinaryExpression::CompareType compType =
            ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    std::sort(values.begin(), values.end(), [&attrName, oper, compType](auto& val1, auto& val2) {
        Value cmpRes;
        if (attrName.isEmpty())
            cmpRes = boost::apply_visitor(visitors::BinaryMathOperation(oper, compType), val1.data(), val2.data());
        else
            cmpRes = boost::apply_visitor(visitors::BinaryMathOperation(oper, compType), val1.subscript(attrName).data(), val2.subscript(attrName).data());

        return ConvertToBool(cmpRes);
    });

    return values;
}

Attribute::Attribute(FilterParams params)
{

}

Value Attribute::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

Default::Default(FilterParams params)
{
    ParseParams({{"default_value", false, Value("")}, {"boolean", false, Value(false)}}, params);
}

Value Default::Filter(const Value& baseVal, RenderContext& context)
{
    Value defaultVal = GetArgumentValue("default_value", context);
    Value conditionResult = GetArgumentValue("boolean", context);

    if (baseVal.isEmpty())
        return defaultVal;

    if (ConvertToBool(conditionResult) && !ConvertToBool(baseVal))
        return defaultVal;

    return baseVal;
}

DictSort::DictSort(FilterParams params)
{

}

Value DictSort::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

GroupBy::GroupBy(FilterParams params)
{

}

Value GroupBy::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

Map::Map(FilterParams params)
{

}

Value Map::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

PrettyPrint::PrettyPrint(FilterParams params)
{

}

Value PrettyPrint::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

Random::Random(FilterParams params)
{

}

Value Random::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
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
        ParseParams({{"case_sensitive", false, Value(false)}, {"attribute", false}}, params);
        break;
    case MinItemMode:
        ParseParams({{"case_sensitive", false, Value(false)}, {"attribute", false}}, params);
        break;
    case ReverseMode:
        break;
    case SumItemsMode:
        break;
    case UniqueItemsMode:
        break;
    }
}

Value SequenceAccessor::Filter(const Value& baseVal, RenderContext& context)
{
    GenericList list = AsGenericList(baseVal);
    Value result;

    if (!list.IsValid())
        return result;

    Value attrName = GetArgumentValue("attribute", context);
    Value isCsVal = GetArgumentValue("case_sensitive", context, Value(false));

    BinaryExpression::CompareType compType =
            ConvertToBool(isCsVal) ? BinaryExpression::CaseSensitive : BinaryExpression::CaseInsensitive;

    auto lessComparator = [&attrName, &compType](auto& val1, auto& val2) {
        Value cmpRes;

        if (attrName.isEmpty())
            // cmpRes = Apply2<visitors::BinaryMathOperation>(val1, val2, BinaryExpression::LogicalLt, compType);
            cmpRes = boost::apply_visitor(visitors::BinaryMathOperation(BinaryExpression::LogicalLt, compType), val1.data(), val2.data());
        else
            cmpRes = Apply2<visitors::BinaryMathOperation>(val1.subscript(attrName), val2.subscript(attrName), BinaryExpression::LogicalLt, compType);

        return ConvertToBool(cmpRes);
    };

    switch (m_mode)
    {
    case FirstItemMode:
        result = list.GetSize() == 0 ? Value() : list.GetValueByIndex(0);
        break;
    case LastItemMode:
        result = list.GetSize() == 0 ? Value() : list.GetValueByIndex(list.GetSize() - 1);
        break;
    case LengthMode:
        result = static_cast<int64_t>(list.GetSize());
        break;
    case MaxItemMode:
    {
        auto b = begin(list);
        auto e = end(list);
        auto p = std::max_element(b, e, lessComparator);
        result = p != e ? *p : Value();
        break;
    }
    case MinItemMode:
    {
        auto b = begin(list);
        auto e = end(list);
        auto p = std::min_element(b, e, lessComparator);
        result = p != e ? *p : Value();
        break;
        break;
    }
    case ReverseMode:
        break;
    case SumItemsMode:
        break;
    case UniqueItemsMode:
        break;
    }

    return result;
}

Serialize::Serialize(FilterParams params, Serialize::Mode mode)
{

}

Value Serialize::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

Slice::Slice(FilterParams params, Slice::Mode mode)
{

}

Value Slice::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

StringConverter::StringConverter(FilterParams params, StringConverter::Mode mode)
{

}

Value StringConverter::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

StringFormat::StringFormat(FilterParams params, StringFormat::Mode mode)
{

}

Value StringFormat::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

Tester::Tester(FilterParams params, Tester::Mode mode)
{

}

Value Tester::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}

ValueConverter::ValueConverter(FilterParams params, ValueConverter::Mode mode)
{

}

Value ValueConverter::Filter(const Value& baseVal, RenderContext& context)
{
    return Value();
}


} // filters
} // jinja2
