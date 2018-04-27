#include "expression_evaluator.h"
#include "filters.h"
#include "testers.h"
#include "value_visitors.h"

#include <cmath>

namespace jinja2
{

std::unordered_map<std::string, ExpressionFilter::FilterFactoryFn> ExpressionFilter::s_filters = {
    {"join", &FilterFactory<filters::Join>::Create},
};

std::unordered_map<std::string, IsExpression::TesterFactoryFn> IsExpression::s_testers = {
    {"defined", &TesterFactory<testers::Defined>::Create},
    {"startsWith", &TesterFactory<testers::StartsWith>::Create},
};

Value FullExpressionEvaluator::Evaluate(RenderContext& values)
{
    if (!m_expression)
        return Value();

    Value origVal = m_expression->Evaluate(values);
    if (m_filter)
        origVal = m_filter->Evaluate(origVal, values);

    if (m_tester && !m_tester->Evaluate(values))
        return m_tester->EvaluateAltValue(values);

    return origVal;
}

Value ValueRefExpression::Evaluate(RenderContext& values)
{
    bool found = false;
    auto p = values.FindValue(m_valueName, found);
    if (found)
        return p->second;

    return Value();
}

Value SubscriptExpression::Evaluate(RenderContext& values)
{
    return m_value->Evaluate(values).subscript(m_subscriptExpr->Evaluate(values));
}

Value UnaryExpression::Evaluate(RenderContext& values)
{
    return boost::apply_visitor(visitors::UnaryOperation(m_oper), m_expr->Evaluate(values).data());
}

Value BinaryExpression::Evaluate(RenderContext& context)
{
    Value leftVal = m_leftExpr->Evaluate(context);
    Value rightVal = m_rightExpr->Evaluate(context);
    Value result;

    switch (m_oper)
    {
    case jinja2::BinaryExpression::LogicalAnd:
    case jinja2::BinaryExpression::LogicalOr:
        break;
    case jinja2::BinaryExpression::LogicalEq:
    case jinja2::BinaryExpression::LogicalNe:
    case jinja2::BinaryExpression::LogicalGt:
    case jinja2::BinaryExpression::LogicalLt:
    case jinja2::BinaryExpression::LogicalGe:
    case jinja2::BinaryExpression::LogicalLe:
    case jinja2::BinaryExpression::In:
    case jinja2::BinaryExpression::Plus:
    case jinja2::BinaryExpression::Minus:
    case jinja2::BinaryExpression::Mul:
    case jinja2::BinaryExpression::Div:
    case jinja2::BinaryExpression::DivReminder:
    case jinja2::BinaryExpression::DivInteger:
    case jinja2::BinaryExpression::Pow:
        result = boost::apply_visitor(visitors::BinaryMathOperation(m_oper), leftVal.data(), rightVal.data());
        break;
    case jinja2::BinaryExpression::StringConcat:
    default:
        break;
    }
    return result;
}

Value TupleCreator::Evaluate(RenderContext& context)
{
    ValuesList result;
    for (auto& e : m_exprs)
    {
        result.push_back(e->Evaluate(context));
    }

    return Value(result);
}

Value DictCreator::Evaluate(RenderContext& context)
{
    ValuesMap result;
    for (auto& e : m_exprs)
    {
        result[e.first] = e.second->Evaluate(context);
    }

    return Value(result);
}

ExpressionFilter::ExpressionFilter(std::string filterName, std::unordered_map<std::string, ExpressionEvaluatorPtr<> > params)
{
    auto p = s_filters.find(filterName);
    if (p == s_filters.end())
        throw std::runtime_error("Can't find filter '" + filterName + "'");

    m_filter = p->second(std::move(params));
}

Value ExpressionFilter::Evaluate(const Value& baseVal, RenderContext& context)
{
    if (m_parentFilter)
        return m_filter->Filter(m_parentFilter->Evaluate(baseVal, context), context);

    return m_filter->Filter(baseVal, context);
}

IsExpression::IsExpression(ExpressionEvaluatorPtr<> value, std::string tester, CallParamsList params)
    : m_value(value)
{
    auto p = s_testers.find(tester);
    if (p == s_testers.end())
        throw std::runtime_error("Can't find tester '" + tester + "'");

    m_tester = p->second(std::move(params));
}

Value IsExpression::Evaluate(RenderContext& context)
{
    return m_tester->Test(m_value->Evaluate(context), context);
}

bool IfExpression::Evaluate(RenderContext& context)
{
    return boost::apply_visitor(visitors::BooleanEvaluator(), m_testExpr->Evaluate(context).data());
}

Value IfExpression::EvaluateAltValue(RenderContext& context)
{
    return m_altValue ? m_altValue->Evaluate(context) : Value();
}

}
