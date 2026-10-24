#include "expression_evaluator.h"

#include "engine_errors.h"
#include "filters.h"
#include "internal_value.h"
#include "lookup_result.h"
#include "loop_attr.h"
#include "markup.h"
#include "node_arena.h"
#include "out_stream.h"
#include "python_format.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "small_vector.h"
#include "span.h"
#include "template_dict.h"
#include "testers.h"
#include "undefined.h"
#include "value_methods.h"
#include "value_visitors.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <array>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{
inline namespace v2
{

// Out of line, so that the paths without autoescape stay small
void WriteEscaped(OutStream& stream, const InternalValue& val, IRendererCallback* callback)
{
    // The escape buffer is the thread's: a sink called while the stream still holds the
    // escaped text could render, and escape, into it
    const auto view = callback->IsWideTarget() || stream.ReachesSink() ? std::nullopt : AsStringView<char>(val);
    if (view && view->size() <= detail::maxBufferedHtmlEscape)
    {
        auto escaped = detail::EscapeHtmlToBuffer(*view);
        stream.WriteBuffer(escaped.data(), escaped.size());
        return;
    }
    stream.WriteValue(MarkupEscape(val, callback));
}

void ExpressionEvaluatorBase::Render(OutStream& stream, RenderContext& values)
{
    if (const auto value = EvaluateRef(values))
    {
        WriteOutput(stream, *value, values);
        return;
    }
    WriteOutput(stream, Evaluate(values), values);
}

InternalValue FullExpressionEvaluator::Evaluate(RenderContext& values)
{
    CheckStack();
    if (!m_expression)
    {
        return InternalValue();
    }

    // Python evaluates the condition first, then only the branch it picks
    if (m_tester && !values.Nodes()[m_tester].Evaluate(values))
    {
        return values.Nodes()[m_tester].EvaluateAltValue(values);
    }

    return values.Nodes()[m_expression].Evaluate(values);
}

namespace
{
// Out of line: inlined into a final class's Render, the value's temporaries cost every
// render of a plain expression a larger frame
JINJA2CPP_NOINLINE_INLINE void RenderEvaluated(Expression& expr, OutStream& stream, RenderContext& values)
{
    expr.Expression::Render(stream, values);
}
} // namespace

void FullExpressionEvaluator::Render(OutStream& stream, RenderContext& values)
{
    if (!m_tester)
    {
        values.Nodes()[m_expression].Render(stream, values);
    }
    else if (m_expression && values.Nodes()[m_tester].GetAltValue())
    {
        // The branch the condition picks renders itself, a variable without a copy
        CheckStack();
        if (values.Nodes()[m_tester].Evaluate(values))
        {
            values.Nodes()[m_expression].Render(stream, values);
        }
        else
        {
            values.Nodes()[values.Nodes()[m_tester].GetAltValue()].Render(stream, values);
        }
    }
    else
    {
        RenderEvaluated(*this, stream, values);
    }
}

#ifdef JINJA2CPP_CHECK_SLOTS
namespace
{
// A name read from its slot must be what a lookup by name finds: the resolver kept every
// store of the name, every frame and every view in mind, or this aborts with the name
JINJA2CPP_NOINLINE_INLINE void CheckSlotRead(RenderContext& values, const HashedName& name, LookupResult fromSlot)
{
    const auto byName = values.FindValue(name);
    if (!byName.IsSame(fromSlot))
    {
        std::fprintf(stderr, "jinja2cpp: the slot of '%.*s' differs from its lookup by name\n", static_cast<int>(name.name.size()), name.name.data());
        std::abort();
    }
}
} // namespace
#endif

LookupResult ValueRefExpression::ReadSlot(RenderContext& values) const
{
    const auto value = values.ReadSlot(m_slot, m_unit);
#ifdef JINJA2CPP_CHECK_SLOTS
    if (value)
    {
        CheckSlotRead(values, GetHashedName(values.Nodes()), value);
    }
#endif
    return value;
}

LookupResult ValueRefExpression::EvaluateRef(RenderContext& values)
{
    if (!m_slot.IsDynamic())
    {
        if (const auto value = ReadSlot(values))
        {
            return value;
        }
    }
    return values.FindValueCached(values.Nodes().KeyOf(m_valueName), m_cacheSlot, [this, &values] { return GetHashedName(values.Nodes()); });
}

InternalValue ValueRefExpression::Evaluate(RenderContext& values)
{
    // EvaluateRef's steps spelled out, so that the lookup by name inlines here as before
    if (!m_slot.IsDynamic())
    {
        if (const auto value = ReadSlot(values))
        {
            return *value;
        }
    }
    if (const auto value = values.FindValueCached(values.Nodes().KeyOf(m_valueName), m_cacheSlot, [this, &values] { return GetHashedName(values.Nodes()); }))
    {
        return *value;
    }

    return MakeUndefined(values, std::string(GetName(values.Nodes())));
}

LookupResult SelfRefExpression::EvaluateRef(RenderContext& values)
{
    // Short enough for the string's own buffer: no allocation
    const std::string self = "self";
    return values.FindSelf(self);
}

InternalValue SelfRefExpression::Evaluate(RenderContext& values)
{
    const std::string self = "self";
    if (const auto value = values.FindSelf(self))
    {
        return *value;
    }

    return MakeUndefined(values, self);
}

namespace
{
// A missing attribute: an undefined that knows where it came from, out of line
JINJA2CPP_NOINLINE_INLINE void MakeMissingAttr(InternalValue& result, const InternalValue& cur, const std::string& attrName, RenderContext& values)
{
    if (!GetUndefinedInfo(result))
    {
        result = MakeUndefined(&values, cur, InternalValue(attrName));
    }
}
} // namespace

void SubscriptExpression::AddIndex(const NodeArena& nodes, NodeRef<Expression> value, std::string attrName)
{
    Index idx;
    idx.expr = value;
    idx.isAttr = !attrName.empty();
    idx.maybeMethod = idx.isAttr && methods::IsMethodName(attrName);
    idx.maybeDictMethod = idx.maybeMethod && methods::IsDictMethodName(attrName);
    if (m_subscriptExprs.empty())
    {
        m_firstIndexIsPure = idx.isAttr || nodes[idx.expr].IsPure(nodes);
    }
    idx.attrName = std::move(attrName);
    m_subscriptExprs.push_back(std::move(idx));
}

namespace
{
// The parser builds chains of operators, postfixes and filters without recursing, so a
// foldability check that recurses through one can reach the end of the stack: such an
// expression is not folded, which leaves it to the evaluator, which reports its depth
// (docs/tasks/0216)
bool FoldWalkTooDeep()
{
    return StackNearlyExhausted();
}

std::uint8_t PrepareOperandFold(const NodeArena& nodes, NodeRef<Expression> expr)
{
    return expr ? nodes[expr].PrepareFold(nodes) : std::uint8_t{ 0 };
}

bool OperandFoldsAt(RenderContext& values, NodeRef<Expression> expr, std::uint8_t fold)
{
    return (fold & FoldAlways) != 0 || ((fold & FoldByChoice) != 0 && values.Nodes()[expr].IsFoldableAt(values));
}

// The truth of a foldable value, or nothing when evaluating it fails: Jinja2 then does not fold
std::optional<bool> TryFoldTruth(RenderContext& values, NodeRef<Expression> expr)
{
    try
    {
        return ConvertToBool(values.Nodes()[expr].Evaluate(values));
    }
    catch (const std::exception&)
    {
        return std::nullopt;
    }
}
} // namespace

std::uint8_t Expression::PrepareFold(const NodeArena& nodes)
{
    return IsFoldable(nodes) ? FoldAlways : 0;
}

bool UnaryExpression::IsFoldable(const NodeArena& nodes) const
{
    return !FoldWalkTooDeep() && nodes[m_expr].IsFoldable(nodes);
}

bool IsExpression::IsFoldable(const NodeArena& nodes) const
{
    return m_foldable && !FoldWalkTooDeep() && nodes[m_value].IsFoldable(nodes);
}

bool SubscriptExpression::IsFoldable(const NodeArena& nodes) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    return nodes[m_value].IsFoldable(nodes) && std::all_of(m_subscriptExprs.begin(), m_subscriptExprs.end(), [&nodes](const Index& idx) { return idx.isAttr || nodes[idx.expr].IsFoldable(nodes); });
}

InternalValue SubscriptExpression::ApplyIndex(const InternalValue& cur, const Index& idx, RenderContext& values)
{
    if (idx.isAttr)
    {
        return LookupIndex(cur, idx, nullptr, values);
    }
    InternalValue key = values.Nodes()[idx.expr].Evaluate(values);
    return LookupIndex(cur, idx, &key, values);
}

