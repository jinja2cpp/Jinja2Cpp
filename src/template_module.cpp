// Template::MakeModule: a template's exports for the host, as Python Jinja2's Template.module
// and make_module (docs/tasks/0284)
#include "borrowed_cells.h"
#include "cycle_collector.h"
#include "engine_errors.h"
#include "error_boundary.h"
#include "expression_evaluator.h"
#include "internal_value.h"
#include "make_unexpected.h"
#include "out_stream.h"
#include "public_host_object.h"
#include "recursion_guard.h"
#include "render_workspace.h"
#include "render_context.h"
#include "render_failure.h"
#include "statements.h"
#include "template_dict.h"
#include "template_env_impl.h"
#include "template_impl.h"
#include "thread_annotations.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/markup.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <variant>
#include <vector>

namespace jinja2
{
inline namespace v2
{

namespace detail
{
namespace
{
// Few at once: one per Load that host code replaced during the Load, until that Load returns
struct LoadingKeepList
{
    Mutex mutex;
    std::vector<std::shared_ptr<const ITemplateImpl>> kept JINJA2CPP_GUARDED_BY(mutex);
};

LoadingKeepList& LoadingKept()
{
    static LoadingKeepList list;
    return list;
}
} // namespace

void KeepTemplateWhileLoading(std::shared_ptr<const ITemplateImpl> impl)
{
    auto& list = LoadingKept();
    const WriterLock lock(list.mutex);
    list.kept.push_back(std::move(impl));
}

std::shared_ptr<const ITemplateImpl> TakeTemplateKeptWhileLoading(const ITemplateImpl* impl) noexcept
{
    std::shared_ptr<const ITemplateImpl> taken;
    auto& list = LoadingKept();
    const WriterLock lock(list.mutex);
    const auto found = std::find_if(list.kept.begin(), list.kept.end(), [impl](const auto& kept) { return kept.get() == impl; });
    if (found != list.kept.end())
    {
        taken = std::move(*found);
        list.kept.erase(found);
    }
    // Dropped by the caller, outside the lock: its destructor may run host code that loads
    return taken;
}
} // namespace detail

namespace
{
// A host's live view of an object a module made (ModuleObjectView): what the module's own code needs of it when the host
// hands it back as an argument
struct IModuleObjectView
{
    IModuleObjectView() = default;
    IModuleObjectView(const IModuleObjectView&) = delete;
    IModuleObjectView& operator=(const IModuleObjectView&) = delete;
    IModuleObjectView(IModuleObjectView&&) = delete;
    IModuleObjectView& operator=(IModuleObjectView&&) = delete;
    virtual ~IModuleObjectView() = default;

    // The module the object belongs to (a TemplateModuleState), for comparing only
    [[nodiscard]] virtual const void* StateId() const noexcept = 0;
    // The object itself, which the module owns
    [[nodiscard]] virtual const InternalValue& Object() const noexcept = 0;
    // The object as a copy, as the exports of the default mode (ModuleExports::Copies) have it, which refers to nothing of
    // the module but its callables
    [[nodiscard]] virtual Value Snapshot() const = 0;
};

// The view `value` is, of an object of the module `self`; null for any other value
const IModuleObjectView* ModuleViewOf(const Value& value, const void* self)
{
    const auto* map = self ? value.GetIf<GenericMap>() : nullptr;
    if (!map || !map->GetAccessor() || !map->GetObjectClass())
    {
        return nullptr;
    }
    const auto* view = dynamic_cast<const IModuleObjectView*>(map->GetAccessor());
    return view && view->StateId() == self ? view : nullptr;
}

// Copies a value of the module into one the host owns, so that nothing the host keeps refers
// to the module's context or is changed by a later macro call. Lists, dicts and namespaces are
// copied item by item; a container that contains itself is cut where it repeats. Objects (a
// host's reflected object, a cycler) keep their own state and are passed on as views, which own
// what they view: the module converts its params and globals by value (TemplateModuleState)
class ExportCopier
{
public:
    // What a template callable becomes: a call in the module `state`, whose errors name `name`
    // (TemplateModuleState::Wrap). A plain function: a module call makes a copier for its
    // result, which mostly holds no callable
    using WrapCallable = Value (*)(void* state, const InternalValue& callable, const std::string& name);
    // What a namespace, cycler or joiner becomes in Objects mode: a live view of it in the module `state` (ModuleObjectView)
    using WrapObject = Value (*)(void* state, const InternalValue& object, const std::string& name);

    // `objects`: ModuleExports::Objects, which keeps an undefined value, a tuple and a dict's own keys
    ExportCopier(WrapCallable wrap, void* state, const std::string& name, bool objects = false, WrapObject wrapObject = nullptr)
        : m_wrap(wrap)
        , m_wrapObject(wrapObject)
        , m_state(state)
        , m_name(name)
        , m_objects(objects)
    {
    }

    Value Copy(const InternalValue& value)
    {
        // A macro anywhere in a value, not only at the module's top level (docs/tasks/0300)
        if (const auto* callable = GetIf<Callable>(&value); callable && callable->GetKind() != Callable::UserCallable)
        {
            return m_wrap(m_state, value, m_name);
        }
        // Nesting comes from the template (a list built in a loop): stop where a render would
        CheckStack();
        if (m_objects)
        {
            // An undefined value the host's own (UndefinedToHost owns its strings), a dictsort pair as the
            // 2-tuple it is in Python
            if (const auto* undefined = GetIf<UndefinedValue>(&value))
            {
                return UndefinedToHost(*undefined);
            }
            if (const auto* pair = GetIf<KeyValuePair>(&value))
            {
                ValuesList items;
                items.reserve(2);
                items.push_back(Copy(pair->key));
                items.push_back(Copy(pair->value));
                return MakeOwnedTuple(std::move(items));
            }
        }
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
            // A namespace, a cycler and a joiner stay the module's: the host reads and calls the object itself
            if (m_objects && m_wrapObject && !map->IsHost() && IsLiveObjectClass(map->GetObjectClass()))
            {
                return m_wrapObject(m_state, value, m_name);
            }
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
    // Whether the objects of the Python class `objectClass` (or null) are views in Objects mode. Not `loop` and a module, which
    // keep the copy of what they were
    static bool IsLiveObjectClass(const char* objectClass)
    {
        if (!objectClass)
        {
            return false;
        }
        const std::string_view name(objectClass);
        return name == "Namespace" || name == "Cycler" || name == "Joiner";
    }

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
        if (m_objects && list.IsTuple())
        {
            return MakeOwnedTuple(std::move(items));
        }
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
        // Within the limits before the entries are read out: a host map, such as a JSON view
        // (docs/tasks/0370), may be larger than the render could have built
        CheckSequenceSize(map.GetSize(), "a dict");
        ValuesMap items;
        const auto entries = map.GetEntries();
        // Objects keeps the real keys of a dict that has a key that is no string
        const bool keyed = m_objects && std::any_of(entries.begin(), entries.end(), [](const KeyValuePair& entry) { return !GetAsSameString(std::string(), entry.key); });
        std::vector<std::pair<Value, Value>> keyedItems;
        for (const auto& entry : entries)
        {
            auto key = GetAsSameString(std::string(), entry.key);
            auto copy = Copy(entry.value);
            if (keyed)
            {
                keyedItems.emplace_back(Copy(entry.key), copy);
            }
            items[key ? std::move(*key) : DictKeyText(entry.key)] = std::move(copy);
        }
        Leave(identity);
        if (keyed)
        {
            return MakeOwnedKeyedMap(std::move(items), std::move(keyedItems));
        }
        return Value(std::move(items));
    }

    WrapCallable m_wrap;
    WrapObject m_wrapObject;
    void* m_state;
    const std::string& m_name;
    bool m_objects;
    // The containers being copied
    std::unordered_set<const void*> m_path;
};

// Whether `value`, set under the name with hash `hash`, is only the undefined a frame binds at
// its entry to a name it sets later (RenderContext::IsEntryBinding): a name never set
bool IsEntryBinding(const InternalValue& value, std::size_t hash)
{
    const auto* undefined = GetIf<UndefinedValue>(&value);
    return undefined != nullptr && undefined->atEntry && undefined->entryHash == static_cast<std::uint32_t>(hash);
}

// Sets the filter policies of a module's call on its callback, and puts back those of the call it runs in
class PoliciesScope
{
public:
    PoliciesScope(IRendererCallback& callback, const FilterPolicies* policies) noexcept
        : m_callback(callback)
        , m_outer(callback.OwnFilterPolicies())
    {
        callback.SetFilterPolicies(policies);
    }
    ~PoliciesScope() { m_callback.SetFilterPolicies(m_outer); }
    PoliciesScope(const PoliciesScope&) = delete;
    PoliciesScope& operator=(const PoliciesScope&) = delete;
    PoliciesScope(PoliciesScope&&) = delete;
    PoliciesScope& operator=(PoliciesScope&&) = delete;

private:
    IRendererCallback& m_callback;
    const FilterPolicies* m_outer;
};

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

// How many times the stack a copy of nested lists takes their destruction may take: Value's
// destructor recurses through several frames a level where the copy takes one
constexpr std::uintptr_t DestroyStackFactor = 8;

// Whether the copy started at the frame `entry` may not go a level deeper: the stack is nearly
// used up, or what is left would not destroy the copy. The copy and its destruction run at
// about the depth of `entry` (a macro call, MakeModule), and the destructor of a partial copy
// unwinding from here as deep as the copy (docs/tasks/0418)
bool CopyTooDeep(std::uintptr_t entry)
{
    if (StackNearlyExhausted())
    {
        return true;
    }
#if defined(_MSC_VER) && !defined(__clang__)
    const auto frame = reinterpret_cast<std::uintptr_t>(_AddressOfReturnAddress());
#else
    const auto frame = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
#endif
    const auto& stack = detail::t_threadStack;
    // Bounds unknown (limit 0), or a frame on another stack: unchecked, as StackNearlyExhausted
    if (stack.limit == 0 || frame < stack.low || frame > entry)
    {
        return false;
    }
    return (entry - frame) * DestroyStackFactor > frame - stack.low;
}

// Whether a host's GenericList or GenericMap is copied as a list or dict of its items (OwnedCopy): not an invalid one,
// nor a map of the library's own bridge (a host object, an undefined value), which keeps its object alive itself, nor
// a template object of the engine, which keeps its class (LendGenericItem)
bool IsCopiedGeneric(const Value& value)
{
    if (const auto* list = value.GetIf<GenericList>())
    {
        return list->IsValid();
    }
    const auto* map = value.GetIf<GenericMap>();
    if (!map || !map->GetAccessor())
    {
        return false;
    }
    return !IsSelfOwningHostMap(*map->GetAccessor()) && map->GetObjectClass() == nullptr;
}

// A copy of the host's `value` the module owns, which stops with RecursionLimitError where its
// nesting, copied or destroyed, would overflow the stack: Value's copy and destructor recurse
// once per level of its lists and dicts (docs/tasks/0418, 0449). A view of the host's text
// becomes a string of its own, since the module's values outlive the call
// With `generic`, a host's GenericList or GenericMap becomes a list or dict of its items too: a render nested in
// another one, whose values outlive it, may get one that views a container the host frees once the render returns
// (docs/tasks/0524). Its items are taken once, at most the render's maxSequenceSize of them
Value OwnedCopy(const Value& value, std::uintptr_t entry, bool generic, const void* self)
{
    // A view of an object of the module in a list or dict that its macro may store: a copy, not a reference that would make
    // the module hold the view that holds the module
    if (const auto* view = ModuleViewOf(value, self))
    {
        return view->Snapshot();
    }
    if (const auto* view = value.GetIf<std::string_view>())
    {
        return std::string(*view);
    }
    if (const auto* view = value.GetIf<std::wstring_view>())
    {
        return std::wstring(*view);
    }
    const auto& data = value.GetData();
    const bool isGeneric = generic && IsCopiedGeneric(value);
    if (!isGeneric && !std::holds_alternative<RecWrapper<ValuesList>>(data) && !std::holds_alternative<RecWrapper<ValuesMap>>(data))
    {
        return value;
    }
    if (CopyTooDeep(entry))
    {
        throw RecursionLimitError();
    }
    if (const auto* list = isGeneric ? value.GetIf<GenericList>() : nullptr)
    {
        ValuesList copy;
        for (const auto& item : *list)
        {
            CheckSequenceSize(static_cast<uint64_t>(copy.size()) + 1, "a generic list param");
            copy.push_back(OwnedCopy(item, entry, generic, self));
        }
        return Value(std::move(copy));
    }
    if (const auto* map = isGeneric ? value.GetIf<GenericMap>() : nullptr)
    {
        ValuesMap copy;
        for (const auto& [key, item] : *map)
        {
            copy.emplace(key, OwnedCopy(item, entry, generic, self));
        }
        return Value(std::move(copy));
    }
    if (const auto* list = value.GetIf<ValuesList>())
    {
        ValuesList copy;
        copy.reserve(list->size());
        for (const auto& item : *list)
        {
            copy.push_back(OwnedCopy(item, entry, generic, self));
        }
        return Value(std::move(copy));
    }
    ValuesMap copy;
    for (const auto& [key, item] : value.As<ValuesMap>())
    {
        copy.emplace(key, OwnedCopy(item, entry, generic, self));
    }
    return Value(std::move(copy));
}

} // namespace

// A host Value as a value the module owns: copied and converted by value, so that a module
// value the macro stores it in (a namespace) never refers to the host's
InternalValue OwnedArgument(const Value& value, bool generic, const void* self)
{
    // An object of the module that the host reads through its view is the module's own object once it is an argument:
    // the macro changes the namespace itself, and what it stores is no view
    if (const auto* view = ModuleViewOf(value, self))
    {
        return view->Object();
    }
    // A scalar or a host's list or map, converted by value as it is
    const auto& data = value.GetData();
    if (!std::holds_alternative<RecWrapper<ValuesList>>(data) && !std::holds_alternative<RecWrapper<ValuesMap>>(data) && !std::holds_alternative<std::string_view>(data)
        && !std::holds_alternative<std::wstring_view>(data) && !(generic && IsCopiedGeneric(value)))
    {
        return visit(visitors::InputValueConvertor(true, false), data);
    }
#if defined(_MSC_VER) && !defined(__clang__)
    const auto entry = reinterpret_cast<std::uintptr_t>(_AddressOfReturnAddress());
#else
    const auto entry = reinterpret_cast<std::uintptr_t>(__builtin_frame_address(0));
#endif
    Value copy;
    try
    {
        copy = OwnedCopy(value, entry, generic, self);
    }
    catch (const RecursionLimitError&)
    {
        if (!generic)
        {
            throw;
        }
        // Too deep a generic value, a cyclic one most likely (a reflected struct that points back at its parent):
        // the host's view of it, as without a copy, which the host keeps alive (docs/tasks/0524)
        copy = OwnedCopy(value, entry, false, self);
    }
    return visit(visitors::InputValueConvertor(true, false), copy.GetData());
}

template<typename CharT>
struct TemplateModuleState;
// The host's callable for `callable`, a template callable of the module `state`, named `name`
// in its errors: a call of it in the module (ModuleCall). Defined after ModuleCall
template<typename CharT>
Value MakeModuleCall(const std::shared_ptr<TemplateModuleState<CharT>>& state, const InternalValue& callable, const std::string& name);
// The host's live view of `object`, a namespace, cycler or joiner of the module `state`, exported as `name`
// (ModuleObjectView). Defined after it
template<typename CharT>
Value MakeModuleObject(const std::shared_ptr<TemplateModuleState<CharT>>& state, const InternalValue& object, const std::string& name);

// The state the exports of one module share: the template and the module's context their calls
// run in, kept alive by every export (docs/tasks/0284, decision D1)
template<typename CharT>
struct TemplateModuleState : std::enable_shared_from_this<TemplateModuleState<CharT>>
{
    using Impl = TemplateImpl<CharT>;

    // The render callback of the module, which keeps what the host's calls load apart
    class Callback final : public Impl::RendererCallback
    {
    public:
        explicit Callback(const Impl* host)
            : Impl::RendererCallback(host)
        {
            // The trees its calls replace are freed once nothing of a call reaches them (docs/tasks/0482)
            this->SetKeepsTreesByValue();
        }

        // The modules a call imports with context end with it (CallScope), but those a module
        // shared without context imports, which live as long as it does
        [[nodiscard]] bool ModulesEndWithCall() const override { return calls != 0 && sharedDepth == 0; }
        std::size_t EnterSharedModule() override
        {
            pendingMarks.push_back(pending.size());
            ++sharedDepth;
            return kept.size();
        }
        void LeaveSharedModule(std::size_t token, bool shared) override
        {
            --sharedDepth;
            const auto mark = pendingMarks.back();
            pendingMarks.pop_back();
            const auto from = kept.begin() + static_cast<std::ptrdiff_t>(token);
            if (shared)
            {
                // What it imported is its own from now on, even if a module importing it fails
                std::for_each(from, kept.end(), [](KeptModule& entry) { entry.owned = true; });
                // The module ShareModule just shared imported those at its top level: it keeps them for its life, and
                // ends with them once nothing else holds the group (EndReplacedSharedModules, 0528, 0541). When noting
                // it fails, what it imported stays kept: what it named of theirs holds their values, so that it ends
                // only once nothing does
                if (lastShared)
                {
                    try
                    {
                        std::for_each(pending.begin() + static_cast<std::ptrdiff_t>(mark), pending.end(), [this](const ImportedModule* imported) {
                            if (imported)
                            {
                                imports.Add(lastShared, imported);
                            }
                        });
                    }
                    catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): the comment above says what is kept
                    {
                    }
                }
                pending.resize(mark);
                return;
            }
            pending.resize(mark);
            // A module that failed is not shared: what it imported ends with the call too, but
            // what a module it imported that was shared imported
            kept.erase(std::remove_if(from, kept.end(), [](const KeptModule& entry) { return !entry.owned; }), kept.end());
        }
        void ShareModule(const IRendererCallback::LoadTemplateResult& tpl, ImportedModule& sharedModule) override
        {
            Impl::RendererCallback::ShareModule(tpl, sharedModule);
            lastShared = &sharedModule;
        }
        ImportedModule& AddModule(std::unique_ptr<ImportedModule> newModule) override
        {
            auto& added = Impl::RendererCallback::AddModule(std::move(newModule));
            if (sharedDepth != 0)
            {
                kept.push_back({ &added, false });
                pending.push_back(&added);
            }
            return added;
        }
        // A shared module a shared module's top level imports again is one it imported too
        [[nodiscard]] ImportedModule* FindModule(const IRendererCallback::LoadTemplateResult& tpl) const override
        {
            auto* found = Impl::RendererCallback::FindModule(tpl);
            if (found && sharedDepth != 0)
            {
                pending.push_back(found);
            }
            return found;
        }
        [[nodiscard]] bool IsKept(const ImportedModule* candidate) const
        {
            return std::any_of(kept.begin(), kept.end(), [candidate](const KeptModule& entry) { return entry.module == candidate; });
        }
        // Forgets `gone`, which ends or which no module imports any more (EndReplacedSharedModules). Erasing from
        // `kept` moves the tokens EnterSharedModule gave: it runs only once the outermost call returned, when no top level
        // that took one renders (a top level with no call running is MakeModule's own, before any export exists)
        void Forget(const ImportedModule* gone) noexcept
        {
            kept.erase(std::remove_if(kept.begin(), kept.end(), [gone](const KeptModule& entry) { return entry.module == gone; }), kept.end());
            // Cleared in place: the marks of the top levels rendering index it
            std::replace(pending.begin(), pending.end(), gone, static_cast<const ImportedModule*>(nullptr));
        }

        // The host's calls running, nested when a host callable calls the module again
        std::size_t calls = 0;
        // The top levels of modules imported without context rendering, and the modules they
        // imported, which EndCall keeps; `owned` once the module that imported it is shared
        std::size_t sharedDepth = 0;
        struct KeptModule
        {
            const ImportedModule* module;
            bool owned;
        };
        std::vector<KeptModule> kept;
        // Which shared module's top level imported which module (EndReplacedSharedModules)
        typename Impl::RendererCallback::ModuleImports imports;
        // The modules the top levels of modules imported without context rendering imported, added or shared, from the
        // mark of each (EnterSharedModule); LeaveSharedModule notes them in `imports`
        mutable std::vector<const ImportedModule*> pending;
        std::vector<std::size_t> pendingMarks;
        // The module ShareModule shared last, which LeaveSharedModule then leaves
        const ImportedModule* lastShared = nullptr;
    };

    // One host's call of an export: what it loads into the module's callback ends with it,
    // so that a module called once per request does not grow (docs/tasks/0301). A nested
    // call ends only what it added itself
    class CallScope
    {
    public:
        // `holdsHandles`: the module, `outer`, holds handles of the callback's table of templates (its blocks)
        CallScope(Callback& callback, const ImportedModule& outer, KeptObjects& kept)
            : m_callback(callback)
            , m_outer(outer)
            , m_holdsHandles(outer.HoldsTemplateHandles())
            , m_kept(kept)
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
            const auto& owner = m_callback;
            // What outlives the call: the module's own values and the modules that stay, which EndCall adds
            const auto& outer = m_outer;
            m_callback.EndCall(
                m_mark, [&owner](const ImportedModule* candidate) { return owner.IsKept(candidate); }, m_callback.calls == 0,
                [&outer](std::vector<const ValueObject*>& roots) { outer.AppendHeldObjects(roots); });
            // A call that failed leaves what its error views as it is until the error is made,
            // then prunes (PruneAfterFailedCall, docs/tasks/0503)
            // What closures the host's calls let go of kept goes now, before the slots are asked who holds them
            if (m_callback.calls == 0)
            {
                m_kept.ReleaseExpired();
            }
            if (m_succeeded && m_callback.calls == 0 && m_callback.HasReplaced())
            {
                // The shared modules of replaced templates first, so that their trees go in the same pass (0528)
                auto& ending = m_callback;
                ending.EndReplacedSharedModules(
                    ending.imports, [&owner](const ImportedModule* candidate) { return owner.IsKept(candidate); },
                    [&ending](const ImportedModule* gone) { ending.Forget(gone); });
                m_callback.PruneReplaced(m_holdsHandles);
            }
        }

        // The call returns its result: the trees a source change replaced may go
        void Succeeded() noexcept { m_succeeded = true; }

    private:
        Callback& m_callback;
        const ImportedModule& m_outer;
        bool m_holdsHandles;
        KeptObjects& m_kept;
        typename Callback::CallMark m_mark;
        // A call made from a shared module's top level (a host callable there) imports for
        // itself: its modules end with it
        std::size_t m_sharedDepth;
        // Set by Succeeded
        bool m_succeeded = false;
    };

