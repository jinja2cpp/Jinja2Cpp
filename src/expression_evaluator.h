#ifndef JINJA2CPP_SRC_EXPRESSION_EVALUATOR_H
#define JINJA2CPP_SRC_EXPRESSION_EVALUATOR_H

#include "internal_value.h"
#include "render_context.h"

#include <jinja2cpp/utils/i_comparable.h>

#include <memory>
#include <limits>

namespace jinja2
{

enum
{
    InvalidFn = -1,
    LoopCycleFn = 2
};

class ExpressionEvaluatorBase : public IComparable
{
public:
    ~ExpressionEvaluatorBase() override = default;

    virtual InternalValue Evaluate(RenderContext& values) = 0;
    virtual void Render(OutStream& stream, RenderContext& values);
};

template<typename T = ExpressionEvaluatorBase>
using ExpressionEvaluatorPtr = std::shared_ptr<T>;
using Expression = ExpressionEvaluatorBase;

inline bool operator==(const ExpressionEvaluatorPtr<>& lhs, const ExpressionEvaluatorPtr<>& rhs)
{
    if (lhs && rhs && !lhs->IsEqual(*rhs))
        return false;
    if ((lhs && !rhs) || (!lhs && rhs))
        return false;
    return true;
}
inline bool operator!=(const ExpressionEvaluatorPtr<>& lhs, const ExpressionEvaluatorPtr<>& rhs)
{
    return !(lhs == rhs);
}

struct CallParams
{
    InternalDict kwParams;
    std::vector<InternalValue> posParams;
};

inline bool operator==(const CallParams& lhs, const CallParams& rhs)
{
    if (lhs.kwParams != rhs.kwParams)
        return false;
    if (lhs.posParams != rhs.posParams)
        return false;
    return true;
}

inline bool operator!=(const CallParams& lhs, const CallParams& rhs)
{
    return !(lhs == rhs);
}

struct CallParamsInfo
{
    OrderedMap<std::string, ExpressionEvaluatorPtr<>> kwParams;
    std::vector<ExpressionEvaluatorPtr<>> posParams;
};

inline bool operator==(const CallParamsInfo& lhs, const CallParamsInfo& rhs)
{
    if (lhs.kwParams != rhs.kwParams)
        return false;
    if (lhs.posParams != rhs.posParams)
        return false;
    return true;
}

inline bool operator!=(const CallParamsInfo& lhs, const CallParamsInfo& rhs)
{
    return !(lhs == rhs);
}

struct ArgumentInfo
{
    std::string name;
    bool mandatory = false;
    InternalValue defaultVal;

    ArgumentInfo(std::string argName, bool isMandatory = false, InternalValue def = InternalValue())
        : name(std::move(argName))
        , mandatory(isMandatory)
        , defaultVal(std::move(def))
    {
    }
};

inline bool operator==(const ArgumentInfo& lhs, const ArgumentInfo& rhs)
{
    if (lhs.name != rhs.name)
        return false;
    if (lhs.mandatory != rhs.mandatory)
        return false;
    if (!(lhs.defaultVal == rhs.defaultVal))
        return false;
    return true;
}

inline bool operator!=(const ArgumentInfo& lhs, const ArgumentInfo& rhs)
{
    return !(lhs == rhs);
}

struct ParsedArgumentsInfo
{
    std::unordered_map<std::string, ExpressionEvaluatorPtr<>> args;
    OrderedMap<std::string, ExpressionEvaluatorPtr<>> extraKwArgs;
    std::vector<ExpressionEvaluatorPtr<>> extraPosArgs;

    ExpressionEvaluatorPtr<> operator[](const std::string& name) const
    {
        auto p = args.find(name);
        if (p == args.end())
            return ExpressionEvaluatorPtr<>();

        return p->second;
    }
};

inline bool operator==(const ParsedArgumentsInfo& lhs, const ParsedArgumentsInfo& rhs)
{
    if (lhs.args != rhs.args)
        return false;
    if (lhs.extraKwArgs != rhs.extraKwArgs)
        return false;
    if (lhs.extraPosArgs != rhs.extraPosArgs)
        return false;
    return true;
}

inline bool operator!=(const ParsedArgumentsInfo& lhs, const ParsedArgumentsInfo& rhs)
{
    return !(lhs == rhs);
}

struct ParsedArguments
{
    std::unordered_map<std::string, InternalValue> args;
    InternalDict extraKwArgs;
    std::vector<InternalValue> extraPosArgs;

