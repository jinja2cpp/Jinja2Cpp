#include "expression_evaluator.h"

#include "filters.h"
#include "internal_value.h"
#include "markup.h"
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
    if (const auto* value = EvaluateRef(values))
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
    if (m_tester && !m_tester->Evaluate(values))
    {
        return m_tester->EvaluateAltValue(values);
    }

    return m_expression->Evaluate(values);
}

void FullExpressionEvaluator::Render(OutStream& stream, RenderContext& values)
{
    if (!m_tester)
    {
        m_expression->Render(stream, values);
    }
    else if (m_expression && m_tester->GetAltValue())
    {
        // The branch the condition picks renders itself, a variable without a copy
        CheckStack();
        if (m_tester->Evaluate(values))
        {
            m_expression->Render(stream, values);
        }
        else
        {
            m_tester->GetAltValue()->Render(stream, values);
        }
    }
    else
    {
        Expression::Render(stream, values);
    }
}

const InternalValue* ValueRefExpression::EvaluateRef(RenderContext& values)
{
    return values.FindValueCached(this, GetHashedName());
}

InternalValue ValueRefExpression::Evaluate(RenderContext& values)
{
    if (const auto* value = values.FindValueCached(this, GetHashedName()))
    {
        return *value;
    }

    return MakeUndefined(values, m_valueName);
}

const InternalValue* SelfRefExpression::EvaluateRef(RenderContext& values)
{
    return values.FindSelf(GetName());
}

InternalValue SelfRefExpression::Evaluate(RenderContext& values)
{
    if (const auto* value = values.FindSelf(GetName()))
    {
        return *value;
    }

    return MakeUndefined(values, GetName());
}

