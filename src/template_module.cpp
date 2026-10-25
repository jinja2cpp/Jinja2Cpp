// Template::MakeModule: a template's exports for the host, as Python Jinja2's Template.module
// and make_module (docs/tasks/0284)
#include "cycle_collector.h"
#include "engine_errors.h"
#include "error_boundary.h"
#include "expression_evaluator.h"
#include "internal_value.h"
#include "make_unexpected.h"
#include "out_stream.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "render_failure.h"
#include "statements.h"
#include "template_dict.h"
#include "template_env_impl.h"
#include "template_impl.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/markup.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <new>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2
{
inline namespace v2
{

namespace
{
// Copies a value of the module into one the host owns, so that nothing the host keeps refers
// to the module's context or is changed by a later macro call. Lists, dicts and namespaces are
// copied item by item; a container that contains itself is cut where it repeats. Objects (a
// host's reflected object, a cycler) keep their own state and are passed on as views, which own
// what they view: the module converts its params and globals by value (TemplateModuleState)
class ExportCopier
{
public:
    Value Copy(const InternalValue& value)
    {
        // Nesting comes from the template (a list built in a loop): stop where a render would
        CheckStack();
        // A string may view the template's source, which a Load replaces, or the module's
        // params: the host gets its own copy, a Markup kept a Markup
        if (auto text = AsStringView<char>(value))
        {
            return value.IsMarkup() ? Value(Markup(*text)) : Value(std::string(*text));
        }
        if (auto text = AsStringView<wchar_t>(value))
        {
            return value.IsMarkup() ? Value(WMarkup(*text)) : Value(std::wstring(*text));
        }
        if (auto list = AsList(value))
        {
            return CopyList(*list);
        }
        if (auto map = AsMap(value))
        {
            // A dict, the template's or the host's, or a namespace; not an object
            const bool isDict = map->GetMutableItems() != nullptr || map->IsNamespace() || (!map->HasAttributes() && map->GetObjectClass() == nullptr);
            if (isDict)
            {
                return CopyMap(*map);
            }
        }
        return IntValue2Value(value);
    }

private:
    // Starts copying the container `identity`; false when it is being copied already: the copy
    // reached it again
    bool Enter(const void* identity)
    {
        return identity == nullptr || m_path.insert(identity).second;
    }
    void Leave(const void* identity) { m_path.erase(identity); }

    Value CopyList(const ListRef& list)
    {
        const auto* identity = list.GetIdentity();
        if (!Enter(identity))
        {
            return Value();
        }
        // A lazy list (range, a filter's generator) is read out here, within the limits
        ValuesList items;
        list.ForEach([this, &items](InternalValue&& item) {
            CheckSequenceSize(items.size() + 1, "a list");
            const InternalValue owned(std::move(item));
            items.push_back(Copy(owned));
            return true;
        });
        Leave(identity);
        return Value(std::move(items));
    }

    Value CopyMap(const MapRef& map)
    {
        const auto* identity = map.GetIdentity();
        if (!Enter(identity))
        {
            return Value();
        }
        // A ValuesMap has string keys only: another key is spelled as str() spells it, as the
        // public view of such a dict has it (SharedDictAdapter::CreateSnapshot), and keys that
        // spell alike collapse into the first one's position with the later value
        ValuesMap items;
        for (const auto& entry : map.GetEntries())
        {
            auto key = GetAsSameString(std::string(), entry.key);
            items[key ? std::move(*key) : DictKeyText(entry.key)] = Copy(entry.value);
        }
        Leave(identity);
        return Value(std::move(items));
    }

    // The containers being copied
    std::unordered_set<const void*> m_path;
};

// Whether `value` is still the object `imported` an import bound: Python's `is`, as `is sameas`
// tests it
bool IsSameImport(const InternalValue& imported, const InternalValue& value)
{
    if (const auto* callable = GetIf<Callable>(&imported))
    {
        const auto* other = GetIf<Callable>(&value);
        return other != nullptr && callable->IsSame(*other);
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
    return GetIf<Callable>(&value) == nullptr && !AsMap(value) && !AsList(value) && IntValue2Value(imported) == IntValue2Value(value);
}

// Whether `value`, set under the name with hash `hash`, is only the undefined a frame binds at
// its entry to a name it sets later (RenderContext::IsEntryBinding): a name never set
bool IsEntryBinding(const InternalValue& value, std::size_t hash)
{
    const auto* undefined = GetIf<UndefinedValue>(&value);
    return undefined != nullptr && undefined->atEntry && undefined->entryHash == static_cast<std::uint32_t>(hash);
}

// Calls `callable` on `context` as a call expression does (InvokeCallable in
// expression_evaluator.cpp): a macro's output is Markup when the module autoescapes. A macro
// renders into `output`, within maxOutputSize when `limitOutput`; `output` stays registered
// with the limits, so it must outlive their scope
template<typename CharT>
InternalValue InvokeInModule(RenderContext& context, const Callable& callable, const CallParams& params, std::basic_string<CharT>& output, bool limitOutput)
{
    if (callable.GetType() == Callable::Type::Expression)
    {
        return callable.GetExpressionCallable()(params, context);
    }
    OutStream stream(output);
    if (limitOutput)
    {
        LimitRenderOutput(output, 0);
    }
    callable.GetStatementCallable()(params, stream, context);
    stream.Flush();
    if (limitOutput)
    {
        CheckRenderOutput();
    }
    InternalValue result(std::move(output));
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

        // The modules a call imports with context end with it (CallScope), but those a module
        // shared without context imports, which live as long as it does
        [[nodiscard]] bool ModulesEndWithCall() const override { return calls != 0 && sharedDepth == 0; }
        std::size_t EnterSharedModule() override
        {
            ++sharedDepth;
            return kept.size();
        }
        void LeaveSharedModule(std::size_t token, bool shared) override
        {
            --sharedDepth;
            // A module that failed is not shared: what it imported ends with the call too
            if (!shared)
            {
                kept.resize(token);
            }
        }
        ImportedModule& AddModule(std::unique_ptr<ImportedModule> newModule) override
        {
            auto& added = Impl::RendererCallback::AddModule(std::move(newModule));
            if (sharedDepth != 0)
            {
                kept.push_back(&added);
            }
            return added;
        }

        const RenderContext* moduleContext = nullptr;
        // The names the top level imported, with the values imported
        std::vector<std::pair<std::string, InternalValue>> imported;
        // The host's calls running, nested when a host callable calls the module again
        std::size_t calls = 0;
        // The top levels of modules imported without context rendering, and the modules they
        // imported, which EndCall keeps
        std::size_t sharedDepth = 0;
        std::vector<const ImportedModule*> kept;
    };

    // One host's call of an export: what it loads into the module's callback ends with it,
    // so that a module called once per request does not grow (docs/tasks/0301). A nested
    // call ends only what it added itself
    class CallScope
    {
    public:
        explicit CallScope(Callback& callback)
            : m_callback(callback)
            , m_mark(callback.MarkCall())
            , m_sharedDepth(std::exchange(callback.sharedDepth, 0))
        {
            ++m_callback.calls;
        }
        CallScope(const CallScope&) = delete;
        CallScope(CallScope&&) = delete;
        CallScope& operator=(const CallScope&) = delete;
        CallScope& operator=(CallScope&&) = delete;
        ~CallScope()
        {
            --m_callback.calls;
            m_callback.sharedDepth = m_sharedDepth;
            m_callback.EndCall(m_mark, m_callback.kept);
        }

    private:
        Callback& m_callback;
        typename Callback::CallMark m_mark;
        // A call made from a shared module's top level (a host callable there) imports for
        // itself: its modules end with it
        std::size_t m_sharedDepth;
    };

    TemplateModuleState(std::shared_ptr<const Impl> tpl, const ValuesMap& params)
        : impl(std::move(tpl))
        , generation(impl->m_loadGeneration.load(std::memory_order_relaxed))
    {
        for (const auto& [name, value] : params)
        {
            convertedParams[name] = visit(visitors::InputValueConvertor(true, false), value.GetData());
        }
    }
    TemplateModuleState(const TemplateModuleState&) = delete;
    TemplateModuleState(TemplateModuleState&&) = delete;
    TemplateModuleState& operator=(const TemplateModuleState&) = delete;
    TemplateModuleState& operator=(TemplateModuleState&&) = delete;
    ~TemplateModuleState()
    {
        // The module's values may hold themselves (`l.append(l)`): a collector frees those
        // cycles once nothing else holds them, as a render's does when it ends. Noting them
        // allocates; when that fails, the cycles noted so far are still freed and the rest leak
        const CycleCollector cycles;
        if (module)
        {
            try
            {
                for (const auto& [name, value] : module->Names())
                {
                    CycleCollector::NoteStore(value);
                }
            }
            catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment above says what is lost
            {
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

    // Renders the template's top level into the module and its output into `body`, which
    // stays registered with the limits, so it must outlive their scope
    void Render(std::basic_string<CharT>& body)
    {
        // The globals converted for this module alone: a render's are shared with the thread's
        // later renders, which may change a value in place (GlobalsSnapshot). The params and
        // globals are converted by value, so that an exported view (of a host's map, say)
        // owns what it views and outlives the module
        if (auto* env = impl->m_env)
        {
            auto& envImpl = *detail::TemplateEnvAccess::GetImpl(*env);
            {
                std::shared_lock<std::shared_timed_mutex> lock(envImpl.guard);
                globals.source = envImpl.globalValues;
            }
            for (const auto& [name, value] : *globals.source)
            {
                globals.values[name] = visit(visitors::InputValueConvertor(true, false), value.GetData());
            }
        }

        callback = std::make_unique<Callback>(impl.get());
        callback->SetGlobals(&globals);
        module = std::make_unique<ImportedModule>(std::move(convertedParams), globals.values, callback.get(), &GetBuiltinGlobals(impl->m_settings->settings.extensions.i18n));
        callback->moduleContext = &module->Context();

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
    }

    // The names the module exports, in name order: what its top level set, but not the names
    // that start with an underscore, the ones it imported or those bound at entry only
    [[nodiscard]] std::vector<std::pair<std::string, const InternalValue*>> ExportedNames() const
    {
        std::vector<std::pair<std::string, const InternalValue*>> names;
        const auto& imported = callback->imported;
        for (const auto& [name, value] : module->Names())
        {
            if (name.empty() || name[0] == '_' || IsEntryBinding(value, HashedName::Hash(name)))
            {
                continue;
            }
            // Set again after the import, it is exported (Jinja2's exported_vars)
            const auto isImported = std::any_of(imported.begin(), imported.end(), [&exportName = name, &exportValue = value](const auto& entry) {
                return entry.first == exportName && IsSameImport(entry.second, exportValue);
            });
            if (isImported)
            {
                continue;
            }
            names.emplace_back(name, &value);
        }
        std::sort(names.begin(), names.end(), [](const auto& lhs, const auto& rhs) { return lhs.first < rhs.first; });
        return names;
    }

    // A macro the call `name` called ran in a module that ended with an earlier call
    // (ClosureSite::Resolve), which reports it without a name
    [[nodiscard]] BasicErrorInfo<CharT> Named(BasicErrorInfo<CharT> error, const std::string& name) const
    {
        if (error.GetCode() != ErrorCode::TemplateExpired || !error.GetExtraParams().empty())
        {
            return error;
        }
        return ExpiredError(name, true);
    }

    // `ended`: a module made by an earlier call had ended, not the tree replaced
    [[nodiscard]] BasicErrorInfo<CharT> ExpiredError(const std::string& name, bool ended = false) const
    {
        typename BasicErrorInfo<CharT>::Data errorData;
        errorData.code = ErrorCode::TemplateExpired;
        errorData.srcLoc.fileName = impl->m_templateName;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.col = 1;
        errorData.extraParams.emplace_back(name);
        if (ended)
        {
            errorData.extraParams.emplace_back(std::string("ended"));
        }
        return BasicErrorInfo<CharT>(std::move(errorData));
    }

    // A host's call of the exported callable `fn`, named `name`, in the module's context
    Value Call(const Callable& fn, const std::string& name, const UserCallableParams& ucParams)
    {
        const std::scoped_lock lock(mutex);
        // The tree the callable runs in was replaced
        if (impl->m_loadGeneration.load(std::memory_order_relaxed) != generation)
        {
            throw BadResultAccess<CharT>(ExpiredError(name));
        }
        try
        {
            // First, so that it ends last: when nothing of the call holds values any more
            const CycleCollector cycles;
            // Then what the call loads, which its collector sees go
            const CallScope callScope(*callback);
            // Then the module's context as the call found it, which a failed call would leave
            // with its scopes pushed
            const HomeCall::Restore restore(module->Context());
            // Before the limits, which keep a pointer to it
            std::basic_string<CharT> output;
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
            auto result = ExportCopier().Copy(InvokeInModule(module->Context(), fn, callParams, output, impl->m_settings->hasRenderLimits));
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
            throw BadResultAccess<CharT>(Named(failure.ErrorAs<CharT>(), name));
        }
        catch (...)
        {
            throw BadResultAccess<CharT>(Named(detail::CurrentExceptionToError<CharT>(impl->m_templateName), name));
        }
    }

    std::shared_ptr<const Impl> impl;
    // The template's Load count when the module was made (TemplateImpl::m_loadGeneration)
    uint32_t generation;
    // The params, until the module takes them
    InternalValueMap convertedParams;
    GlobalsSnapshot globals;
    std::unique_ptr<Callback> callback;
    // The module's context, where the macros were defined; after the callback, which it uses
    std::unique_ptr<ImportedModule> module;
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
        // Into result.body, which outlives the limits that keep a pointer to it
        state->Render(result.body);
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