    explicit TemplateModuleState(std::shared_ptr<const Impl> tpl)
        : impl(std::move(tpl))
        , dropped(std::make_shared<DroppedExports>())
    {
        cells.SetDroppedExports(dropped);
    }
    TemplateModuleState(const TemplateModuleState&) = delete;
    TemplateModuleState(TemplateModuleState&&) = delete;
    TemplateModuleState& operator=(const TemplateModuleState&) = delete;
    TemplateModuleState& operator=(TemplateModuleState&&) = delete;
    ~TemplateModuleState()
    {
        // The module's values may hold themselves (`l.append(l)`): a collector frees those
        // cycles once nothing else holds them, as a render's does when it ends. Noting them
        // allocates; when that fails, the cycles noted so far are still freed and the rest leak.
        // The cells freeze first, noted to the collector: a cycle through one is then one among
        // the module's values (docs/tasks/0331)
        // What only the copies the closures kept held is freed rather than counted as held; a
        // cycle among it is noted first (docs/tasks/0407)
        CycleCollector cycles;
        try
        {
            kept.NoteNames();
        }
        catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment above says what is lost
        {
        }
        kept.Clear();
        cells.Release(&cycles);
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
        // So do the values of the modules its calls imported that stay with it: shared ones, and those a stored
        // closure kept (docs/tasks/0312)
        if (callback)
        {
            callback->NoteModuleNames();
        }
        // A cycle a call made among the module's values, below a value that is in none, has no
        // note left: the walk reaches it from the module's names
        cycles.WalkFromEveryNote();
        module.reset();
        callback.reset();
        // What the host let go of during the walk: nothing writes it any more
        CollectDroppedExports(dropped->Close());
    }

