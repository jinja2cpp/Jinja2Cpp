#ifndef EXPRESSION_EVALUATOR_H
#define EXPRESSION_EVALUATOR_H

#include "internal_value.h"
#include "render_context.h"

#include <memory>
#include <limits>

namespace jinja2
{

class ExpressionEvaluatorBase
{
public:
    virtual ~ExpressionEvaluatorBase() {}

    virtual InternalValue Evaluate(RenderContext& values) = 0;
};

template<typename T = ExpressionEvaluatorBase>
using ExpressionEvaluatorPtr = std::shared_ptr<T>;
using Expression = ExpressionEvaluatorBase;

struct CallParams
{
    std::unordered_map<std::string, ExpressionEvaluatorPtr<>> kwParams;
    std::vector<ExpressionEvaluatorPtr<>> posParams;
};

struct ArgumentInfo
{
    std::string name;
    bool mandatory;
    InternalValue defaultVal;

    ArgumentInfo(std::string argName, bool isMandatory = false, InternalValue def = InternalValue())
        : name(std::move(argName))
        , mandatory(isMandatory)
        , defaultVal(std::move(def))
    {
    }
};

struct ParsedArguments
{
    std::unordered_map<std::string, ExpressionEvaluatorPtr<>> args;
    std::unordered_map<std::string, ExpressionEvaluatorPtr<>> extraKwArgs;
    std::vector<ExpressionEvaluatorPtr<>> extraPosArgs;

    ExpressionEvaluatorPtr<> operator[](std::string name) const
    {
        auto p = args.find(name);
        if (p == args.end())
            return ExpressionEvaluatorPtr<>();

        return p->second;
    }
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
    InternalValue Evaluate(RenderContext& values) override;
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
    InternalValue Evaluate(RenderContext& values) override;
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
    InternalValue Evaluate(RenderContext& values) override;
private:
    ExpressionEvaluatorPtr<Expression> m_value;
    ExpressionEvaluatorPtr<Expression> m_subscriptExpr;
};

class ConstantExpression : public Expression
{
public:
    ConstantExpression(InternalValue constant)
        : m_constant(constant)
    {}
    InternalValue Evaluate(RenderContext&) override
    {
        return m_constant;
    }
private:
    InternalValue m_constant;
};

class TupleCreator : public Expression
{
public:
    TupleCreator(std::vector<ExpressionEvaluatorPtr<>> exprs)
        : m_exprs(std::move(exprs))
    {
    }

    InternalValue Evaluate(RenderContext&) override;

private:
    std::vector<ExpressionEvaluatorPtr<>> m_exprs;
};
/*
class DictionaryCreator : public Expression
{
public:
    DictionaryCreator(std::unordered_map<std::string, ExpressionEvaluatorPtr<>> items)
        : m_items(std::move(items))
    {
    }

    InternalValue Evaluate(RenderContext&) override;

private:
    std::unordered_map<std::string, ExpressionEvaluatorPtr<>> m_items;
};*/

class DictCreator : public Expression
{
public:
    DictCreator(std::unordered_map<std::string, ExpressionEvaluatorPtr<>> exprs)
        : m_exprs(std::move(exprs))
    {
    }

    InternalValue Evaluate(RenderContext&) override;

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
    InternalValue Evaluate(RenderContext&) override;
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

    enum CompareType
    {
        Undefined = 0,
        CaseSensitive = 0,
        CaseInsensitive = 1
    };

    BinaryExpression(Operation oper, ExpressionEvaluatorPtr<> leftExpr, ExpressionEvaluatorPtr<> rightExpr)
        : m_oper(oper)
        , m_leftExpr(leftExpr)
        , m_rightExpr(rightExpr)
    {}
    InternalValue Evaluate(RenderContext&) override;
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
        virtual bool Test(const InternalValue& baseVal, RenderContext& context) = 0;
    };

    using TesterFactoryFn = std::function<std::shared_ptr<ITester> (CallParams params)>;

    IsExpression(ExpressionEvaluatorPtr<> value, std::string tester, CallParams params);
    InternalValue Evaluate(RenderContext& context) override;

private:
    ExpressionEvaluatorPtr<> m_value;
    std::shared_ptr<ITester> m_tester;
};

class CallExpression : public Expression
{
public:
    virtual ~CallExpression() {}

    CallExpression(std::vector<std::string> valueRef, CallParams params)
        : m_valueRef(std::move(valueRef))
        , m_params(std::move(params))
    {
    }

    InternalValue Evaluate(RenderContext &values);

    auto& GetValueRef() const {return m_valueRef;}
    auto& GetParams() const {return m_params;}

private:
    InternalValue CallGlobalRange(RenderContext &values);
    InternalValue CallLoopCycle(RenderContext &values);

private:
    std::vector<std::string> m_valueRef;
    CallParams m_params;
};

class ExpressionFilter
{
public:
    virtual ~ExpressionFilter() {}

    struct IExpressionFilter
    {
        virtual ~IExpressionFilter() {}
        virtual InternalValue Filter(const InternalValue& baseVal, RenderContext& context) = 0;
    };

    using FilterFactoryFn = std::function<std::shared_ptr<IExpressionFilter> (CallParams params)>;

    ExpressionFilter(std::string filterName, CallParams params);

    InternalValue Evaluate(const InternalValue& baseVal, RenderContext& context);
    void SetParentFilter(std::shared_ptr<ExpressionFilter> parentFilter)
    {
        m_parentFilter = parentFilter;
    }
private:
    std::shared_ptr<IExpressionFilter> m_filter;
    std::shared_ptr<ExpressionFilter> m_parentFilter;
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
    InternalValue EvaluateAltValue(RenderContext& context);

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
ParsedArguments ParseCallParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParams& params, bool& isSucceeded);

//constexpr size_t NoPosParam = std::numeric_limits<size_t>::max();

//inline bool FindParam(const CallParams& params, size_t pos, std::string paramName, ExpressionEvaluatorPtr<>& value)
//{
//    auto p = params.kwParams.find(paramName);
//    if (p != params.kwParams.end())
//    {
//        value = p->second;
//        return true;
//    }

//    if (pos < params.posParams.size())
//    {
//        value = params.posParams[pos];
//        return true;
//    }

//    return false;
//}
}
} // jinja2

#endif // EXPRESSION_EVALUATOR_H
