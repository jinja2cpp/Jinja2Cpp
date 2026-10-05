#ifndef JINJA2CPP_SRC_EXPRESSION_EVALUATOR_H
#define JINJA2CPP_SRC_EXPRESSION_EVALUATOR_H

#include "internal_value.h"
#include "ordered_map.h"
#include "render_context.h"

#include <jinja2cpp/utils/i_comparable.h>

#include <cstddef>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

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
    // The value without a copy when it already lives somewhere (a variable's scope slot, a
    // constant), else null and the caller uses Evaluate. The reference is valid only until
    // the next expression is evaluated: consume it before evaluating anything else
    virtual const InternalValue* EvaluateRef(RenderContext& /*values*/) { return nullptr; }
    // A constant or a plain variable: evaluating it runs no template code that could change
    // a variable
    [[nodiscard]] virtual bool IsPure() const { return false; }
    virtual void Render(OutStream& stream, RenderContext& values);
};

template<typename T = ExpressionEvaluatorBase>
using ExpressionEvaluatorPtr = std::shared_ptr<T>;
using Expression = ExpressionEvaluatorBase;

inline bool operator==(const ExpressionEvaluatorPtr<>& lhs, const ExpressionEvaluatorPtr<>& rhs)
{
    if (lhs && rhs && !lhs->IsEqual(*rhs))
    {
        return false;
    }
    if ((lhs && !rhs) || (!lhs && rhs))
    {
        return false;
    }
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
    {
        return false;
    }
    if (lhs.posParams != rhs.posParams)
    {
        return false;
    }
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
    {
        return false;
    }
    if (lhs.posParams != rhs.posParams)
    {
        return false;
    }
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

    ArgumentInfo(std::string argName, bool isMandatory = false, InternalValue def = InternalValue()) // NOLINT(google-explicit-constructor)
        : name(std::move(argName))
        , mandatory(isMandatory)
        , defaultVal(std::move(def))
    {
    }
};

inline bool operator==(const ArgumentInfo& lhs, const ArgumentInfo& rhs)
{
    if (lhs.name != rhs.name)
    {
        return false;
    }
    if (lhs.mandatory != rhs.mandatory)
    {
        return false;
    }
    if (!(lhs.defaultVal == rhs.defaultVal))
    {
        return false;
    }
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
        {
            return ExpressionEvaluatorPtr<>();
        }

        return p->second;
    }
};

inline bool operator==(const ParsedArgumentsInfo& lhs, const ParsedArgumentsInfo& rhs)
{
    if (lhs.args != rhs.args)
    {
        return false;
    }
    if (lhs.extraKwArgs != rhs.extraKwArgs)
    {
        return false;
    }
    if (lhs.extraPosArgs != rhs.extraPosArgs)
    {
        return false;
    }
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
        {
            return InternalValue();
        }

        return p->second;
    }
};

inline bool operator==(const ParsedArguments& lhs, const ParsedArguments& rhs)
{
    if (lhs.args != rhs.args)
    {
        return false;
    }
    if (lhs.extraKwArgs != rhs.extraKwArgs)
    {
        return false;
    }
    if (lhs.extraPosArgs != rhs.extraPosArgs)
    {
        return false;
    }
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
    const InternalValue* EvaluateRef(RenderContext& values) override { return m_expression && !m_tester ? m_expression->EvaluateRef(values) : nullptr; }
    [[nodiscard]] bool IsPure() const override { return m_expression && !m_tester && m_expression->IsPure(); }
    void Render(OutStream& stream, RenderContext& values) override;
    // The wrapped expression when there is no inline `if`, else null
    [[nodiscard]] const Expression* GetPlainExpression() const { return m_tester ? nullptr : m_expression.get(); }
    [[nodiscard]] ExpressionEvaluatorPtr<Expression> GetPlainExpressionPtr() const { return m_tester ? nullptr : m_expression; }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* eval = dynamic_cast<const FullExpressionEvaluator*>(&other);
        if (!eval)
        {
            return false;
        }
        if (m_expression != eval->m_expression)
        {
            return false;
        }
        if (m_tester != eval->m_tester)
        {
            return false;
        }
        return true;
    }
private:
    ExpressionEvaluatorPtr<Expression> m_expression;
    ExpressionEvaluatorPtr<IfExpression> m_tester;
};

class ValueRefExpression : public Expression
{
public:
    explicit ValueRefExpression(std::string valueName)
        : m_valueName(std::move(valueName))
        , m_nameHash(HashedName::Hash(m_valueName))
    {
    }
    ValueRefExpression(const ValueRefExpression&) = delete;
    ValueRefExpression(ValueRefExpression&&) = delete;
    ValueRefExpression& operator=(const ValueRefExpression&) = delete;
    ValueRefExpression& operator=(ValueRefExpression&&) = delete;
    ~ValueRefExpression() override { LookupCache::ForThisThread().Forget(this); }
    InternalValue Evaluate(RenderContext& values) override;
    const InternalValue* EvaluateRef(RenderContext& values) override;
    [[nodiscard]] bool IsPure() const override { return true; }
    [[nodiscard]] const std::string& GetName() const { return m_valueName; }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* value = dynamic_cast<const ValueRefExpression*>(&other);
        if (!value)
        {
            return false;
        }
        return m_valueName == value->m_valueName;
    }
