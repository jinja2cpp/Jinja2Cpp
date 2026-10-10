#include "expression_evaluator.h"

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
#include "testers.h"
#include "undefined.h"
#include "value_methods.h"
#include "value_visitors.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/value.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/container/small_vector.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <initializer_list>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{

// Out of line, so that the paths without autoescape stay small
void WriteEscaped(OutStream& stream, const InternalValue& val, IRendererCallback* callback)
{
    std::string_view view;
    if (!callback->IsWideTarget() && GetStringView(val, view) && view.size() <= detail::maxBufferedHtmlEscape)
    {
        auto escaped = detail::EscapeHtmlToBuffer(view);
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

void SubscriptExpression::AddIndex(const NodeArena& nodes, NodeRef<Expression> value, std::string attrName)
{
    Index idx;
    idx.expr = value;
    idx.isAttr = !attrName.empty();
    idx.maybeMethod = idx.isAttr && methods::IsMethodName(attrName);
    if (m_subscriptExprs.empty())
    {
        m_firstIndexIsPure = idx.isAttr || nodes[idx.expr].IsPure(nodes);
    }
    idx.attrName = std::move(attrName);
    m_subscriptExprs.push_back(std::move(idx));
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
        // x.name, the common case
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
        if (const auto* loop = GetIf<MapAdapter>(&*root))
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

bool LoopAttrExpression::TryCallCycle(RenderContext& values, ArenaSpan<NodeRef<Expression>> params, InternalValue& result) const
{
    if (m_attr != LoopAttr::Cycle || m_subscriptExprs.size() != 1)
    {
        return false;
    }
    const auto root = values.Nodes()[m_value].EvaluateRef(values);
    const auto* loop = root ? GetIf<MapAdapter>(&*root) : nullptr;
    InternalValue index0;
    if (!loop || !loop->GetLoopAttr(LoopAttr::Index0, index0))
    {
        return false;
    }
    // As CallLoopCycle does for a `loop` found by name
    const auto nodes = values.Nodes();
    const auto items = nodes[params];
    if (items.empty())
    {
        throw std::runtime_error("loop.cycle() expects at least one positional argument");
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
    const auto& data = value.GetData();
    return std::holds_alternative<EmptyValue>(data) || std::holds_alternative<bool>(data) || std::holds_alternative<int64_t>(data) || std::holds_alternative<double>(data) || std::holds_alternative<std::string>(data) || std::holds_alternative<TargetString>(data);
}
} // namespace

BinaryExpression::BinaryExpression(const NodeArena& nodes, BinaryExpression::Operation oper, NodeRef<Expression> leftExpr, NodeRef<Expression> rightExpr)
    : m_oper(oper)
    , m_leftExpr(leftExpr)
    , m_rightExpr(rightExpr)
    , m_rightByRef(nodes[rightExpr].IsPure(nodes))
{
    m_leftByRef = m_rightByRef && nodes[m_leftExpr].IsPure(nodes);
}

NodeRef<Expression> BinaryExpression::Make(NodeArena& nodes, Operation oper, NodeRef<Expression> leftExpr, NodeRef<Expression> rightExpr)
{
    if (oper == DivRemainder)
    {
        // Markup and wide literals keep the general path
        const auto* constant = nodes[leftExpr].GetConstant(nodes);
        auto format = constant ? NarrowStringView(*constant) : std::nullopt;
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
std::optional<InternalValue> ApplyPercentFormat(const InternalValue& leftVal, const InternalValue& rightVal, RenderContext& context)
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

    // Markup % args escapes the arguments and stays Markup
    InternalValue escapedArgs;
    if (leftVal.IsMarkup())
    {
        escapedArgs = EscapeFormatArgs(rightVal, context.GetRendererCallback());
    }
    const auto& values = leftVal.IsMarkup() ? escapedArgs : rightVal;
    auto narrow = NarrowStringView(leftVal);
    auto formatted = narrow ? PythonPercentFormat(*narrow, values)
                            : PythonPercentFormat(ApplyStringConverter(leftVal, [](auto str) { return ConvertString<std::string>(str); }), values);
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

// a ~ b: both operands as strings of the template's width
InternalValue ConcatAsStrings(const InternalValue& leftVal, const InternalValue& rightVal, RenderContext& context)
{
    // The right operand is rendered straight onto the left one's text
    auto result = context.GetRendererCallback()->GetAsTargetString(leftVal);
    if (auto* str = std::get_if<std::string>(&result))
    {
        Apply<visitors::ValueRenderer<char>>(rightVal, *str);
    }
    else
    {
        Apply<visitors::ValueRenderer<wchar_t>>(rightVal, std::get<std::wstring>(result));
    }
    return InternalValue(std::move(result));
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
        if (auto formatted = ApplyPercentFormat(leftVal, rightVal, context))
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
    case jinja2::BinaryExpression::StringConcat:
        result = ConcatAsStrings(leftVal, rightVal, context);
        break;
    default:
        break;
    }
    return result;
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

InternalValue SliceExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    auto part = [&context](const NodeRef<Expression>& expr) { return expr ? context.Nodes()[expr].Evaluate(context) : InternalValue(); };
    InternalValue value = context.Nodes()[m_value].Evaluate(context);
    auto start = part(m_start);
    auto stop = part(m_stop);
    auto step = part(m_step);
    return Slice(value, start, stop, step);
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

namespace
{
// Mapping keys are strings in the value model; integer and boolean keys are stored as
// their decimal or Python spelling, so {1: 'x'} has the key '1'
struct DictKeyGetter : public visitors::BaseVisitor<std::string>
{
    using BaseVisitor::operator();

    template<typename CharT>
    std::string operator()(const std::basic_string<CharT>& str) const
    {
        return ConvertString<std::string>(str);
    }
    template<typename CharT>
    std::string operator()(const std::basic_string_view<CharT>& str) const
    {
        return ConvertString<std::string>(str);
    }
    std::string operator()(int64_t val) const { return std::to_string(val); }
    std::string operator()(bool val) const { return val ? "True" : "False"; }
};
} // namespace

InternalValue DictCreator::Evaluate(RenderContext& context)
{
    CheckStack();
    const auto nodes = context.Nodes();
    InternalDict result;
    for (const auto& item : nodes[m_exprs])
    {
        // Python evaluates the key before the value; an assignment does not fix that order
        auto key = Apply<DictKeyGetter>(nodes[item.key].Evaluate(context));
        auto value = nodes[item.value].Evaluate(context);
        result[std::move(key)] = std::move(value);
    }

    return CreateMapAdapter(std::move(result));
}

NodeRef<ExpressionFilter> ExpressionFilter::Make(NodeArena& nodes, const std::string& filterName, const CallParamsInfo& params, InternalValue registered)
{
    // Filters added to the environment take precedence over the builtins, as in Jinja2's env.filters
    const auto filter = GetIf<Callable>(&registered)
                            ? nodes.MakeObject<IExpressionFilter, filters::UserDefinedFilter>(filterName, params, std::move(registered))
                            : CreateFilter(nodes, filterName, params);
    ArenaText argsError;
    if (auto error = nodes[filter].GetArgumentsError(); !error.empty())
    {
        argsError = nodes.MakeText(filterName + "() " + error);
    }
    return nodes.Make<ExpressionFilter>(filter, argsError);
}

void ExpressionFilter::SetConstantBase(const NodeArena& nodes, const InternalValue& base)
{
    if (m_parentFilter)
    {
        nodes[m_parentFilter].SetConstantBase(nodes, base);
    }
    else
    {
        nodes[m_filter].SetConstantBase(base);
    }
}

namespace
{
// Out of line, so that the filters that fit their call pay only the test
[[noreturn]] JINJA2CPP_NOINLINE_INLINE void ThrowArgumentsError(std::string_view error)
{
    throw std::runtime_error(std::string(error));
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

NodeRef<IsExpression> IsExpression::Make(NodeArena& nodes, NodeRef<Expression> value, const std::string& tester, const CallParamsInfo& params, InternalValue registered)
{
    if (GetIf<Callable>(&registered))
    {
        const auto test = nodes.MakeObject<ITester, testers::UserDefinedTester>(tester, params, std::move(registered));
        return nodes.Make<IsExpression>(value, test, false);
    }
    const bool testInPlace = params.posParams.empty() && params.kwParams.empty();
    const auto test = CreateTester(nodes, tester, params);
    return nodes.Make<IsExpression>(value, test, testInPlace);
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
    if (const auto loopAttr = nodes.As<LoopAttrExpression>(m_valueRef); loopAttr && nodes[loopAttr].TryCallCycle(values, m_params.posParams, result))
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
        auto* map = GetIf<MapAdapter>(&receiver);
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
        if (callee.IsUndefined() && !IsEmpty(receiver) && !GetIf<MapAdapter>(&receiver))
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
    if (ConvertToInt(fnVal, InvalidFn) == LoopCycleFn)
    {
        return CallLoopCycle(values);
    }
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
        throw std::runtime_error("'"s + Apply<visitors::PythonTypeNameGetter>(fnVal) + "' object is not callable");
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
    // A macro returns Markup when autoescape is on where it is called
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

InternalValue CallExpression::CallLoopCycle(RenderContext& values)
{
    const auto loopVal = values.FindValue(std::string("loop"));
    const auto* loop = loopVal ? GetIf<MapAdapter>(&*loopVal) : nullptr;
    if (!loop)
    {
        return InternalValue();
    }

    const auto nodes = values.Nodes();
    const auto params = nodes[m_params.posParams];
    if (params.empty())
    {
        throw std::runtime_error("loop.cycle() expects at least one positional argument");
    }
    int64_t baseIdx = Apply<visitors::IntegerEvaluator>(loop->GetValueByName("index0"));
    // Unsigned on purpose: a user-defined `loop` may carry a negative index0
    auto idx = static_cast<size_t>(baseIdx) % params.size();
    return nodes[params[idx]].Evaluate(values);
}


namespace helpers
{
enum ArgState
{
    NotFound,
    NotFoundMandatory,
    Keyword,
    Positional,
    Ignored
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

using ArgInfoList = boost::container::small_vector<ArgInfo, 8>;

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
            for (; startPosArg < argsCount && (argsInfo[startPosArg].state == Keyword || argsInfo[startPosArg].state == Positional); ++startPosArg)
                ;

            isFirstTime = false;
            if (startPosArg == argsCount)
            {
                break;
            }
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
Result ParseCallParamsImpl(const T& args, const P& params, bool& isSucceeded)
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

    for (const auto& param : nodes[info.kwParams])
    {
        result.kwParams[std::string(nodes.Text(param.name))] = nodes[param.value].Evaluate(context);
    }

    return result;
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

ArenaCallParams ArenaCallParams::Make(NodeArena& nodes, const CallParamsInfo& params)
{
    ArenaCallParams result;
    result.posParams = nodes.MakeSpan(params.posParams);
    if (params.kwParams.empty())
    {
        return result;
    }
    boost::container::small_vector<ArenaKwParam, 4> kwParams;
    kwParams.reserve(params.kwParams.size());
    for (const auto& [name, expr] : params.kwParams)
    {
        kwParams.push_back({ nodes.MakeText(name), expr });
    }
    result.kwParams = nodes.MakeSpan(kwParams);
    return result;
}
} // namespace jinja2