    // Notes to the collector running on this thread the containers of the module's calls that the
    // host let go of since (DroppedExports): its walk frees a cycle among them. When noting one
    // cannot allocate, a cycle through it and the rest leaks
    void NoteDropped() noexcept
    {
        if (dropped->MayHoldAny())
        {
            NoteDroppedExports(dropped->Take());
        }
    }

    // The poll of a run in the module: the host's (RenderOptions::poll), or the thread's without one
    [[nodiscard]] detail::HostPoll Poll() const { return poll ? &poll : detail::t_hostPoll; }

    [[nodiscard]] RenderLimitsScope MakeLimitsScope() const
    {
        const auto& settings = *impl->m_settings;
        // A poll of the module's own counts the steps to it, also where the template's limits count none
        if (poll && !HasRenderLimits(settings.settings.limits))
        {
            return RenderLimitsScope(&PollOnlyLimits(), settings.bigIntBackend);
        }
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
        if (hostGlobals)
        {
            // The host's own (RenderOptions::globals), in place of the environment's
            globals.source = hostGlobals;
            for (const auto& [name, value] : *globals.source)
            {
                globals.values[name] = OwnedArgument(value);
            }
        }
        else if (auto* env = impl->m_env)
        {
            auto& envImpl = *detail::TemplateEnvAccess::GetImpl(*env);
            {
                const detail::ReaderLock lock(envImpl.guard);
                globals.source = envImpl.globalValues;
            }
            for (const auto& [name, value] : *globals.source)
            {
                globals.values[name] = OwnedArgument(value);
            }
        }

        callback = std::make_unique<Callback>(impl.get());
        callback->SetGlobals(&globals);
        callback->SetFilterPolicies(filterPolicies.get());
        module = std::make_unique<ImportedModule>(std::move(convertedParams), globals.values, callback.get(), &GetBuiltinGlobals(impl->m_settings->settings.extensions));

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
        for (const auto& [name, value] : module->Names())
        {
            // Imported, and not set again since: not exported (Jinja2's exported_vars)
            if (name.empty() || name[0] == '_' || IsEntryBinding(value, HashedName::Hash(name)) || !module->Exports(name))
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
        return ExpiredError(name);
    }

    // A macro of a module that ended with an earlier call, called through `name`
    [[nodiscard]] BasicErrorInfo<CharT> ExpiredError(const std::string& name) const
    {
        typename BasicErrorInfo<CharT>::Data errorData;
        errorData.code = ErrorCode::TemplateExpired;
        errorData.srcLoc.fileName = impl->m_templateName;
        errorData.srcLoc.line = 1;
        errorData.srcLoc.col = 1;
        errorData.extraParams.emplace_back(name);
        errorData.extraParams.emplace_back(std::string("ended"));
        return BasicErrorInfo<CharT>(std::move(errorData));
    }

    // A run in the module for the host: a call of an export (Call) or a read of an object it exports
    // (ModuleObjectView). Holds the module's lock, as a host's call does, so it may be nested on the same thread (a host's
    // callable in a macro that reads or calls the module again). `body` gets the output string that the limits keep a
    // pointer to, and returns what the host gets, converted by the body itself: no view of the module's context, its cells
    // or the template's source may leave it. A failure is a BadResultAccess naming `name`. `policies` are the filter
    // policies of this run, or null for the module's
    template<typename Body>
    auto Run(const std::string& name, const FilterPolicies* policies, Body&& body) -> std::invoke_result_t<Body, std::basic_string<CharT>&>
    {
        std::unique_lock<std::recursive_mutex> lock(mutex, std::try_to_lock);
        if (!lock.owns_lock())
        {
            // Another thread runs in the module: the host may let go of what that thread's callables need meanwhile
            if (waitForTurn)
            {
                waitForTurn([&lock] { lock.lock(); });
            }
            if (!lock.owns_lock())
            {
                lock.lock();
            }
        }
        std::optional<BasicErrorInfo<CharT>> error;
        try
        {
            // First, so that it ends last: when nothing of the call holds values any more
            ModuleCallCollector cycles(cells, kept, &LiveRoots, this);
            // The containers of earlier calls the host let go of: this call's walk frees a cycle
            // among them (docs/tasks/0480)
            NoteDropped();
            // Then what the call loads, which its collector sees go
            CallScope callScope(*callback, *module, kept);
            // Then the module's context as the call found it, which a failed call would leave
            // with its scopes pushed
            const HomeCall::Restore restore(module->Context());
            // The host's poll of the module, before the limits, which start counting the steps to it
            const HostPollScope pollScope(Poll());
            // The call's policies, the outer call's again after it: a host's callable it calls may call the module back
            const PoliciesScope policiesScope(*callback, policies ? policies : filterPolicies.get());
            // Before the limits, which keep a pointer to it
            std::basic_string<CharT> output;
            const auto limitsScope = MakeLimitsScope();
            auto result = std::forward<Body>(body)(output);
            if (impl->m_settings->hasRenderLimits)
            {
                detail::PollDeadline();
            }
            cycles.Complete();
            callScope.Succeeded();
            return result;
        }
        catch (const std::bad_alloc&)
        {
            throw;
        }
        catch (const detail::RenderFailure& failure)
        {
            error.emplace(Named(failure.ErrorAs<CharT>(), name));
        }
        catch (...)
        {
            error.emplace(Named(detail::CurrentExceptionToError<CharT>(impl->m_templateName), name));
        }
        // Out of the handler, the exception gone: it may view a replaced tree (its message or
        // values), the error made of it is owned
        PruneAfterFailedCall();
        throw BadResultAccess<CharT>(std::move(*error));
    }

    // A host's call of the exported callable `fn`, named `name`, in the module's context, with the filter policies
    // `policies` (CallModuleMacro) or, when null, the module's
    Value Call(const Callable& fn, const std::string& name, const UserCallableParams& ucParams, const FilterPolicies* policies = nullptr)
    {
        return Run(name, policies, [&](std::basic_string<CharT>& output) {
            CallParams callParams;
            if (const auto* posArgs = ucParams.extraPosArgs.GetIf<ValuesList>())
            {
                callParams.posParams.reserve(posArgs->size());
                for (const auto& arg : *posArgs)
                {
                    callParams.posParams.push_back(OwnedArgument(arg, false, this));
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
                    callParams.kwParams[argName] = OwnedArgument(arg, false, this);
                }
            }
            else if (!ucParams.extraKwArgs.IsNone())
            {
                throw TemplateRuntimeError("the keyword arguments of a module's macro must be a ValuesMap");
            }
            for (const auto& [argName, arg] : ucParams.args)
            {
                callParams.kwParams[argName] = OwnedArgument(arg, false, this);
            }
            return MakeCopier(name).Copy(InvokeInModule(module->Context(), fn, callParams, output, impl->m_settings->hasRenderLimits));
        });
    }

    // The copier of a run's results, in the module's export mode
    ExportCopier MakeCopier(const std::string& name) { return ExportCopier(&Wrap, this, name, exportObjects, &WrapObject); }

    // A call that failed frees the trees a source change replaced as one that returned does
    // (CallScope), once its error is made (docs/tasks/0503)
    void PruneAfterFailedCall() noexcept
    {
        if (callback->calls == 0 && callback->HasReplaced())
        {
            callback->PruneReplaced(module->HoldsTemplateHandles());
        }
    }

    // ModuleScope::LiveRootsFn: the objects the module's names hold, and those the attributes of
    // a namespace among them hold, where a call keeps its state (docs/tasks/0450). Not the items
    // of a list or dict: there may be as many as calls, and gathering them would cost what the
    // walk saves
    static void LiveRoots(const void* state, std::vector<const ValueObject*>& roots)
    {
        const auto& self = *static_cast<const TemplateModuleState*>(state);
        for (const auto& [name, value] : self.module->Names())
        {
            const auto* object = detail::InternalValueAccess::Object(value);
            if (!object)
            {
                continue;
            }
            roots.push_back(object);
            const auto own = OwnObjectOf(value);
            if (own.get() != object)
            {
                roots.push_back(own.get());
            }
            if (const auto map = AsMap(value); map && map->IsNamespace())
            {
                own->Traverse([&roots](const ValueObject* attribute) { roots.push_back(attribute); });
            }
        }
    }

    // A template callable of the module found in a value handed to the host, whose errors name
    // `name`, as a call in the module: a macro in an exported dict, or one a call returns
    static Value Wrap(void* state, const InternalValue& callable, const std::string& name)
    {
        return MakeModuleCall(static_cast<TemplateModuleState*>(state)->shared_from_this(), callable, name);
    }

    // A namespace, cycler or joiner of the module found in a value handed to the host (ExportCopier::WrapObject)
    static Value WrapObject(void* state, const InternalValue& object, const std::string& name)
    {
        return MakeModuleObject(static_cast<TemplateModuleState*>(state)->shared_from_this(), object, name);
    }

    std::shared_ptr<const Impl> impl;
    // The host's poll of the module's render and of the calls of its exports (RenderOptions::poll), or empty
    std::function<void()> poll;
    // The host's globals in place of the environment's (RenderOptions::globals), or null
    std::shared_ptr<const ValuesMap> hostGlobals;
    // The host's filter policies of the module's render and of its calls (RenderOptions::filterPolicies), or null for
    // the template's; a call may pass its own (CallModuleMacro)
    std::shared_ptr<const FilterPolicies> filterPolicies;
    // ModuleOptions::exports is Objects: what the exports and the results of calls keep of the template's values
    bool exportObjects = false;
    // ModuleOptions::waitForTurn: how a thread waits for another thread's run in the module
    std::function<void(const std::function<void()>&)> waitForTurn;
    // The params, until the module takes them
    InternalValueMap convertedParams;
    GlobalsSnapshot globals;
    std::unique_ptr<Callback> callback;
    // The module's context, where the macros were defined; after the callback, which it uses
    std::unique_ptr<ImportedModule> module;
    // The module's copies of the host's data it wrote to: its values live across its calls
    // (docs/tasks/0049). Released before the module's values, by the destructor
    BorrowedCells cells;
    // What the closures its calls keep past their frames hold (CycleCollector::KeepUntilEnd):
    // the module's values may store them. Released first by the destructor
    KeptObjects kept;
    // Calls of the exports take turns: the context is not shared by concurrent calls
    std::recursive_mutex mutex;
    // The containers its calls handed out that the host let go of (docs/tasks/0480); the views
    // hold it weakly. Closed by the destructor
    std::shared_ptr<DroppedExports> dropped;
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
    Value Call(const UserCallableParams& params, const FilterPolicies* policies) const { return state->Call(*GetIf<Callable>(&callable), name, params, policies); }
};
} // namespace

