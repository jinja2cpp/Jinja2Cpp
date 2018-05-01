#ifndef EXPRESSION_EVALUATOR_H
#define EXPRESSION_EVALUATOR_H

#include "jinja2cpp/value.h"
#include "render_context.h"

#include <memory>

namespace jinja2
{

class ExpressionEvaluatorBase
{
public:
    virtual ~ExpressionEvaluatorBase() {}

    virtual Value Evaluate(RenderContext& values) = 0;
};

template<typename T = ExpressionEvaluatorBase>
using ExpressionEvaluatorPtr = std::shared_ptr<T>;
using Expression = ExpressionEvaluatorBase;

struct CallParams
{
    std::unordered_map<std::string, ExpressionEvaluatorPtr<>> kwParams;
    std::vector<ExpressionEvaluatorPtr<>> posParams;
};

class ExpressionFilter;
class IfExpression;

class FullExpressionEvaluator : public ExpressionEvaluatorBase
{
public:
    void SetExpression(ExpressionEvaluatorPtr<Expression> expr)
    {
        m_expression = expr;
    }
    void SetFilter(ExpressionEvaluatorPtr<ExpressionFilter> expr)
    {
        m_filter = expr;
    }
    void SetTester(ExpressionEvaluatorPtr<IfExpression> expr)
    {
        m_tester = expr;
    }
    Value Evaluate(RenderContext& values) override;
private:
    ExpressionEvaluatorPtr<Expression> m_expression;
    ExpressionEvaluatorPtr<ExpressionFilter> m_filter;
    ExpressionEvaluatorPtr<IfExpression> m_tester;
};

class ValueRefExpression : public Expression
{
public:
    ValueRefExpression(std::string valueName)
        : m_valueName(valueName)
    {
    }
    Value Evaluate(RenderContext& values) override;
private:
    std::string m_valueName;
};

class SubscriptExpression : public Expression
{
public:
    SubscriptExpression(ExpressionEvaluatorPtr<Expression> value, ExpressionEvaluatorPtr<Expression> subscriptExpr)
        : m_value(value)
        , m_subscriptExpr(subscriptExpr)
    {
    }
    Value Evaluate(RenderContext& values) override;
private:
    ExpressionEvaluatorPtr<Expression> m_value;
    ExpressionEvaluatorPtr<Expression> m_subscriptExpr;
};

class ConstantExpression : public Expression
{
public:
    ConstantExpression(Value constant)
        : m_constant(constant)
    {}
    Value Evaluate(RenderContext&) override
    {
        return m_constant;
    }
private:
    Value m_constant;
};

class TupleCreator : public Expression
{
public:
    TupleCreator(std::vector<ExpressionEvaluatorPtr<>> exprs)
        : m_exprs(std::move(exprs))
    {
    }

    Value Evaluate(RenderContext&) override;

private:
    std::vector<ExpressionEvaluatorPtr<>> m_exprs;
};

class DictionaryCreator : public Expression
{
public:
    DictionaryCreator(std::unordered_map<std::string, ExpressionEvaluatorPtr<>> items)
        : m_items(std::move(items))
    {
    }

    Value Evaluate(RenderContext&) override;

private:
    std::unordered_map<std::string, ExpressionEvaluatorPtr<>> m_items;
};

class DictCreator : public Expression
{
public:
    DictCreator(std::unordered_map<std::string, ExpressionEvaluatorPtr<>> exprs)
        : m_exprs(std::move(exprs))
    {
    }

    Value Evaluate(RenderContext&) override;

private:
    std::unordered_map<std::string, ExpressionEvaluatorPtr<>> m_exprs;
};

class UnaryExpression : public Expression
{
public:
    enum Operation
    {
        LogicalNot,
        UnaryPlus,
        UnaryMinus
    };

    UnaryExpression(Operation oper, ExpressionEvaluatorPtr<> expr)
        : m_oper(oper)
        , m_expr(expr)
    {}
    Value Evaluate(RenderContext&) override;
private:
    Operation m_oper;
    ExpressionEvaluatorPtr<> m_expr;
};

class BinaryExpression : public Expression
{
public:
    enum Operation
    {
        LogicalAnd,
        LogicalOr,
        LogicalEq,
        LogicalNe,
        LogicalGt,
        LogicalLt,
        LogicalGe,
        LogicalLe,
        In,
        Plus,
        Minus,
        Mul,
        Div,
        DivReminder,
        DivInteger,
        Pow,
        StringConcat
    };

    BinaryExpression(Operation oper, ExpressionEvaluatorPtr<> leftExpr, ExpressionEvaluatorPtr<> rightExpr)
        : m_oper(oper)
        , m_leftExpr(leftExpr)
        , m_rightExpr(rightExpr)
    {}
    Value Evaluate(RenderContext&) override;
private:
    Operation m_oper;
    ExpressionEvaluatorPtr<> m_leftExpr;
    ExpressionEvaluatorPtr<> m_rightExpr;
};

class IsExpression : public Expression
{
public:
    virtual ~IsExpression() {}

    struct ITester
    {
        virtual ~ITester() {}
        virtual bool Test(const Value& baseVal, RenderContext& context) = 0;
    };

    using TesterFactoryFn = std::function<std::shared_ptr<ITester> (CallParams params)>;

    IsExpression(ExpressionEvaluatorPtr<> value, std::string tester, CallParams params);
    Value Evaluate(RenderContext& context) override;

private:
    ExpressionEvaluatorPtr<> m_value;
    std::shared_ptr<ITester> m_tester;
    static std::unordered_map<std::string, TesterFactoryFn> s_testers;
};

class ExpressionFilter
{
public:
    virtual ~ExpressionFilter() {}

    struct IExpressionFilter
    {
        virtual ~IExpressionFilter() {}
        virtual Value Filter(const Value& baseVal, RenderContext& context) = 0;
    };

    using FilterFactoryFn = std::function<std::shared_ptr<IExpressionFilter> (CallParams params)>;

    ExpressionFilter(std::string filterName, CallParams params);

    Value Evaluate(const Value& baseVal, RenderContext& context);
    void SetParentFilter(std::shared_ptr<ExpressionFilter> parentFilter)
    {
        m_parentFilter = parentFilter;
    }
private:
    std::shared_ptr<IExpressionFilter> m_filter;
    std::shared_ptr<ExpressionFilter> m_parentFilter;

    static std::unordered_map<std::string, FilterFactoryFn> s_filters;
};

class IfExpression
{
public:
    virtual ~IfExpression() {}

    IfExpression(ExpressionEvaluatorPtr<> testExpr, ExpressionEvaluatorPtr<> altValue)
        : m_testExpr(testExpr)
        , m_altValue(altValue)
    {
    }

    bool Evaluate(RenderContext& context);
    Value EvaluateAltValue(RenderContext& context);

    void SetAltValue(ExpressionEvaluatorPtr<> altValue)
    {
        m_altValue = altValue;
    }

private:
    ExpressionEvaluatorPtr<> m_testExpr;
    ExpressionEvaluatorPtr<> m_altValue;
};

namespace helpers
{
constexpr size_t NoPosParam = std::numeric_limits<size_t>::max();

inline bool FindParam(const CallParams& params, size_t pos, std::string paramName, ExpressionEvaluatorPtr<>& value)
{
    auto p = params.kwParams.find(paramName);
    if (p != params.kwParams.end())
    {
        value = p->second;
        return true;
    }

    if (pos < params.posParams.size())
    {
        value = params.posParams[pos];
        return true;
    }

    return false;
}
}
} // jinja2

#endif // EXPRESSION_EVALUATOR_H