    InternalValue operator[](const std::string& name) const
    {
        auto p = args.find(name);
        if (p == args.end())
            return InternalValue();

        return p->second;
    }
};

inline bool operator==(const ParsedArguments& lhs, const ParsedArguments& rhs)
{
    if (lhs.args != rhs.args)
        return false;
    if (lhs.extraKwArgs != rhs.extraKwArgs)
        return false;
    if (lhs.extraPosArgs != rhs.extraPosArgs)
        return false;
    return true;
}

inline bool operator!=(const ParsedArguments& lhs, const ParsedArguments& rhs)
{
    return !(lhs == rhs);
}

class ExpressionFilter;
class IfExpression;

class FullExpressionEvaluator : public ExpressionEvaluatorBase
{
public:
    void SetExpression(ExpressionEvaluatorPtr<Expression> expr)
    {
        m_expression = std::move(expr);
    }
    void SetTester(ExpressionEvaluatorPtr<IfExpression> expr)
    {
        m_tester = std::move(expr);
    }
    InternalValue Evaluate(RenderContext& values) override;
    void Render(OutStream& stream, RenderContext& values) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* eval = dynamic_cast<const FullExpressionEvaluator*>(&other);
        if (!eval)
            return false;
        if (m_expression != eval->m_expression)
            return false;
        if (m_tester != eval->m_tester)
            return false;
        return true;
    }
private:
    ExpressionEvaluatorPtr<Expression> m_expression;
    ExpressionEvaluatorPtr<IfExpression> m_tester;
};

class ValueRefExpression : public Expression
{
public:
    ValueRefExpression(std::string valueName)
        : m_valueName(std::move(valueName))
    {
    }
    InternalValue Evaluate(RenderContext& values) override;
    const std::string& GetName() const { return m_valueName; }

    bool IsEqual(const IComparable& other) const override
    {
        const auto* value = dynamic_cast<const ValueRefExpression*>(&other);
        if (!value)
            return false;
        return m_valueName == value->m_valueName;
    }
private:
    std::string m_valueName;
};

class SubscriptExpression : public Expression
{
public:
    SubscriptExpression(ExpressionEvaluatorPtr<Expression> value)
        : m_value(value)
    {
    }
    InternalValue Evaluate(RenderContext& values) override;
    // x[expr], or x.name when attrName is set: x.name finds Python's methods before the
    // items, x[expr] the items first (Jinja2's getattr and getitem)
    void AddIndex(ExpressionEvaluatorPtr<Expression> value, std::string attrName = std::string());

    // For a call x.name(...): the name when the last index is an attribute, else null
    const std::string* GetCallName() const
    {
        return !m_subscriptExprs.empty() && m_subscriptExprs.back().isAttr ? &m_subscriptExprs.back().attrName : nullptr;
    }
    // x without the last index. With forMutation, every list and dict on the way is made
    // one the template owns and stored back where it came from, so that a method that
    // changes it in place changes the variable (l.append(x), d['k'].append(x))
    InternalValue EvaluateReceiver(RenderContext& values, bool forMutation);
    // The whole expression, for a mutating method called on it
    InternalValue EvaluateMutable(RenderContext& values);

    bool IsEqual(const IComparable& other) const override
    {
        const auto* otherPtr = dynamic_cast<const SubscriptExpression*>(&other);
        if (!otherPtr)
            return false;
        if (m_value != otherPtr->m_value)
            return false;
        if (m_subscriptExprs.size() != otherPtr->m_subscriptExprs.size())
            return false;
        for (size_t n = 0; n < m_subscriptExprs.size(); ++n)
        {
            const auto& lhs = m_subscriptExprs[n];
            const auto& rhs = otherPtr->m_subscriptExprs[n];
            if (lhs.isAttr != rhs.isAttr || lhs.attrName != rhs.attrName || lhs.expr != rhs.expr)
                return false;
        }
        return true;
    }

private:
    struct Index
    {
        ExpressionEvaluatorPtr<Expression> expr;
        std::string attrName;
        bool isAttr = false;
        // Some value kind has a method of this name (decided once, at parse time)
        bool maybeMethod = false;
    };

