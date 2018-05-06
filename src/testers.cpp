#include "testers.h"
#include "value_visitors.h"

namespace jinja2
{
namespace testers
{

bool Defined::Test(const Value& baseVal, RenderContext& /*context*/)
{
    return boost::get<EmptyValue>(&baseVal.data()) == nullptr;
}

StartsWith::StartsWith(TesterParams params)
{
    bool parsed = true;
    auto args = helpers::ParseCallParams({{"str", true}}, params, parsed);
    m_stringEval = args["str"];
}

bool StartsWith::Test(const Value& baseVal, RenderContext& context)
{
    Value val = m_stringEval->Evaluate(context);
    std::string baseStr = baseVal.asString();
    std::string str = val.asString();
    return baseStr.find(str) == 0;
}

}
}
