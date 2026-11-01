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
// applymacro, format, a filter of the environment): the expressions of a call bound at Load, or
// the values of a call that unpacks. The values are behind a pointer, so a call bound at Load
// pays one null check for them
class ForwardedArgs
{
public:
    ForwardedArgs() = default;
    explicit ForwardedArgs(CallParamsInfo exprs)
        : m_exprs(std::move(exprs))
    {
    }
    explicit ForwardedArgs(CallParams values)
        : m_values(std::make_unique<CallParams>(std::move(values)))
    {
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator FunctionArgs() const { return m_values ? FunctionArgs(*m_values) : FunctionArgs(m_exprs); }
    [[nodiscard]] bool HasPositional() const { return m_values ? !m_values->posParams.empty() : !m_exprs.posParams.empty(); }
    [[nodiscard]] bool HasKeywords() const { return m_values ? !m_values->kwParams.empty() : !m_exprs.kwParams.empty(); }
    // Makes them expressions, to fill
    CallParamsInfo& EmplaceExprs()
    {
        m_values.reset();
        m_exprs = CallParamsInfo();
        return m_exprs;
    }
    [[nodiscard]] CallParams Evaluate(RenderContext& context) const
    {
        if (m_values)
        {
            return *m_values;
        }
        return helpers::EvaluateCallParams(m_exprs, context);
    }

    void VisitRefs(detail::RefChecker& refs) const { VisitCallParams(refs, m_exprs); }

private:
    CallParamsInfo m_exprs;
    // Only a call that unpacks has them
    std::unique_ptr<CallParams> m_values;
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

    // The value of a declared parameter: its argument, else its default, else defVal. given,
    // when not null, tells whether the call passes the parameter, an undefined value included
    InternalValue Evaluate(std::string_view name, RenderContext& context, InternalValue defVal, bool* given = nullptr) const
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
                if (given)
                {
                    *given = true;
                }
                return context.Nodes()[m_exprs[idx]].Evaluate(context);
            }
            if (m_values && (*m_values)[idx])
            {
                if (given)
                {
                    *given = true;
                }
                return *(*m_values)[idx];
            }
            if (IsEmpty(info.defaultVal))
            {
                return defVal;
            }
            return info.defaultVal;
        }
        return defVal;
    }

    void VisitRefs(detail::RefChecker& refs) const
    {
        if (m_exprs)
        {
            refs.All(Span<const NodeRef<Expression>>(m_exprs.get(), m_table->size()));
        }
    }

private:
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
    // The value of an optional parameter, and in given whether the call passes it: Python passes
    // an undefined argument as the Undefined object, which a parameter left out is not
    // (docs/tasks/0310). Read in the same lookup as the value
    // Out of line, so as not to grow the render paths that call it
    InternalValue GetArgumentValue(std::string_view argName, RenderContext& context, bool& given) const;
    // Whether an optional argument read as `value` is set, as Python's `is not None` tells: None
    // and a parameter left out are not, an undefined value passed is
    [[nodiscard]] static bool IsArgumentSet(const InternalValue& value, bool given) { return !value.IsNone() && (!value.IsUndefined() || given); }

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

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_FUNCTION_BASE_H
