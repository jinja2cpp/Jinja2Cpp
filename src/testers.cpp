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
    helpers::FindParam(params, 0, "", m_stringEval);
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
