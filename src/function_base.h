#ifndef JINJA2CPP_SRC_FUNCTION_BASE_H
#define JINJA2CPP_SRC_FUNCTION_BASE_H

#include "expression_evaluator.h"
#include "internal_value.h"

#include <algorithm>
#include <initializer_list>
#include <string>
#include <utility>
#include <vector>

namespace jinja2
{
// The parameters of a filter or tester kind, built once (a function-local static) instead of
// on every construction: the defaults become constant nodes that every instance shares
using ArgumentsTable = std::vector<ArgumentInfo>;

inline ArgumentsTable MakeArgumentsTable(std::initializer_list<ArgumentInfo> args)
{
    ArgumentsTable result(args);
    for (auto& arg : result)
    {
        if (!IsEmpty(arg.defaultVal))
        {
            arg.defaultExpr = std::make_shared<ConstantExpression>(arg.defaultVal);
        }
    }
    return result;
}

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

    bool ParseParams(const ArgumentsTable& argsInfo, const CallParamsInfo& params, ExtraArgs extraArgs = ExtraArgs::Reject)
    {
        return ParseParamsImpl(argsInfo, params, extraArgs);
    }
    bool ParseParams(const std::initializer_list<ArgumentInfo>& argsInfo, const CallParamsInfo& params, ExtraArgs extraArgs = ExtraArgs::Reject)
    {
        return ParseParamsImpl(argsInfo, params, extraArgs);
    }
    InternalValue GetArgumentValue(const std::string& argName, RenderContext& context, InternalValue defVal = InternalValue());

    ParsedArgumentsInfo m_args;
    std::string m_argsError;

private:
    template<typename Args>
    bool ParseParamsImpl(const Args& argsInfo, const CallParamsInfo& params, ExtraArgs extraArgs);
};

//bool operator==(const FunctionBase& lhs, const FunctionBase& rhs)
//{
//    return
//}

template<typename Args>
bool FunctionBase::ParseParamsImpl(const Args& argsInfo, const CallParamsInfo& params, ExtraArgs extraArgs)
{
    bool result = true;
    m_args = helpers::ParseCallParamsInfo(argsInfo, params, result);

    m_argsError.clear();
    for (const auto& arg : argsInfo)
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
    {
        return result;
    }
    if (!m_args.extraPosArgs.empty() && !isDeclared("*args"))
    {
        m_argsError = "got " + std::to_string(m_args.extraPosArgs.size()) + " more positional argument(s) than it takes";
    }
    else if (!m_args.extraKwArgs.empty() && !isDeclared("**kwargs"))
    {
        m_argsError = "got an unexpected keyword argument '" + m_args.extraKwArgs.begin()->first + "'";
    }

    return result;
}

inline InternalValue FunctionBase::GetArgumentValue(const std::string& argName, RenderContext& context, InternalValue defVal)
{
    const auto& argExpr = m_args[argName];
    return argExpr ? argExpr->Evaluate(context) : std::move(defVal);
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_FUNCTION_BASE_H
