#include "expression_evaluator.h"
#include "filters.h"
#include "testers.h"
#include "value_visitors.h"

#include <boost/algorithm/string/join.hpp>
#include <boost/container/small_vector.hpp>

#include <cmath>

namespace jinja2
{

std::unordered_map<std::string, ExpressionFilter::FilterFactoryFn> ExpressionFilter::s_filters = {
    {"attr", &FilterFactory<filters::Attribute>::Create},
    {"batch", FilterFactory<filters::Slice>::MakeCreator(filters::Slice::BatchMode)},
    {"camelize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::CamelMode)},
    {"capitalize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::CapitalMode)},
    {"default", &FilterFactory<filters::Default>::Create},
    {"dictsort", &FilterFactory<filters::DictSort>::Create},
    {"escape", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::EscapeHtmlMode)},
    {"escapecpp", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::EscapeCppMode)},
    {"first", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::FirstItemMode)},
    {"float", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ToFloatMode)},
    {"format", FilterFactory<filters::StringFormat>::MakeCreator(filters::StringFormat::PythonMode)},
    {"groupby", &FilterFactory<filters::GroupBy>::Create},
    {"int", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ToIntMode)},
    {"join", &FilterFactory<filters::Join>::Create},
    {"last", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::LastItemMode)},
    {"length", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::LengthMode)},
    {"list", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::ToListMode)},
    {"map", &FilterFactory<filters::Map>::Create},
    {"max", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::MaxItemMode)},
    {"min", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::MinItemMode)},
    {"pprint", &FilterFactory<filters::PrettyPrint>::Create},
    {"random", &FilterFactory<filters::Random>::Create},
    {"reject", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::RejectMode)},
    {"rejectattr", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::RejectAttrMode)},
    {"replace", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::ReplaceMode)},
    {"round", FilterFactory<filters::ValueConverter>::MakeCreator(filters::ValueConverter::RoundMode)},
    {"reverse", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::ReverseMode)},
    {"select", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::SelectMode)},
    {"selectattr", FilterFactory<filters::Tester>::MakeCreator(filters::Tester::SelectAttrMode)},
    {"slice", FilterFactory<filters::Slice>::MakeCreator(filters::Slice::SliceMode)},
    {"sort", &FilterFactory<filters::Sort>::Create},
    {"sum", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::SumItemsMode)},
    {"title", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::TitleMode)},
    {"tojson", FilterFactory<filters::Serialize>::MakeCreator(filters::Serialize::JsonMode)},
    {"toxml", FilterFactory<filters::Serialize>::MakeCreator(filters::Serialize::XmlMode)},
    {"toyaml", FilterFactory<filters::Serialize>::MakeCreator(filters::Serialize::YamlMode)},
    {"trim", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::TrimMode)},
    {"truncate", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::TruncateMode)},
    {"unique", FilterFactory<filters::SequenceAccessor>::MakeCreator(filters::SequenceAccessor::UniqueItemsMode)},
    {"upper", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UpperMode)},
    {"wordcount", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::WordCountMode)},
    {"wordwrap", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::WordWrapMode)},
    {"underscorize", FilterFactory<filters::StringConverter>::MakeCreator(filters::StringConverter::UnderscoreMode)},};

std::unordered_map<std::string, IsExpression::TesterFactoryFn> IsExpression::s_testers = {
    {"defined", &TesterFactory<testers::Defined>::Create},
    {"startsWith", &TesterFactory<testers::StartsWith>::Create},
};

Value FullExpressionEvaluator::Evaluate(RenderContext& values)
{
    if (!m_expression)
        return Value();

    Value origVal = m_expression->Evaluate(values);
    if (m_filter)
        origVal = m_filter->Evaluate(origVal, values);

    if (m_tester && !m_tester->Evaluate(values))
        return m_tester->EvaluateAltValue(values);

    return origVal;
}

Value ValueRefExpression::Evaluate(RenderContext& values)
{
    bool found = false;
    auto p = values.FindValue(m_valueName, found);
    if (found)
        return p->second;

    return Value();
}

Value SubscriptExpression::Evaluate(RenderContext& values)
{
    return m_value->Evaluate(values).subscript(m_subscriptExpr->Evaluate(values));
}

Value UnaryExpression::Evaluate(RenderContext& values)
{
    return boost::apply_visitor(visitors::UnaryOperation(m_oper), m_expr->Evaluate(values).data());
}

