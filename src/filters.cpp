#include "filters.h"
#include "value_visitors.h"

namespace jinja2
{
namespace filters
{

Join::Join(FilterParams params)
{
    if (!helpers::FindParam(params, "param0", "d", m_delimiterEval))
        m_delimiterEval = std::make_shared<ConstantExpression>("");

    helpers::FindParam(params, "param1", "attribute", m_attribute);
}

Value Join::Filter(const Value& baseVal, RenderContext& context)
{
    if (!baseVal.isList())
        return Value();

    auto& values = baseVal.asList();
    bool isFirst = true;
    Value result;
    Value delimiter = m_delimiterEval->Evaluate(context);
    for (const Value& val : values)
    {
        if (isFirst)
            isFirst = false;
        else
            result = boost::apply_visitor(visitors::StringJoiner(), result.data(), delimiter.data());

        result = boost::apply_visitor(visitors::StringJoiner(), result.data(), val.data());
    }

    return result;
}

} // filters
} // jinja2