// An attribute or item of a named undefined fails unless it is chainable; a missing one is
// an undefined that knows where it came from
InternalValue SubscriptExpression::LookupIndex(const InternalValue& cur, const Index& idx, const InternalValue* key, RenderContext& values)
{
    if (GetUndefinedInfo(cur))
    {
        CheckUndefinedUse(cur, UndefinedUse::Attribute);
        return cur;
    }
    return LookupDefinedIndex(cur, idx, key, values);
}

// One returned object, so that it is constructed in the caller's storage
InternalValue SubscriptExpression::LookupDefinedIndex(const InternalValue& cur, const Index& idx, const InternalValue* key, RenderContext& values)
{
    auto result = !key ? (idx.maybeMethod ? methods::GetAttr(cur, idx.attrName, &values) : Subscript(cur, idx.attrName, &values))
                       : methods::GetItem(cur, *key, &values);
    if (result.IsUndefined() && !GetUndefinedInfo(result))
    {
        result = MakeUndefined(&values, cur, key ? *key : InternalValue(idx.attrName));
    }
    return result;
}

InternalValue SubscriptExpression::EvaluateIndices(InternalValue cur, size_t first, size_t count, RenderContext& values, bool forMutation) const
{
    for (size_t n = first; n < count; ++n)
    {
        const auto& idx = m_subscriptExprs[n];
        InternalValue newVal;
        if (!forMutation)
        {
            newVal = ApplyIndex(cur, idx, values);
        }
        else
        {
            InternalValue key = idx.isAttr ? InternalValue(idx.attrName) : values.Nodes()[idx.expr].Evaluate(values);
            newVal = LookupIndex(cur, idx, idx.isAttr ? nullptr : &key, values);
            // A borrowed list or dict inside one the template owns is replaced by its own copy
            if (methods::IsContainer(newVal) && !methods::IsMutable(newVal) && methods::IsMutable(cur))
            {
                newVal = methods::MakeMutable(newVal);
                methods::StoreItem(cur, key, newVal);
            }
        }
        if (cur.ShouldExtendLifetime())
        {
            newVal.SetParentData(cur);
        }
        cur = std::move(newVal);
    }

    return cur;
}

InternalValue SubscriptExpression::Evaluate(RenderContext& values)
{
    CheckStack();
    const auto root = m_firstIndexIsPure ? values.Nodes()[m_value].EvaluateRef(values) : LookupResult();
    if (!root)
    {
        return EvaluateIndices(values.Nodes()[m_value].Evaluate(values), 0, m_subscriptExprs.size(), values, false);
    }
    // The first index is applied to the variable in place, without copying it
    if (m_subscriptExprs.size() == 1)
    {
        // x.name of a mapping, the common case, straight to the map's lookup (docs/tasks/0154)
        const auto& idx = m_subscriptExprs[0];
        if (idx.isAttr && !idx.maybeDictMethod && AsMap(*root))
        {
            auto result = Subscript(*root, idx.attrName, &values);
            if (result.IsUndefined())
            {
                MakeMissingAttr(result, *root, idx.attrName, values);
            }
            if (root->ShouldExtendLifetime())
            {
                result.SetParentData(*root);
            }
            return result;
        }
        return ApplyFirstIndex(*root, values);
    }
    return EvaluateIndices(ApplyFirstIndex(*root, values), 1, m_subscriptExprs.size(), values, false);
}

// One returned object, so that it is constructed in the caller's storage
InternalValue SubscriptExpression::ApplyFirstIndex(const InternalValue& root, RenderContext& values) const
{
    InternalValue cur = ApplyIndex(root, m_subscriptExprs[0], values);
    if (root.ShouldExtendLifetime())
    {
        cur.SetParentData(root);
    }
    return cur;
}

InternalValue LoopAttrExpression::Evaluate(RenderContext& values)
{
    CheckStack();
    if (const auto root = values.Nodes()[m_value].EvaluateRef(values))
    {
        if (const auto loop = AsMap(*root))
        {
            InternalValue value;
            if (loop->GetLoopAttr(m_attr, value))
            {
                if (m_subscriptExprs.size() == 1)
                {
                    return value;
                }
                return EvaluateIndices(std::move(value), 1, m_subscriptExprs.size(), values, false);
            }
        }
    }
    return SubscriptExpression::Evaluate(values);
}

bool LoopAttrExpression::TryCallCycle(RenderContext& values, const ArenaCallParams& params, InternalValue& result) const
{
    // loop.cycle(*items) takes the general call
    if (m_attr != LoopAttr::Cycle || m_subscriptExprs.size() != 1 || params.HasUnpacking())
    {
        return false;
    }
    const auto root = values.Nodes()[m_value].EvaluateRef(values);
    const auto loop = root ? AsMap(*root) : std::nullopt;
    InternalValue index0;
    if (!loop || !loop->GetLoopAttr(LoopAttr::Index0, index0))
    {
        return false;
    }
    // As loop.cycle(...) called on a loop found by name (MakeLoopCycle)
    const auto nodes = values.Nodes();
    if (!params.kwParams.empty())
    {
        throw TemplateRuntimeError("LoopContext.cycle() got an unexpected keyword argument '" + std::string(nodes.Text(nodes[params.kwParams][0].name)) + "'");
    }
    const auto items = nodes[params.posParams];
    if (items.empty())
    {
        throw TemplateRuntimeError("no items for cycling given");
    }
    const auto idx = static_cast<size_t>(Apply<visitors::IntegerEvaluator>(index0)) % items.size();
    result = nodes[items[idx]].Evaluate(values);
    return true;
}

namespace
{
// The value of expr for a method that changes it in place: a list or dict stored in a
// variable becomes one the template owns, stored back in the variable
InternalValue EvaluateMutableRoot(NodeRef<Expression> expr, RenderContext& values)
{
    const auto nodes = values.Nodes();
    if (const auto subscript = nodes.As<SubscriptExpression>(expr))
    {
        return nodes[subscript].EvaluateMutable(values);
    }
    if (const auto ref = nodes.As<ValueRefExpression>(expr))
    {
        if (const auto slot = values.FindForWrite(std::string(nodes[ref].GetName(nodes))))
        {
            if (methods::IsContainer(*slot) && !methods::IsMutable(*slot))
            {
                *slot = methods::MakeMutable(*slot);
            }
            return *slot;
        }
    }
    return nodes[expr].Evaluate(values);
}
} // namespace

InternalValue SubscriptExpression::EvaluateReceiver(RenderContext& values, bool forMutation)
{
    auto root = forMutation ? EvaluateMutableRoot(m_value, values) : values.Nodes()[m_value].Evaluate(values);
    return EvaluateIndices(std::move(root), 0, m_subscriptExprs.size() - 1, values, forMutation);
}

InternalValue SubscriptExpression::EvaluateMutable(RenderContext& values)
{
    return EvaluateIndices(EvaluateMutableRoot(m_value, values), 0, m_subscriptExprs.size(), values, true);
}

FilteredExpression::FilteredExpression(const NodeArena& nodes, NodeRef<Expression> expression, NodeRef<ExpressionFilter> filter)
    : m_expression(expression)
    , m_filter(filter)
{
    if (const auto* constant = nodes[m_expression].GetConstant(nodes); constant && m_filter)
    {
        nodes[m_filter].SetConstantBase(nodes, *constant);
    }
}

bool FilteredExpression::IsFoldable(const NodeArena& nodes) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    return nodes[m_expression].IsFoldable(nodes) && nodes[m_filter].IsFoldable(nodes);
}

InternalValue FilteredExpression::Evaluate(RenderContext& values)
{
    CheckStack();
    auto origResult = values.Nodes()[m_expression].Evaluate(values);
    return values.Nodes()[m_filter].Evaluate(origResult, values);
}

InternalValue UnaryExpression::Evaluate(RenderContext& values)
{
    CheckStack();
    auto value = values.Nodes()[m_expr].Evaluate(values);
    if (m_oper == LogicalNot)
    {
        return !ConvertToBool(value);
    }
    CheckUndefinedUse(value, UndefinedUse::Arithmetic);
    return Apply<visitors::UnaryOperation>(value, m_oper);
}

namespace
{
// A value no template code can change in place: copies of it are independent
bool IsImmutableScalar(const InternalValue& value)
{
    switch (value.Kind())
    {
    case ValueKind::None:
    case ValueKind::Bool:
    case ValueKind::Int:
    case ValueKind::BigInt:
    case ValueKind::Double:
        return true;
    case ValueKind::String:
        // A view is not a copy: it reads text someone else owns
        return !IsStringView(value);
    default:
        return false;
    }
}
} // namespace

