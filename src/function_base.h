#ifndef JINJA2CPP_SRC_FUNCTION_BASE_H
#define JINJA2CPP_SRC_FUNCTION_BASE_H

#include "expression_evaluator.h"
#include "internal_value.h"
#include "node_arena.h"
#include "span.h"

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2
{
inline namespace v2
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

// The arguments a filter or test is made with: the expressions of its call, bound at Load, or
// the values of a call that unpacks `*args` or `**kwargs`, bound each time the call is evaluated
// (docs/tasks/0306). A view of arguments that outlive it
class FunctionArgs
{
public:
    FunctionArgs(const CallParamsInfo& exprs) noexcept // NOLINT(google-explicit-constructor)
        : m_exprs(&exprs)
    {
    }
    FunctionArgs(const CallParams& values) noexcept // NOLINT(google-explicit-constructor)
        : m_values(&values)
    {
    }

    // Calls fn with the CallParamsInfo or the CallParams; both have posParams and kwParams
    template<typename Fn>
    decltype(auto) Visit(Fn&& fn) const
    {
        return m_exprs ? std::forward<Fn>(fn)(*m_exprs) : std::forward<Fn>(fn)(*m_values);
    }
    [[nodiscard]] std::size_t PosCount() const { return m_exprs ? m_exprs->posParams.size() : m_values->posParams.size(); }
    [[nodiscard]] std::size_t KwCount() const { return m_exprs ? m_exprs->kwParams.size() : m_values->kwParams.size(); }

private:
    const CallParamsInfo* m_exprs = nullptr;
    const CallParams* m_values = nullptr;
};

// The arguments a filter passes on to the filter, test or callable it calls (map, select,
// applymacro, format, a filter of the environment): expressions or values, as it was made with
class ForwardedArgs
{
public:
    ForwardedArgs() = default;
    explicit ForwardedArgs(CallParamsInfo exprs)
        : m_args(std::move(exprs))
    {
    }
    explicit ForwardedArgs(CallParams values)
        : m_args(std::move(values))
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator FunctionArgs() const
    {
        return std::visit([](const auto& args) { return FunctionArgs(args); }, m_args);
    }
    [[nodiscard]] bool HasPositional() const
    {
        return std::visit([](const auto& args) { return !args.posParams.empty(); }, m_args);
    }
    [[nodiscard]] bool HasKeywords() const
    {
        return std::visit([](const auto& args) { return !args.kwParams.empty(); }, m_args);
    }
    [[nodiscard]] CallParams Evaluate(RenderContext& context) const
    {
        if (const auto* exprs = std::get_if<CallParamsInfo>(&m_args))
        {
            return helpers::EvaluateCallParams(*exprs, context);
        }
        return std::get<CallParams>(m_args);
    }

    void VisitRefs(detail::RefChecker& refs) const
    {
        if (const auto* exprs = std::get_if<CallParamsInfo>(&m_args))
        {
            VisitCallParams(refs, *exprs);
        }
    }

private:
    std::variant<CallParamsInfo, CallParams> m_args;
};

// What a filter or tester call binds: one node per declared parameter, in the order of its
// table, null where the call passes none; or, for a call that unpacks, the values it passes. A
// parameter left unbound reads the table's default, so no default is stored, or made into a
// node, per instance
class BoundArguments
{
public:
    BoundArguments() = default;
    BoundArguments(const ArgumentsTable& table, std::vector<NodeRef<Expression>> exprs)
        : m_table(&table)
    {
        if (std::any_of(exprs.begin(), exprs.end(), [](const auto& expr) { return static_cast<bool>(expr); }))
        {
            m_exprs = std::make_unique<NodeRef<Expression>[]>(exprs.size());
            std::copy(exprs.begin(), exprs.end(), m_exprs.get());
        }
    }
    BoundArguments(const ArgumentsTable& table, std::vector<std::optional<InternalValue>> values)
        : m_table(&table)
        , m_values(std::make_unique<std::vector<std::optional<InternalValue>>>(std::move(values)))
    {
    }

    // The value of a declared parameter: its argument, else its default, else defVal
    InternalValue Evaluate(std::string_view name, RenderContext& context, InternalValue defVal) const
    {
        const auto idx = IndexOf(name);
        if (idx == NotDeclared)
        {
            return defVal;
        }
        if (m_exprs && m_exprs[idx])
        {
            return context.Nodes()[m_exprs[idx]].Evaluate(context);
        }
        if (m_values)
        {
            if (const auto& value = (*m_values)[idx])
            {
                return value.value();
            }
        }
        const auto& info = (*m_table)[idx];
        if (IsEmpty(info.defaultVal))
        {
            return defVal;
        }
        return info.defaultVal;
    }
    // Whether the call passes the declared parameter, an undefined value included
    [[nodiscard]] bool IsGiven(std::string_view name) const
    {
        const auto idx = IndexOf(name);
        return idx != NotDeclared && ((m_exprs && m_exprs[idx]) || (m_values && (*m_values)[idx]));
    }

    void VisitRefs(detail::RefChecker& refs) const
    {
        if (m_exprs)
        {
            refs.All(Span<const NodeRef<Expression>>(m_exprs.get(), m_table->size()));
        }
    }

private:
    static constexpr std::size_t NotDeclared = static_cast<std::size_t>(-1);
    [[nodiscard]] std::size_t IndexOf(std::string_view name) const
    {
        if (!m_table)
        {
            return NotDeclared;
        }
        for (std::size_t idx = 0; idx < m_table->size(); ++idx)
        {
            if ((*m_table)[idx].name == name)
            {
                return idx;
            }
        }
        return NotDeclared;
    }

    const ArgumentsTable* m_table = nullptr;
    std::unique_ptr<NodeRef<Expression>[]> m_exprs;
    // Only a call that unpacks has them
    std::unique_ptr<std::vector<std::optional<InternalValue>>> m_values;
};

class FunctionBase
{
public:
    void VisitRefs(detail::RefChecker& refs) const { m_args.VisitRefs(refs); }

    // Why the call does not fit the declared parameters, as Python's TypeError says it; empty if it fits
    [[nodiscard]] const std::string& GetArgumentsError() const
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
    bool ParseParams(const ArgumentsTable& argsInfo, FunctionArgs params, ExtraArgs extraArgs = ExtraArgs::Reject, ForwardedArgs* extras = nullptr);
    InternalValue GetArgumentValue(std::string_view argName, RenderContext& context, InternalValue defVal = InternalValue()) const
    {
        return m_args.Evaluate(argName, context, std::move(defVal));
    }
    // Whether the call passes the parameter: Python passes an undefined argument as the
    // Undefined object, which a parameter left out is not (docs/tasks/0310)
    [[nodiscard]] bool IsArgumentGiven(std::string_view argName) const { return m_args.IsGiven(argName); }
    // Whether an optional argument read as `value` is set, as Python's `is not None` tells: None
    // and a parameter left out are not, an undefined value passed is
    [[nodiscard]] bool IsArgumentSet(const InternalValue& value, std::string_view argName) const
    {
        return !value.IsNone() && (!value.IsUndefined() || IsArgumentGiven(argName));
    }

    void SetArgumentsError(std::string message) { m_argsError = std::make_unique<std::string>(std::move(message)); }

private:
    // The checks Python makes as it binds the call, once the parameters are bound
    // isBound(idx): whether the call binds the parameter argsInfo[idx]
    template<typename IsBound, typename Extras>
    void CheckBound(const ArgumentsTable& argsInfo, const IsBound& isBound, const ArgumentInfo* multipleValues, std::size_t extraPos, const Extras& extraKw, ExtraArgs extraArgs);

protected:
    BoundArguments m_args;
    // Only a call that does not fit allocates its message
    std::unique_ptr<std::string> m_argsError;
};

template<typename IsBound, typename Extras>
void FunctionBase::CheckBound(const ArgumentsTable& argsInfo, const IsBound& isBound, const ArgumentInfo* multipleValues, std::size_t extraPos, const Extras& extraKw, ExtraArgs extraArgs)
{
    m_argsError.reset();
    for (std::size_t idx = 0; idx < argsInfo.size(); ++idx)
    {
        if (argsInfo[idx].mandatory && !isBound(idx))
        {
            SetArgumentsError("missing required argument: '" + argsInfo[idx].name + "'");
            break;
        }
    }
    // Python reports it before a missing argument
    if (multipleValues)
    {
        SetArgumentsError("got multiple values for argument '" + multipleValues->name + "'");
    }

    auto isDeclared = [&argsInfo](const char* name) {
        return std::any_of(argsInfo.begin(), argsInfo.end(), [name](auto& arg) { return arg.name == name; });
    };
    if (!m_argsError && extraArgs == ExtraArgs::Reject)
    {
        if (extraPos != 0 && !isDeclared("*args"))
        {
            SetArgumentsError("got " + std::to_string(extraPos) + " more positional argument(s) than it takes");
        }
        else if (!extraKw.empty() && !isDeclared("**kwargs"))
        {
            SetArgumentsError("got an unexpected keyword argument '" + extraKw.begin()->first + "'");
        }
    }
}

inline bool FunctionBase::ParseParams(const ArgumentsTable& argsInfo, FunctionArgs params, ExtraArgs extraArgs, ForwardedArgs* extras)
{
    bool result = true;
    params.Visit([&](const auto& args) {
        using Args = std::decay_t<decltype(args)>;
        if constexpr (std::is_same_v<Args, CallParamsInfo>)
        {
            auto parsed = helpers::ParseCallParamsInfo(argsInfo, args, result);
            CheckBound(argsInfo, [&parsed](std::size_t idx) { return static_cast<bool>(parsed.args[idx]); }, parsed.multipleValues, parsed.extraPosArgs.size(), parsed.extraKwArgs, extraArgs);
            if (extras)
            {
                CallParamsInfo forwarded;
                forwarded.kwParams = std::move(parsed.extraKwArgs);
                forwarded.posParams = std::move(parsed.extraPosArgs);
                *extras = ForwardedArgs(std::move(forwarded));
            }
            m_args = BoundArguments(argsInfo, std::move(parsed.args));
        }
        else
        {
            // Bound without the defaults, so that what the call passes can be told from them
            std::vector<ArgumentInfo> declared(argsInfo.begin(), argsInfo.end());
            for (auto& info : declared)
            {
                info.defaultVal = InternalValue();
            }
            auto parsed = helpers::ParseCallParams(declared, args, result);
            std::vector<std::optional<InternalValue>> values(argsInfo.size());
            for (std::size_t idx = 0; idx < argsInfo.size(); ++idx)
            {
                auto arg = parsed.args.find(argsInfo[idx].name);
                if (arg != parsed.args.end())
                {
                    values[idx] = std::move(arg->second);
                }
            }
            const auto* multipleValues = parsed.multipleValues ? &argsInfo[static_cast<std::size_t>(parsed.multipleValues - declared.data())] : nullptr;
            CheckBound(argsInfo, [&values](std::size_t idx) { return values[idx].has_value(); }, multipleValues, parsed.extraPosArgs.size(), parsed.extraKwArgs, extraArgs);
            if (extras)
            {
                CallParams forwarded;
                forwarded.kwParams = std::move(parsed.extraKwArgs);
                forwarded.posParams = std::move(parsed.extraPosArgs);
                *extras = ForwardedArgs(std::move(forwarded));
            }
            m_args = BoundArguments(argsInfo, std::move(values));
        }
    });
    return result;
}

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_FUNCTION_BASE_H