private:
    [[nodiscard]] HashedName GetHashedName() const { return HashedName{ m_valueName, m_nameHash }; }

    std::string m_valueName;
    size_t m_nameHash;
};

class SubscriptExpression : public Expression
{
public:
    explicit SubscriptExpression(ExpressionEvaluatorPtr<Expression> value)
        : m_value(std::move(value))
    {
    }
    InternalValue Evaluate(RenderContext& values) override;
    // x[expr], or x.name when attrName is set: x.name finds Python's methods before the
    // items, x[expr] the items first (Jinja2's getattr and getitem)
    void AddIndex(ExpressionEvaluatorPtr<Expression> value, std::string attrName = std::string());

    // For a call x.name(...): the name when the last index is an attribute, else null
    [[nodiscard]] const std::string* GetCallName() const
    {
        return !m_subscriptExprs.empty() && m_subscriptExprs.back().isAttr ? &m_subscriptExprs.back().attrName : nullptr;
    }
    // x without the last index. With forMutation, every list and dict on the way is made
    // one the template owns and stored back where it came from, so that a method that
    // changes it in place changes the variable (l.append(x), d['k'].append(x))
    InternalValue EvaluateReceiver(RenderContext& values, bool forMutation);
    // The whole expression, for a mutating method called on it
    InternalValue EvaluateMutable(RenderContext& values);

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* otherPtr = dynamic_cast<const SubscriptExpression*>(&other);
        if (!otherPtr)
        {
            return false;
        }
        if (m_value != otherPtr->m_value)
        {
            return false;
        }
        if (m_subscriptExprs.size() != otherPtr->m_subscriptExprs.size())
        {
            return false;
        }
        for (size_t n = 0; n < m_subscriptExprs.size(); ++n)
        {
            const auto& lhs = m_subscriptExprs[n];
            const auto& rhs = otherPtr->m_subscriptExprs[n];
            if (lhs.isAttr != rhs.isAttr || lhs.attrName != rhs.attrName || lhs.expr != rhs.expr)
            {
                return false;
            }
        }
        return true;
    }

private:
    struct Index
    {
        // Null for an attribute, which is looked up by attrName
        ExpressionEvaluatorPtr<Expression> expr;
        std::string attrName;
        bool isAttr = false;
        // Some value kind has a method of this name (decided once, at parse time)
        bool maybeMethod = false;
    };

    static InternalValue ApplyIndex(const InternalValue& cur, const Index& idx, RenderContext& values);
    // key is the evaluated item key, or null for an attribute
    static InternalValue LookupIndex(const InternalValue& cur, const Index& idx, const InternalValue* key, RenderContext& values);
    InternalValue ApplyFirstIndex(const InternalValue& root, RenderContext& values) const;
    static InternalValue LookupDefinedIndex(const InternalValue& cur, const Index& idx, const InternalValue* key, RenderContext& values);
    InternalValue EvaluateIndices(InternalValue cur, size_t first, size_t count, RenderContext& values, bool forMutation) const;

    ExpressionEvaluatorPtr<Expression> m_value;
    std::vector<Index> m_subscriptExprs;
    // The first index is an attribute name or a constant, so the value it is applied to
    // can be read in place: nothing runs between reading the value and indexing it
    bool m_firstIndexIsPure = false;
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
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* otherPtr = dynamic_cast<const FilteredExpression*>(&other);
        if (!otherPtr)
        {
            return false;
        }
        if (m_expression != otherPtr->m_expression)
        {
            return false;
        }
        if (m_filter != otherPtr->m_filter)
        {
            return false;
        }
        return true;
    }

private:
    ExpressionEvaluatorPtr<Expression> m_expression;
    ExpressionEvaluatorPtr<ExpressionFilter> m_filter;
};

class ConstantExpression : public Expression
{
public:
    explicit ConstantExpression(InternalValue constant)
        : m_constant(std::move(constant))
    {}
    InternalValue Evaluate(RenderContext&) override
    {
        return m_constant;
    }
    const InternalValue* EvaluateRef(RenderContext&) override { return &m_constant; }
    [[nodiscard]] bool IsPure() const override { return true; }
    [[nodiscard]] const InternalValue& GetValue() const { return m_constant; }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* otherVal = dynamic_cast<const ConstantExpression*>(&other);
        if (!otherVal)
        {
            return false;
        }
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
    [[nodiscard]] const std::vector<ExpressionEvaluatorPtr<>>& GetItems() const { return m_exprs; }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const TupleCreator*>(&other);
        if (!val)
        {
            return false;
        }
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

    explicit DictCreator(Items exprs)
        : m_exprs(std::move(exprs))
    {
    }