BinaryExpression::BinaryExpression(const NodeArena& nodes, BinaryExpression::Operation oper, NodeRef<Expression> leftExpr, NodeRef<Expression> rightExpr)
    : m_oper(oper)
    , m_leftExpr(leftExpr)
    , m_rightExpr(rightExpr)
    , m_rightByRef(nodes[rightExpr].IsPure(nodes))
{
    m_leftByRef = (m_rightByRef && nodes[m_leftExpr].IsPure(nodes)) || nodes.Is<ConstantExpression>(m_leftExpr);
}

bool BinaryExpression::IsFoldable(const NodeArena& nodes) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    // `and` and `or` stop at a literal left operand that decides them, as Python's do
    if (m_oper == LogicalAnd || m_oper == LogicalOr)
    {
        const auto* left = nodes[m_leftExpr].GetConstant(nodes);
        if (left && ConvertToBool(*left) == (m_oper == LogicalOr))
        {
            return true;
        }
    }
    return nodes[m_leftExpr].IsFoldable(nodes) && nodes[m_rightExpr].IsFoldable(nodes);
}

std::uint8_t BinaryExpression::PrepareFold(const NodeArena& nodes)
{
    if (FoldWalkTooDeep())
    {
        return 0;
    }
    const bool isLogical = m_oper == LogicalAnd || m_oper == LogicalOr;
    m_leftFold = PrepareOperandFold(nodes, m_leftExpr);
    m_rightFold = PrepareOperandFold(nodes, m_rightExpr);
    // As IsFoldable: a literal left operand that decides `and`/`or`, or two folded operands
    const auto* left = isLogical ? nodes[m_leftExpr].GetConstant(nodes) : nullptr;
    if ((left && ConvertToBool(*left) == (m_oper == LogicalOr)) || (m_leftFold == FoldAlways && m_rightFold == FoldAlways))
    {
        return FoldAlways;
    }
    return m_leftFold != 0 && (isLogical || m_rightFold != 0) ? FoldByChoice : 0;
}

bool BinaryExpression::IsFoldableAt(RenderContext& values) const
{
    if (FoldWalkTooDeep() || !OperandFoldsAt(values, m_leftExpr, m_leftFold))
    {
        return false;
    }
    if (m_oper == LogicalAnd || m_oper == LogicalOr)
    {
        const auto left = TryFoldTruth(values, m_leftExpr);
        if (!left)
        {
            return false;
        }
        if (*left == (m_oper == LogicalOr))
        {
            return true;
        }
    }
    return OperandFoldsAt(values, m_rightExpr, m_rightFold);
}

NodeRef<Expression> BinaryExpression::Make(NodeArena& nodes, Operation oper, NodeRef<Expression> leftExpr, NodeRef<Expression> rightExpr)
{
    if (oper == DivRemainder)
    {
        // Markup and wide literals keep the general path
        const auto* constant = nodes[leftExpr].GetConstant(nodes);
        auto format = constant ? AsStringView<char>(*constant) : std::nullopt;
        if (format && !constant->IsMarkup())
        {
            return nodes.Make<ConstFormatExpression>(nodes, leftExpr, rightExpr, std::make_unique<const CompiledPercentFormat>(std::string(*format)));
        }
    }
    const auto literal = oper == In ? nodes.As<TupleCreator>(rightExpr) : NodeRef<TupleCreator>();
    if (!literal)
    {
        return nodes.Make<BinaryExpression>(nodes, oper, leftExpr, rightExpr);
    }
    const auto literalItems = nodes[nodes[literal].GetItems()];
    InternalValueList items;
    items.reserve(literalItems.size());
    for (const auto item : literalItems)
    {
        const auto* constant = nodes[item].GetConstant(nodes);
        if (!constant || !IsImmutableScalar(*constant))
        {
            return nodes.Make<BinaryExpression>(nodes, oper, leftExpr, rightExpr);
        }
        items.push_back(*constant);
    }
    return nodes.Make<InLiteralExpression>(nodes, leftExpr, rightExpr, std::move(items));
}

template<typename F>
InternalValue BinaryExpression::WithLeft(RenderContext& context, const F& f)
{
    // A plain variable or constant is read in place when the right operand cannot change it
    if (m_leftByRef)
    {
        if (const auto leftVal = context.Nodes()[m_leftExpr].EvaluateRef(context))
        {
            return f(*leftVal);
        }
    }
    return f(context.Nodes()[m_leftExpr].Evaluate(context));
}

template<typename F>
InternalValue BinaryExpression::WithRight(RenderContext& context, const F& f)
{
    if (m_rightByRef)
    {
        if (const auto rightVal = context.Nodes()[m_rightExpr].EvaluateRef(context))
        {
            return f(*rightVal);
        }
    }
    return f(context.Nodes()[m_rightExpr].Evaluate(context));
}

InternalValue BinaryExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    // A plain variable or constant is read in place when the right operand cannot change it
    if (m_leftByRef)
    {
        if (const auto leftVal = context.Nodes()[m_leftExpr].EvaluateRef(context))
        {
            return EvaluateWithLeft(*leftVal, context);
        }
    }
    return EvaluateWithLeft(context.Nodes()[m_leftExpr].Evaluate(context), context);
}

InternalValue InLiteralExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    return WithLeft(context, [this](const InternalValue& leftVal) {
        CheckUndefinedUse(leftVal, UndefinedUse::Operator);
        return InternalValue(testers::IsValueInList(leftVal, m_items));
    });
}

ConstFormatExpression::ConstFormatExpression(const NodeArena& nodes, NodeRef<Expression> leftExpr, NodeRef<Expression> rightExpr, std::unique_ptr<const CompiledPercentFormat> format)
    : BinaryExpression(nodes, DivRemainder, leftExpr, rightExpr)
    , m_format(std::move(format))
{
}
ConstFormatExpression::ConstFormatExpression(ConstFormatExpression&&) noexcept = default;
ConstFormatExpression::~ConstFormatExpression() = default;

InternalValue ConstFormatExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    return WithRight(context, [this](const InternalValue& rightVal) {
        // What Apply does for a narrow string on the left
        CheckUndefinedUse(rightVal, UndefinedUse::Operator);
        return InternalValue(TargetString(m_format->Format(rightVal)));
    });
}

InternalValue BinaryExpression::EvaluateWithLeft(const InternalValue& leftVal, RenderContext& context)
{
    // `and` and `or` short-circuit and return the deciding operand, as in Python
    if (m_oper == LogicalAnd)
    {
        return ConvertToBool(leftVal) ? context.Nodes()[m_rightExpr].Evaluate(context) : leftVal;
    }
    if (m_oper == LogicalOr)
    {
        return ConvertToBool(leftVal) ? leftVal : context.Nodes()[m_rightExpr].Evaluate(context);
    }

    if (m_rightByRef)
    {
        if (const auto rightVal = context.Nodes()[m_rightExpr].EvaluateRef(context))
        {
            return Apply(leftVal, *rightVal, context);
        }
    }
    return Apply(leftVal, context.Nodes()[m_rightExpr].Evaluate(context), context);
}

namespace
{
// str % values is Python's printf-style formatting; the result keeps the string's width.
// Empty when the left operand is not a string
std::optional<InternalValue> ApplyPercentFormat(const InternalValue& leftVal, const InternalValue& rightVal)
{
    bool isWide = false;
    bool isString = ApplyStringConverter(leftVal, [&isWide](auto str) {
        isWide = sizeof(str[0]) != sizeof(char);
        return true;
    });
    if (!isString)
    {
        return std::nullopt;
    }

    // Markup % args escapes what the arguments convert to and stays Markup
    bool markup = leftVal.IsMarkup();
    auto narrow = AsStringView<char>(leftVal);
    auto formatted = narrow ? PythonPercentFormat(*narrow, rightVal, markup)
                            : PythonPercentFormat(ApplyStringConverter(leftVal, [](auto str) { return ConvertString<std::string>(str); }), rightVal, markup);
    InternalValue formattedVal = isWide ? TargetString(ConvertString<std::wstring>(formatted)) : TargetString(std::move(formatted));
    formattedVal.SetMarkup(leftVal.IsMarkup());
    return formattedVal;
}

} // namespace

