#ifndef JINJA2CPP_SRC_EXPRESSION_EVALUATOR_H
#define JINJA2CPP_SRC_EXPRESSION_EVALUATOR_H

#include "internal_value.h"
#include "loop_attr.h"
#include "lookup_result.h"
#include "node_arena.h"
#include "ordered_map.h"
#include "render_context.h"
#include "slot_frame.h"

#include <boost/container/small_vector.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
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

class ExpressionEvaluatorBase : public ArenaNode
{
public:
    static constexpr KindRange Family{ NodeKind::FullExpr, NodeKind::CallExpr };
    using FamilyOwner = ExpressionEvaluatorBase;

    ExpressionEvaluatorBase() = default;
    ExpressionEvaluatorBase(const ExpressionEvaluatorBase&) = delete;
    ExpressionEvaluatorBase& operator=(const ExpressionEvaluatorBase&) = delete;
    ExpressionEvaluatorBase& operator=(ExpressionEvaluatorBase&&) = delete;
    virtual ~ExpressionEvaluatorBase() = default;

    virtual InternalValue Evaluate(RenderContext& values) = 0;
    // The value without a copy when it already lives somewhere (a variable's scope slot, a
    // constant), else null and the caller uses Evaluate. The reference is valid only until
    // the next expression is evaluated: consume it before evaluating anything else
    virtual LookupResult EvaluateRef(RenderContext& /*values*/) { return {}; }
    // A constant or a plain variable: evaluating it runs no template code that could change
    // a variable
    [[nodiscard]] virtual bool IsPure(const NodeArena& /*nodes*/) const { return false; }
    // The value of a template literal, else null. A virtual, as parse-time dynamic_casts
    // show up in Load
    [[nodiscard]] virtual const InternalValue* GetConstant(const NodeArena& /*nodes*/) const { return nullptr; }
    virtual void Render(OutStream& stream, RenderContext& values);
protected:
    // Only the arena moves a node, when it seals the tree into one buffer
    ExpressionEvaluatorBase(ExpressionEvaluatorBase&&) = default;
};

using Expression = ExpressionEvaluatorBase;

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
    OrderedMap<std::string, NodeRef<Expression>> kwParams;
    std::vector<NodeRef<Expression>> posParams;
};

inline void VisitCallParams(detail::RefChecker& refs, const CallParamsInfo& params)
{
    refs.All(params.kwParams);
    refs.All(params.posParams);
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

// The arguments a call binds by name: a handful at most, so a flat list that keeps the
// first few in place costs no allocation and a few compares per lookup
template<typename V>
class ArgumentsMap
{
public:
    using value_type = std::pair<std::string, V>;
    using Storage = boost::container::small_vector<value_type, 3>;
    using iterator = typename Storage::iterator;
    using const_iterator = typename Storage::const_iterator;

    [[nodiscard]] const_iterator find(std::string_view name) const
    {
        return std::find_if(m_items.begin(), m_items.end(), [name](const value_type& item) { return item.first == name; });
    }
    [[nodiscard]] iterator find(std::string_view name)
    {
        return std::find_if(m_items.begin(), m_items.end(), [name](const value_type& item) { return item.first == name; });
    }
    V& operator[](const std::string& name)
    {
        auto p = find(name);
        if (p != m_items.end())
        {
            return p->second;
        }
        return m_items.emplace_back(name, V()).second;
    }
    [[nodiscard]] const_iterator begin() const { return m_items.begin(); }
    [[nodiscard]] const_iterator end() const { return m_items.end(); }
    [[nodiscard]] iterator begin() { return m_items.begin(); }
    [[nodiscard]] iterator end() { return m_items.end(); }
    [[nodiscard]] std::size_t size() const { return m_items.size(); }
    [[nodiscard]] bool empty() const { return m_items.empty(); }

    // As for a map: the same names with the same values, in any order
    friend bool operator==(const ArgumentsMap& lhs, const ArgumentsMap& rhs)
    {
        if (lhs.size() != rhs.size())
        {
            return false;
        }
        return std::all_of(lhs.begin(), lhs.end(), [&rhs](const value_type& item) {
            auto p = rhs.find(item.first);
            return p != rhs.end() && p->second == item.second;
        });
    }
    friend bool operator!=(const ArgumentsMap& lhs, const ArgumentsMap& rhs) { return !(lhs == rhs); }

private:
    Storage m_items;
};

// What a call binds at Load: a node per declared parameter, in the order of the declaration,
// null where the call passes none (the parameter then takes its declared default)
struct ParsedArgumentsInfo
{
    std::vector<NodeRef<Expression>> args;
    OrderedMap<std::string, NodeRef<Expression>> extraKwArgs;
    std::vector<NodeRef<Expression>> extraPosArgs;
};

struct ParsedArguments
{
    ArgumentsMap<InternalValue> args;
    InternalDict extraKwArgs;
    std::vector<InternalValue> extraPosArgs;

    InternalValue operator[](std::string_view name) const
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

class CompiledPercentFormat;
class ExpressionFilter;
class IfExpression;

class FullExpressionEvaluator : public ExpressionEvaluatorBase
{
public:
    static constexpr NodeKind Kind = NodeKind::FullExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expression);
        refs(m_tester);
    }

    void SetExpression(NodeRef<Expression> expr)
    {
        m_expression = std::move(expr);
    }
    void SetTester(NodeRef<IfExpression> expr)
    {
        m_tester = std::move(expr);
    }
    InternalValue Evaluate(RenderContext& values) override;
    LookupResult EvaluateRef(RenderContext& values) override { return m_expression && !m_tester ? values.Nodes()[m_expression].EvaluateRef(values) : LookupResult(); }
    [[nodiscard]] bool IsPure(const NodeArena& nodes) const override { return m_expression && !m_tester && nodes[m_expression].IsPure(nodes); }
    [[nodiscard]] const InternalValue* GetConstant(const NodeArena& nodes) const override { return m_expression && !m_tester ? nodes[m_expression].GetConstant(nodes) : nullptr; }
    void Render(OutStream& stream, RenderContext& values) override;
private:
    NodeRef<Expression> m_expression;
    NodeRef<IfExpression> m_tester;
};

class ValueRefExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::NameRef;
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void VisitRefs(detail::RefChecker& /*refs*/) const {}
    // `self` is a variable too
    static bool MatchesKind(NodeKind kind) { return kind == NodeKind::NameRef || kind == NodeKind::SelfRef; }

    explicit ValueRefExpression(std::string valueName)
        : m_valueName(std::move(valueName))
        , m_nameHash(HashedName::Hash(m_valueName))
    {
    }
    ValueRefExpression(const ValueRefExpression&) = delete;
    // For the arena, which moves the nodes when it seals the tree
    ValueRefExpression(ValueRefExpression&&) = default;
    ValueRefExpression& operator=(const ValueRefExpression&) = delete;
    ValueRefExpression& operator=(ValueRefExpression&&) = delete;
    ~ValueRefExpression() override { LookupCache::ForThisThread().Forget(this, m_cacheSlot); }
    InternalValue Evaluate(RenderContext& values) override;
    LookupResult EvaluateRef(RenderContext& values) override;
    [[nodiscard]] bool IsPure(const NodeArena& /*nodes*/) const override { return true; }
    [[nodiscard]] const std::string& GetName() const { return m_valueName; }
    // The name is read from slot `slot` of the frame of `unit` (0117 P1)
    void SetSlot(SlotIndex slot, UnitId unit)
    {
        m_slot = slot;
        m_unit = unit;
    }
    [[nodiscard]] SlotIndex GetSlot() const { return m_slot; }
    [[nodiscard]] UnitId GetUnit() const { return m_unit; }
    [[nodiscard]] HashedName GetHashedName() const { return HashedName{ m_valueName, m_nameHash }; }

private:
    // The slot's value, when the frame of the unit is installed and the slot bound
    [[nodiscard]] LookupResult ReadSlot(RenderContext& values) const;


    std::string m_valueName;
    size_t m_nameHash;
    uint32_t m_cacheSlot = LookupCache::NewSlot();
    SlotIndex m_slot;
    UnitId m_unit;
};

// The name `self` (docs/tasks/0139): the running template, unless a scope of it sets the name.
// The template makes it only when this asks for it
class SelfRefExpression final : public ValueRefExpression
{
public:
    static constexpr NodeKind Kind = NodeKind::SelfRef;
    static bool MatchesKind(NodeKind kind) { return kind == Kind; }

    SelfRefExpression()
        : ValueRefExpression("self")
    {
    }
    InternalValue Evaluate(RenderContext& values) override;
    LookupResult EvaluateRef(RenderContext& values) override;
};

class SubscriptExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::SubscriptExpr;
    static bool MatchesKind(NodeKind kind) { return kind == NodeKind::SubscriptExpr || kind == NodeKind::LoopAttrExpr; }
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_value);
        refs.All(m_subscriptExprs, [](detail::RefChecker& r, const Index& idx) { r(idx.expr); });
    }

    explicit SubscriptExpression(NodeRef<Expression> value)
        : m_value(std::move(value))
    {
    }
    InternalValue Evaluate(RenderContext& values) override;
    // x[expr], or x.name when attrName is set: x.name finds Python's methods before the
    // items, x[expr] the items first (Jinja2's getattr and getitem)
    void AddIndex(const NodeArena& nodes, NodeRef<Expression> value, std::string attrName = std::string());

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

