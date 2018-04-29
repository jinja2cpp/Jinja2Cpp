#include "filters.h"
#include "value_visitors.h"

#include <algorithm>

namespace jinja2
{
namespace filters
{

Join::Join(FilterParams params)
{
    if (!helpers::FindParam(params, "param0", "d", m_delimiterEval))
        m_delimiterEval = std::make_shared<ConstantExpression>("");

    helpers::FindParam(params, "param1", "attribute", m_attribute);
}

Value Join::Filter(const Value& baseVal, RenderContext& context)
{
    if (!baseVal.isList())
        return Value();

    auto& values = baseVal.asList();
    bool isFirst = true;
    Value result;
    Value delimiter = m_delimiterEval->Evaluate(context);
    for (const Value& val : values)
    {
        if (isFirst)
            isFirst = false;
        else
            result = boost::apply_visitor(visitors::StringJoiner(), result.data(), delimiter.data());

        result = boost::apply_visitor(visitors::StringJoiner(), result.data(), val.data());
    }

    return result;
}

Sort::Sort(FilterParams params)
{
    helpers::FindParam(params, "", "attribute", m_attrNameEvaluator);
}

struct GetAsValuesListVisitor : boost::static_visitor<ValuesList>
{
    GetAsValuesListVisitor(Value attr = Value())
        : m_attr(std::move(attr))
    {}

    ValuesList operator() (const ValuesList& values) const
    {
        if (m_attr.isEmpty())
            return values;

        ValuesList result;
        std::transform(values.begin(), values.end(), std::back_inserter(result), [this](const Value& val) {return val.subscript(m_attr);});
        return result;
    }

    ValuesList operator() (const GenericList& values) const
    {
        int64_t size = values.GetSize();

        ValuesList result;
        for (int64_t idx = 0; idx < size; ++ idx)
        {
            auto val = values.GetValueByIndex(idx);
            if (!m_attr.isEmpty())
                result.push_back(val.subscript(m_attr));
            else
                result.push_back(val);
        }

        return result;
    }

    template<typename U>
    ValuesList operator() (U&&) const
    {
        return ValuesList();
    }

    Value m_attr;
};

Value Sort::Filter(const Value& baseVal, RenderContext& context)
{
    Value attrName = m_attrNameEvaluator->Evaluate(context);
    ValuesList values = boost::apply_visitor(GetAsValuesListVisitor(), baseVal.data());

    std::sort(values.begin(), values.end(), [&attrName](auto& val1, auto& val2) {
        Value cmpRes;
        if (attrName.isEmpty())
            cmpRes = boost::apply_visitor(visitors::BinaryMathOperation(BinaryExpression::LogicalLt), val1.data(), val2.data());
        else
            cmpRes = boost::apply_visitor(visitors::BinaryMathOperation(BinaryExpression::LogicalLt), val1.subscript(attrName).data(), val2.subscript(attrName).data());

        return boost::apply_visitor(visitors::BooleanEvaluator(), cmpRes.data());
    });

    return values;
}

} // filters
} // jinja2