namespace
{
// The comparison and arithmetic operators on values that are not both numbers
InternalValue ApplyMathOperation(BinaryExpression::Operation oper, const InternalValue& leftVal, const InternalValue& rightVal, RenderContext& context)
{
    // Markup + str escapes the other operand, Markup * n stays Markup
    if (oper == BinaryExpression::Plus && (leftVal.IsMarkup() || rightVal.IsMarkup()) && IsStringValue(leftVal) && IsStringValue(rightVal))
    {
        auto* callback = context.GetRendererCallback();
        InternalValue result = Apply2<visitors::BinaryMathOperation>(MarkupEscape(leftVal, callback), MarkupEscape(rightVal, callback), oper);
        result.SetMarkup();
        return result;
    }
    InternalValue result = Apply2<visitors::BinaryMathOperation>(leftVal, rightVal, oper);
    if (oper == BinaryExpression::Mul && (leftVal.IsMarkup() || rightVal.IsMarkup()))
    {
        result.SetMarkup(IsStringValue(result));
    }
    return result;
}

// The operands of a `~` chain as one string of the template's width, built once
template<typename CharT>
InternalValue JoinAsStrings(Span<const LookupResult> values)
{
    // Strings of the template's width are copied into the result, allocated once (docs/tasks/0151)
    size_t size = 0;
    bool allText = true;
    for (const auto& value : values)
    {
        if (const auto text = AsStringView<CharT>(*value))
        {
            size += text->size();
        }
        else
        {
            allText = false;
        }
    }
    if (allText)
    {
        return InternalValue::MakeText<CharT>(size, [&values](Span<CharT> chars) {
            auto out = chars.begin();
            for (const auto& value : values)
            {
                const auto text = *AsStringView<CharT>(*value);
                out = std::copy(text.begin(), text.end(), out);
            }
        });
    }

    // The others are rendered straight onto the text before them
    std::basic_string<CharT> result;
    result.reserve(size);
    for (const auto& value : values)
    {
        Apply<visitors::ValueRenderer<CharT>>(*value, result);
    }
    return InternalValue(TargetString(std::move(result)));
}

// Kept out of line so that the plain join stays small
JINJA2CPP_NOINLINE_INLINE InternalValue MarkupJoin(Span<const LookupResult> values, RenderContext& context)
{
    auto* callback = context.GetRendererCallback();
    SmallVector<InternalValue, 4> escaped;
    SmallVector<LookupResult, 4> parts;
    escaped.reserve(values.size());
    for (const auto& value : values)
    {
        parts.emplace_back(escaped.emplace_back(MarkupEscape(*value, callback)));
    }
    InternalValue result = callback->IsWideTarget() ? JoinAsStrings<wchar_t>(parts) : JoinAsStrings<char>(parts);
    result.SetMarkup();
    return result;
}
} // namespace

InternalValue BinaryExpression::Apply(const InternalValue& leftVal, const InternalValue& rightVal, RenderContext& context) const
{
    if (m_oper >= LogicalEq && m_oper <= Pow && m_oper != In && visitors::IsNumber(leftVal) && visitors::IsNumber(rightVal))
    {
        return visitors::ApplyToNumbers(leftVal, rightVal, m_oper);
    }
    InternalValue result;
    // StrictUndefined fails on any operator; the others fail in the arithmetic below
    CheckUndefinedUse(leftVal, UndefinedUse::Operator);
    CheckUndefinedUse(rightVal, UndefinedUse::Operator);

    if (m_oper == DivRemainder)
    {
        if (auto formatted = ApplyPercentFormat(leftVal, rightVal))
        {
            return std::move(*formatted);
        }
    }

    switch (m_oper)
    {
    case jinja2::BinaryExpression::LogicalEq:
    case jinja2::BinaryExpression::LogicalNe:
    case jinja2::BinaryExpression::LogicalGt:
    case jinja2::BinaryExpression::LogicalLt:
    case jinja2::BinaryExpression::LogicalGe:
    case jinja2::BinaryExpression::LogicalLe:
    case jinja2::BinaryExpression::Plus:
    case jinja2::BinaryExpression::Minus:
    case jinja2::BinaryExpression::Mul:
    case jinja2::BinaryExpression::Div:
    case jinja2::BinaryExpression::DivRemainder:
    case jinja2::BinaryExpression::DivInteger:
    case jinja2::BinaryExpression::Pow:
        result = ApplyMathOperation(m_oper, leftVal, rightVal, context);
        break;
    case jinja2::BinaryExpression::In:
    {
        result = testers::IsValueIn(leftVal, rightVal);
        break;
    }
    default:
        break;
    }
    return result;
}

// `a ~ b`, the common case, without the containers of a longer run: each operand is read in
// place where it can be, as BinaryExpression::WithLeft does, or lives in a temporary for the call
JINJA2CPP_HOT_INLINE InternalValue ConcatExpression::EvaluatePair(NodeRef<Expression> leftExpr, NodeRef<Expression> rightExpr, RenderContext& context) const
{
    const auto& nodes = context.Nodes();
    auto withOperand = [&nodes, &context](NodeRef<Expression> expr, bool lend, const auto& f) {
        if (lend || nodes.Is<ConstantExpression>(expr))
        {
            if (const auto value = nodes[expr].EvaluateRef(context))
            {
                return f(*value);
            }
        }
        return f(nodes[expr].Evaluate(context));
    };
    return withOperand(leftExpr, m_lendFrom == 0, [&](const InternalValue& left) {
        return withOperand(rightExpr, m_lendFrom <= 1, [&](const InternalValue& right) {
            // Both are evaluated first, as Jinja2 builds the operand list before joining it
            CheckUndefinedUse(left, UndefinedUse::Operator);
            CheckUndefinedUse(right, UndefinedUse::Operator);
            const std::array<LookupResult, 2> values{ LookupResult(left), LookupResult(right) };
            if ((left.IsMarkup() || right.IsMarkup()) && !m_folded && context.IsOutputAutoescape() && !context.IsVolatileAutoescape())
            {
                return MarkupJoin(values, context);
            }
            return context.GetRendererCallback()->IsWideTarget() ? JoinAsStrings<wchar_t>(values) : JoinAsStrings<char>(values);
        });
    });
}

InternalValue ConcatExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    const auto operands = context.Nodes()[m_operands];
    if (operands.size() == 2)
    {
        return EvaluatePair(operands[0], operands[1], context);
    }
    // Eight operands fit without allocating (chat templates join five or six). A constant, or a
    // pure operand after the last impure one (m_lendFrom), is read in place, as BinaryExpression
    // reads its operands; the others are evaluated into `owned`, reserved so that the references stay valid
    SmallVector<InternalValue, 8> owned;
    SmallVector<LookupResult, 8> values;
    owned.reserve(operands.size());
    values.reserve(operands.size());
    bool anyMarkup = false;
    for (uint32_t idx = 0; idx < operands.size(); ++idx)
    {
        const auto operand = operands[idx];
        auto& node = context.Nodes()[operand];
        LookupResult value;
        if (idx >= m_lendFrom || context.Nodes().Is<ConstantExpression>(operand))
        {
            value = node.EvaluateRef(context);
        }
        if (!value)
        {
            value = LookupResult(owned.emplace_back(node.Evaluate(context)));
        }
        anyMarkup = anyMarkup || value->IsMarkup();
        values.push_back(value);
    }
    // StrictUndefined fails on any operator, once every operand is evaluated: Jinja2 builds the
    // operand list before joining it, so `missing ~ f()` calls f and `missing ~ 1 / 0` fails
    // dividing. The others render as empty text
    for (const auto value : values)
    {
        CheckUndefinedUse(*value, UndefinedUse::Operator);
    }

    // Jinja2's markup_join, `~` under autoescape when any operand is Markup: the others are
    // escaped and the result is Markup. Jinja2 3.1.6 joins as str in volatile code: it reads
    // eval_ctx.volatile at run time, which is never set (docs/tasks/0216)
    auto* callback = context.GetRendererCallback();
    if (anyMarkup && !m_folded && context.IsOutputAutoescape() && !context.IsVolatileAutoescape())
    {
        return MarkupJoin(values, context);
    }
    return callback->IsWideTarget() ? JoinAsStrings<wchar_t>(values) : JoinAsStrings<char>(values);
}

bool ConcatExpression::IsFoldable(const NodeArena& nodes) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    const auto operands = nodes[m_operands];
    return std::all_of(operands.begin(), operands.end(), [&nodes](const NodeRef<Expression>& operand) { return nodes[operand].IsFoldable(nodes); });
}

