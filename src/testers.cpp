#include "testers.h"
#include "value_visitors.h"

namespace jinja2
{
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
