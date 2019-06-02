#include "expression_evaluator.h"
#include "filters.h"
#include "generic_adapters.h"
#include "internal_value.h"
#include "out_stream.h"
#include "testers.h"
#include "value_visitors.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/container/small_vector.hpp>

#include <cmath>
#include <stack>

namespace jinja2
{

void ExpressionEvaluatorBase::Render(OutStream& stream, RenderContext& values)
{
    auto val = Evaluate(values);
    stream.WriteValue(val);
}


InternalValue FullExpressionEvaluator::Evaluate(RenderContext& values)
{
    if (!m_expression)
        return InternalValue();

    InternalValue origVal = m_expression->Evaluate(values);
    if (m_filter)
        origVal = m_filter->Evaluate(origVal, values);

    if (m_tester && !m_tester->Evaluate(values))
        return m_tester->EvaluateAltValue(values);

    return origVal;
}

void FullExpressionEvaluator::Render(OutStream& stream, RenderContext& values)
{
    if (!m_filter && !m_tester)
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

    return InternalValue();
}

InternalValue SubscriptExpression::Evaluate(RenderContext& values)
{
    InternalValue cur = m_value->Evaluate(values);

    for (auto idx : m_subscriptExprs)
    {
        auto subscript = idx->Evaluate(values);
        auto newVal = Subscript(cur, subscript);
        if (cur.ShouldExtendLifetime())
            newVal.SetParentData(cur);
        std::swap(newVal, cur);
    }

    return cur;
}

InternalValue UnaryExpression::Evaluate(RenderContext& values)
{
    return Apply<visitors::UnaryOperation>(m_expr->Evaluate(values), m_oper);
}

BinaryExpression::BinaryExpression(BinaryExpression::Operation oper, ExpressionEvaluatorPtr<> leftExpr, ExpressionEvaluatorPtr<> rightExpr)
    : m_oper(oper)
    , m_leftExpr(leftExpr)
    , m_rightExpr(rightExpr)
{
    if (m_oper == In)
    {
        CallParams params;
        params.kwParams["seq"] = rightExpr;
        m_inTester = CreateTester("in", params);
    }
}

InternalValue BinaryExpression::Evaluate(RenderContext& context)
{
    InternalValue leftVal = m_leftExpr->Evaluate(context);
    InternalValue rightVal = m_oper == In ? InternalValue() : m_rightExpr->Evaluate(context);
    InternalValue result;

    switch (m_oper)
    {
    case jinja2::BinaryExpression::LogicalAnd:
    {
        bool left = ConvertToBool(leftVal);
        if (left)
            left = ConvertToBool(rightVal);
        result = static_cast<bool>(left);
        break;
    }
    case jinja2::BinaryExpression::LogicalOr:
    {
        bool left = ConvertToBool(leftVal);
        if (!left)
            left = ConvertToBool(rightVal);
        result = static_cast<bool>(left);
        break;
    }
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
    case jinja2::BinaryExpression::DivReminder:
    case jinja2::BinaryExpression::DivInteger:
    case jinja2::BinaryExpression::Pow:
        result = Apply2<visitors::BinaryMathOperation>(leftVal, rightVal, m_oper);
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

InternalValue TupleCreator::Evaluate(RenderContext& context)
{
    InternalValueList result;
    for (auto& e : m_exprs)
    {
        result.push_back(e->Evaluate(context));
    }

    return ListAdapter::CreateAdapter(std::move(result));
}

InternalValue DictCreator::Evaluate(RenderContext& context)
{
    InternalValueMap result;
    for (auto& e : m_exprs)
    {
        result[e.first] = e.second->Evaluate(context);
    }

    return MapAdapter::CreateAdapter(std::move(result));;
}

ExpressionFilter::ExpressionFilter(const std::string& filterName, CallParams params)
{
    m_filter = CreateFilter(filterName, std::move(params));
    if (!m_filter)
        throw std::runtime_error("Can't find filter '" + filterName + "'");
}

InternalValue ExpressionFilter::Evaluate(const InternalValue& baseVal, RenderContext& context)
{
    if (m_parentFilter)
        return m_filter->Filter(m_parentFilter->Evaluate(baseVal, context), context);

    return m_filter->Filter(baseVal, context);
}

IsExpression::IsExpression(ExpressionEvaluatorPtr<> value, const std::string& tester, CallParams params)
    : m_value(value)
{
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

InternalValue CallExpression::Evaluate(RenderContext& values)
{
    enum
    {
        InvalidFn = -1,
        RangeFn = 1,
        LoopCycleFn = 2
    };

    auto& scope = values.EnterScope();
    scope["range"] = InternalValue(static_cast<int64_t>(RangeFn));
    scope["loop"] = MapAdapter::CreateAdapter(InternalValueMap{{"cycle", InternalValue(static_cast<int64_t>(LoopCycleFn))}});
    auto fn = m_valueRef->Evaluate(values);
    values.ExitScope();

    auto fnId = ConvertToInt(fn, InvalidFn);


    switch (fnId)
    {
    case RangeFn:
        return CallGlobalRange(values);
    case LoopCycleFn:
        return CallLoopCycle(values);
    default:
        return CallArbitraryFn(values);
    }

    return InternalValue();
}

void CallExpression::Render(OutStream& stream, RenderContext& values)
{
    auto fnVal = m_valueRef->Evaluate(values);
    const Callable* callable = GetIf<Callable>(&fnVal);
    if (callable == nullptr)
    {
        fnVal = Subscript(fnVal, std::string("operator()"));
        callable = GetIf<Callable>(&fnVal);
        if (callable == nullptr)
        {
            Expression::Render(stream, values);
            return;
        }
    }

    if (callable->GetType() == Callable::Type::Expression)
    {
        stream.WriteValue(callable->GetExpressionCallable()(m_params, values));
    }
    else
    {
        callable->GetStatementCallable()(m_params, stream, values);
    }
}

InternalValue CallExpression::CallArbitraryFn(RenderContext& values)
{
    auto fnVal = m_valueRef->Evaluate(values);
    Callable* callable = GetIf<Callable>(&fnVal);
    if (callable == nullptr)
    {
        fnVal = Subscript(fnVal, std::string("operator()"));
        callable = GetIf<Callable>(&fnVal);
        if (callable == nullptr)
            return InternalValue();
    }

    auto kind = callable->GetKind();
    if (kind != Callable::GlobalFunc && kind != Callable::UserCallable)
        return InternalValue();

    if (callable->GetType() == Callable::Type::Expression)
    {
        return callable->GetExpressionCallable()(m_params, values);
    }

    TargetString resultStr;
    auto stream = values.GetRendererCallback()->GetStreamOnString(resultStr);
    callable->GetStatementCallable()(m_params, stream, values);
    return resultStr;
}

InternalValue CallExpression::CallGlobalRange(RenderContext& values)
{
    bool isArgsParsed = true;

    auto args = helpers::ParseCallParams({{"start"}, {"stop", true}, {"step"}}, m_params, isArgsParsed);
    if (!isArgsParsed)
        return InternalValue();


    auto startExpr = args["start"];
    auto stopExpr = args["stop"];
    auto stepExpr = args["step"];

    InternalValue startVal = startExpr ? startExpr->Evaluate(values) : InternalValue();
    InternalValue stopVal = stopExpr ? stopExpr->Evaluate(values) : InternalValue();
    InternalValue stepVal = stepExpr ? stepExpr->Evaluate(values) : InternalValue();

    int64_t start = Apply<visitors::IntegerEvaluator>(startVal);
    int64_t stop = Apply<visitors::IntegerEvaluator>(stopVal);
    int64_t step = Apply<visitors::IntegerEvaluator>(stepVal);

    if (!stepExpr)
    {
        step = 1;
    }
    else
    {
        if (step == 0)
            return InternalValue();
    }

    class RangeGenerator : public ListAccessorImpl<RangeGenerator>
    {
    public:
        RangeGenerator(int64_t start, int64_t stop, int64_t step)
            : m_start(start)
            , m_stop(stop)
            , m_step(step)
        {
        }

        size_t GetSize() const override
        {
            auto distance = m_stop - m_start;
            auto count = distance / m_step;
            return count < 0 ? 0 : static_cast<size_t>(count);
        }
        InternalValue GetItem(int64_t idx) const override
        {
            return m_start + m_step * idx;
        }

        bool ShouldExtendLifetime() const override {return false;}
        GenericList CreateGenericList() const override
        {
            return GenericList([accessor = *this]() -> const ListItemAccessor* {return &accessor;});
        }


    private:
        int64_t m_start;
        int64_t m_stop;
        int64_t m_step;
    };

    return ListAdapter([accessor = RangeGenerator(start, stop, step)]() -> const IListAccessor* {return &accessor;});
}

InternalValue CallExpression::CallLoopCycle(RenderContext& values)
{
    bool loopFound = false;
    auto loopValP = values.FindValue("loop", loopFound);
    if (!loopFound)
        return InternalValue();

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

template<typename T>
ParsedArguments ParseCallParamsImpl(const T& args, const CallParams& params, bool& isSucceeded)
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

    ParsedArguments result;

    int argIdx = 0;
    int firstMandatoryIdx = -1;
    int prevNotFound = -1;
    int foundKwArgs = 0;

    // Find all provided keyword args
    for (auto& argInfo : args)
    {
        argsInfo[argIdx].info = &argInfo;

        if (argInfo.name == "*args" || argInfo.name=="**kwargs")
        {
            argsInfo[argIdx ++].state = Ignored;
            continue;
        }

        auto p = params.kwParams.find(argInfo.name);
        if (p != params.kwParams.end())
        {
            result.args[argInfo.name] = p->second;
            argsInfo[argIdx].state = Keyword;
            ++ foundKwArgs;
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


        ++ argIdx;
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
            for (; startPosArg < args.size() && (argsInfo[startPosArg].state == Keyword || argsInfo[startPosArg].state == Positional); ++ startPosArg)
                ;

            isFirstTime = false;
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
    for (std::size_t idx = 0; idx < eatenPosArgs && curArg != -1 && static_cast<size_t>(curArg) < argsInfo.size(); ++ idx, curArg = argsInfo[curArg].nextNotFound)
    {
        if (argsInfo[curArg].state == Ignored)
            continue;
            
        result.args[argsInfo[curArg].info->name] = params.posParams[idx];
        argsInfo[curArg].state = Positional;
    }

    // Fill default arguments (if missing) and check for mandatory
    for (std::size_t idx = 0; idx < argsInfo.size(); ++ idx)
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
                result.args[argInfo.info->name] = std::make_shared<ConstantExpression>(argInfo.info->defaultVal);
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

    for (auto idx = eatenPosArgs; idx < params.posParams.size(); ++ idx)
        result.extraPosArgs.push_back(params.posParams[idx]);


    return result;
}

ParsedArguments ParseCallParams(const std::initializer_list<ArgumentInfo>& args, const CallParams& params, bool& isSucceeded)
{
    return ParseCallParamsImpl(args, params, isSucceeded);
}

ParsedArguments ParseCallParams(const std::vector<ArgumentInfo>& args, const CallParams& params, bool& isSucceeded)
{
    return ParseCallParamsImpl(args, params, isSucceeded);
}

}
}
