// Template::MakeModule: a template's exports for the host, as Python Jinja2's Template.module
// and make_module (docs/tasks/0284)
#include "cycle_collector.h"
#include "error_boundary.h"
#include "expression_evaluator.h"
#include "internal_value.h"
#include "make_unexpected.h"
#include "out_stream.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "render_failure.h"
#include "statements.h"
#include "template_impl.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{

namespace
{
// Copies a value of the module into one the host owns, so that nothing the host keeps refers
// to the module's context or is changed by a later macro call. Lists, dicts and namespaces are
// copied item by item; a container that contains itself is cut where it repeats. Other objects
// (a host's GenericMap, a cycler) keep their own state and are passed on as views
class ExportCopier
{
public:
    Value Copy(const InternalValue& value)
    {
        if (auto list = AsList(value))
        {
            return CopyList(*list);
        }
        if (auto map = AsMap(value))
        {
            if (map->GetMutableItems() || map->IsNamespace())
            {
                return CopyMap(*map, value);
            }
        }
        return IntValue2Value(value);
    }

private:
    // Whether `identity` is a container being copied: the copy reached it again
    bool Enter(const void* identity)
    {
        if (identity && std::find(m_path.begin(), m_path.end(), identity) != m_path.end())
        {
            return false;
        }
        m_path.push_back(identity);
        return true;
    }

    Value CopyList(const ListRef& list)
    {
        if (!Enter(list.GetIdentity()))
        {
            return Value();
        }
        // A lazy list (range, a filter's generator) is read out here, within the limits
        ValuesList items;
        list.ForEach([this, &items](InternalValue&& item) {
            CheckSequenceSize(items.size() + 1, "a list");
            items.push_back(Copy(item));
            return true;
        });
        m_path.pop_back();
        return Value(std::move(items));
    }

    Value CopyMap(const MapRef& map, const InternalValue& value)
    {
        auto entries = map.GetEntries();
        // A ValuesMap has string keys only: a dict with others is passed on as a view
        const bool stringKeys = std::all_of(entries.begin(), entries.end(), [](const KeyValuePair& entry) { return GetAsSameString(std::string(), entry.key).has_value(); });
        if (!stringKeys)
        {
            return IntValue2Value(value);
        }
        if (!Enter(map.GetIdentity()))
        {
            return Value();
        }
        ValuesMap items;
        for (const auto& entry : entries)
        {
            items[*GetAsSameString(std::string(), entry.key)] = Copy(entry.value);
        }
        m_path.pop_back();
        return Value(std::move(items));
    }

    // The containers being copied, outermost first
    std::vector<const void*> m_path;
};

// Whether `value` is still the object `imported` an import bound: Python's `is`, as `is sameas`
// tests it
bool IsSameImport(const InternalValue& imported, const InternalValue& value)
{
    if (const auto* callable = GetIf<Callable>(&imported))
    {
        const auto* other = GetIf<Callable>(&value);
        return other && callable->IsSame(*other);
    }
    if (auto map = AsMap(imported))
    {
        auto other = AsMap(value);
        return other && other->GetIdentity() == map->GetIdentity();
    }
    if (auto list = AsList(imported))
    {
        auto other = AsList(value);
        return other && other->GetIdentity() == list->GetIdentity();
    }
    // A scalar has no identity: an equal one counts as the same
    return !GetIf<Callable>(&value) && !AsMap(value) && !AsList(value) && IntValue2Value(imported) == IntValue2Value(value);
}

// Whether `value`, set under the name with hash `hash`, is only the undefined a frame binds at
// its entry to a name it sets later (RenderContext::IsEntryBinding): a name never set
bool IsEntryBinding(const InternalValue& value, std::size_t hash)
{
    const auto* undefined = GetIf<UndefinedValue>(&value);
    return undefined && undefined->atEntry && undefined->entryHash == static_cast<std::uint32_t>(hash);
}

// Calls `callable` on `context` as a call expression does (InvokeCallable in
// expression_evaluator.cpp): a macro's output is Markup when the module autoescapes
InternalValue InvokeInModule(RenderContext& context, const Callable& callable, const CallParams& params)
{
    if (callable.GetType() == Callable::Type::Expression)
    {
        return callable.GetExpressionCallable()(params, context);
    }
    InternalValue result(RenderToString(context.GetRendererCallback(), [&](OutStream& stream) { callable.GetStatementCallable()(params, stream, context); }));
    result.SetMarkup(context.IsAutoescape());
    return result;
}

// A host Value as an argument the call owns: copied and converted by value, so that a module
// value the macro stores it in (a namespace) never refers to the host's
InternalValue OwnedArgument(const Value& value)
{
    Value copy = value;
    return visit(visitors::InputValueConvertor(true, false), copy.GetData());
}
} // namespace

// The state the exports of one module share: the template and the module's context their calls
// run in, kept alive by every export (docs/tasks/0284, decision D1)
template<typename CharT>
struct TemplateModuleState
{
    using Impl = TemplateImpl<CharT>;

