#include "testers.h"
#include "value_visitors.h"

namespace jinja2
{
namespace testers
{

bool Defined::Test(const Value& baseVal, RenderContext& context)
{
    return boost::get<EmptyValue>(&baseVal.data()) == nullptr;
}

}
}