InternalValue CompareExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    InternalValue left = context.Nodes()[m_first].Evaluate(context);
    CheckUndefinedUse(left, UndefinedUse::Operator);
    for (const auto& operand : context.Nodes()[m_operands])
    {
        InternalValue right = context.Nodes()[operand.expr].Evaluate(context);
        CheckUndefinedUse(right, UndefinedUse::Operator);
        bool result = false;
        if (operand.operation == BinaryExpression::In)
        {
            result = testers::IsValueIn(left, right);
        }
        else
        {
            result = ConvertToBool(visitors::IsNumber(left) && visitors::IsNumber(right)
                                       ? visitors::ApplyToNumbers(left, right, operand.operation)
                                       : Apply2<visitors::BinaryMathOperation>(left, right, operand.operation));
        }

        if (result == operand.negated)
        {
            return InternalValue(false);
        }
        std::swap(left, right);
    }

    return InternalValue(true);
}

bool SliceExpression::IsFoldable(const NodeArena& nodes) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    auto part = [&nodes](const NodeRef<Expression>& expr) { return !expr || nodes[expr].IsFoldable(nodes); };
    return nodes[m_value].IsFoldable(nodes) && part(m_start) && part(m_stop) && part(m_step);
}

InternalValue SliceExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    auto part = [&context](const NodeRef<Expression>& expr) { return expr ? context.Nodes()[expr].Evaluate(context) : InternalValue(); };
    InternalValue value = context.Nodes()[m_value].Evaluate(context);
    auto start = part(m_start);
    auto stop = part(m_stop);
    auto step = part(m_step);
    auto result = Slice(value, start, stop, step);
    // A slice of Markup is Markup
    if (value.IsMarkup() && IsStringValue(result))
    {
        result.SetMarkup();
    }
    return result;
}

bool TupleCreator::IsFoldable(const NodeArena& nodes) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    const auto items = nodes[m_exprs];
    return std::all_of(items.begin(), items.end(), [&nodes](const NodeRef<Expression>& item) { return nodes[item].IsFoldable(nodes); });
}

InternalValue TupleCreator::Evaluate(RenderContext& context)
{
    CheckStack();
    const auto nodes = context.Nodes();
    const auto exprs = nodes[m_exprs];
    InternalValueList result;
    result.reserve(exprs.size());
    for (const auto e : exprs)
    {
        result.push_back(nodes[e].Evaluate(context));
    }

    auto list = ListAdapter::CreateAdapter(std::move(result));
    if (m_isTuple)
    {
        list.MarkAsTuple();
    }
    return list;
}

bool DictCreator::IsFoldable(const NodeArena& nodes) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    const auto items = nodes[m_exprs];
    return std::all_of(items.begin(), items.end(), [&nodes](const Item& item) { return nodes[item.key].IsFoldable(nodes) && nodes[item.value].IsFoldable(nodes); });
}

InternalValue DictCreator::Evaluate(RenderContext& context)
{
    CheckStack();
    const auto nodes = context.Nodes();
    TemplateDict result;
    for (const auto& item : nodes[m_exprs])
    {
        // Python evaluates the key before the value; an assignment does not fix that order
        const auto key = nodes[item.key].Evaluate(context);
        auto value = nodes[item.value].Evaluate(context);
        // Any hashable key; an unhashable one is a TypeError. A repeated key (1 and 1.0) keeps
        // its first spelling and position and takes the last value
        result.Set(key, std::move(value));
    }

    return CreateMapAdapter(std::move(result));
}

NodeRef<ExpressionFilter> ExpressionFilter::Make(NodeArena& nodes, const std::string& filterName, const CallParamsInfo& params, InternalValue registered, bool* unknown, bool canFold)
{
    // Filters added to the environment take precedence over the builtins, as in Jinja2's env.filters
    const auto filter = GetIf<Callable>(&registered)
                            ? nodes.MakeObject<IExpressionFilter, filters::UserDefinedFilter>(filterName, params, std::move(registered))
                            : CreateFilter(nodes, filterName, params, unknown);
    ArenaText argsError;
    if (auto error = nodes[filter].GetArgumentsError(); !error.empty())
    {
        argsError = nodes.MakeText(filterName + "() " + error);
    }
    // Jinja2 does not fold the filters that take the context, nor can it be told whether a
    // filter of the environment does
    static constexpr std::string_view contextFilters[] = { "map", "random", "reject", "rejectattr", "select", "selectattr" };
    bool foldable = canFold && GetIf<Callable>(&registered) == nullptr && std::find(std::begin(contextFilters), std::end(contextFilters), filterName) == std::end(contextFilters);
    for (const auto& arg : params.posParams)
    {
        foldable = foldable && nodes[arg].IsFoldable(nodes);
    }
    for (const auto& [name, arg] : params.kwParams)
    {
        foldable = foldable && nodes[arg].IsFoldable(nodes);
    }
    return nodes.Make<ExpressionFilter>(filter, argsError, foldable);
}

bool ExpressionFilter::IsFoldable(const NodeArena& nodes) const
{
    // A chain of filters is walked without recursing (FoldWalkTooDeep)
    for (const auto* filter = this;; filter = &nodes[filter->m_parentFilter])
    {
        if (!filter->m_foldable)
        {
            return false;
        }
        if (!filter->m_parentFilter)
        {
            return true;
        }
    }
}

void ExpressionFilter::SetConstantBase(const NodeArena& nodes, const InternalValue& base)
{
    // The first filter of the chain takes the base, found without recursing (IsFoldable)
    const auto* filter = this;
    while (filter->m_parentFilter)
    {
        filter = &nodes[filter->m_parentFilter];
    }
    nodes[filter->m_filter].SetConstantBase(base);
}

namespace
{
// Out of line, so that the filters that fit their call pay only the test
[[noreturn]] JINJA2CPP_NOINLINE_INLINE void ThrowArgumentsError(std::string_view error)
{
    throw TemplateRuntimeError(std::string(error));
}
} // namespace

InternalValue ExpressionFilter::Evaluate(const InternalValue& baseVal, RenderContext& context)
{
    CheckStack();
    if (!m_argsError.empty())
    {
        ThrowArgumentsError(context.Nodes().Text(m_argsError));
    }
    const auto& nodes = context.Nodes();
    if (m_parentFilter)
    {
        return nodes[m_filter].Filter(nodes[m_parentFilter].Evaluate(baseVal, context), context);
    }

    return nodes[m_filter].Filter(baseVal, context);
}

NodeRef<IsExpression> IsExpression::Make(NodeArena& nodes, NodeRef<Expression> value, const std::string& tester, const CallParamsInfo& params, InternalValue registered, bool* unknown, bool canFold)
{
    if (GetIf<Callable>(&registered))
    {
        const auto test = nodes.MakeObject<ITester, testers::UserDefinedTester>(tester, params, std::move(registered));
        return nodes.Make<IsExpression>(value, test, false);
    }
    const bool testInPlace = params.posParams.empty() && params.kwParams.empty();
    // Jinja2 folds a built-in test whose arguments it can fold (Expression::IsFoldable)
    bool foldable = canFold;
    for (const auto& arg : params.posParams)
    {
        foldable = foldable && nodes[arg].IsFoldable(nodes);
    }
    for (const auto& [name, arg] : params.kwParams)
    {
        foldable = foldable && nodes[arg].IsFoldable(nodes);
    }
    const auto test = CreateTester(nodes, tester, params, unknown);
    return nodes.Make<IsExpression>(value, test, testInPlace, foldable);
}

InternalValue IsExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    const auto& nodes = context.Nodes();
    if (m_testInPlace)
    {
        if (const auto value = nodes[m_value].EvaluateRef(context))
        {
            return nodes[m_tester].Test(*value, context);
        }
    }
    return nodes[m_tester].Test(nodes[m_value].Evaluate(context), context);
}

bool IfExpression::IsFoldable(const NodeArena& nodes, NodeRef<Expression> value) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    if (const auto* test = nodes[m_testExpr].GetConstant(nodes))
    {
        const auto chosen = ConvertToBool(*test) ? value : m_altValue;
        return chosen && nodes[chosen].IsFoldable(nodes);
    }
    return nodes[m_testExpr].IsFoldable(nodes) && m_altValue && nodes[value].IsFoldable(nodes) && nodes[m_altValue].IsFoldable(nodes);
}

std::uint8_t IfExpression::PrepareFold(const NodeArena& nodes, NodeRef<Expression> value)
{
    if (FoldWalkTooDeep())
    {
        return 0;
    }
    m_testFold = PrepareOperandFold(nodes, m_testExpr);
    m_valueFold = PrepareOperandFold(nodes, value);
    m_altFold = PrepareOperandFold(nodes, m_altValue);
    // As IsFoldable: a literal test and the branch it picks, or a folded test and two branches
    if (const auto* test = nodes[m_testExpr].GetConstant(nodes))
    {
        return ConvertToBool(*test) ? m_valueFold : m_altFold;
    }
    if (m_testFold == FoldAlways && m_altValue && m_valueFold == FoldAlways && m_altFold == FoldAlways)
    {
        return FoldAlways;
    }
    return m_testFold != 0 && (m_valueFold != 0 || m_altFold != 0) ? FoldByChoice : 0;
}