    // The render callback of the module, which also notes the names its top level imports:
    // those are not exported, as in Jinja2
    class Callback final : public Impl::RendererCallback
    {
    public:
        using Impl::RendererCallback::RendererCallback;

        void ImportBound(const RenderContext& values, std::string_view name, const InternalValue& value) override
        {
            // At the module's top level only: its own first scope, not a macro's or a loop's
            if (&values == moduleContext && values.GetScopesCount() == 1)
            {
                imported.emplace_back(std::string(name), value);
            }
        }

        const RenderContext* moduleContext = nullptr;
        // The names the top level imported, with the values imported
        std::vector<std::pair<std::string, InternalValue>> imported;
    };

    TemplateModuleState(std::shared_ptr<const Impl> tpl, const ValuesMap& params)
        : impl(std::move(tpl))
        , generation(impl->m_loadGeneration.load(std::memory_order_relaxed))
        , paramSource(params)
    {
    }
    TemplateModuleState(const TemplateModuleState&) = delete;
    TemplateModuleState(TemplateModuleState&&) = delete;
    TemplateModuleState& operator=(const TemplateModuleState&) = delete;
    TemplateModuleState& operator=(TemplateModuleState&&) = delete;
    ~TemplateModuleState()
    {
        // The module's values may hold themselves (`l.append(l)`): a collector frees those
        // cycles once nothing else holds them, as a render's does when it ends
        const CycleCollector cycles;
        if (module)
        {
            for (const auto& [name, value] : module->Names())
            {
                CycleCollector::NoteStore(value);
            }
        }
        module.reset();
        callback.reset();
    }

    [[nodiscard]] RenderLimitsScope MakeLimitsScope() const
    {
        const auto& settings = *impl->m_settings;
        return RenderLimitsScope(settings.hasRenderLimits ? &settings.settings.limits : nullptr, settings.bigIntBackend);
    }

    // Renders the template's top level into the module; returns its output
    std::basic_string<CharT> Render()
    {
        // The globals converted for this module alone: a render's are shared with the thread's
        // later renders, which may change a value in place (GlobalsSnapshot)
        if (auto* env = impl->m_env)
        {
            auto& envImpl = *detail::TemplateEnvAccess::GetImpl(*env);
            {
                std::shared_lock<std::shared_timed_mutex> lock(envImpl.guard);
                globals.source = envImpl.globalValues;
            }
            for (const auto& [name, value] : *globals.source)
            {
                globals.values[name] = visit(visitors::InputValueConvertor(false, true), value.GetData());
            }
        }
        for (const auto& [name, value] : paramSource)
        {
            convertedParams[name] = visit(visitors::InputValueConvertor(false, true), value.GetData());
        }

        callback.emplace(impl.get());
        callback->SetGlobals(&globals);
        module.emplace(convertedParams, globals.values, &*callback, &GetBuiltinGlobals(impl->m_settings->settings.extensions.i18n));
        callback->moduleContext = &module->Context();

        std::basic_string<CharT> body;
        OutStream stream(body);
        if (impl->m_settings->hasRenderLimits)
        {
            LimitRenderOutput(body, 0);
        }
        module->Render(*impl, stream);
        stream.Flush();
        if (impl->m_settings->hasRenderLimits)
        {
            CheckRenderOutput();
            detail::PollDeadline();
        }
        return body;
    }