Value BinaryExpression::Evaluate(RenderContext& context)
{
    Value leftVal = m_leftExpr->Evaluate(context);
    Value rightVal = m_rightExpr->Evaluate(context);
    Value result;

    switch (m_oper)
    {
    case jinja2::BinaryExpression::LogicalAnd:
    case jinja2::BinaryExpression::LogicalOr:
        break;
    case jinja2::BinaryExpression::LogicalEq:
    case jinja2::BinaryExpression::LogicalNe:
    case jinja2::BinaryExpression::LogicalGt:
    case jinja2::BinaryExpression::LogicalLt:
    case jinja2::BinaryExpression::LogicalGe:
    case jinja2::BinaryExpression::LogicalLe:
    case jinja2::BinaryExpression::In:
    case jinja2::BinaryExpression::Plus:
    case jinja2::BinaryExpression::Minus:
    case jinja2::BinaryExpression::Mul:
    case jinja2::BinaryExpression::Div:
    case jinja2::BinaryExpression::DivReminder:
    case jinja2::BinaryExpression::DivInteger:
    case jinja2::BinaryExpression::Pow:
        result = boost::apply_visitor(visitors::BinaryMathOperation(m_oper), leftVal.data(), rightVal.data());
        break;
    case jinja2::BinaryExpression::StringConcat:
    default:
        break;
    }
    return result;
}

Value TupleCreator::Evaluate(RenderContext& context)
{
    ValuesList result;
    for (auto& e : m_exprs)
    {
        result.push_back(e->Evaluate(context));
    }

    return Value(result);
}

Value DictCreator::Evaluate(RenderContext& context)
{
    ValuesMap result;
    for (auto& e : m_exprs)
    {
        result[e.first] = e.second->Evaluate(context);
    }

    return Value(result);
}

ExpressionFilter::ExpressionFilter(std::string filterName, CallParams params)
{
    auto p = s_filters.find(filterName);
    if (p == s_filters.end())
        throw std::runtime_error("Can't find filter '" + filterName + "'");

    m_filter = p->second(std::move(params));
}

Value ExpressionFilter::Evaluate(const Value& baseVal, RenderContext& context)
{
    if (m_parentFilter)
        return m_filter->Filter(m_parentFilter->Evaluate(baseVal, context), context);

    return m_filter->Filter(baseVal, context);
}

IsExpression::IsExpression(ExpressionEvaluatorPtr<> value, std::string tester, CallParams params)
    : m_value(value)
{
    auto p = s_testers.find(tester);
    if (p == s_testers.end())
        throw std::runtime_error("Can't find tester '" + tester + "'");

    m_tester = p->second(std::move(params));
}

Value IsExpression::Evaluate(RenderContext& context)
{
    return m_tester->Test(m_value->Evaluate(context), context);
}

bool IfExpression::Evaluate(RenderContext& context)
{
    return boost::apply_visitor(visitors::BooleanEvaluator(), m_testExpr->Evaluate(context).data());
}

Value IfExpression::EvaluateAltValue(RenderContext& context)
{
    return m_altValue ? m_altValue->Evaluate(context) : Value();
}

Value DictionaryCreator::Evaluate(RenderContext& context)
{
    ValuesMap result;
    for (auto& i : m_items)
    {
        result[i.first] = i.second->Evaluate(context);
    }

    return result;
}

Value CallExpression::Evaluate(RenderContext& values)
{
    std::string valueRef = boost::algorithm::join(m_valueRef, ".");

    if (valueRef == "range")
        return CallGlobalRange(values);
    else if (valueRef == "loop.cycle")
        return CallLoopCycle(values);

    return Value();
}