bool IfExpression::IsFoldableAt(RenderContext& values, NodeRef<Expression> value) const
{
    if (FoldWalkTooDeep() || !OperandFoldsAt(values, m_testExpr, m_testFold))
    {
        return false;
    }
    const auto test = TryFoldTruth(values, m_testExpr);
    if (!test)
    {
        return false;
    }
    return *test ? OperandFoldsAt(values, value, m_valueFold) : (m_altValue && OperandFoldsAt(values, m_altValue, m_altFold));
}

std::uint8_t FullExpressionEvaluator::PrepareFold(const NodeArena& nodes)
{
    if (FoldWalkTooDeep() || !m_expression)
    {
        return 0;
    }
    return m_tester ? nodes[m_tester].PrepareFold(nodes, m_expression) : nodes[m_expression].PrepareFold(nodes);
}

bool FullExpressionEvaluator::IsFoldableAt(RenderContext& values) const
{
    if (FoldWalkTooDeep() || !m_expression)
    {
        return false;
    }
    return m_tester ? values.Nodes()[m_tester].IsFoldableAt(values, m_expression) : values.Nodes()[m_expression].IsFoldableAt(values);
}

bool FullExpressionEvaluator::IsFoldable(const NodeArena& nodes) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    if (!m_expression)
    {
        return false;
    }
    return m_tester ? nodes[m_tester].IsFoldable(nodes, m_expression) : nodes[m_expression].IsFoldable(nodes);
}

bool CompareExpression::IsFoldable(const NodeArena& nodes) const
{
    if (FoldWalkTooDeep())
    {
        return false;
    }
    const auto operands = nodes[m_operands];
    return nodes[m_first].IsFoldable(nodes) && std::all_of(operands.begin(), operands.end(), [&nodes](const Operand& operand) { return nodes[operand.expr].IsFoldable(nodes); });
}

bool IfExpression::Evaluate(RenderContext& context)
{
    return ConvertToBool(context.Nodes()[m_testExpr].Evaluate(context));
}

InternalValue IfExpression::EvaluateAltValue(RenderContext& context)
{
    return m_altValue ? context.Nodes()[m_altValue].Evaluate(context) : InternalValue();
}

/*
InternalValue DictionaryCreator::Evaluate(RenderContext& context)
{
    CheckStack();
    ValuesMap result;
    for (auto& [name, expr] : m_items)
    {
        result[name] = expr->Evaluate(context);
    }

    return result;
}*/

bool CallExpression::TryCallMethod(RenderContext& values, InternalValue& result, InternalValue& callee)
{
    const auto nodes = values.Nodes();
    if (const auto loopAttr = nodes.As<LoopAttrExpression>(m_valueRef); loopAttr && nodes[loopAttr].TryCallCycle(values, m_params, result))
    {
        return true;
    }
    const auto subscriptRef = nodes.As<SubscriptExpression>(m_valueRef);
    auto* subscript = subscriptRef ? &nodes[subscriptRef] : nullptr;
    const std::string* name = subscript ? subscript->GetCallName() : nullptr;
    if (!name)
    {
        callee = values.Nodes()[m_valueRef].Evaluate(values);
        return false;
    }

    const bool mayMutate = methods::IsMutatingName(*name);
    auto receiver = subscript->EvaluateReceiver(values, mayMutate);
    CheckUndefinedUse(receiver, UndefinedUse::Attribute);
    const auto* method = methods::FindMethod(receiver, *name);
    if (method)
    {
        // A host object's own key comes before a dict method (MapAttrPolicy::KeysFirst)
        const auto map = AsMap(receiver);
        if (map && map->GetAttrPolicy() == MapAttrPolicy::KeysFirst && map->HasValue(*name))
        {
            method = nullptr;
        }
    }

    if (!method)
    {
        callee = Subscript(receiver, *name, &values);
        if (receiver.ShouldExtendLifetime())
        {
            callee.SetParentData(receiver);
        }
        // Python raises AttributeError; calling the missing attribute of a map, None or a
        // chainable undefined is an UndefinedError
        if (callee.IsUndefined() && !IsEmpty(receiver) && !AsMap(receiver))
        {
            methods::ThrowNoAttribute(receiver, *name);
        }
        if (callee.IsUndefined() && !GetUndefinedInfo(callee))
        {
            callee = MakeUndefined(&values, receiver, InternalValue(*name));
        }
        return false;
    }

    if (method->isMutating)
    {
        receiver = methods::MakeMutable(receiver);
    }
    auto callParams = helpers::EvaluateCallParams(m_params, values);
    result = method->invoke(receiver, callParams, values);
    return true;
}

InternalValue CallExpression::CallWithCallee(RenderContext& values, InternalValue fnVal)
{
    return CallArbitraryFn(values, std::move(fnVal));
}

std::optional<Callable> CallExpression::FindNamedCallable(RenderContext& values) const
{
    if (!m_isNamedCallee)
    {
        return std::nullopt;
    }
    const auto value = values.Nodes()[m_valueRef].EvaluateRef(values);
    const auto* callable = value ? GetIf<Callable>(&*value) : nullptr;
    if (!callable)
    {
        return std::nullopt;
    }
    return *callable;
}

InternalValue CallExpression::Evaluate(RenderContext& values)
{
    CheckStack();
    if (auto callable = FindNamedCallable(values))
    {
        return CallCallable(values, *callable);
    }
    InternalValue result;
    InternalValue fnVal;
    if (TryCallMethod(values, result, fnVal))
    {
        return result;
    }
    return CallWithCallee(values, std::move(fnVal));
}

void CallExpression::Render(OutStream& stream, RenderContext& values)
{
    if (auto callable = FindNamedCallable(values))
    {
        RenderCallable(stream, values, *callable);
        return;
    }
    InternalValue result;
    InternalValue fnVal;
    if (TryCallMethod(values, result, fnVal))
    {
        WriteOutput(stream, result, values);
        return;
    }
    const Callable* callable = GetIf<Callable>(&fnVal);
    if (!callable)
    {
        auto callOperator = Subscript(fnVal, "operator()"s, &values);
        if (!GetIf<Callable>(&callOperator))
        {
            WriteOutput(stream, CallWithCallee(values, std::move(fnVal)), values);
            return;
        }
        fnVal = std::move(callOperator);
        callable = GetIf<Callable>(&fnVal);
    }

    RenderCallable(stream, values, *callable);
}

void CallExpression::RenderCallable(OutStream& stream, RenderContext& values, const Callable& callable)
{
    auto callParams = helpers::EvaluateCallParams(m_params, values);

    if (callable.GetType() == Callable::Type::Expression)
    {
        WriteOutput(stream, callable.GetExpressionCallable()(callParams, values), values);
    }
    else if (callable.GetKind() == Callable::Macro && values.IsOutputAutoescape() && !values.IsAutoescape())
    {
        // Autoescape off at run time but on where the code is written: the result is a plain
        // string, which the output escapes (InvokeCallable)
        InternalValue result(RenderToString(values.GetRendererCallback(), [&](OutStream& out) { callable.GetStatementCallable()(callParams, out, values); }));
        WriteOutput(stream, result, values);
    }
    else
    {
        callable.GetStatementCallable()(callParams, stream, values);
    }
}

namespace
{
// fnVal itself or its operator() as a callable, stored in fnVal; null for an undefined value
const Callable* ResolveCallee(InternalValue& fnVal)
{
    if (const auto* callable = GetIf<Callable>(&fnVal))
    {
        return callable;
    }
    auto callOperator = Subscript(fnVal, "operator()"s, nullptr);
    if (!GetIf<Callable>(&callOperator))
    {
        // Calling a named undefined is an UndefinedError; any other value is not callable
        CheckUndefinedUse(fnVal, UndefinedUse::Call);
        if (fnVal.IsUndefined())
        {
            return nullptr;
        }
        throw TemplateRuntimeError("'"s + Apply<visitors::PythonTypeNameGetter>(fnVal) + "' object is not callable");
    }
    fnVal = std::move(callOperator);
    return GetIf<Callable>(&fnVal);
}

bool IsCallableKind(const Callable& callable)
{
    auto kind = callable.GetKind();
    return kind == Callable::GlobalFunc || kind == Callable::UserCallable || kind == Callable::Macro;
}

InternalValue InvokeCallable(RenderContext& values, const Callable& callable, const CallParams& callParams)
{
    if (callable.GetType() == Callable::Type::Expression)
    {
        return callable.GetExpressionCallable()(callParams, values);
    }

    TargetString resultStr =
        RenderToString(values.GetRendererCallback(), [&](OutStream& stream) { callable.GetStatementCallable()(callParams, stream, values); });
    // A macro returns Markup when autoescape is on where it is called, at run time
    InternalValue result(std::move(resultStr));
    result.SetMarkup(values.IsAutoescape());
    return result;
}
} // namespace