    InternalValue Evaluate(RenderContext&) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const DictCreator*>(&other);
        if (!val)
        {
            return false;
        }
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
        , m_expr(std::move(expr))
    {}
    InternalValue Evaluate(RenderContext&) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const UnaryExpression*>(&other);
        if (!val)
        {
            return false;
        }
        if (m_oper != val->m_oper)
        {
            return false;
        }
        if (m_expr != val->m_expr)
        {
            return false;
        }
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

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const IsExpression*>(&other);
        if (!val)
        {
            return false;
        }
        if (m_value != val->m_value)
        {
            return false;
        }
        if (m_tester != val->m_tester)
        {
            return false;
        }
        if (m_tester && val->m_tester && !m_tester->IsEqual(*val->m_tester))
        {
            return false;
        }
        return true;
    }

private:
    ExpressionEvaluatorPtr<> m_value;
    TesterPtr m_tester;
    // A built-in test without arguments runs nothing that could replace the variable it
    // reads, so it can test the variable in place
    bool m_testInPlace = false;
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

    BinaryExpression(Operation oper, ExpressionEvaluatorPtr<> leftExpr, const ExpressionEvaluatorPtr<>& rightExpr);
    InternalValue Evaluate(RenderContext&) override;
    InternalValue EvaluateWithLeft(const InternalValue& leftVal, RenderContext& context);
    // The operator applied to evaluated operands (not `and`/`or`)
    InternalValue Apply(const InternalValue& leftVal, const InternalValue& rightVal, RenderContext& context) const;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const BinaryExpression*>(&other);
        if (!val)
        {
            return false;
        }
        if (m_oper != val->m_oper)
        {
            return false;
        }
        if (m_leftExpr != val->m_leftExpr)
        {
            return false;
        }
        if (m_rightExpr != val->m_rightExpr)
        {
            return false;
        }
        return true;
    }
private:
    Operation m_oper;
    ExpressionEvaluatorPtr<> m_leftExpr;
    ExpressionEvaluatorPtr<> m_rightExpr;
    // Operands that are a plain variable or constant are read in place; the left one only
    // when the right one cannot change a variable
    bool m_leftByRef = false;
    bool m_rightByRef = false;
    // `x in [1, 2]` with a literal of scalar constants: the items, built once. They are
    // never handed out, so the literal still makes a fresh list wherever it is a value
    InternalValueList m_constItems;
    bool m_hasConstItems = false;
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

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const CompareExpression*>(&other);
        if (!val)
        {
            return false;
        }
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

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const SliceExpression*>(&other);
        if (!val)
        {
            return false;
        }
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
        , m_isNamedCallee(dynamic_cast<const ValueRefExpression*>(m_valueRef.get()) != nullptr)
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
        {
            return false;
        }
        if (m_valueRef != val->m_valueRef)
        {
            return false;
        }
        return m_params == val->m_params;
    }
private:
    InternalValue CallArbitraryFn(RenderContext& values, InternalValue fnVal);
    InternalValue CallCallable(RenderContext& values, const Callable& callable);
    void RenderCallable(OutStream& stream, RenderContext& values, const Callable& callable);
    // The callable a plain variable holds, copied out of its scope slot: a Callable copy shares
    // the function, while copying the InternalValue would allocate a new wrapper
    std::optional<Callable> FindNamedCallable(RenderContext& values) const;
    InternalValue CallLoopCycle(RenderContext& values);
    InternalValue CallWithCallee(RenderContext& values, InternalValue fnVal);
    // Evaluates the callee once. For x.name(...) where name is a Python method of x (s.upper(),
    // l.append(1)) it calls the method and returns true; otherwise it stores the callee.
    bool TryCallMethod(RenderContext& values, InternalValue& result, InternalValue& callee);

    ExpressionEvaluatorPtr<> m_valueRef;
    CallParamsInfo m_params;
    bool m_isNamedCallee = false;
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
        [[nodiscard]] virtual std::string GetArgumentsError() const { return std::string(); }
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
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* valuePtr = dynamic_cast<const ExpressionFilter*>(&other);
        if (!valuePtr)
        {
            return false;
        }
        if (m_filter && valuePtr->m_filter && !m_filter->IsEqual(*valuePtr->m_filter))
        {
            return false;
        }
        if ((m_filter && !valuePtr->m_filter) || (!m_filter && !valuePtr->m_filter))
        {
            return false;
        }
        if (m_parentFilter != valuePtr->m_parentFilter)
        {
            return false;
        }
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
        : m_testExpr(std::move(testExpr))
        , m_altValue(std::move(altValue))
    {
    }

    bool Evaluate(RenderContext& context);
    InternalValue EvaluateAltValue(RenderContext& context);
    [[nodiscard]] const ExpressionEvaluatorPtr<>& GetAltValue() const { return m_altValue; }

    void SetAltValue(ExpressionEvaluatorPtr<> altValue)
    {
        m_altValue = std::move(altValue);
    }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* valPtr = dynamic_cast<const IfExpression*>(&other);
        if (!valPtr)
        {
            return false;
        }
        if (m_testExpr != valPtr->m_testExpr)
        {
            return false;
        }
        if (m_altValue != valPtr->m_altValue)
        {
            return false;
        }
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