protected:
    struct Index
    {
        // Null for an attribute, which is looked up by attrName
        NodeRef<Expression> expr;
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

    NodeRef<Expression> m_value;
    // Most subscripts are one attribute or item: a.b, x[0]
    boost::container::small_vector<Index, 1> m_subscriptExprs;
    // The first index is an attribute name or a constant, so the value it is applied to
    // can be read in place: nothing runs between reading the value and indexing it
    bool m_firstIndexIsPure = false;
};

// `loop.<attribute>`: a subscript of the name `loop` whose first index is one of the loop
// object's attributes, resolved at Load (0117 P1). When `loop` is a for loop's object the
// attribute is read directly; any other value bound to the name `loop` takes the generic
// subscript path
class LoopAttrExpression final : public SubscriptExpression
{
public:
    static constexpr NodeKind Kind = NodeKind::LoopAttrExpr;
    static bool MatchesKind(NodeKind kind) { return kind == Kind; }

    LoopAttrExpression(NodeRef<Expression> value, LoopAttr attr)
        : SubscriptExpression(value)
        , m_attr(attr)
    {
    }
    InternalValue Evaluate(RenderContext& values) override;

    // `loop.cycle(...)` called on a for loop's object: puts the argument for the current item
    // in `result` and returns true; false when `loop` is something else
    bool TryCallCycle(RenderContext& values, const CallParamsInfo& params, InternalValue& result) const;

private:
    LoopAttr m_attr;
};

class FilteredExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::FilteredExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expression);
        refs(m_filter);
    }

    // A constant operand is handed to the first filter at Load, which may prepare for it
    FilteredExpression(const NodeArena& nodes, NodeRef<Expression> expression, NodeRef<ExpressionFilter> filter);
    InternalValue Evaluate(RenderContext&) override;

private:
    NodeRef<Expression> m_expression;
    NodeRef<ExpressionFilter> m_filter;
};

class ConstantExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::ConstantExpr;
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void VisitRefs(detail::RefChecker& /*refs*/) const {}

    explicit ConstantExpression(InternalValue constant)
        : m_constant(std::move(constant))
    {}
    InternalValue Evaluate(RenderContext&) override
    {
        return m_constant;
    }
    LookupResult EvaluateRef(RenderContext&) override { return LookupResult(m_constant); }
    [[nodiscard]] bool IsPure(const NodeArena& /*nodes*/) const override { return true; }
    [[nodiscard]] const InternalValue* GetConstant(const NodeArena& /*nodes*/) const override { return &m_constant; }
    [[nodiscard]] const InternalValue& GetValue() const { return m_constant; }
private:
    InternalValue m_constant;
};

class TupleCreator : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::TupleExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_exprs);
    }

    // Builds both list and tuple literals; isTuple makes the value print as (a, b)
    explicit TupleCreator(ArenaSpan<NodeRef<Expression>> exprs, bool isTuple = false)
        : m_exprs(exprs)
        , m_isTuple(isTuple)
    {
    }

    InternalValue Evaluate(RenderContext&) override;
    [[nodiscard]] ArenaSpan<NodeRef<Expression>> GetItems() const { return m_exprs; }
private:
    ArenaSpan<NodeRef<Expression>> m_exprs;
    bool m_isTuple = false;
};
class DictCreator : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::DictExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_exprs, [](detail::RefChecker& r, const Item& item) {
            r(item.key);
            r(item.value);
        });
    }

    struct Item
    {
        NodeRef<Expression> key;
        NodeRef<Expression> value;
    };
    // Key and value expressions in source order
    using Items = ArenaSpan<Item>;

    explicit DictCreator(Items exprs)
        : m_exprs(exprs)
    {
    }

    InternalValue Evaluate(RenderContext&) override;
private:
    Items m_exprs;
};

class UnaryExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::UnaryExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expr);
    }

    enum Operation
    {
        LogicalNot,
        UnaryPlus,
        UnaryMinus
    };

    UnaryExpression(Operation oper, NodeRef<Expression> expr)
        : m_oper(oper)
        , m_expr(std::move(expr))
    {}
    InternalValue Evaluate(RenderContext&) override;

private:
    Operation m_oper;
    NodeRef<Expression> m_expr;
};

class IsExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::IsExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_value);
        refs(m_tester);
    }

    IsExpression(const IsExpression&) = delete;
    // For the arena, which moves the nodes when it seals the tree
    IsExpression(IsExpression&&) = default;
    IsExpression& operator=(const IsExpression&) = delete;
    IsExpression& operator=(IsExpression&&) = delete;
    ~IsExpression() override = default;

    // A test. Those of the template live in its arena, those a filter names at render time
    // on the heap
    struct ITester : ArenaObjectBase
    {
        static constexpr NodeKind Kind = NodeKind::TesterObject;

        // The handles the test holds (RefChecker)
        virtual void VisitRefs(detail::RefChecker& refs) const = 0;

        virtual bool Test(const InternalValue& baseVal, RenderContext& context) = 0;
    };

    // The test is made first: an arena makes one object at a time.
    // registered: the test the environment adds under this name (TemplateEnv::AddTest), if any
    static NodeRef<IsExpression> Make(NodeArena& nodes, NodeRef<Expression> value, const std::string& tester, const CallParamsInfo& params, InternalValue registered = InternalValue());

    IsExpression(NodeRef<Expression> value, NodeRef<ITester> tester, bool testInPlace)
        : m_value(value)
        , m_tester(tester)
        , m_testInPlace(testInPlace)
    {
    }
    InternalValue Evaluate(RenderContext& context) override;

private:
    NodeRef<Expression> m_value;
    NodeRef<ITester> m_tester;
    // A built-in test without arguments runs nothing that could replace the variable it
    // reads, so it can test the variable in place
    bool m_testInPlace = false;
};

class BinaryExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::BinaryExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_leftExpr);
        refs(m_rightExpr);
    }

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

    BinaryExpression(const NodeArena& nodes, Operation oper, NodeRef<Expression> leftExpr, NodeRef<Expression> rightExpr);
    InternalValue Evaluate(RenderContext&) override;
    InternalValue EvaluateWithLeft(const InternalValue& leftVal, RenderContext& context);
    // The operator applied to evaluated operands (not `and`/`or`)
    InternalValue Apply(const InternalValue& leftVal, const InternalValue& rightVal, RenderContext& context) const;
    // A literal format % values, with the format parsed at Load
    [[nodiscard]] InternalValue FormatConstant(const InternalValue& rightVal) const;
private:
    Operation m_oper;
    NodeRef<Expression> m_leftExpr;
    NodeRef<Expression> m_rightExpr;
    // Operands that are a plain variable or constant are read in place; the left one only
    // when the right one cannot change a variable
    bool m_leftByRef = false;
    bool m_rightByRef = false;
    // `x in [1, 2]` with a literal of scalar constants: the items, built once. They are
    // never handed out, so the literal still makes a fresh list wherever it is a value
    InternalValueList m_constItems;
    bool m_hasConstItems = false;
    // 'literal' % values: the format, parsed once. Null for any other operands
    std::shared_ptr<const CompiledPercentFormat> m_constFormat;
};

// A chain of comparisons, a < b <= c: each operand is evaluated once and the chain stops
// at the first false link, as in Python. A single comparison is a BinaryExpression.
class CompareExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::CompareExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_first);
        refs.All(m_operands, [](detail::RefChecker& r, const Operand& operand) { r(operand.expr); });
    }

    struct Operand
    {
        BinaryExpression::Operation operation = BinaryExpression::LogicalEq;
        bool negated = false; // not in
        NodeRef<Expression> expr;
    };
    using Operands = std::vector<Operand>;

    CompareExpression(NodeRef<Expression> first, Operands operands)
        : m_first(std::move(first))
        , m_operands(std::move(operands))
    {
    }
    InternalValue Evaluate(RenderContext&) override;

private:
    NodeRef<Expression> m_first;
    Operands m_operands;
};

// value[start:stop:step]; omitted parts are null
class SliceExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::SliceExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_value);
        refs(m_start);
        refs(m_stop);
        refs(m_step);
    }

    SliceExpression(NodeRef<Expression> value, NodeRef<Expression> start, NodeRef<Expression> stop, NodeRef<Expression> step)
        : m_value(std::move(value))
        , m_start(std::move(start))
        , m_stop(std::move(stop))
        , m_step(std::move(step))
    {
    }
    InternalValue Evaluate(RenderContext&) override;

private:
    NodeRef<Expression> m_value;
    NodeRef<Expression> m_start;
    NodeRef<Expression> m_stop;
    NodeRef<Expression> m_step;
};

