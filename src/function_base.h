#ifndef FUNCTION_BASE_H
#define FUNCTION_BASE_H

#include "expression_evaluator.h"
#include "internal_value.h"

namespace jinja2
{
class FunctionBase
{
public:
protected:
    bool ParseParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParamsInfo& params);
    InternalValue GetArgumentValue(const std::string& argName, RenderContext& context, InternalValue defVal = InternalValue());

protected:
    ParsedArgumentsInfo m_args;
};

inline bool FunctionBase::ParseParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParamsInfo& params)
{
    bool result = true;
    m_args = helpers::ParseCallParamsInfo(argsInfo, params, result);

    return result;
}

inline InternalValue FunctionBase::GetArgumentValue(const std::string& argName, RenderContext& context, InternalValue defVal)
{
    auto argExpr = m_args[argName];
    return argExpr ? argExpr->Evaluate(context) : std::move(defVal);
}

} // jinja2

#endif // FUNCTION_BASE_H