template<typename CharT>
Value MakeModuleCall(const std::shared_ptr<TemplateModuleState<CharT>>& state, const InternalValue& callable, const std::string& name)
{
    return Value(UserCallable(ModuleCall<CharT>{ state, callable, name }, { ArgInfo(ArgInfo::VarArgs), ArgInfo(ArgInfo::VarKwArgs) }));
}

// A namespace, a cycler or a joiner the module made, as the host sees it in ModuleExports::Objects: the object itself, which
// the module's macros read and change, not a copy of it (Jinja2's `module.ns.count` is what a macro set last). Each read runs
// in the module, under its lock and limits, as a call of a macro does, and gives the host a copy (ExportCopier): nothing of
// the module's context, cells or source leaves it. The methods of a cycler and a joiner are calls in the module. It owns
// the module, so that the object outlives the template and every other export; the module's own values hold the object,
// never a view (a view the host hands back as an argument is the object again: OwnedArgument)
template<typename CharT>
class ModuleObjectView final : public IMapItemAccessor
    , public IModuleObjectView
{
public:
    using State = TemplateModuleState<CharT>;

    ModuleObjectView(std::shared_ptr<State> state, InternalValue object, std::string name)
        : m_state(std::move(state))
        , m_object(std::move(object))
        , m_name(std::move(name))
    {
        const auto map = AsMap(m_object);
        m_class = map ? map->GetObjectClass() : nullptr;
        m_identity = map ? map->GetIdentity() : nullptr;
    }
    ModuleObjectView(const ModuleObjectView&) = delete;
    ModuleObjectView& operator=(const ModuleObjectView&) = delete;
    ModuleObjectView(ModuleObjectView&&) = delete;
    ModuleObjectView& operator=(ModuleObjectView&&) = delete;
    // The object may hold itself (`{% set ns.me = ns %}`) and then nothing else frees it once the module lets go of it: the
    // module's next call walks from it, or the module when it goes (DroppedExports, docs/tasks/0480). Neither takes the
    // module's lock here, which a call on another thread may hold
    ~ModuleObjectView() override
    {
        const auto* object = detail::InternalValueAccess::Object(m_object);
        if (!object || !object->IsTraversable())
        {
            return;
        }
        // The module lives while this view does: the registry is open
        if (!m_state->dropped->Hand(m_object))
        {
            // Not reached while the registry is open; the destructor does not throw, so the fallback may lose the object (a leak) on bad_alloc
            try
            {
                CollectDroppedExports(std::vector<InternalValue>{ std::move(m_object) });
            }
            catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment above says what is lost
            {
            }
        }
    }

    [[nodiscard]] size_t GetSize() const override
    {
        return m_state->Run(m_name, nullptr, [this](auto&) {
            const auto map = AsMap(m_object);
            return map ? map->GetSize() : size_t{ 0 };
        });
    }
    [[nodiscard]] std::optional<Value> Find(std::string_view name) const override
    {
        return m_state->Run(m_name, nullptr, [this, name](auto&) { return Read(std::string(name)); });
    }
    [[nodiscard]] bool Contains(std::string_view name) const override { return HasValue(std::string(name)); }
    [[nodiscard]] bool HasValue(const std::string& name) const override
    {
        return m_state->Run(m_name, nullptr, [this, &name](auto&) {
            const auto map = AsMap(m_object);
            return map && map->HasValue(name);
        });
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        return m_state->Run(m_name, nullptr, [this](auto&) { return Keys(); });
    }
    [[nodiscard]] std::vector<std::pair<Value, Value>> GetItems() const override
    {
        return m_state->Run(m_name, nullptr, [this](auto&) {
            std::vector<std::pair<Value, Value>> items;
            for (const auto& key : Keys())
            {
                if (auto item = Read(key))
                {
                    items.emplace_back(Value(key), std::move(*item));
                }
            }
            return items;
        });
    }
    [[nodiscard]] const void* GetIdentity() const override { return m_identity; }
    [[nodiscard]] const char* GetObjectClass() const override { return m_class; }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* view = dynamic_cast<const ModuleObjectView*>(&other);
        return view != nullptr && view->m_state == m_state && view->m_identity == m_identity;
    }

    [[nodiscard]] const void* StateId() const noexcept override { return m_state.get(); }
    [[nodiscard]] const InternalValue& Object() const noexcept override { return m_object; }
    [[nodiscard]] Value Snapshot() const override
    {
        return m_state->Run(m_name, nullptr, [this](auto&) { return ExportCopier(&State::Wrap, m_state.get(), m_name, false).Copy(m_object); });
    }

