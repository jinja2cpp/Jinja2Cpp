#include "function_base.h"

#include "expression_evaluator.h"
#include "internal_value.h"

#include <algorithm>
#include <cstddef>
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
// Out of line: every filter and test binds its call through ParseParams at Load. Inlined into
// each file that makes one, it grew those files enough to change GCC's inlining of their render
// paths (docs/tasks/0306)
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

bool FunctionBase::ParseParams(const ArgumentsTable& argsInfo, FunctionArgs params, ExtraArgs extraArgs, ForwardedArgs* extras)
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
                auto& forwarded = extras->EmplaceExprs();
                forwarded.kwParams = std::move(parsed.extraKwArgs);
                forwarded.posParams = std::move(parsed.extraPosArgs);
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

InternalValue FunctionBase::GetArgumentValue(std::string_view argName, RenderContext& context, bool& given) const
{
    given = false;
    return m_args.Evaluate(argName, context, InternalValue(), &given);
}

} // namespace v2
} // namespace jinja2
