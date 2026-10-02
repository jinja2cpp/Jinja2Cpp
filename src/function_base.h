#ifndef JINJA2CPP_SRC_FUNCTION_BASE_H
#define JINJA2CPP_SRC_FUNCTION_BASE_H

#include "expression_evaluator.h"
#include "internal_value.h"

#include <algorithm>
#include <string>

namespace jinja2
{
class FunctionBase
{
public:
    bool operator==(const FunctionBase& other) const
    {
        return m_args == other.m_args;
    }
    bool operator!=(const FunctionBase& other) const
    {
        return !(*this == other);
    }
    // Why the call does not fit the declared parameters, as Python's TypeError says it; empty if it fits
    const std::string& GetArgumentsError() const { return m_argsError; }

protected:
    // Whether arguments beyond the declared ones are an error or are collected for the
    // implementation (extraPosArgs, extraKwArgs), as `map` and `select` pass them on
    enum class ExtraArgs
    {
        Reject,
        Accept
    };

    bool ParseParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParamsInfo& params, ExtraArgs extraArgs = ExtraArgs::Reject);
    InternalValue GetArgumentValue(const std::string& argName, RenderContext& context, InternalValue defVal = InternalValue());

protected:
    ParsedArgumentsInfo m_args;
    std::string m_argsError;
};

//bool operator==(const FunctionBase& lhs, const FunctionBase& rhs)
//{
//    return
//}

inline bool FunctionBase::ParseParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParamsInfo& params, ExtraArgs extraArgs)
{
    bool result = true;
    m_args = helpers::ParseCallParamsInfo(argsInfo, params, result);

    m_argsError.clear();
    for (auto& arg : argsInfo)
    {
        if (arg.mandatory && !m_args[arg.name])
        {
            m_argsError = "missing required argument: '" + arg.name + "'";
            return result;
        }
    }

    auto isDeclared = [&argsInfo](const char* name) {
        return std::any_of(argsInfo.begin(), argsInfo.end(), [name](auto& arg) { return arg.name == name; });
    };
    if (extraArgs == ExtraArgs::Accept)
        return result;
    if (!m_args.extraPosArgs.empty() && !isDeclared("*args"))
        m_argsError = "got " + std::to_string(m_args.extraPosArgs.size()) + " more positional argument(s) than it takes";
    else if (!m_args.extraKwArgs.empty() && !isDeclared("**kwargs"))
        m_argsError = "got an unexpected keyword argument '" + m_args.extraKwArgs.begin()->first + "'";

    return result;
}

inline InternalValue FunctionBase::GetArgumentValue(const std::string& argName, RenderContext& context, InternalValue defVal)
{
    auto argExpr = m_args[argName];
    return argExpr ? argExpr->Evaluate(context) : std::move(defVal);
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_FUNCTION_BASE_H