private:
    // In a run: the names of the object's attributes
    [[nodiscard]] std::vector<std::string> Keys() const
    {
        const auto map = AsMap(m_object);
        const auto* accessor = map ? map->GetAccessor() : nullptr;
        return accessor ? accessor->GetKeys() : std::vector<std::string>();
    }

    // In a run: the attribute `key` as the host gets it, empty when the object has none. Read as the template's `obj.key`
    // reads it: a cycler's `pos` and `current` are computed. A method is a call in the module
    [[nodiscard]] std::optional<Value> Read(const std::string& key) const
    {
        const auto map = AsMap(m_object);
        if (!map || !map->HasValue(key))
        {
            return std::nullopt;
        }
        const auto item = Subscript(m_object, InternalValue(key), &m_state->module->Context());
        return m_state->MakeCopier(m_name + "." + key).Copy(item);
    }

    // The module first, so that the object, whose callables refer to its context, goes before it
    std::shared_ptr<State> m_state;
    InternalValue m_object;
    std::string m_name;
    const char* m_class = nullptr;
    const void* m_identity = nullptr;
};

template<typename CharT>
Value MakeModuleObject(const std::shared_ptr<TemplateModuleState<CharT>>& state, const InternalValue& object, const std::string& name)
{
    return Value(GenericMap(std::make_shared<ModuleObjectView<CharT>>(state, object, name)));
}