    // The names the module exports, in name order: what its top level set, but not the names
    // that start with an underscore, the ones it imported or those bound at entry only
    std::vector<std::pair<std::string, const InternalValue*>> ExportedNames() const
    {
        std::vector<std::pair<std::string, const InternalValue*>> names;
        for (const auto& [name, value] : module->Names())
        {
            if (name.empty() || name[0] == '_' || IsEntryBinding(value, HashedName::Hash(name)))
            {
                continue;
            }
            // Set again after the import, it is exported (Jinja2's exported_vars)
            const auto imported = std::find_if(callback->imported.begin(), callback->imported.end(), [&name = name, &value = value](const auto& entry) {
                return entry.first == name && IsSameImport(entry.second, value);
            });
            if (imported != callback->imported.end())
            {
                continue;
            }
            names.emplace_back(name, &value);
        }
        std::sort(names.begin(), names.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
        return names;
    }

    [[nodiscard]] BasicErrorInfo<CharT> ExpiredError(const std::string& name) const
    {
        typename BasicErrorInfo<CharT>::Data errorData;
        errorData.code = ErrorCode::TemplateExpired;
        errorData.srcLoc.fileName = impl->m_templateName;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.col = 1;
        errorData.extraParams.emplace_back(name);
        return BasicErrorInfo<CharT>(std::move(errorData));
    }

    // A host's call of the exported callable `fn`, named `name`, in the module's context
    Value Call(const Callable& fn, const std::string& name, const UserCallableParams& ucParams)
    {
        const std::lock_guard<std::recursive_mutex> lock(mutex);
        // The tree the callable runs in was replaced
        if (impl->m_loadGeneration.load(std::memory_order_relaxed) != generation)
        {
            throw BadResultAccess<CharT>(ExpiredError(name));
        }
        std::optional<BasicErrorInfo<CharT>> error;
        try
        {
            // First, so that it ends last: when nothing of the call holds values any more
            const CycleCollector cycles;
            const auto limitsScope = MakeLimitsScope();
            CallParams callParams;
            if (const auto* posArgs = ucParams.extraPosArgs.GetIf<ValuesList>())
            {
                for (const auto& arg : *posArgs)
                {
                    callParams.posParams.push_back(OwnedArgument(arg));
                }
            }
            else if (!ucParams.extraPosArgs.IsNone())
            {
                throw TemplateRuntimeError("the positional arguments of a module's macro must be a ValuesList");
            }
            if (const auto* kwArgs = ucParams.extraKwArgs.GetIf<ValuesMap>())
            {
                for (const auto& [argName, arg] : *kwArgs)
                {
                    callParams.kwParams[argName] = OwnedArgument(arg);
                }
            }
            else if (!ucParams.extraKwArgs.IsNone())
            {
                throw TemplateRuntimeError("the keyword arguments of a module's macro must be a ValuesMap");
            }
            for (const auto& [argName, arg] : ucParams.args)
            {
                callParams.kwParams[argName] = OwnedArgument(arg);
            }
            auto result = ExportCopier().Copy(InvokeInModule(module->Context(), fn, callParams));
            if (impl->m_settings->hasRenderLimits)
            {
                detail::PollDeadline();
            }
            return result;
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const detail::RenderFailure& failure)
        {
            error = failure.ErrorAs<CharT>();
        }
        catch (...)
        {
            error = detail::CurrentExceptionToError<CharT>(impl->m_templateName);
        }
        throw BadResultAccess<CharT>(std::move(*error));
    }

    std::shared_ptr<const Impl> impl;
    // The template's Load count when the module was made (TemplateImpl::m_loadGeneration)
    uint64_t generation;
    // Converted by reference: they live as long as the module
    ValuesMap paramSource;
    InternalValueMap convertedParams;
    GlobalsSnapshot globals;
    std::optional<Callback> callback;
    // The module's context, where the macros were defined; after the callback, which it uses
    std::optional<ImportedModule> module;
    // Calls of the exports take turns: the context is not shared by concurrent calls
    std::recursive_mutex mutex;
};

namespace
{
// An exported callable: called by the host, it runs in its module
template<typename CharT>
struct ModuleCall
{
    // First, so that the callable, whose closure refers to the module's context, goes before it
    std::shared_ptr<TemplateModuleState<CharT>> state;
    InternalValue callable;
    std::string name;

    Value operator()(const UserCallableParams& params) const { return state->Call(*GetIf<Callable>(&callable), name, params); }
};
} // namespace

template<typename CharT>
nonstd::expected<BasicTemplateModule<CharT>, BasicErrorInfo<CharT>> TemplateImpl<CharT>::MakeModule(const std::shared_ptr<const TemplateImpl>& self, const ValuesMap& params)
{
    if (!self->m_renderer)
    {
        typename BasicErrorInfo<CharT>::Data errorData;
        errorData.code = ErrorCode::TemplateNotParsed;
        errorData.srcLoc.col = 1;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.fileName = "<unknown file>";
        return MakeUnexpected(BasicErrorInfo<CharT>(errorData));
    }

    auto state = std::make_shared<TemplateModuleState<CharT>>(self, params);
    BasicTemplateModule<CharT> result;
    try
    {
        const CycleCollector cycles;
        const auto limitsScope = state->MakeLimitsScope();
        result.body = state->Render();
        const std::vector<ArgInfo> argsInfo{ ArgInfo(ArgInfo::VarArgs), ArgInfo(ArgInfo::VarKwArgs) };
        for (const auto& [name, value] : state->ExportedNames())
        {
            if (const auto* callable = GetIf<Callable>(value))
            {
                result.exports[name] = UserCallable(ModuleCall<CharT>{ state, *value, name }, argsInfo);
                continue;
            }
            // Copied within the render's limits, which bound a lazy list read out
            result.exports[name] = ExportCopier().Copy(*value);
        }
    }
    catch (const std::bad_alloc&)
    {
        throw;
    }
    catch (const detail::RenderFailure& failure)
    {
        return MakeUnexpected(failure.ErrorAs<CharT>());
    }
    catch (...)
    {
        return MakeUnexpected(detail::CurrentExceptionToError<CharT>(self->m_templateName));
    }
    return result;
}

template nonstd::expected<BasicTemplateModule<char>, BasicErrorInfo<char>> TemplateImpl<char>::MakeModule(const std::shared_ptr<const TemplateImpl>& self,
                                                                                                          const ValuesMap& params);
template nonstd::expected<BasicTemplateModule<wchar_t>, BasicErrorInfo<wchar_t>> TemplateImpl<wchar_t>::MakeModule(const std::shared_ptr<const TemplateImpl>& self,
                                                                                                                   const ValuesMap& params);

} // namespace v2
} // namespace jinja2
