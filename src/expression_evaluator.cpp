#include "expression_evaluator.h"
#include "filters.h"
#include "generic_adapters.h"
#include "internal_value.h"
#include "markup.h"
#include "out_stream.h"
#include "python_format.h"
#include "testers.h"
#include "undefined.h"
#include "value_methods.h"
#include "value_visitors.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/container/small_vector.hpp>

#include <cmath>
#include <stack>

namespace jinja2
{

void ExpressionEvaluatorBase::Render(OutStream& stream, RenderContext& values)
{
    stream.WriteValue(OutputValue(Evaluate(values), values));
}


InternalValue FullExpressionEvaluator::Evaluate(RenderContext& values)
{
    if (!m_expression)
        return InternalValue();

    // Python evaluates the condition first, then only the branch it picks
    if (m_tester && !m_tester->Evaluate(values))
        return m_tester->EvaluateAltValue(values);

    return m_expression->Evaluate(values);
}

void FullExpressionEvaluator::Render(OutStream& stream, RenderContext& values)
{
    if (!m_tester)
        m_expression->Render(stream, values);
    else
        Expression::Render(stream, values);
}

InternalValue ValueRefExpression::Evaluate(RenderContext& values)
{
    bool found = false;
    auto p = values.FindValue(m_valueName, found);
    if (found)
        return p->second;

    return MakeUndefined(values, m_valueName);
}

void SubscriptExpression::AddIndex(ExpressionEvaluatorPtr<Expression> value, std::string attrName)
{
    Index idx;
    idx.expr = std::move(value);
    idx.isAttr = !attrName.empty();
    idx.maybeMethod = idx.isAttr && methods::IsMethodName(attrName);
    idx.attrName = std::move(attrName);
    m_subscriptExprs.push_back(std::move(idx));
}

InternalValue SubscriptExpression::ApplyIndex(const InternalValue& cur, const Index& idx, RenderContext& values) const
{
    InternalValue key = idx.isAttr ? InternalValue(idx.attrName) : idx.expr->Evaluate(values);
    return LookupIndex(cur, idx, key, values);
}

// An attribute or item of a named undefined fails unless it is chainable; a missing one is
// an undefined that knows where it came from
InternalValue SubscriptExpression::LookupIndex(const InternalValue& cur, const Index& idx, const InternalValue& key, RenderContext& values) const
{
    if (GetUndefinedInfo(cur) != nullptr)
    {
        CheckUndefinedUse(cur, UndefinedUse::Attribute);
        return cur;
    }
    auto result = idx.isAttr ? (idx.maybeMethod ? methods::GetAttr(cur, idx.attrName, &values) : Subscript(cur, idx.attrName, &values))
                             : methods::GetItem(cur, key, &values);
    if (result.IsUndefined() && GetUndefinedInfo(result) == nullptr)
        return MakeUndefined(&values, cur, key);
    return result;
}

InternalValue SubscriptExpression::EvaluateIndices(InternalValue cur, size_t count, RenderContext& values, bool forMutation) const
{
    for (size_t n = 0; n < count; ++n)
    {
        auto& idx = m_subscriptExprs[n];
        InternalValue newVal;
        if (!forMutation)
        {
            newVal = ApplyIndex(cur, idx, values);
        }
        else
        {
            InternalValue key = idx.isAttr ? InternalValue(idx.attrName) : idx.expr->Evaluate(values);
            newVal = LookupIndex(cur, idx, key, values);
            // A borrowed list or dict inside one the template owns is replaced by its own copy
            if (methods::IsContainer(newVal) && !methods::IsMutable(newVal) && methods::IsMutable(cur))
            {
                newVal = methods::MakeMutable(newVal);
                methods::StoreItem(cur, key, newVal);
            }
        }
        if (cur.ShouldExtendLifetime())
            newVal.SetParentData(cur);
        std::swap(newVal, cur);
    }

    return cur;
}

InternalValue SubscriptExpression::Evaluate(RenderContext& values)
{
    return EvaluateIndices(m_value->Evaluate(values), m_subscriptExprs.size(), values, false);
}

namespace
{
// The value of expr for a method that changes it in place: a list or dict stored in a
// variable becomes one the template owns, stored back in the variable
InternalValue EvaluateMutableRoot(const ExpressionEvaluatorPtr<Expression>& expr, RenderContext& values)
{
    if (auto* subscript = dynamic_cast<SubscriptExpression*>(expr.get()))
        return subscript->EvaluateMutable(values);
    if (auto* ref = dynamic_cast<ValueRefExpression*>(expr.get()))
    {
        if (auto* slot = values.FindValueSlot(ref->GetName()))
        {
            if (methods::IsContainer(*slot) && !methods::IsMutable(*slot))
                *slot = methods::MakeMutable(*slot);
            return *slot;
        }
    }
    return expr->Evaluate(values);
}
} // namespace

InternalValue SubscriptExpression::EvaluateReceiver(RenderContext& values, bool forMutation)
{
    auto root = forMutation ? EvaluateMutableRoot(m_value, values) : m_value->Evaluate(values);
    return EvaluateIndices(std::move(root), m_subscriptExprs.size() - 1, values, forMutation);
}

InternalValue SubscriptExpression::EvaluateMutable(RenderContext& values)
{
    return EvaluateIndices(EvaluateMutableRoot(m_value, values), m_subscriptExprs.size(), values, true);
}

InternalValue FilteredExpression::Evaluate(RenderContext& values)
{
    auto origResult = m_expression->Evaluate(values);
    return m_filter->Evaluate(origResult, values);
}

InternalValue UnaryExpression::Evaluate(RenderContext& values)
{
    auto value = m_expr->Evaluate(values);
    if (m_oper == LogicalNot)
        return !ConvertToBool(value);
    CheckUndefinedUse(value, UndefinedUse::Arithmetic);
    return Apply<visitors::UnaryOperation>(value, m_oper);
}

BinaryExpression::BinaryExpression(BinaryExpression::Operation oper, ExpressionEvaluatorPtr<> leftExpr, ExpressionEvaluatorPtr<> rightExpr)
    : m_oper(oper)
    , m_leftExpr(leftExpr)
    , m_rightExpr(rightExpr)
{
    if (m_oper == In)
    {
        CallParamsInfo params;
        params.kwParams["seq"] = rightExpr;
        m_inTester = CreateTester("in", params);
    }
}

InternalValue BinaryExpression::Evaluate(RenderContext& context)
{
    InternalValue leftVal = m_leftExpr->Evaluate(context);

    // `and` and `or` short-circuit and return the deciding operand, as in Python
    if (m_oper == LogicalAnd)
        return ConvertToBool(leftVal) ? m_rightExpr->Evaluate(context) : leftVal;
    if (m_oper == LogicalOr)
        return ConvertToBool(leftVal) ? leftVal : m_rightExpr->Evaluate(context);

    InternalValue rightVal = m_oper == In ? InternalValue() : m_rightExpr->Evaluate(context);
    InternalValue result;
    // StrictUndefined fails on any operator; the others fail in the arithmetic below
    CheckUndefinedUse(leftVal, UndefinedUse::Operator);
    CheckUndefinedUse(rightVal, UndefinedUse::Operator);

    // str % values is Python's printf-style formatting; the result keeps the string's width
    if (m_oper == DivRemainder)
    {
        bool isWide = false;
        bool isString = ApplyStringConverter(leftVal, [&isWide](auto str) {
            isWide = sizeof(str[0]) != sizeof(char);
            return true;
        });
        if (isString)
        {
            // Markup % args escapes the arguments and stays Markup
            if (leftVal.IsMarkup())
                rightVal = EscapeFormatArgs(rightVal, context.GetRendererCallback());
            auto formatted = PythonPercentFormat(ApplyStringConverter(leftVal, [](auto str) { return ConvertString<std::string>(str); }), rightVal);
            InternalValue formattedVal = isWide ? TargetString(ConvertString<std::wstring>(formatted)) : TargetString(std::move(formatted));
            formattedVal.SetMarkup(leftVal.IsMarkup());
            return formattedVal;
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
        // Markup + str escapes the other operand, Markup * n stays Markup
        if (m_oper == Plus && (leftVal.IsMarkup() || rightVal.IsMarkup()) && IsStringValue(leftVal) && IsStringValue(rightVal))
        {
            auto* callback = context.GetRendererCallback();
            result = Apply2<visitors::BinaryMathOperation>(MarkupEscape(leftVal, callback), MarkupEscape(rightVal, callback), m_oper);
            result.SetMarkup();
            break;
        }
        result = Apply2<visitors::BinaryMathOperation>(leftVal, rightVal, m_oper);
        if (m_oper == Mul && (leftVal.IsMarkup() || rightVal.IsMarkup()))
            result.SetMarkup(IsStringValue(result));
        break;
    case jinja2::BinaryExpression::In:
    {
        result = m_inTester->Test(leftVal, context);
        break;
    }
    case jinja2::BinaryExpression::StringConcat:
    {
        auto leftStr = context.GetRendererCallback()->GetAsTargetString(leftVal);
        auto rightStr = context.GetRendererCallback()->GetAsTargetString(rightVal);
        TargetString resultStr;
        std::string* nleftStr = GetIf<std::string>(&leftStr);
        if (nleftStr != nullptr)
        {
            auto* nrightStr = GetIf<std::string>(&rightStr);
            resultStr = *nleftStr + *nrightStr;
        }
        else
        {
            auto* wleftStr = GetIf<std::wstring>(&leftStr);
            auto* wrightStr = GetIf<std::wstring>(&rightStr);
            resultStr = *wleftStr + *wrightStr;
        }
        result = InternalValue(std::move(resultStr));
        break;
    }
    default:
        break;
    }
    return result;
}

InternalValue CompareExpression::Evaluate(RenderContext& context)
{
    InternalValue left = m_first->Evaluate(context);
    CheckUndefinedUse(left, UndefinedUse::Operator);
    for (auto& operand : m_operands)
    {
        InternalValue right = operand.expr->Evaluate(context);
        CheckUndefinedUse(right, UndefinedUse::Operator);
        bool result = false;
        if (operand.operation == BinaryExpression::In)
        {
            CallParamsInfo params;
            params.kwParams["seq"] = std::make_shared<ConstantExpression>(right);
            result = CreateTester("in", std::move(params))->Test(left, context);
        }
        else
        {
            result = ConvertToBool(Apply2<visitors::BinaryMathOperation>(left, right, operand.operation));
        }

        if (result == operand.negated)
            return InternalValue(false);
        left = std::move(right);
    }

    return InternalValue(true);
}

InternalValue SliceExpression::Evaluate(RenderContext& context)
{
    auto part = [&context](const ExpressionEvaluatorPtr<>& expr) { return expr ? expr->Evaluate(context) : InternalValue(); };
    InternalValue value = m_value->Evaluate(context);
    auto start = part(m_start);
    auto stop = part(m_stop);
    auto step = part(m_step);
    return Slice(value, start, stop, step);
}

InternalValue TupleCreator::Evaluate(RenderContext& context)
{
    InternalValueList result;
    for (auto& e : m_exprs)
    {
        result.push_back(e->Evaluate(context));
    }

    auto list = ListAdapter::CreateAdapter(std::move(result));
    if (m_isTuple)
        list.MarkAsTuple();
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
    std::string operator()(const nonstd::basic_string_view<CharT>& str) const
    {
        return ConvertString<std::string>(str);
    }
    std::string operator()(int64_t val) const { return std::to_string(val); }
    std::string operator()(bool val) const { return val ? "True" : "False"; }
};
} // namespace

InternalValue DictCreator::Evaluate(RenderContext& context)
{
    InternalDict result;
    for (auto& e : m_exprs)
    {
        // Python evaluates the key before the value; an assignment does not fix that order
        auto key = Apply<DictKeyGetter>(e.first->Evaluate(context));
        auto value = e.second->Evaluate(context);
        result[std::move(key)] = std::move(value);
    }

    return CreateMapAdapter(std::move(result));
}

ExpressionFilter::ExpressionFilter(const std::string& filterName, CallParamsInfo params, InternalValue registered)
{
    // Filters added to the environment take precedence over the builtins, as in Jinja2's env.filters
    if (GetIf<Callable>(&registered))
        m_filter = std::make_shared<filters::UserDefinedFilter>(filterName, std::move(params), std::move(registered));
    else
        m_filter = CreateFilter(filterName, std::move(params));
    if (!m_filter)
        throw std::runtime_error("Can't find filter '" + filterName + "'");
    auto argsError = m_filter->GetArgumentsError();
    if (!argsError.empty())
        m_argsError = filterName + "() " + argsError;
}

InternalValue ExpressionFilter::Evaluate(const InternalValue& baseVal, RenderContext& context)
{
    if (!m_argsError.empty())
        throw std::runtime_error(m_argsError);
    if (m_parentFilter)
        return m_filter->Filter(m_parentFilter->Evaluate(baseVal, context), context);

    return m_filter->Filter(baseVal, context);
}

IsExpression::IsExpression(ExpressionEvaluatorPtr<> value, const std::string& tester, CallParamsInfo params, InternalValue registered)
    : m_value(value)
{
    if (GetIf<Callable>(&registered))
        m_tester = std::make_shared<testers::UserDefinedTester>(tester, std::move(params), std::move(registered));
    else
        m_tester = CreateTester(tester, std::move(params));
    if (!m_tester)
        throw std::runtime_error("Can't find tester '" + tester + "'");
}

InternalValue IsExpression::Evaluate(RenderContext& context)
{
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
    ValuesMap result;
    for (auto& i : m_items)
    {
        result[i.first] = i.second->Evaluate(context);
    }

    return result;
}*/

bool CallExpression::TryCallMethod(RenderContext& values, InternalValue& result, InternalValue& callee)
{
    auto* subscript = dynamic_cast<SubscriptExpression*>(m_valueRef.get());
    const std::string* name = subscript != nullptr ? subscript->GetCallName() : nullptr;
    if (name == nullptr)
    {
        callee = m_valueRef->Evaluate(values);
        return false;
    }

    const bool mayMutate = methods::IsMutatingName(*name);
    auto receiver = subscript->EvaluateReceiver(values, mayMutate);
    CheckUndefinedUse(receiver, UndefinedUse::Attribute);
    auto* method = methods::FindMethod(receiver, *name);
    if (method != nullptr)
    {
        // A host object's own key comes before a dict method (MapAttrPolicy::KeysFirst)
        auto* map = GetIf<MapAdapter>(&receiver);
        if (map != nullptr && map->GetAttrPolicy() == MapAttrPolicy::KeysFirst && map->HasValue(*name))
            method = nullptr;
    }

    if (method == nullptr)
    {
        callee = Subscript(receiver, *name, &values);
        if (receiver.ShouldExtendLifetime())
            callee.SetParentData(receiver);
        // Python raises AttributeError; calling the missing attribute of a map, None or a
        // chainable undefined is an UndefinedError
        if (callee.IsUndefined() && !IsEmpty(receiver) && GetIf<MapAdapter>(&receiver) == nullptr)
            methods::ThrowNoAttribute(receiver, *name);
        if (callee.IsUndefined() && GetUndefinedInfo(callee) == nullptr)
            callee = MakeUndefined(&values, receiver, InternalValue(*name));
        return false;
    }

    if (method->isMutating)
        receiver = methods::MakeMutable(receiver);
    auto callParams = helpers::EvaluateCallParams(m_params, values);
    result = method->invoke(receiver, callParams, values);
    return true;
}

InternalValue CallExpression::CallWithCallee(RenderContext& values, InternalValue fnVal)
{
    if (ConvertToInt(fnVal, InvalidFn) == LoopCycleFn)
        return CallLoopCycle(values);
    return CallArbitraryFn(values, std::move(fnVal));
}

InternalValue CallExpression::Evaluate(RenderContext& values)
{
    InternalValue result;
    InternalValue fnVal;
    if (TryCallMethod(values, result, fnVal))
        return result;
    return CallWithCallee(values, std::move(fnVal));
}

void CallExpression::Render(OutStream& stream, RenderContext& values)
{
    InternalValue result;
    InternalValue fnVal;
    if (TryCallMethod(values, result, fnVal))
    {
        stream.WriteValue(OutputValue(std::move(result), values));
        return;
    }
    const Callable* callable = GetIf<Callable>(&fnVal);
    if (callable == nullptr)
    {
        auto callOperator = Subscript(fnVal, std::string("operator()"), &values);
        if (GetIf<Callable>(&callOperator) == nullptr)
        {
            stream.WriteValue(OutputValue(CallWithCallee(values, std::move(fnVal)), values));
            return;
        }
        fnVal = std::move(callOperator);
        callable = GetIf<Callable>(&fnVal);
    }

    auto callParams = helpers::EvaluateCallParams(m_params, values);

    if (callable->GetType() == Callable::Type::Expression)
    {
        stream.WriteValue(OutputValue(callable->GetExpressionCallable()(callParams, values), values));
    }
    else
    {
        callable->GetStatementCallable()(callParams, stream, values);
    }
}

InternalValue CallExpression::CallArbitraryFn(RenderContext& values, InternalValue fnVal)
{
    Callable* callable = GetIf<Callable>(&fnVal);
    if (callable == nullptr)
    {
        auto callOperator = Subscript(fnVal, std::string("operator()"), nullptr);
        callable = GetIf<Callable>(&callOperator);
        if (callable == nullptr)
        {
            // Calling a named undefined is an UndefinedError; any other value is not callable
            CheckUndefinedUse(fnVal, UndefinedUse::Call);
            if (fnVal.IsUndefined())
                return InternalValue();
            throw std::runtime_error(std::string("'") + Apply<visitors::PythonTypeNameGetter>(fnVal) + "' object is not callable");
        }
        fnVal = std::move(callOperator);
        callable = GetIf<Callable>(&fnVal);
    }

    auto kind = callable->GetKind();
    if (kind != Callable::GlobalFunc && kind != Callable::UserCallable && kind != Callable::Macro)
        return InternalValue();

    auto callParams = helpers::EvaluateCallParams(m_params, values);

    if (callable->GetType() == Callable::Type::Expression)
    {
        return callable->GetExpressionCallable()(callParams, values);
    }

    TargetString resultStr;
    auto stream = values.GetRendererCallback()->GetStreamOnString(resultStr);
    callable->GetStatementCallable()(callParams, stream, values);
    // A macro returns Markup when autoescape is on where it is called
    InternalValue result(std::move(resultStr));
    result.SetMarkup(values.IsAutoescape());
    return result;
}

InternalValue CallExpression::CallLoopCycle(RenderContext& values)
{
    bool loopFound = false;
    auto loopValP = values.FindValue("loop", loopFound);
    if (!loopFound)
        return InternalValue();

    if (m_params.posParams.empty())
        throw std::runtime_error("loop.cycle() expects at least one positional argument");
    auto loop = GetIf<MapAdapter>(&loopValP->second);
    int64_t baseIdx = Apply<visitors::IntegerEvaluator>(loop->GetValueByName("index0"));
    auto idx = static_cast<size_t>(baseIdx % m_params.posParams.size());
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

enum ParamState
{
    UnknownPos,
    UnknownKw,
    MappedPos,
    MappedKw,
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

template<typename Result, typename T, typename P>
Result ParseCallParamsImpl(const T& args, const P& params, bool& isSucceeded)
{
    struct ArgInfo
    {
        ArgState state = NotFound;
        int prevNotFound = -1;
        int nextNotFound = -1;
        const ArgumentInfo* info = nullptr;
    };

    boost::container::small_vector<ArgInfo, 8> argsInfo(args.size());
    boost::container::small_vector<ParamState, 8> posParamsInfo(params.posParams.size());

    isSucceeded = true;

    Result result;

    int argIdx = 0;
    int firstMandatoryIdx = -1;
    int prevNotFound = -1;
    int foundKwArgs = 0;
    (void)foundKwArgs; // extremely odd bug in clang warning
                       // Wunused-but-set-variable

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
            ++foundKwArgs;
        }
        else
        {
            if (argInfo.mandatory)
            {
                argsInfo[argIdx].state = NotFoundMandatory;
                if (firstMandatoryIdx == -1)
                    firstMandatoryIdx = argIdx;
            }
            else
            {
                argsInfo[argIdx].state = NotFound;
            }


            if (prevNotFound != -1)
                argsInfo[prevNotFound].nextNotFound = argIdx;

            argsInfo[argIdx].prevNotFound = prevNotFound;
            prevNotFound = argIdx;
        }


        ++argIdx;
    }

    std::size_t startPosArg = firstMandatoryIdx == -1 ? 0 : firstMandatoryIdx;
    std::size_t curPosArg = startPosArg;
    std::size_t eatenPosArgs = 0;

    // Determine the range for positional arguments scanning
    bool isFirstTime = true;
    for (; eatenPosArgs < posParamsInfo.size() && startPosArg < args.size(); eatenPosArgs = eatenPosArgs + (argsInfo[startPosArg].state == Ignored ? 0 : 1))
    {
        if (isFirstTime)
        {
            for (; startPosArg < args.size() && (argsInfo[startPosArg].state == Keyword || argsInfo[startPosArg].state == Positional); ++startPosArg)
                ;

            isFirstTime = false;
            if (startPosArg == args.size())
                break;
            continue;
        }

        prevNotFound = argsInfo[startPosArg].prevNotFound;
        if (prevNotFound != -1)
        {
            startPosArg = static_cast<std::size_t>(prevNotFound);
        }
        else if (curPosArg == args.size())
        {
            break;
        }
        else
        {
            int nextPosArg = argsInfo[curPosArg].nextNotFound;
            if (nextPosArg == -1)
                break;
            curPosArg = static_cast<std::size_t>(nextPosArg);
        }
    }

    // Map positional params to the desired arguments
    auto curArg = static_cast<int>(startPosArg);
    for (std::size_t idx = 0; idx < eatenPosArgs && curArg != -1 && static_cast<size_t>(curArg) < argsInfo.size(); ++idx, curArg = argsInfo[curArg].nextNotFound)
    {
        if (argsInfo[curArg].state == Ignored)
            continue;

        result.args[argsInfo[curArg].info->name] = params.posParams[idx];
        argsInfo[curArg].state = Positional;
    }

    // Fill default arguments (if missing) and check for mandatory
    for (std::size_t idx = 0; idx < argsInfo.size(); ++idx)
    {
        auto& argInfo = argsInfo[idx];
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
#if __cplusplus >= 201703L
                if constexpr (std::is_same<Result, ParsedArgumentsInfo>::value)
                    result.args[argInfo.info->name] = std::make_shared<ConstantExpression>(argInfo.info->defaultVal);
                else
                    result.args[argInfo.info->name] = argInfo.info->defaultVal;
#else
                result.args[argInfo.info->name] = ParsedArgumentDefaultValGetter<Result>::Get(argInfo.info->defaultVal);
#endif
            }
            break;
        }
        case NotFoundMandatory:
            isSucceeded = false;
            break;
        }
    }

    // Fill the extra positional and kw-args
    for (auto& kw : params.kwParams)
    {
        if (result.args.find(kw.first) != result.args.end())
            continue;

        result.extraKwArgs[kw.first] = kw.second;
    }

    for (auto idx = eatenPosArgs; idx < params.posParams.size(); ++idx)
        result.extraPosArgs.push_back(params.posParams[idx]);


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

    for (auto& p : info.posParams)
        result.posParams.push_back(p->Evaluate(context));

    for (auto& kw : info.kwParams)
        result.kwParams[kw.first] = kw.second->Evaluate(context);

    return result;
}

} // namespace helpers
} // namespace jinja2