namespace
{
template<typename CharT>
const Callable* ModuleMacroOf(const UserCallable& callable)
{
    const auto* call = callable.callable.template target<ModuleCall<CharT>>();
    return call ? GetIf<Callable>(&call->callable) : nullptr;
}
} // namespace

Value CallModuleMacro(const UserCallable& callable, const UserCallableParams& params, const ModuleCallOptions& options)
{
    // The options' policies stay alive through the call, which points to them
    const std::shared_ptr<const FilterPolicies> policies = options.filterPolicies;
    if (const auto* call = callable.callable.target<ModuleCall<char>>())
    {
        return call->Call(params, policies.get());
    }
    if (const auto* call = callable.callable.target<ModuleCall<wchar_t>>())
    {
        return call->Call(params, policies.get());
    }
    return callable.callable(params);
}

ValuesMap MacroAttributes(const UserCallable& callable)
{
    const Callable* macro = ModuleMacroOf<char>(callable);
    if (!macro)
    {
        macro = ModuleMacroOf<wchar_t>(callable);
    }
    ValuesMap result;
    if (!macro || !macro->GetAttributes())
    {
        return result;
    }
    for (const auto& [name, value] : *macro->GetAttributes())
    {
        auto converted = IntValue2Value(value);
        if (name == "arguments")
        {
            // A tuple of names, as a list of values
            ValuesList names;
            if (const auto* list = converted.GetIf<GenericList>())
            {
                for (const auto& item : *list)
                {
                    names.emplace_back(item.AsString());
                }
                converted = Value(std::move(names));
            }
        }
        result.emplace(name, std::move(converted));
    }
    return result;
}