    InternalValue ApplyIndex(const InternalValue& cur, const Index& idx, RenderContext& values) const;
    InternalValue LookupIndex(const InternalValue& cur, const Index& idx, const InternalValue& key, RenderContext& values) const;
    InternalValue EvaluateIndices(InternalValue cur, size_t count, RenderContext& values, bool forMutation) const;

    ExpressionEvaluatorPtr<Expression> m_value;
    std::vector<Index> m_subscriptExprs;
};

class FilteredExpression : public Expression
{
public:
    explicit FilteredExpression(ExpressionEvaluatorPtr<Expression> expression, ExpressionEvaluatorPtr<ExpressionFilter> filter)
        : m_expression(std::move(expression))
        , m_filter(std::move(filter))
    {
    }
    InternalValue Evaluate(RenderContext&) override;
    bool IsEqual(const IComparable& other) const override
    {
        const auto* otherPtr = dynamic_cast<const FilteredExpression*>(&other);
        if (!otherPtr)
            return false;
        if (m_expression != otherPtr->m_expression)
            return false;
        if (m_filter != otherPtr->m_filter)
            return false;
        return true;
    }

private:
    ExpressionEvaluatorPtr<Expression> m_expression;
    ExpressionEvaluatorPtr<ExpressionFilter> m_filter;
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

    bool IsEqual(const IComparable& other) const override
    {
        const auto* otherVal = dynamic_cast<const ConstantExpression*>(&other);
        if (!otherVal)
            return false;
        return m_constant == otherVal->m_constant;
    }
private:
    InternalValue m_constant;
};

class TupleCreator : public Expression
{
public:
    // Builds both list and tuple literals; isTuple makes the value print as (a, b)
    explicit TupleCreator(std::vector<ExpressionEvaluatorPtr<>> exprs, bool isTuple = false)
        : m_exprs(std::move(exprs))
        , m_isTuple(isTuple)
    {
    }

    InternalValue Evaluate(RenderContext&) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const TupleCreator*>(&other);
        if (!val)
            return false;
        return m_exprs == val->m_exprs && m_isTuple == val->m_isTuple;
    }
private:
    std::vector<ExpressionEvaluatorPtr<>> m_exprs;
    bool m_isTuple = false;
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
    // Key and value expressions in source order
    using Items = std::vector<std::pair<ExpressionEvaluatorPtr<>, ExpressionEvaluatorPtr<>>>;

    DictCreator(Items exprs)
        : m_exprs(std::move(exprs))
    {
    }

    InternalValue Evaluate(RenderContext&) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const DictCreator*>(&other);
        if (!val)
            return false;
        return m_exprs == val->m_exprs;
    }
private:
    Items m_exprs;
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

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const UnaryExpression*>(&other);
        if (!val)
            return false;
        if (m_oper != val->m_oper)
            return false;
        if (m_expr != val->m_expr)
            return false;
        return true;
    }

private:
    Operation m_oper;
    ExpressionEvaluatorPtr<> m_expr;
};

class IsExpression : public Expression
{
public:
    ~IsExpression() override = default;

    struct ITester : IComparable
    {
        ~ITester() override = default;
        virtual bool Test(const InternalValue& baseVal, RenderContext& context) = 0;
    };
    using TesterPtr = std::shared_ptr<ITester>;
    using TesterFactoryFn = std::function<TesterPtr(CallParamsInfo params)>;