void SubscriptExpression::AddIndex(ExpressionEvaluatorPtr<Expression> value, std::string attrName)
{
    Index idx;
    idx.expr = std::move(value);
    idx.isAttr = !attrName.empty();
    idx.maybeMethod = idx.isAttr && methods::IsMethodName(attrName);
    if (m_subscriptExprs.empty())
    {
        m_firstIndexIsPure = idx.isAttr || idx.expr->IsPure();
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
    InternalValue key = idx.expr->Evaluate(values);
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
            InternalValue key = idx.isAttr ? InternalValue(idx.attrName) : idx.expr->Evaluate(values);
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
    const auto* root = m_firstIndexIsPure ? m_value->EvaluateRef(values) : nullptr;
    if (!root)
    {
        return EvaluateIndices(m_value->Evaluate(values), 0, m_subscriptExprs.size(), values, false);
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

namespace
{
// The value of expr for a method that changes it in place: a list or dict stored in a
// variable becomes one the template owns, stored back in the variable
InternalValue EvaluateMutableRoot(const ExpressionEvaluatorPtr<Expression>& expr, RenderContext& values)
{
    if (auto* subscript = dynamic_cast<SubscriptExpression*>(expr.get()))
    {
        return subscript->EvaluateMutable(values);
    }
    if (auto* ref = dynamic_cast<ValueRefExpression*>(expr.get()))
    {
        if (auto* slot = values.FindValueSlot(ref->GetName()))
        {
            if (methods::IsContainer(*slot) && !methods::IsMutable(*slot))
            {
                *slot = methods::MakeMutable(*slot);
            }
            return *slot;
        }
    }
    return expr->Evaluate(values);
}
} // namespace

InternalValue SubscriptExpression::EvaluateReceiver(RenderContext& values, bool forMutation)
{
    auto root = forMutation ? EvaluateMutableRoot(m_value, values) : m_value->Evaluate(values);
    return EvaluateIndices(std::move(root), 0, m_subscriptExprs.size() - 1, values, forMutation);
}

InternalValue SubscriptExpression::EvaluateMutable(RenderContext& values)
{
    return EvaluateIndices(EvaluateMutableRoot(m_value, values), 0, m_subscriptExprs.size(), values, true);
}

FilteredExpression::FilteredExpression(ExpressionEvaluatorPtr<Expression> expression, ExpressionEvaluatorPtr<ExpressionFilter> filter)
    : m_expression(std::move(expression))
    , m_filter(std::move(filter))
{
    if (const auto* constant = m_expression->GetConstant(); constant && m_filter)
    {
        m_filter->SetConstantBase(*constant);
    }
}

InternalValue FilteredExpression::Evaluate(RenderContext& values)
{
    CheckStack();
    auto origResult = m_expression->Evaluate(values);
    return m_filter->Evaluate(origResult, values);
}

InternalValue UnaryExpression::Evaluate(RenderContext& values)
{
    CheckStack();
    auto value = m_expr->Evaluate(values);
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

BinaryExpression::BinaryExpression(BinaryExpression::Operation oper, ExpressionEvaluatorPtr<> leftExpr, const ExpressionEvaluatorPtr<>& rightExpr)
    : m_oper(oper)
    , m_leftExpr(std::move(leftExpr))
    , m_rightExpr(rightExpr)
    , m_rightByRef(rightExpr->IsPure())
{
    m_leftByRef = m_rightByRef && m_leftExpr->IsPure();
    if (m_oper == DivRemainder)
    {
        // Markup and wide literals keep the general path
        const auto* constant = m_leftExpr->GetConstant();
        auto format = constant ? NarrowStringView(*constant) : std::nullopt;
        if (format && !constant->IsMarkup())
        {
            m_constFormat = std::make_shared<const CompiledPercentFormat>(std::string(*format));
        }
    }
    const auto* literal = m_oper == In ? dynamic_cast<const TupleCreator*>(rightExpr.get()) : nullptr;
    if (!literal)
    {
        return;
    }
    InternalValueList items;
    items.reserve(literal->GetItems().size());
    for (const auto& item : literal->GetItems())
    {
        const auto* constant = dynamic_cast<const ConstantExpression*>(item.get());
        if (!constant || !IsImmutableScalar(constant->GetValue()))
        {
            return;
        }
        items.push_back(constant->GetValue());
    }
    m_constItems = std::move(items);
    m_hasConstItems = true;
}

InternalValue BinaryExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    if (m_constFormat)
    {
        if (m_rightByRef)
        {
            if (const auto* rightVal = m_rightExpr->EvaluateRef(context))
            {
                return FormatConstant(*rightVal);
            }
        }
        return FormatConstant(m_rightExpr->Evaluate(context));
    }
    // A plain variable or constant is read in place when the right operand cannot change it
    if (m_leftByRef)
    {
        if (const auto* leftVal = m_leftExpr->EvaluateRef(context))
        {
            return EvaluateWithLeft(*leftVal, context);
        }
    }
    return EvaluateWithLeft(m_leftExpr->Evaluate(context), context);
}

InternalValue BinaryExpression::EvaluateWithLeft(const InternalValue& leftVal, RenderContext& context)
{
    // `and` and `or` short-circuit and return the deciding operand, as in Python
    if (m_oper == LogicalAnd)
    {
        return ConvertToBool(leftVal) ? m_rightExpr->Evaluate(context) : leftVal;
    }
    if (m_oper == LogicalOr)
    {
        return ConvertToBool(leftVal) ? leftVal : m_rightExpr->Evaluate(context);
    }

    if (m_hasConstItems)
    {
        CheckUndefinedUse(leftVal, UndefinedUse::Operator);
        return InternalValue(testers::IsValueInList(leftVal, m_constItems));
    }

    if (m_rightByRef)
    {
        if (const auto* rightVal = m_rightExpr->EvaluateRef(context))
        {
            return Apply(leftVal, *rightVal, context);
        }
    }
    return Apply(leftVal, m_rightExpr->Evaluate(context), context);
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

InternalValue BinaryExpression::FormatConstant(const InternalValue& rightVal) const
{
    // What Apply does for a narrow string on the left
    CheckUndefinedUse(rightVal, UndefinedUse::Operator);
    return InternalValue(TargetString(m_constFormat->Format(rightVal)));
}

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
    InternalValue left = m_first->Evaluate(context);
    CheckUndefinedUse(left, UndefinedUse::Operator);
    for (auto& operand : m_operands)
    {
        InternalValue right = operand.expr->Evaluate(context);
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
        left = std::move(right);
    }

    return InternalValue(true);
}

InternalValue SliceExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    auto part = [&context](const ExpressionEvaluatorPtr<>& expr) { return expr ? expr->Evaluate(context) : InternalValue(); };
    InternalValue value = m_value->Evaluate(context);
    auto start = part(m_start);
    auto stop = part(m_stop);
    auto step = part(m_step);
    return Slice(value, start, stop, step);
}

InternalValue TupleCreator::Evaluate(RenderContext& context)
{
    CheckStack();
    InternalValueList result;
    result.reserve(m_exprs.size());
    for (auto& e : m_exprs)
    {
        result.push_back(e->Evaluate(context));
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
    InternalDict result;
    for (auto& [keyExpr, valueExpr] : m_exprs)
    {
        // Python evaluates the key before the value; an assignment does not fix that order
        auto key = Apply<DictKeyGetter>(keyExpr->Evaluate(context));
        auto value = valueExpr->Evaluate(context);
        result[std::move(key)] = std::move(value);
    }

    return CreateMapAdapter(std::move(result));
}

ExpressionFilter::ExpressionFilter(const std::string& filterName, const CallParamsInfo& params, InternalValue registered)
{
    // Filters added to the environment take precedence over the builtins, as in Jinja2's env.filters
    if (GetIf<Callable>(&registered))
    {
        m_filter = std::make_shared<filters::UserDefinedFilter>(filterName, params, std::move(registered));
    }
    else
    {
        m_filter = CreateFilter(filterName, params);
    }
    if (!m_filter)
    {
        throw std::runtime_error("Can't find filter '" + filterName + "'");
    }
    auto argsError = m_filter->GetArgumentsError();
    if (!argsError.empty())
    {
        m_argsError = filterName + "() " + argsError;
    }
}

void ExpressionFilter::SetConstantBase(const InternalValue& base)
{
    if (m_parentFilter)
    {
        m_parentFilter->SetConstantBase(base);
    }
    else if (m_filter)
    {
        m_filter->SetConstantBase(base);
    }
}

InternalValue ExpressionFilter::Evaluate(const InternalValue& baseVal, RenderContext& context)
{
    CheckStack();
    if (!m_argsError.empty())
    {
        throw std::runtime_error(m_argsError);
    }
    if (m_parentFilter)
    {
        return m_filter->Filter(m_parentFilter->Evaluate(baseVal, context), context);
    }

    return m_filter->Filter(baseVal, context);
}

IsExpression::IsExpression(ExpressionEvaluatorPtr<> value, const std::string& tester, CallParamsInfo params, InternalValue registered)
    : m_value(std::move(value))
{
    if (GetIf<Callable>(&registered))
    {
        m_tester = std::make_shared<testers::UserDefinedTester>(tester, std::move(params), std::move(registered));
    }
    else
    {
        m_testInPlace = params.posParams.empty() && params.kwParams.empty();
        m_tester = CreateTester(tester, std::move(params));
    }
    if (!m_tester)
    {
        throw std::runtime_error("Can't find tester '" + tester + "'");
    }
}

InternalValue IsExpression::Evaluate(RenderContext& context)
{
    CheckStack();
    if (m_testInPlace)
    {
        if (const auto* value = m_value->EvaluateRef(context))
        {
            return m_tester->Test(*value, context);
        }
    }
    return m_tester->Test(m_value->Evaluate(context), context);
}

bool IfExpression::Evaluate(RenderContext& context)
{
    return ConvertToBool(m_testExpr->Evaluate(context));
}

InternalValue IfExpression::EvaluateAltValue(RenderContext& context)
{
    return m_altValue ? m_altValue->Evaluate(context) : InternalValue();
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
    auto* subscript = dynamic_cast<SubscriptExpression*>(m_valueRef.get());
    const std::string* name = subscript ? subscript->GetCallName() : nullptr;
    if (!name)
    {
        callee = m_valueRef->Evaluate(values);
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
    const auto* value = m_valueRef->EvaluateRef(values);
    const auto* callable = value ? GetIf<Callable>(value) : nullptr;
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

InternalValue CallExpression::CallArbitraryFn(RenderContext& values, InternalValue fnVal)
{
    const auto* callable = GetIf<Callable>(&fnVal);
    if (!callable)
    {
        auto callOperator = Subscript(fnVal, "operator()"s, nullptr);
        callable = GetIf<Callable>(&callOperator);
        if (!callable)
        {
            // Calling a named undefined is an UndefinedError; any other value is not callable
            CheckUndefinedUse(fnVal, UndefinedUse::Call);
            if (fnVal.IsUndefined())
            {
                return InternalValue();
            }
            throw std::runtime_error("'"s + Apply<visitors::PythonTypeNameGetter>(fnVal) + "' object is not callable");
        }
        fnVal = std::move(callOperator);
        callable = GetIf<Callable>(&fnVal);
    }

    return CallCallable(values, *callable);
}

InternalValue CallExpression::CallCallable(RenderContext& values, const Callable& callable)
{
    auto kind = callable.GetKind();
    if (kind != Callable::GlobalFunc && kind != Callable::UserCallable && kind != Callable::Macro)
    {
        return InternalValue();
    }

    auto callParams = helpers::EvaluateCallParams(m_params, values);

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

InternalValue CallExpression::CallLoopCycle(RenderContext& values)
{
    bool loopFound = false;
    const auto* loopValP = values.FindValue("loop", loopFound);
    if (!loopFound)
    {
        return InternalValue();
    }

    if (m_params.posParams.empty())
    {
        throw std::runtime_error("loop.cycle() expects at least one positional argument");
    }
    const auto* loop = GetIf<MapAdapter>(&loopValP->second);
    int64_t baseIdx = Apply<visitors::IntegerEvaluator>(loop->GetValueByName("index0"));
    // Unsigned on purpose: a user-defined `loop` may carry a negative index0
    auto idx = static_cast<size_t>(baseIdx) % m_params.posParams.size();
    return m_params.posParams[idx]->Evaluate(values);
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

template<typename Result>
struct ParsedArgumentDefaultValGetter;

template<>
struct ParsedArgumentDefaultValGetter<ParsedArguments>
{
    static auto Get(const InternalValue& val) { return val; }
};

template<>
struct ParsedArgumentDefaultValGetter<ParsedArgumentsInfo>
{
    static auto Get(const InternalValue& val) { return std::make_shared<ConstantExpression>(val); }
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
            result.args[argInfo.name] = p->second;
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
        auto& arg = argsInfo[static_cast<std::size_t>(curArg)];
        curArg = arg.nextNotFound;
        if (arg.state == Ignored)
        {
            continue;
        }

        result.args[arg.info->name] = params.posParams[idx];
        arg.state = Positional;
    }
}

template<typename Result>
void SetDefaultArg(const ArgumentInfo& info, Result& result)
{
#if __cplusplus >= 201703L
    if constexpr (std::is_same_v<Result, ParsedArgumentsInfo>)
    {
        result.args[info.name] = info.defaultExpr ? info.defaultExpr : std::make_shared<ConstantExpression>(info.defaultVal);
    }
    else
    {
        result.args[info.name] = info.defaultVal;
    }
#else
    result.args[info.name] = ParsedArgumentDefaultValGetter<Result>::Get(info.defaultVal);
#endif
}

// Fill default arguments (if missing) and check for mandatory
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
            if (!IsEmpty(argInfo.info->defaultVal))
            {
                SetDefaultArg(*argInfo.info, result);
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
void FillExtraArgs(const P& params, std::size_t eatenPosArgs, Result& result)
{
    for (auto& [name, value] : params.kwParams)
    {
        if (result.args.find(name) != result.args.end())
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

    int firstMandatoryIdx = MapKeywordArgs(args, params, argsInfo, result);
    auto posArgRange = FindPosArgRange(argsInfo, params.posParams.size(), firstMandatoryIdx);
    MapPositionalArgs(argsInfo, params, posArgRange, result);
    FillDefaultArgs(argsInfo, result, isSucceeded);
    FillExtraArgs(params, posArgRange.eatenPosArgs, result);

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

ParsedArgumentsInfo ParseCallParamsInfo(const std::initializer_list<ArgumentInfo>& args, const CallParamsInfo& params, bool& isSucceeded)
{
    return ParseCallParamsImpl<ParsedArgumentsInfo>(args, params, isSucceeded);
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
        result.posParams.push_back(p->Evaluate(context));
    }

    for (const auto& [name, expr] : info.kwParams)
    {
        result.kwParams[name] = expr->Evaluate(context);
    }

    return result;
}

} // namespace helpers
} // namespace jinja2
