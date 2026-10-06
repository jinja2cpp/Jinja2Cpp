#ifndef JINJA2CPP_SRC_FUNCTION_BASE_H
#define JINJA2CPP_SRC_FUNCTION_BASE_H

#include "expression_evaluator.h"
#include "internal_value.h"

#include <algorithm>
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
// The parameters of a filter or tester kind, built once (a function-local static): an instance
// keeps a pointer to its table, so the names and defaults are stored once per kind
using ArgumentsTable = std::vector<ArgumentInfo>;

inline ArgumentsTable MakeArgumentsTable(std::initializer_list<ArgumentInfo> args)
{
    return ArgumentsTable(args);
}

// For a filter or tester that takes no parameters
inline const ArgumentsTable& NoArguments()
{
    static const ArgumentsTable table;
    return table;
}

// What a filter or tester call binds at Load: one node per declared parameter, in the order of
// its table, null where the call passes none. A parameter left unbound reads the table's default,
// so no default is stored, or made into a node, per instance
class BoundArguments
{
public:
    BoundArguments() = default;
    BoundArguments(const ArgumentsTable& table, std::vector<ExpressionEvaluatorPtr<>> exprs)
        : m_table(&table)
    {
        if (std::any_of(exprs.begin(), exprs.end(), [](const auto& expr) { return expr != nullptr; }))
        {
            m_exprs = std::make_unique<ExpressionEvaluatorPtr<>[]>(exprs.size());
            std::move(exprs.begin(), exprs.end(), m_exprs.get());
        }
    }

    // The value of a declared parameter: its argument, else its default, else defVal
    InternalValue Evaluate(std::string_view name, RenderContext& context, InternalValue defVal) const
    {
        if (!m_table)
        {
            return defVal;
        }
        for (std::size_t idx = 0; idx < m_table->size(); ++idx)
        {
            const auto& info = (*m_table)[idx];
            if (info.name != name)
            {
                continue;
            }
            if (m_exprs && m_exprs[idx])
            {
                return m_exprs[idx]->Evaluate(context);
            }
            return IsEmpty(info.defaultVal) ? std::move(defVal) : info.defaultVal;
        }
        return defVal;
    }

    friend bool operator==(const BoundArguments& lhs, const BoundArguments& rhs)
    {
        if (lhs.m_table != rhs.m_table || !lhs.m_exprs != !rhs.m_exprs)
        {
            return false;
        }
        if (!lhs.m_exprs)
        {
            return true;
        }
        for (std::size_t idx = 0; idx < lhs.m_table->size(); ++idx)
        {
            if (!(lhs.m_exprs[idx] == rhs.m_exprs[idx]))
            {
                return false;
            }
        }
        return true;
    }
    friend bool operator!=(const BoundArguments& lhs, const BoundArguments& rhs) { return !(lhs == rhs); }

private:
    const ArgumentsTable* m_table = nullptr;
    std::unique_ptr<ExpressionEvaluatorPtr<>[]> m_exprs;
};

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
    const std::string& GetArgumentsError() const
    {
        static const std::string none;
        return m_argsError ? *m_argsError : none;
    }

protected:
    // Whether arguments beyond the declared ones are an error or are collected for the
    // implementation, as `map` and `select` pass them on
    enum class ExtraArgs
    {
        Reject,
        Accept
    };

    // argsInfo must outlive the instance (a function-local static). The arguments beyond the
    // declared ones go to extras when it is given
    bool ParseParams(const ArgumentsTable& argsInfo, const CallParamsInfo& params, ExtraArgs extraArgs = ExtraArgs::Reject, CallParamsInfo* extras = nullptr);
    InternalValue GetArgumentValue(std::string_view argName, RenderContext& context, InternalValue defVal = InternalValue()) const
    {
        return m_args.Evaluate(argName, context, std::move(defVal));
    }

    void SetArgumentsError(std::string message) { m_argsError = std::make_unique<std::string>(std::move(message)); }

    BoundArguments m_args;
    // Only a call that does not fit allocates its message
    std::unique_ptr<std::string> m_argsError;
};

inline bool FunctionBase::ParseParams(const ArgumentsTable& argsInfo, const CallParamsInfo& params, ExtraArgs extraArgs, CallParamsInfo* extras)
{
    bool result = true;
    auto parsed = helpers::ParseCallParamsInfo(argsInfo, params, result);

    m_argsError.reset();
    for (std::size_t idx = 0; idx < argsInfo.size(); ++idx)
    {
        if (argsInfo[idx].mandatory && !parsed.args[idx])
        {
            SetArgumentsError("missing required argument: '" + argsInfo[idx].name + "'");
            break;
        }
    }

    auto isDeclared = [&argsInfo](const char* name) {
        return std::any_of(argsInfo.begin(), argsInfo.end(), [name](auto& arg) { return arg.name == name; });
    };
    if (!m_argsError && extraArgs == ExtraArgs::Reject)
    {
        if (!parsed.extraPosArgs.empty() && !isDeclared("*args"))
        {
            SetArgumentsError("got " + std::to_string(parsed.extraPosArgs.size()) + " more positional argument(s) than it takes");
        }
        else if (!parsed.extraKwArgs.empty() && !isDeclared("**kwargs"))
        {
            SetArgumentsError("got an unexpected keyword argument '" + parsed.extraKwArgs.begin()->first + "'");
        }
    }

    if (extras)
    {
        extras->kwParams = std::move(parsed.extraKwArgs);
        extras->posParams = std::move(parsed.extraPosArgs);
    }
    m_args = BoundArguments(argsInfo, std::move(parsed.args));
    return result;
}

} // namespace jinja2

#endif // JINJA2CPP_SRC_FUNCTION_BASE_H