template<typename CharT>
nonstd::expected<BasicTemplateModule<CharT>, BasicErrorInfo<CharT>> TemplateImpl<CharT>::MakeModule(const std::shared_ptr<const TemplateImpl>& self, const ValuesMap& params, const RenderOptions* options, const ModuleOptions* module)
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

    auto state = std::make_shared<TemplateModuleState<CharT>>(self);
    if (options)
    {
        state->poll = options->poll;
        state->hostGlobals = options->globals;
        state->filterPolicies = options->filterPolicies;
    }
    if (module)
    {
        state->exportObjects = module->exports == ModuleExports::Objects;
        state->waitForTurn = module->waitForTurn;
    }
    BasicTemplateModule<CharT> result;
    try
    {
        // Before the limits, which start counting the steps to it
        const HostPollScope pollScope(state->Poll());
        // In the try: a param nested too deep fails as a render does
        for (const auto& [name, value] : params)
        {
            state->convertedParams[name] = OwnedArgument(value);
        }
        if (const auto* safeParams = self->m_settings->PromptSafeParamsToMark())
        {
            TemplateImpl::MarkPromptSafeParams(*safeParams, state->convertedParams);
        }
        ModuleCallCollector cycles(state->cells, state->kept);
        const auto limitsScope = state->MakeLimitsScope();
        // Into result.body, which outlives the limits that keep a pointer to it
        state->Render(result.body);
        for (const auto& [name, value] : state->ExportedNames())
        {
            if (GetIf<Callable>(value))
            {
                result.exports[name] = MakeModuleCall(state, *value, name);
                continue;
            }
            // Copied within the render's limits, which bound a lazy list read out
            result.exports[name] = state->MakeCopier(name).Copy(*value);
        }
        cycles.Complete();
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

template<typename CharT>
std::optional<BasicErrorInfo<CharT>> TemplateImpl<CharT>::LoadReplacing(std::shared_ptr<ITemplateImpl>& impl, std::basic_string<CharT>&& source, std::string&& name)
{
    auto loaded = std::make_shared<TemplateImpl>(*static_cast<TemplateImpl*>(impl.get()), nullptr);
    auto error = loaded->Load(std::move(source), std::move(name));
    if (!error)
    {
        // The template this replaces may be parsing still: host code its first Load called (an
        // autoescape selector) loads it a second time, maybe on another thread. That Load runs on
        // the template until it returns, render or not (docs/tasks/0517, 0522)
        KeepWhileLoading(impl);
        // A render on this thread (one that called the host code loading now) may be rendering
        // the template this replaces: the workspace keeps it (RenderWorkspace::Retire)
        RenderWorkspace::ForThisThread().Retire(impl);
        impl = std::move(loaded);
    }
    return error;
}

template<typename CharT>
std::optional<BasicErrorInfo<CharT>> TemplateImpl<CharT>::LoadReplacing(std::shared_ptr<ITemplateImpl>& impl, std::basic_string_view<CharT> source, std::string&& name)
{
    return LoadReplacing(impl, std::basic_string<CharT>(source), std::move(name));
}

template std::optional<BasicErrorInfo<char>> TemplateImpl<char>::LoadReplacing(std::shared_ptr<ITemplateImpl>& impl, std::string&& source, std::string&& name);
template std::optional<BasicErrorInfo<char>> TemplateImpl<char>::LoadReplacing(std::shared_ptr<ITemplateImpl>& impl, std::string_view source, std::string&& name);
template std::optional<BasicErrorInfo<wchar_t>> TemplateImpl<wchar_t>::LoadReplacing(std::shared_ptr<ITemplateImpl>& impl, std::wstring_view source, std::string&& name);
template std::optional<BasicErrorInfo<wchar_t>> TemplateImpl<wchar_t>::LoadReplacing(std::shared_ptr<ITemplateImpl>& impl, std::wstring&& source, std::string&& name);

template nonstd::expected<BasicTemplateModule<char>, BasicErrorInfo<char>> TemplateImpl<char>::MakeModule(const std::shared_ptr<const TemplateImpl>& self,
                                                                                                          const ValuesMap& params,
                                                                                                          const RenderOptions* options,
                                                                                                          const ModuleOptions* module);
template nonstd::expected<BasicTemplateModule<wchar_t>, BasicErrorInfo<wchar_t>> TemplateImpl<wchar_t>::MakeModule(const std::shared_ptr<const TemplateImpl>& self,
                                                                                                                   const ValuesMap& params,
                                                                                                                   const RenderOptions* options,
                                                                                                                   const ModuleOptions* module);

} // namespace v2
} // namespace jinja2
