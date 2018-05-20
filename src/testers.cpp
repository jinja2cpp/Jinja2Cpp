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
};

std::unordered_map<std::string, IsExpression::TesterFactoryFn> s_testers = {
    {"defined", &TesterFactory<testers::Defined>::Create},
    {"startsWith", &TesterFactory<testers::StartsWith>::Create},
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

bool Defined::Test(const InternalValue& baseVal, RenderContext& /*context*/)
{
    return boost::get<EmptyValue>(&baseVal) == nullptr;
}

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

}
}