    // registered: the test the environment adds under this name (TemplateEnv::AddTest), if any
    IsExpression(ExpressionEvaluatorPtr<> value, const std::string& tester, CallParamsInfo params, InternalValue registered = InternalValue());
    InternalValue Evaluate(RenderContext& context) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const IsExpression*>(&other);
        if (!val)
            return false;
        if (m_value != val->m_value)
            return false;
        if (m_tester != val->m_tester)
            return false;
        if (m_tester && val->m_tester && !m_tester->IsEqual(*val->m_tester))
            return false;
        return true;
    }

private:
    ExpressionEvaluatorPtr<> m_value;
    TesterPtr m_tester;
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
        DivRemainder,
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

    BinaryExpression(Operation oper, ExpressionEvaluatorPtr<> leftExpr, ExpressionEvaluatorPtr<> rightExpr);
    InternalValue Evaluate(RenderContext&) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const BinaryExpression*>(&other);
        if (!val)
            return false;
        if (m_oper != val->m_oper)
            return false;
        if (m_leftExpr != val->m_leftExpr)
            return false;
        if (m_rightExpr != val->m_rightExpr)
            return false;
        if (m_inTester && val->m_inTester && !m_inTester->IsEqual(*val->m_inTester))
            return false;
        if ((!m_inTester && val->m_inTester) || (m_inTester && !val->m_inTester))
            return false;
        return true;
    }
private:
    Operation m_oper;
    ExpressionEvaluatorPtr<> m_leftExpr;
    ExpressionEvaluatorPtr<> m_rightExpr;
    IsExpression::TesterPtr m_inTester;
};


// A chain of comparisons, a < b <= c: each operand is evaluated once and the chain stops
// at the first false link, as in Python. A single comparison is a BinaryExpression.
class CompareExpression : public Expression
{
public:
    struct Operand
    {
        BinaryExpression::Operation operation = BinaryExpression::LogicalEq;
        bool negated = false; // not in
        ExpressionEvaluatorPtr<> expr;

        bool operator==(const Operand& other) const
        {
            return operation == other.operation && negated == other.negated && expr == other.expr;
        }
        bool operator!=(const Operand& other) const { return !(*this == other); }
    };
    using Operands = std::vector<Operand>;

    CompareExpression(ExpressionEvaluatorPtr<> first, Operands operands)
        : m_first(std::move(first))
        , m_operands(std::move(operands))
    {
    }
    InternalValue Evaluate(RenderContext&) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const CompareExpression*>(&other);
        if (!val)
            return false;
        return m_first == val->m_first && m_operands == val->m_operands;
    }

private:
    ExpressionEvaluatorPtr<> m_first;
    Operands m_operands;
};

// value[start:stop:step]; omitted parts are null
class SliceExpression : public Expression
{
public:
    SliceExpression(ExpressionEvaluatorPtr<> value, ExpressionEvaluatorPtr<> start, ExpressionEvaluatorPtr<> stop, ExpressionEvaluatorPtr<> step)
        : m_value(std::move(value))
        , m_start(std::move(start))
        , m_stop(std::move(stop))
        , m_step(std::move(step))
    {
    }
    InternalValue Evaluate(RenderContext&) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const SliceExpression*>(&other);
        if (!val)
            return false;
        return m_value == val->m_value && m_start == val->m_start && m_stop == val->m_stop && m_step == val->m_step;
    }

private:
    ExpressionEvaluatorPtr<> m_value;
    ExpressionEvaluatorPtr<> m_start;
    ExpressionEvaluatorPtr<> m_stop;
    ExpressionEvaluatorPtr<> m_step;
};


class CallExpression : public Expression
{
public:
    ~CallExpression() override = default;

    CallExpression(ExpressionEvaluatorPtr<> valueRef, CallParamsInfo params)
        : m_valueRef(std::move(valueRef))
        , m_params(std::move(params))
    {
    }

    InternalValue Evaluate(RenderContext& values) override;
    void Render(OutStream& stream, RenderContext& values) override;

    auto& GetValueRef() const { return m_valueRef; }
    auto& GetParams() const { return m_params; }

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const CallExpression*>(&other);
        if (!val)
            return false;
        if (m_valueRef != val->m_valueRef)
            return false;
        return m_params == val->m_params;
    }