InternalValue CallExpression::CallArbitraryFn(RenderContext& values, InternalValue fnVal)
{
    const auto* callable = ResolveCallee(fnVal);
    return callable ? CallCallable(values, *callable) : InternalValue();
}

InternalValue CallExpression::CallCallable(RenderContext& values, const Callable& callable)
{
    if (!IsCallableKind(callable))
    {
        return InternalValue();
    }
    return InvokeCallable(values, callable, helpers::EvaluateCallParams(m_params, values));
}

InternalValue CallExpression::CallValue(RenderContext& values, InternalValue fnVal, const CallParams& params)
{
    const auto* callable = ResolveCallee(fnVal);
    if (!callable || !IsCallableKind(*callable))
    {
        return InternalValue();
    }
    return InvokeCallable(values, *callable, params);
}


namespace helpers
{
enum ArgState
{
    NotFound,
    NotFoundMandatory,
    Keyword,
    Positional,
    Ignored,
    // `*context`: the engine passes it, so no argument of the call binds to it
    Context
};

namespace
{
struct ArgInfo
{
    ArgState state = NotFound;
    int prevNotFound = -1;
    int nextNotFound = -1;
    const ArgumentInfo* info = nullptr;
};

using ArgInfoList = SmallVector<ArgInfo, 8>;

// Binds the declared parameter idx: by name for a call made at render, by position in the
// declaration for one bound at Load
void BindArg(ParsedArguments& result, std::size_t /*idx*/, const ArgumentInfo& info, const InternalValue& value)
{
    result.args[info.name] = value;
}

void BindArg(ParsedArgumentsInfo& result, std::size_t idx, const ArgumentInfo& /*info*/, const NodeRef<Expression>& value)
{
    result.args[idx] = value;
}

// Marks the arguments given by keyword and links the others into a list of the ones still
// missing; returns the index of the first missing mandatory argument, or -1
template<typename Result, typename T, typename P>
int MapKeywordArgs(const T& args, const P& params, ArgInfoList& argsInfo, Result& result)
{
    std::size_t argIdx = 0;
    int firstMandatoryIdx = -1;
    int prevNotFound = -1;

    // Find all provided keyword args
    for (auto& argInfo : args)
    {
        argsInfo[argIdx].info = &argInfo;

        if (argInfo.name == "*args" || argInfo.name == "**kwargs")
        {
            argsInfo[argIdx++].state = Ignored;
            continue;
        }
        if (argInfo.name == "*context")
        {
            argsInfo[argIdx++].state = Context;
            continue;
        }

        auto p = params.kwParams.find(argInfo.name);
        if (p != params.kwParams.end())
        {
            BindArg(result, argIdx, argInfo, p->second);
            argsInfo[argIdx].state = Keyword;
        }
        else
        {
            if (argInfo.mandatory)
            {
                argsInfo[argIdx].state = NotFoundMandatory;
                if (firstMandatoryIdx == -1)
                {
                    firstMandatoryIdx = static_cast<int>(argIdx);
                }
            }
            else
            {
                argsInfo[argIdx].state = NotFound;
            }


            if (prevNotFound != -1)
            {
                argsInfo[static_cast<std::size_t>(prevNotFound)].nextNotFound = static_cast<int>(argIdx);
            }

            argsInfo[argIdx].prevNotFound = prevNotFound;
            prevNotFound = static_cast<int>(argIdx);
        }


        ++argIdx;
    }

    return firstMandatoryIdx;
}

// The first argument the positional params map to, and how many of the params are mapped
struct PosArgRange
{
    std::size_t startPosArg = 0;
    std::size_t eatenPosArgs = 0;
};

PosArgRange FindPosArgRange(const ArgInfoList& argsInfo, std::size_t posParamsCount, int firstMandatoryIdx)
{
    const std::size_t argsCount = argsInfo.size();
    std::size_t startPosArg = firstMandatoryIdx == -1 ? 0 : static_cast<std::size_t>(firstMandatoryIdx);
    std::size_t curPosArg = startPosArg;
    std::size_t eatenPosArgs = 0;

    // Determine the range for positional arguments scanning
    bool isFirstTime = true;
    for (; eatenPosArgs < posParamsCount && startPosArg < argsCount; eatenPosArgs = eatenPosArgs + (argsInfo[startPosArg].state == Ignored ? 0 : 1))
    {
        if (isFirstTime)
        {
            for (; startPosArg < argsCount && (argsInfo[startPosArg].state == Keyword || argsInfo[startPosArg].state == Positional || argsInfo[startPosArg].state == Context); ++startPosArg)
                ;

            isFirstTime = false;
            if (startPosArg == argsCount)
            {
                break;
            }
            // The scan follows the missing arguments from the first one not skipped
            curPosArg = startPosArg;
            continue;
        }

        int prevNotFound = argsInfo[startPosArg].prevNotFound;
        if (prevNotFound != -1)
        {
            startPosArg = static_cast<std::size_t>(prevNotFound);
        }
        else if (curPosArg == argsCount)
        {
            break;
        }
        else
        {
            int nextPosArg = argsInfo[curPosArg].nextNotFound;
            if (nextPosArg == -1)
            {
                break;
            }
            curPosArg = static_cast<std::size_t>(nextPosArg);
        }
    }

    return PosArgRange{ startPosArg, eatenPosArgs };
}

// Map positional params to the desired arguments
template<typename Result, typename P>
void MapPositionalArgs(ArgInfoList& argsInfo, const P& params, const PosArgRange& range, Result& result)
{
    auto curArg = static_cast<int>(range.startPosArg);
    for (std::size_t idx = 0; idx < range.eatenPosArgs && curArg != -1 && static_cast<size_t>(curArg) < argsInfo.size(); ++idx)
    {
        const auto argIdx = static_cast<std::size_t>(curArg);
        auto& arg = argsInfo[argIdx];
        curArg = arg.nextNotFound;
        if (arg.state == Ignored)
        {
            continue;
        }

        BindArg(result, argIdx, *arg.info, params.posParams[idx]);
        arg.state = Positional;
    }
}

// Python binds the positional arguments first, in order: a keyword naming a parameter that a
// positional argument passed over, or any one before `*args` when positional arguments are left
// over for it, gives that parameter two values (TypeError "got multiple values for argument").
// The parameters after `*args` take keywords only, and so do all of a callable that takes extra
// positional arguments without declaring `*args` (applymacro's `macro`)
template<typename Result>
JINJA2CPP_COLD_INLINE void CheckMultipleValues(const ArgInfoList& argsInfo, std::size_t posParamsCount, std::size_t eatenPosArgs, Result& result, bool& isSucceeded)
{
    std::size_t varArgs = argsInfo.size();
    std::size_t end = 0;
    for (std::size_t idx = 0; idx != argsInfo.size(); ++idx)
    {
        if (argsInfo[idx].state == Ignored && argsInfo[idx].info->name == "*args")
        {
            varArgs = std::min(varArgs, idx);
        }
        if (argsInfo[idx].state == Positional)
        {
            end = idx + 1;
        }
    }
    if (eatenPosArgs < posParamsCount && varArgs != argsInfo.size())
    {
        end = varArgs;
    }
    end = std::min(end, varArgs);
    for (std::size_t idx = 0; idx != end; ++idx)
    {
        if (argsInfo[idx].state == Keyword)
        {
            result.multipleValues = argsInfo[idx].info;
            isSucceeded = false;
            return;
        }
    }
}

// Fill default arguments (if missing) and check for mandatory. A call bound at Load leaves
// them unbound: whoever reads the parameter takes the default from the declaration, so no
// node is made for it
template<typename Result>
void FillDefaultArgs(const ArgInfoList& argsInfo, Result& result, bool& isSucceeded)
{
    for (const auto& argInfo : argsInfo)
    {
        switch (argInfo.state)
        {
        case Positional:
        case Keyword:
        case Ignored:
        case Context:
            continue;
        case NotFound:
        {
            if constexpr (std::is_same_v<Result, ParsedArguments>)
            {
                if (!IsEmpty(argInfo.info->defaultVal))
                {
                    result.args[argInfo.info->name] = argInfo.info->defaultVal;
                }
            }
            break;
        }
        case NotFoundMandatory:
            isSucceeded = false;
            break;
        }
    }
}

// Fill the extra positional and kw-args
template<typename Result, typename P>
void FillExtraArgs(const ArgInfoList& argsInfo, const P& params, std::size_t eatenPosArgs, Result& result)
{
    for (auto& [name, value] : params.kwParams)
    {
        // A keyword that names a declared parameter is bound to it
        const auto& kwName = name;
        if (std::any_of(argsInfo.begin(), argsInfo.end(), [&kwName](const ArgInfo& arg) { return arg.state == Keyword && arg.info->name == kwName; }))
        {
            continue;
        }

        result.extraKwArgs[name] = value;
    }

    for (auto idx = eatenPosArgs; idx < params.posParams.size(); ++idx)
    {
        result.extraPosArgs.push_back(params.posParams[idx]);
    }
}
} // namespace

template<typename Result, typename T, typename P>
JINJA2CPP_HOT_INLINE Result ParseCallParamsImpl(const T& args, const P& params, bool& isSucceeded)
{
    ArgInfoList argsInfo(args.size());

    isSucceeded = true;

    Result result;
    if constexpr (std::is_same_v<Result, ParsedArgumentsInfo>)
    {
        result.args.resize(args.size());
    }

    int firstMandatoryIdx = MapKeywordArgs(args, params, argsInfo, result);
    auto posArgRange = FindPosArgRange(argsInfo, params.posParams.size(), firstMandatoryIdx);
    MapPositionalArgs(argsInfo, params, posArgRange, result);
    // Only a call with both kinds of argument can name one parameter twice
    if (!params.posParams.empty() && !params.kwParams.empty())
    {
        CheckMultipleValues(argsInfo, params.posParams.size(), posArgRange.eatenPosArgs, result, isSucceeded);
    }
    FillDefaultArgs(argsInfo, result, isSucceeded);
    FillExtraArgs(argsInfo, params, posArgRange.eatenPosArgs, result);

    return result;
}

ParsedArguments ParseCallParams(const std::initializer_list<ArgumentInfo>& args, const CallParams& params, bool& isSucceeded)
{
    return ParseCallParamsImpl<ParsedArguments>(args, params, isSucceeded);
}

ParsedArguments ParseCallParams(const std::vector<ArgumentInfo>& args, const CallParams& params, bool& isSucceeded)
{
    return ParseCallParamsImpl<ParsedArguments>(args, params, isSucceeded);
}

ParsedArgumentsInfo ParseCallParamsInfo(const std::vector<ArgumentInfo>& args, const CallParamsInfo& params, bool& isSucceeded)
{
    return ParseCallParamsImpl<ParsedArgumentsInfo>(args, params, isSucceeded);
}

CallParams EvaluateCallParams(const CallParamsInfo& info, RenderContext& context)
{
    CallParams result;

    result.posParams.reserve(info.posParams.size());
    for (const auto& p : info.posParams)
    {
        result.posParams.push_back(context.Nodes()[p].Evaluate(context));
    }

    for (const auto& [name, expr] : info.kwParams)
    {
        result.kwParams[name] = context.Nodes()[expr].Evaluate(context);
    }

    return result;
}

namespace
{
// f(*value): its items after the positional arguments. Python iterates an undefined value as
// empty (StrictUndefined refuses) and raises TypeError for a value that is not iterable
void AppendUnpackedArgs(const InternalValue& value, std::vector<InternalValue>& posParams)
{
    if (value.IsUndefined())
    {
        CheckUndefinedUse(value, UndefinedUse::Operator);
        return;
    }
    // A mapping converts to its keys all at once: check its size before they are copied
    if (const auto map = AsMap(value); map && !map->GetObjectClass())
    {
        CheckSequenceSize(uint64_t{ posParams.size() } + map->GetSize(), "a list");
    }
    bool isConverted = false;
    const auto items = ConvertToList(value, isConverted, false);
    if (!isConverted)
    {
        throw TemplateRuntimeError("Value after * must be an iterable, not "s + Apply<visitors::PythonTypeNameGetter>(value));
    }
    // The arguments are a sequence the template builds: f(*range(10**9)) is held to the cap
    if (const auto size = items.GetSize())
    {
        ReserveHint(posParams, posParams.size() + *size);
    }
    // An item may borrow from the value, a temporary such as a host callable's result: it keeps
    // the value alive, as an item read through a subscript does
    const bool borrows = value.ShouldExtendLifetime();
    for (auto&& item : items)
    {
        CheckSequenceSize(posParams.size() + 1, "a list");
        posParams.push_back(std::forward<decltype(item)>(item));
        if (borrows)
        {
            posParams.back().SetParentData(value);
        }
    }
}

[[noreturn]] void ThrowMultipleValues(const std::string& name)
{
    throw TemplateRuntimeError("got multiple values for keyword argument '" + name + "'");
}

// f(**value): its items after the keyword arguments, each name given once. An undefined value
// fails as Python's Undefined does when its keys are read
void AddUnpackedKwargs(const InternalValue& value, InternalDict& kwParams)
{
    CheckUndefinedUse(value, UndefinedUse::Attribute);
    const auto map = AsMap(value);
    if (!map || map->GetObjectClass() || value.IsUndefined())
    {
        throw TemplateRuntimeError("argument after ** must be a mapping, not "s + Apply<visitors::PythonTypeNameGetter>(value));
    }
    // The keyword arguments are a dict the template builds, held to the cap as one
    CheckSequenceSize(static_cast<uint64_t>(kwParams.size()) + map->GetSize(), "a dict");
    const bool borrows = value.ShouldExtendLifetime();
    for (auto& [key, item] : map->GetEntries())
    {
        if (borrows)
        {
            item.SetParentData(value);
        }
        if (!IsStringValue(key))
        {
            throw TemplateRuntimeError("keywords must be strings");
        }
        auto name = AsString(key);
        if (kwParams.find(name) != kwParams.end())
        {
            ThrowMultipleValues(name);
        }
        kwParams.emplace(std::move(name), std::move(item));
    }
}
} // namespace

CallParams EvaluateCallParams(const ArenaCallParams& info, RenderContext& context)
{
    CallParams result;
    // A copy of the view: evaluating an argument may switch the context's
    const auto nodes = context.Nodes();

    const auto posParams = nodes[info.posParams];
    result.posParams.reserve(posParams.size());
    for (const auto p : posParams)
    {
        result.posParams.push_back(nodes[p].Evaluate(context));
    }
    // Python takes *args before the keywords, wherever it is written
    if (info.dynArgs)
    {
        AppendUnpackedArgs(nodes[info.dynArgs].Evaluate(context), result.posParams);
    }

    for (const auto& param : nodes[info.kwParams])
    {
        result.kwParams[std::string(nodes.Text(param.name))] = nodes[param.value].Evaluate(context);
    }
    if (info.dynKwargs)
    {
        AddUnpackedKwargs(nodes[info.dynKwargs].Evaluate(context), result.kwParams);
    }

    return result;
}

void AddCallerParam(CallParams& params, InternalValue caller)
{
    // `{% call m(**{'caller': x}) %}` passes caller twice
    if (!params.kwParams.emplace("caller"s, std::move(caller)).second)
    {
        ThrowMultipleValues("caller");
    }
}

} // namespace helpers