class CallExpression : public Expression
{
public:
    static constexpr NodeKind Kind = NodeKind::CallExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_valueRef);
        VisitCallParams(refs, m_params);
    }

    CallExpression(const NodeArena& nodes, NodeRef<Expression> valueRef, CallParamsInfo params)
        : m_valueRef(valueRef)
        , m_params(std::move(params))
        , m_isNamedCallee(nodes.Is<ValueRefExpression>(m_valueRef))
    {
    }

    InternalValue Evaluate(RenderContext& values) override;
    void Render(OutStream& stream, RenderContext& values) override;

    [[nodiscard]] NodeRef<Expression> GetValueRef() const { return m_valueRef; }
    [[nodiscard]] const CallParamsInfo& GetParams() const { return m_params; }
    // Calls fnVal with arguments already evaluated, as a call written in the template would
    static InternalValue CallValue(RenderContext& values, InternalValue fnVal, const CallParams& params);
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

    NodeRef<Expression> m_valueRef;
    CallParamsInfo m_params;
    bool m_isNamedCallee = false;
};

class ExpressionFilter : public ArenaNode
{
public:
    static constexpr NodeKind Kind = NodeKind::FilterChain;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_filter);
        refs(m_parentFilter);
    }

    // A filter. Those of the template live in its arena, those a filter names at render time
    // on the heap
    struct IExpressionFilter : ArenaObjectBase
    {
        static constexpr NodeKind Kind = NodeKind::FilterObject;

        // The handles the filter holds (RefChecker)
        virtual void VisitRefs(detail::RefChecker& refs) const = 0;

        virtual InternalValue Filter(const InternalValue& baseVal, RenderContext& context) = 0;
        // Why the arguments do not fit the filter's parameters; empty if they fit
        [[nodiscard]] virtual std::string GetArgumentsError() const { return std::string(); }
        // The value the filter is always applied to, when it is a template literal. Called
        // at Load; a filter may prepare for that value, but must still accept it in Filter
        virtual void SetConstantBase(const InternalValue& /*base*/) {}
    };
    // The filter is made first: an arena makes one object at a time.
    // registered: the filter the environment adds under this name (TemplateEnv::AddFilter), if any
    static NodeRef<ExpressionFilter> Make(NodeArena& nodes, const std::string& filterName, const CallParamsInfo& params, InternalValue registered = InternalValue());

    // argsError: why the call does not fit the filter, null if it fits
    ExpressionFilter(NodeRef<IExpressionFilter> filter, std::unique_ptr<std::string> argsError)
        : m_filter(filter)
        , m_argsError(std::move(argsError))
    {
    }

    InternalValue Evaluate(const InternalValue& baseVal, RenderContext& context);
    void SetParentFilter(NodeRef<ExpressionFilter> parentFilter)
    {
        m_parentFilter = parentFilter;
    }
    // Tells the first filter of the chain that its input is always this literal
    void SetConstantBase(const NodeArena& nodes, const InternalValue& base);

private:
    NodeRef<IExpressionFilter> m_filter;
    // Jinja2 reports a call that does not fit when the filter runs, not when it is parsed;
    // null when it fits
    std::unique_ptr<std::string> m_argsError;
    NodeRef<ExpressionFilter> m_parentFilter;
};

class IfExpression : public ArenaNode
{
public:
    static constexpr NodeKind Kind = NodeKind::IfExpr;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_testExpr);
        refs(m_altValue);
    }

    IfExpression(NodeRef<Expression> testExpr, NodeRef<Expression> altValue)
        : m_testExpr(std::move(testExpr))
        , m_altValue(std::move(altValue))
    {
    }

    bool Evaluate(RenderContext& context);
    InternalValue EvaluateAltValue(RenderContext& context);
    [[nodiscard]] NodeRef<Expression> GetAltValue() const { return m_altValue; }

    void SetAltValue(NodeRef<Expression> altValue)
    {
        m_altValue = altValue;
    }

private:
    NodeRef<Expression> m_testExpr;
    NodeRef<Expression> m_altValue;
};

namespace helpers
{
ParsedArguments ParseCallParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParams& params, bool& isSucceeded);
ParsedArguments ParseCallParams(const std::vector<ArgumentInfo>& args, const CallParams& params, bool& isSucceeded);
ParsedArgumentsInfo ParseCallParamsInfo(const std::vector<ArgumentInfo>& args, const CallParamsInfo& params, bool& isSucceeded);
CallParams EvaluateCallParams(const CallParamsInfo& info, RenderContext& context);
} // namespace helpers
} // namespace jinja2

#endif // JINJA2CPP_SRC_EXPRESSION_EVALUATOR_H