private:
    InternalValue CallArbitraryFn(RenderContext& values, InternalValue fnVal);
    InternalValue CallLoopCycle(RenderContext& values);
    InternalValue CallWithCallee(RenderContext& values, InternalValue fnVal);
    // Evaluates the callee once. For x.name(...) where name is a Python method of x (s.upper(),
    // l.append(1)) it calls the method and returns true; otherwise it stores the callee.
    bool TryCallMethod(RenderContext& values, InternalValue& result, InternalValue& callee);


    ExpressionEvaluatorPtr<> m_valueRef;
    CallParamsInfo m_params;
};

class ExpressionFilter : public IComparable
{
public:
    ~ExpressionFilter() override = default;

    struct IExpressionFilter : IComparable
    {
        ~IExpressionFilter() override = default;
        virtual InternalValue Filter(const InternalValue& baseVal, RenderContext& context) = 0;
        // Why the arguments do not fit the filter's parameters; empty if they fit
        virtual std::string GetArgumentsError() const { return std::string(); }
    };
    using ExpressionFilterPtr = std::shared_ptr<IExpressionFilter>;
    using FilterFactoryFn = std::function<ExpressionFilterPtr(CallParamsInfo params)>;

    // registered: the filter the environment adds under this name (TemplateEnv::AddFilter), if any
    ExpressionFilter(const std::string& filterName, CallParamsInfo params, InternalValue registered = InternalValue());

    InternalValue Evaluate(const InternalValue& baseVal, RenderContext& context);
    void SetParentFilter(std::shared_ptr<ExpressionFilter> parentFilter)
    {
        m_parentFilter = std::move(parentFilter);
    }
    bool IsEqual(const IComparable& other) const override
    {
        const auto* valuePtr = dynamic_cast<const ExpressionFilter*>(&other);
        if (!valuePtr)
            return false;
        if (m_filter && valuePtr->m_filter && !m_filter->IsEqual(*valuePtr->m_filter))
            return false;
        if ((m_filter && !valuePtr->m_filter) || (!m_filter && !valuePtr->m_filter))
            return false;
        if (m_parentFilter != valuePtr->m_parentFilter)
            return false;
        return true;
    }


private:
    ExpressionFilterPtr m_filter;
    // Jinja2 reports a call that does not fit when the filter runs, not when it is parsed
    std::string m_argsError;
    std::shared_ptr<ExpressionFilter> m_parentFilter;
};


class IfExpression : public IComparable
{
public:
    ~IfExpression() override = default;

    IfExpression(ExpressionEvaluatorPtr<> testExpr, ExpressionEvaluatorPtr<> altValue)
        : m_testExpr(testExpr)
        , m_altValue(altValue)
    {
    }

    bool Evaluate(RenderContext& context);
    InternalValue EvaluateAltValue(RenderContext& context);

    void SetAltValue(ExpressionEvaluatorPtr<> altValue)
    {
        m_altValue = std::move(altValue);
    }

    bool IsEqual(const IComparable& other) const override
    {
        const auto* valPtr = dynamic_cast<const IfExpression*>(&other);
        if (!valPtr)
            return false;
        if (m_testExpr != valPtr->m_testExpr)
            return false;
        if (m_altValue != valPtr->m_altValue)
            return false;
        return true;
    }

private:
    ExpressionEvaluatorPtr<> m_testExpr;
    ExpressionEvaluatorPtr<> m_altValue;
};

namespace helpers
{
ParsedArguments ParseCallParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParams& params, bool& isSucceeded);
ParsedArguments ParseCallParams(const std::vector<ArgumentInfo>& args, const CallParams& params, bool& isSucceeded);
ParsedArgumentsInfo ParseCallParamsInfo(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParamsInfo& params, bool& isSucceeded);
ParsedArgumentsInfo ParseCallParamsInfo(const std::vector<ArgumentInfo>& args, const CallParamsInfo& params, bool& isSucceeded);
CallParams EvaluateCallParams(const CallParamsInfo& info, RenderContext& context);
} // namespace helpers
} // namespace jinja2

#endif // JINJA2CPP_SRC_EXPRESSION_EVALUATOR_H