NodeRef<Expression> MakeConstant(NodeArena& nodes, InternalValue constant)
{
    if (InlineScalar::Holds(constant))
    {
        return nodes.Make<ScalarConstantExpression>(constant);
    }
    return nodes.Make<ConstantExpression>(std::move(constant));
}

template<typename Params>
ArenaCallParams ArenaCallParams::Make(NodeArena& nodes, const Params& params)
{
    ArenaCallParams result;
    result.posParams = nodes.MakeSpan(params.posParams);
    if constexpr (std::is_same_v<Params, CallArgsInfo>)
    {
        result.dynArgs = params.dynArgs;
        result.dynKwargs = params.dynKwargs;
    }
    if (params.kwParams.empty())
    {
        return result;
    }
    SmallVector<ArenaKwParam, 4> kwParams;
    kwParams.reserve(params.kwParams.size());
    for (const auto& [name, expr] : params.kwParams)
    {
        kwParams.push_back({ nodes.MakeText(name), expr });
    }
    result.kwParams = nodes.MakeSpan(kwParams);
    return result;
}

template ArenaCallParams ArenaCallParams::Make(NodeArena& nodes, const CallParamsInfo& params);
template ArenaCallParams ArenaCallParams::Make(NodeArena& nodes, const CallArgsInfo& params);
} // namespace v2
} // namespace jinja2
