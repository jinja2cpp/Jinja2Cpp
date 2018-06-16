#include "testers.h"
#include "value_visitors.h"

namespace jinja2
{

template<typename F>
struct TesterFactory
{
    static TesterPtr Create(TesterParams params)
    {
        return std::make_shared<F>(std::move(params));
    }

    template<typename ... Args>
    static IsExpression::TesterFactoryFn MakeCreator(Args&& ... args)
    {
        return [args...](TesterParams params) {return std::make_shared<F>(std::move(params), args...);};
    }
};

std::unordered_map<std::string, IsExpression::TesterFactoryFn> s_testers = {
    {"defined", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsDefinedMode)},
    {"startsWith", &TesterFactory<testers::StartsWith>::Create},
    {"eq", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalEq)},
    {"==", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalEq)},
    {"equalto", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalEq)},
    {"even", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsEvenMode)},
    {"ge", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGe)},
    {">=", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGe)},
    {"gt", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGt)},
    {">", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGt)},
    {"greaterthan", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalGt)},
    {"in", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsInMode)},
    {"iterable", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsIterableMode)},
    {"le", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLe)},
    {"<=", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLe)},
    {"lower", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsLowerMode)},
    {"lt", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLt)},
    {"<", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLt)},
    {"lessthan", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalLt)},
    {"mapping", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsMappingMode)},
    {"ne", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalNe)},
    {"!=", TesterFactory<testers::Comparator>::MakeCreator(BinaryExpression::LogicalNe)},
    {"number", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsNumberMode)},
    {"odd", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsOddMode)},
    {"sequence", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsSequenceMode)},
    {"string", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsStringMode)},
    {"undefined", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsUndefinedMode)},
    {"upper", TesterFactory<testers::ValueTester>::MakeCreator(testers::ValueTester::IsUpperMode)},
};

TesterPtr CreateTester(std::string testerName, CallParams params)
{
    auto p = s_testers.find(testerName);
    if (p == s_testers.end())
        TesterPtr();

    return p->second(std::move(params));
}

namespace testers
{

Comparator::Comparator(TesterParams params, BinaryExpression::Operation op)
    : m_op(op)
{
    ParseParams({{"b", true}}, params);
}

bool Comparator::Test(const InternalValue& baseVal, RenderContext& context)
{
    auto b = GetArgumentValue("b", context);

    auto cmpRes = Apply2<visitors::BinaryMathOperation>(baseVal, b, m_op);
    return ConvertToBool(cmpRes);
}

#if 0
bool Defined::Test(const InternalValue& baseVal, RenderContext& /*context*/)
{
    return boost::get<EmptyValue>(&baseVal) == nullptr;
}
#endif

StartsWith::StartsWith(TesterParams params)
{
    bool parsed = true;
    auto args = helpers::ParseCallParams({{"str", true}}, params, parsed);
    m_stringEval = args["str"];
}

bool StartsWith::Test(const InternalValue& baseVal, RenderContext& context)
{
    InternalValue val = m_stringEval->Evaluate(context);
    std::string baseStr = AsString(baseVal);
    std::string str = AsString(val);
    return baseStr.find(str) == 0;
}

ValueTester::ValueTester(TesterParams params, ValueTester::Mode mode)
    : m_mode(mode)
{}

bool ValueTester::Test(const InternalValue& baseVal, RenderContext& context)
{
    return false;
}

}
}