Value CallExpression::CallGlobalRange(RenderContext& values)
{
    bool isArgsParsed = true;

    auto args = helpers::ParseCallParams({{"start"}, {"stop", true}, {"step"}}, m_params, isArgsParsed);
    if (!isArgsParsed)
        return Value();


    auto startExpr = args["start"];
    auto stopExpr = args["stop"];
    auto stepExpr = args["step"];
#if 0
    helpers::FindParam(m_params, helpers::NoPosParam, "start", startExpr);
    helpers::FindParam(m_params, helpers::NoPosParam, "stop", stopExpr);
    helpers::FindParam(m_params, helpers::NoPosParam, "step", startExpr);

    switch (m_params.posParams.size())
    {
    case 1:
        if (startExpr && stopExpr && stepExpr)
            break;
        else if (startExpr && stopExpr)
            stepExpr = m_params.posParams[0];
        else if (startExpr && stepExpr)
            stopExpr = m_params.posParams[0];
        else if (stopExpr && stepExpr)
            startExpr = m_params.posParams[0];
        else if (startExpr)
            stopExpr = m_params.posParams[0];
        else if (stopExpr)
            startExpr = m_params.posParams[0];
        else if (stepExpr)
            stopExpr = m_params.posParams[0];
        else
            stopExpr = m_params.posParams[0];
        break;
    case 2:
        if (startExpr && stopExpr && stepExpr)
            break;
        else if (startExpr && stopExpr)
            break;
        else if (startExpr && stepExpr)
            break;
        else if (stopExpr && stepExpr)
            break;
        else if (startExpr)
        {
            stopExpr = m_params.posParams[0];
            stepExpr = m_params.posParams[1];
        }
        else if (stopExpr)
        {
            startExpr = m_params.posParams[0];
            stepExpr = m_params.posParams[1];
        }
        else if (stepExpr)
        {
            startExpr = m_params.posParams[0];
            stopExpr = m_params.posParams[1];
        }
        else
        {
            startExpr = m_params.posParams[0];
            stopExpr = m_params.posParams[1];
        }
        break;
    case 3:
        if (startExpr || stopExpr || stepExpr)
            break;
        else
        {
            startExpr = m_params.posParams[0];
            stopExpr = m_params.posParams[1];
            stepExpr = m_params.posParams[2];
        }
        break;
    }
#endif

    Value startVal = startExpr ? startExpr->Evaluate(values) : Value();
    Value stopVal = stopExpr ? stopExpr->Evaluate(values) : Value();
    Value stepVal = stepExpr ? stepExpr->Evaluate(values) : Value();

    int64_t start = boost::apply_visitor(visitors::IntegerEvaluator(), startVal.data());
    int64_t stop = boost::apply_visitor(visitors::IntegerEvaluator(), stopVal.data());
    int64_t step = boost::apply_visitor(visitors::IntegerEvaluator(), stepVal.data());

    if (!stepExpr)
    {
        step = 1;
    }
    else
    {
        if (step == 0)
            return Value();
    }

    class RangeGenerator : public ListItemAccessor
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
            size_t count = static_cast<size_t>(m_stop - m_start);
            return static_cast<size_t>(count / m_step);
        }
        Value GetValueByIndex(int64_t idx) const override
        {
            return m_start + m_step * idx;
        }

    private:
        int64_t m_start;
        int64_t m_stop;
        int64_t m_step;
    };

    return GenericList([accessor = RangeGenerator(start, stop, step)]() -> const ListItemAccessor* {return &accessor;});
}

Value CallExpression::CallLoopCycle(RenderContext& values)
{
    bool loopFound = false;
    auto loopValP = values.FindValue("loop", loopFound);
    if (!loopFound)
        return Value();

    const ValuesMap* loop = boost::get<ValuesMap>(&loopValP->second.data());
    int64_t baseIdx = boost::apply_visitor(visitors::IntegerEvaluator(), (*loop).at("index0").data());
    auto idx = static_cast<size_t>(baseIdx % m_params.posParams.size());
    return m_params.posParams[idx]->Evaluate(values);
}

ParsedArguments helpers::ParseCallParams(const std::initializer_list<ArgumentInfo>& args, const CallParams& params, bool& isSucceeded)
{
    enum ArgState
    {
        NotFound,
        NotFoundMandatory,
        Keyword,
        Positional
    };

    enum ParamState
    {
        UnknownPos,
        UnknownKw,
        MappedPos,
        MappedKw,
    };

    boost::container::small_vector<ArgState, 8> argsInfo(args.size());
    boost::container::small_vector<ParamState, 8> posParamsInfo(params.posParams.size());

    isSucceeded = false;

    ParsedArguments result;

    int argIdx = 0;
    int firstMandatoryIdx = -1;
    int foundKwArgs = 0;
    for (auto& argInfo : args)
    {
        auto p = params.kwParams.find(argInfo.name);
        if (p != params.kwParams.end())
        {
            result.args[argInfo.name] = p->second;
            argsInfo[argIdx] = Keyword;
            ++ foundKwArgs;
        }
        else if (argInfo.mandatory)
        {
            argsInfo[argIdx] = NotFoundMandatory;
            if (firstMandatoryIdx != -1)
                firstMandatoryIdx = argIdx;
        }
        else
            argsInfo[argIdx] = NotFound;

        ++ argIdx;
    }


    return result;
}



}
