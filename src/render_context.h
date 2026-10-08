#ifndef JINJA2CPP_SRC_RENDER_CONTEXT_H
#define JINJA2CPP_SRC_RENDER_CONTEXT_H

#include "internal_value.h"
#include "lookup_result.h"
#include "node_arena.h"
#include "slot_frame.h"
#include "template_slots.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>

#include <nonstd/expected.hpp>

#include <boost/container/small_vector.hpp>
#include <boost/core/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <variant>
#include <vector>

// A name lookup is inlined whole into the name expression that makes it: compilers otherwise
// split it at different points, and a lookup made out of line takes the address of the
// name, which costs the caller a stack protector (docs/tasks/0129)
// The rare paths it calls stay in the header too, out of line: the library exports nothing
// from src/, and the tests use RenderContext against the shared library as well
#ifdef _MSC_VER
#define JINJA2CPP_ALWAYS_INLINE __forceinline
#define JINJA2CPP_NOINLINE_INLINE __declspec(noinline) inline
#else
#define JINJA2CPP_ALWAYS_INLINE inline __attribute__((always_inline))
#define JINJA2CPP_NOINLINE_INLINE __attribute__((noinline)) inline
#endif

namespace jinja2
{
template<typename CharT>
class TemplateImpl;

struct TemplateFrame;

// A `break` or `continue` on its way to the loop it belongs to: set by the statement, it
// stops the bodies that enclose it until the loop takes it
enum class LoopControl
{
    None,
    Break,
    Continue
};

struct IRendererCallback : IComparable
{
    ~IRendererCallback() override = default;
    virtual TargetString GetAsTargetString(const InternalValue& val) = 0;
    // Whether the template renders to std::wstring, so that a string value of that width can be
    // escaped or written without rendering it first
    [[nodiscard]] virtual bool IsWideTarget() const = 0;
    virtual OutStream GetStreamOnString(TargetString& str) = 0;
    using LoadTemplateResult = std::variant<EmptyValue,
                                            nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
                                            nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>>;
    // The template `include`, `extends` or `import` names. A render resolves each name once and keeps the result
    // until it ends, so the reference stays valid for the rest of the render (docs/tasks/0105)
    [[nodiscard]] virtual const LoadTemplateResult& LoadTemplate(const std::string& fileName) const = 0;
    [[nodiscard]] virtual const LoadTemplateResult& LoadTemplate(const InternalValue& fileName) const = 0;
    // Always throws: callers rely on it not returning (docs/tasks/0055)
    [[noreturn]] virtual void ThrowRuntimeError(ErrorCode code, ValuesList extraParams) = 0;
    [[nodiscard]] virtual const Settings& GetSettings() const = 0;
    // The environment the template was loaded in, if any
    [[nodiscard]] virtual TemplateEnv* GetEnv() const { return nullptr; }
    // The random generator of lipsum, one per render
    virtual std::minstd_rand& GetRandomEngine() = 0;
    // A value of the global scope is about to be changed in place: the scope must not be
    // reused by a later render (docs/tasks/0139)
    virtual void GlobalScopeWritten() {}

    // The templates this render runs, for the handles that link to their nodes (0118 P4b)
    virtual TemplateSlots& Templates() = 0;
};

// The slots where names were last found (docs/tasks/0100 idea 7). An entry holds the slot
// a name expression resolved to and the epoch of the context it was looked up in; a context
// takes a new epoch whenever a lookup could resolve elsewhere (a name added to a scope, a
// scope cleared, left or bound) and on each copy, so an entry of an older epoch is never
// used. Epochs never repeat, so one cache serves every render on a thread, and a render
// does not pay for clearing it.
//
// Nothing drops the entries of a template that goes away (0118 P5b): an entry is keyed by the
// address of a name expression, and a render keeps every template whose nodes it runs alive
// until it ends (TemplateSlots, LoadedTemplates), while each render starts with an epoch of
// its own. So another tree at the same addresses is seen only under a later epoch, which
// no entry of the freed one holds. A render must not make nodes, nor run a tree it does not
// keep alive, under an epoch that outlives the tree.
class LookupCache
{
public:
    static LookupCache& ForThisThread()
    {
        thread_local LookupCache cache;
        return cache;
    }

    struct Entry
    {
        const void* key = nullptr;
        uint64_t epoch = 0;
        LookupResult slot;
    };

    uint64_t NewEpoch() { return ++m_lastEpoch; }
    // The entry a new name expression uses. Expressions take entries in turn, so the names of
    // a template share none until it has more than Size of them. Hashing the address of the
    // expression made two names in one loop share an entry or not depending on where the heap
    // put them, and the work of a loop with it (up to +3.5% instructions, docs/tasks/0139)
    static uint32_t NewSlot()
    {
        static std::atomic<uint32_t> next{ 0 };
        return next.fetch_add(1, std::memory_order_relaxed) % Size;
    }
    // `key` is the expression that looks the name up, `slot` its NewSlot
    Entry& At(uint32_t slot) { return m_entries[slot]; }

private:
    static constexpr uint32_t Size = 128;
    uint64_t m_lastEpoch = 0;
    std::array<Entry, Size> m_entries{};
};

class RenderContext;

// A scope of a RenderContext open for writing. Every write to a scope goes through it, so
// that adding a name starts a new lookup epoch; changing the value of a name already there
// does not, since its slot stays in place.
class ScopeRef
{
public:
    ScopeRef(RenderContext& context, InternalValueMap& map)
        : m_context(&context)
        , m_map(&map)
    {
    }

    template<typename Key>
    InternalValue& operator[](Key&& name);
    // By a name the parse tree holds, with its hash
    InternalValue& ForName(const HashedName& name);
    void Erase(const std::string& name);
    void Clear();
    [[nodiscard]] bool empty() const { return m_map->empty(); }
    [[nodiscard]] const InternalValueMap& Map() const { return *m_map; }

private:
    RenderContext* m_context;
    InternalValueMap* m_map;
};

// The scopes of a RenderContext, innermost last, in chunks of ChunkSize. The first chunk
// lives inside the context, so a nested context (an include, a block, a `with`) allocates
// nothing for its scopes (docs/tasks/0129); deeper scopes go to chunks allocated on first
// use and kept until the context ends. A scope stays where it is until it is removed: the
// macro code and ScopeRef hold references to a scope across EnterScope, and lookups keep
// pointers to the values in it (docs/tasks/0088, 0104).
class ScopeStack
{
public:
    // Enough for a loop in a loop in a macro called in a loop, with a `with` or two
    static constexpr size_t ChunkSize = 8;

    ScopeStack() noexcept = default;
    // Delegates, so that the scopes copied so far are destroyed if a copy throws
    ScopeStack(const ScopeStack& other)
        : ScopeStack()
    {
        for (size_t idx = 0; idx != other.m_size; ++idx)
        {
            Push(other[idx]);
        }
    }
    ScopeStack(ScopeStack&&) = delete;
    ScopeStack& operator=(const ScopeStack&) = delete;
    ScopeStack& operator=(ScopeStack&&) = delete;
    ~ScopeStack()
    {
        while (m_size)
        {
            pop_back();
        }
    }

    [[nodiscard]] size_t size() const { return m_size; }
    [[nodiscard]] bool empty() const { return m_size == 0; }

    template<typename... Args>
    InternalValueMap& Push(Args&&... args)
    {
        auto* slot = m_size < ChunkSize ? &m_first.maps[m_size] : DeepSlot();
        auto* result = new (slot) InternalValueMap(std::forward<Args>(args)...);
        ++m_size;
        return *result;
    }
    void pop_back()
    {
        --m_size;
        (*this)[m_size].~InternalValueMap();
    }
    InternalValueMap& back() { return (*this)[m_size - 1]; }
    InternalValueMap& operator[](size_t idx) { return idx < ChunkSize ? m_first.maps[idx] : m_more[(idx / ChunkSize) - 1]->maps[idx % ChunkSize]; }
    const InternalValueMap& operator[](size_t idx) const
    {
        return idx < ChunkSize ? m_first.maps[idx] : m_more[(idx / ChunkSize) - 1]->maps[idx % ChunkSize];
    }

    // The scopes of the first chunk, the first min(size(), ChunkSize) of the stack
    [[nodiscard]] const InternalValueMap* FirstChunk() const { return m_first.maps; }

    bool operator==(const ScopeStack& other) const
    {
        if (m_size != other.m_size)
        {
            return false;
        }
        for (size_t idx = 0; idx != m_size; ++idx)
        {
            if ((*this)[idx] != other[idx])
            {
                return false;
            }
        }
        return true;
    }
    bool operator!=(const ScopeStack& other) const { return !(*this == other); }

private:
    // Raw storage: only the scopes below m_size are constructed
    union Chunk
    {
        Chunk() noexcept {}
        ~Chunk() {}
        Chunk(const Chunk&) = delete;
        Chunk(Chunk&&) = delete;
        Chunk& operator=(const Chunk&) = delete;
        Chunk& operator=(Chunk&&) = delete;
        InternalValueMap maps[ChunkSize];
    };

    // Where scope m_size goes when it is past the first chunk; allocates the chunk if needed
    JINJA2CPP_NOINLINE_INLINE InternalValueMap* DeepSlot()
    {
        const size_t chunk = m_size / ChunkSize;
        if (chunk > m_more.size())
        {
            m_more.push_back(std::make_unique<Chunk>());
        }
        return &m_more[chunk - 1]->maps[m_size % ChunkSize];
    }

    Chunk m_first;
    size_t m_size = 0;
    std::vector<std::unique_ptr<Chunk>> m_more;
};

class RenderContext
{
public:
    // `builtins`, when given, is the last scope: the default globals shared by all renders
    RenderContext(const InternalValueMap& extValues,
                  const InternalValueMap& globalValues,
                  IRendererCallback* rendererCallback,
                  const InternalValueMap* builtins = nullptr)
        : m_rendererCallback(rendererCallback)
        , m_externalScope(&extValues)
        , m_globalScope(&globalValues)
        , m_builtinScope(builtins)
    {
        EnterScope();
    }

    RenderContext(const RenderContext& other)
        : m_rendererCallback(other.m_rendererCallback)
        , m_externalScope(other.m_externalScope)
        , m_globalScope(other.m_globalScope)
        , m_builtinScope(other.m_builtinScope)
        , m_boundScope(other.m_boundScope)
        , m_templateFrame(other.m_templateFrame)
        , m_nodes(other.m_nodes)
        , m_parent(other.m_parent)
        , m_parentDepth(other.m_parentDepth)
        , m_boundDepth(other.m_boundDepth)
        , m_scopes(other.m_scopes)
        , m_frame(other.m_frame)
        , m_views(other.m_views)
        , m_fullWalk(other.m_fullWalk)
        , m_inheritsViews(other.m_inheritsViews)
        , m_autoescape(other.m_autoescape)
        , m_lookupCache(other.m_lookupCache)
    {
        m_currentScope = &m_scopes.back();
        NewEpoch();
    }
    // A move is the copy above: m_currentScope must point into this object's m_scopes.
    // NOLINTNEXTLINE(performance-noexcept-move-constructor,bugprone-exception-escape): copying the scopes can throw
    RenderContext(RenderContext&& other)
        : RenderContext(static_cast<const RenderContext&>(other)) // NOLINT(performance-move-constructor-init)
    {
    }
    RenderContext& operator=(const RenderContext&) = delete;
    RenderContext& operator=(RenderContext&&) = delete;
    ~RenderContext() = default;

    // A context that sees the first `depth` scopes of `other`, plus a fresh one of its own on
    // top. It refers to those scopes instead of copying them (docs/tasks/0108), so it must
    // not outlive `other`, and `other` must not add or remove scopes while it is in use: the
    // child is always rendered inside the call that made it. Names it sets go to its own
    // scopes; a value changed in place (`d.update(...)`) is changed where it is stored, as
    // in Jinja2, where the context is a shallow copy.
    RenderContext(RenderContext& other, size_t depth)
        : m_rendererCallback(other.m_rendererCallback)
        , m_externalScope(other.m_externalScope)
        , m_globalScope(other.m_globalScope)
        , m_builtinScope(other.m_builtinScope)
        , m_boundScope(other.m_boundScope)
        , m_templateFrame(other.m_templateFrame)
        , m_nodes(other.m_nodes)
        , m_parent(&other)
        , m_parentDepth(std::min(depth, other.GetScopesCount()))
        , m_boundDepth(std::min(other.m_boundDepth, m_parentDepth))
        , m_frame(other.m_frame)
        , m_inheritsViews(other.m_inheritsViews || !other.m_views.empty())
        , m_autoescape(other.m_autoescape)
        , m_lookupCache(other.m_lookupCache)
    {
        m_fullWalk = m_boundScope != nullptr || m_inheritsViews;
        // Skip the parents whose own scopes are all hidden, so chains stay short
        while (m_parent->m_parent && m_parentDepth <= m_parent->m_parentDepth)
        {
            m_parent = m_parent->m_parent;
        }
        NewEpoch();
        EnterScope();
    }

    // Lookups in this context and the ones copied from it are cached in `cache`
    void SetLookupCache(LookupCache* cache)
    {
        m_lookupCache = cache;
        NewEpoch();
    }
    // Forgets the cached lookups: a name may now resolve to another slot
    void NewEpoch()
    {
        if (m_lookupCache)
        {
            m_epoch = m_lookupCache->NewEpoch();
        }
    }

    // An empty scope changes no lookup until a name is added to it
    ScopeRef EnterScope()
    {
        if (m_spareScope)
        {
            m_currentScope = &m_scopes.Push(std::move(*m_spareScope));
            m_spareScope.reset();
        }
        else
        {
            m_currentScope = &m_scopes.Push();
        }
        return { *this, *m_currentScope };
    }

    // Enters a scope made of `map`, names and all: the scope a loop kept from its last run
    // (docs/tasks/0133)
    ScopeRef EnterScope(InternalValueMap&& map)
    {
        const bool hasNames = !map.empty();
        m_currentScope = &m_scopes.Push(std::move(map));
        if (hasNames)
        {
            NewEpoch();
        }
        return { *this, *m_currentScope };
    }

    // Leaves the innermost scope, moving its map with its names and nodes into `map`
    void ExitScope(InternalValueMap& map)
    {
        // A view attached to the scope would attach to the next scope pushed at its index
        assert(m_views.empty() || m_views.back().scopeIndex < m_scopes.size() - 1);
        auto& scope = m_scopes.back();
        if (!scope.empty())
        {
            NewEpoch();
        }
        map = std::move(scope);
        m_scopes.pop_back();
        m_currentScope = m_scopes.empty() ? nullptr : &m_scopes.back();
    }

    void ExitScope()
    {
        // A view attached to the scope would attach to the next scope pushed at its index
        assert(m_views.empty() || m_views.back().scopeIndex < m_scopes.size() - 1);
        auto& scope = m_scopes.back();
        if (!scope.empty())
        {
            NewEpoch();
            scope.clear();
        }
        // The emptied map keeps its table and nodes for the next scope: a loop inside a loop
        // or a macro called in a loop then allocates nothing for its scope. A big table is
        // let go, since clearing it costs its size
        if (scope.mask() != 0 && scope.mask() <= MaxSpareScopeMask && !m_spareScope)
        {
            m_spareScope.emplace(std::move(scope));
        }
        m_scopes.pop_back();
        if (!m_scopes.empty())
        {
            m_currentScope = &m_scopes.back();
        }
        else
        {
            m_currentScope = nullptr;
        }
    }

    // The value of the name `val` (a std::string or a HashedName) in the innermost scope that
    // has it; none when no scope does
    template<typename Key>
    [[nodiscard]] JINJA2CPP_ALWAYS_INLINE LookupResult FindValue(const Key& val) const
    {
        if (m_fullWalk)
        {
            // Frame views without a bound module are the common case, walked almost as fast
            if (!m_boundScope)
            {
                return FindValueWithViews(ToHashedName(val));
            }
            return FindValueFull(ToHashedName(val));
        }
        const auto* p = FindEntry(val);
        return p ? LookupResult(p->second) : LookupResult();
    }

    // FindValue for the name expression `key`, through the lookup cache. Only names that are
    // found are cached: an undefined name is rare and may be defined by the next statement.
    JINJA2CPP_ALWAYS_INLINE LookupResult FindValueCached(const void* key, uint32_t slot, const HashedName& name)
    {
        return FindValueCached(key, slot, [&name] { return name; });
    }
    // nameOf() gives the HashedName, read only on a miss
    template<typename NameOf>
    JINJA2CPP_ALWAYS_INLINE LookupResult FindValueCached(const void* key, uint32_t slot, const NameOf& nameOf)
    {
        LookupCache::Entry* entry = nullptr;
        if (m_lookupCache)
        {
            // Epochs are unique per cache, not across caches: a context is used only on the
            // thread whose cache it holds
            assert(m_lookupCache == &LookupCache::ForThisThread());
            entry = &m_lookupCache->At(slot);
            if (entry->key == key && entry->epoch == m_epoch)
            {
                assert(FindValue(nameOf()).IsSame(entry->slot));
                return entry->slot;
            }
        }
        const auto result = FindValue(nameOf());
        if (result && entry)
        {
            *entry = { key, m_epoch, result };
        }
        return result;
    }

    // The variable `self` (docs/tasks/0139): a name set in the scopes of the running template
    // wins, else the template itself, made on first use (Jinja2's TemplateReference). Defined
    // with TemplateFrame in statements.cpp
    LookupResult FindSelf(const std::string& name);

    // Where the variable `name` is stored, so that a list or dict the template changes in
    // place can be stored back (docs/tasks/0020); none when it is not found or cannot be
    // written. The external and global scopes are copies made for this render, so writing
    // to them never changes the caller's data; the global scope is kept for the next render
    // on the thread, so its renderer is told (docs/tasks/0139). The built-in scope is shared
    // and never written. A name of the bound module is not writable.
    MutableLookupResult FindForWrite(const std::string& name)
    {
        MutableLookupResult result;
        const auto hashed = ToHashedName(name);
        const bool isFound = VisitScopes(
            *this,
            [&](InternalValueMap* scope, size_t /*depth*/) {
                if (!scope)
                {
                    return m_boundScope->find(name) != m_boundScope->end();
                }
                auto valP = scope->find(name);
                if (valP == scope->end())
                {
                    return false;
                }
                result = MutableLookupResult(valP->second);
                return true;
            },
            [&](const FrameView& view, size_t /*depth*/) {
                const auto idx = FindInView(view, hashed);
                if (idx == view.names.size())
                {
                    return false;
                }
                result = MutableLookupResult(view.slots[idx]);
                return true;
            });
        if (isFound)
        {
            return result;
        }
        auto* external = const_cast<InternalValueMap*>(m_externalScope);
        auto valP = external->find(name);
        if (valP != external->end())
        {
            return MutableLookupResult(valP->second);
        }
        auto* global = const_cast<InternalValueMap*>(m_globalScope);
        auto globalP = global->find(name);
        if (globalP == global->end())
        {
            return {};
        }
        if (m_rendererCallback)
        {
            m_rendererCallback->GlobalScopeWritten();
        }
        return MutableLookupResult(globalP->second);
    }

    [[nodiscard]] const InternalValueMap& GetCurrentScope() const
    {
        return *m_currentScope;
    }

    ScopeRef GetCurrentScope()
    {
        return { *this, *m_currentScope };
    }
    // Moves the names out of the current scope, leaving it empty
    InternalValueMap TakeCurrentScope()
    {
        NewEpoch();
        return std::move(*m_currentScope);
    }
    // The scopes this context sees, its parents' included
    [[nodiscard]] size_t GetScopesCount() const
    {
        return m_parentDepth + m_scopes.size();
    }
    // Resolves the parse-tree handles of the template whose code runs here (0118)
    [[nodiscard]] const ArenaView& Nodes() const { return m_nodes; }
    // Switches to another template's tree; returns the previous view (ArenaSwitch)
    ArenaView SetNodes(ArenaView nodes)
    {
        std::swap(nodes, m_nodes);
        return nodes;
    }

    auto GetRendererCallback()
    {
        return m_rendererCallback;
    }
    // The environment the template was loaded in, if any
    [[nodiscard]] TemplateEnv* GetEnv() const
    {
        return m_rendererCallback ? m_rendererCallback->GetEnv() : nullptr;
    }
    // A context for a nested render: with `includeCurrentContext` it sees all of this
    // context's names (see the constructor above for how long it may live), otherwise only
    // the global ones
    [[nodiscard]] RenderContext Clone(bool includeCurrentContext)
    {
        if (!includeCurrentContext)
        {
            RenderContext result(m_emptyScope, *m_globalScope, m_rendererCallback, m_builtinScope);
            result.m_templateFrame = m_templateFrame;
            result.m_nodes = m_nodes;
            result.m_autoescape = m_autoescape;
            result.SetLookupCache(m_lookupCache);
            return result;
        }

        return { *this, GetScopesCount() };
    }

    // The template whose code is running: its blocks and the parent set by `extends`
    [[nodiscard]] TemplateFrame* GetTemplateFrame() const
    {
        return m_templateFrame;
    }
    TemplateFrame* SetTemplateFrame(TemplateFrame* frame)
    {
        std::swap(frame, m_templateFrame);
        return frame;
    }

    [[nodiscard]] LoopControl GetLoopControl() const { return m_loopControl; }
    [[nodiscard]] bool HasLoopControl() const { return m_loopControl != LoopControl::None; }
    void SetLoopControl(LoopControl control) { m_loopControl = control; }
    // Takes the pending loop control, leaving none
    LoopControl TakeLoopControl() { return std::exchange(m_loopControl, LoopControl::None); }
    // Whether `{{ }}` output is HTML-escaped here (Jinja2's eval_ctx.autoescape)
    [[nodiscard]] bool IsAutoescape() const { return m_autoescape; }
    bool SetAutoescape(bool autoescape)
    {
        std::swap(autoescape, m_autoescape);
        return autoescape;
    }

    // Makes the names of the module `scope` visible to the imported macro about to be called
    // in this context: below the scopes the macro enters, above the ones it was called in
    void BindScope(InternalValueMap* scope)
    {
        m_boundScope = scope;
        m_boundDepth = GetScopesCount();
        m_fullWalk = true;
        NewEpoch();
    }

    // The slots of the unit running here (docs/design/0117-name-slots-plan.md)
    [[nodiscard]] const SlotFrame& Frame() const { return m_frame; }
    // The value of slot `index` of `unit`'s frame (0117 P1). Nothing when another unit's
    // frame is installed or the slot is unbound: the caller then looks the name up, so a
    // read that misses costs a lookup, never a wrong value. Never an unbound slot's value
    [[nodiscard]] LookupResult ReadSlot(SlotIndex index, UnitId unit) const
    {
        if (m_frame.unit != unit || index.value >= m_frame.slots.size())
        {
            return {};
        }
        const Slot& slot = m_frame.slots[index.value];
        return slot.IsBound() ? LookupResult(slot) : LookupResult();
    }
    // Makes `frame` the unit's frame, returning the one it replaces
    SlotFrame InstallFrame(const SlotFrame& frame) { return std::exchange(m_frame, frame); }
    // Lets lookups by name see the slots of `view`, as if they were names of the current
    // scope set below the ones its map has. The slots are unbound until engaged, so pushing
    // changes no lookup
    void PushFrameView(const FrameView& view)
    {
        assert(!m_scopes.empty() && (m_views.empty() || m_views.back().scopeIndex <= m_scopes.size() - 1));
        // A lookup by name reads the slot of each name
        assert(view.slots.size() >= view.names.size());
        m_views.push_back({ view, m_scopes.size() - 1 });
        m_fullWalk = true;
    }
    // Removes the view pushed last; its slots are unbound by then
    void PopFrameView()
    {
        m_views.pop_back();
        UpdateFullWalk();
    }
    // The views pushed here, so that a statement leaving by an exception can drop its own
    [[nodiscard]] size_t ViewCount() const { return m_views.size(); }
    void TruncateViews(size_t count)
    {
        m_views.resize(std::min(count, m_views.size()));
        UpdateFullWalk();
    }
    // Binds `slot`. A slot bound for the first time may hide a name found before, so it
    // starts a new lookup epoch; giving a bound slot another value does not
    template<typename Value>
    void BindSlot(Slot& slot, Value&& value)
    {
        if (!slot.IsBound())
        {
            NewEpoch();
        }
        slot.Bind(std::forward<Value>(value));
    }
    // Unbinds `slots`, with one new lookup epoch if any was bound
    void UnbindSlots(boost::span<Slot> slots)
    {
        bool isChanged = false;
        for (auto& slot : slots)
        {
            if (slot.IsBound())
            {
                slot.Unbind();
                isChanged = true;
            }
        }
        if (isChanged)
        {
            NewEpoch();
        }
    }

    [[nodiscard]] bool IsEqual(const RenderContext& other) const
    {
        if (!IsEqual(m_rendererCallback, other.m_rendererCallback))
        {
            return false;
        }
        if (!IsEqual(this->m_currentScope, other.m_currentScope))
        {
            return false;
        }
        if (!IsEqual(m_externalScope, other.m_externalScope))
        {
            return false;
        }
        if (!IsEqual(m_globalScope, other.m_globalScope))
        {
            return false;
        }
        if (m_builtinScope != other.m_builtinScope)
        {
            return false;
        }
        if (!IsEqual(m_boundScope, other.m_boundScope))
        {
            return false;
        }
        if (m_emptyScope != other.m_emptyScope)
        {
            return false;
        }
        if (m_parent != other.m_parent || m_parentDepth != other.m_parentDepth || m_boundDepth != other.m_boundDepth)
        {
            return false;
        }
        if (m_scopes != other.m_scopes || m_views != other.m_views)
        {
            return false;
        }
        return m_autoescape == other.m_autoescape;
    }

private:
    // The entry of the name `val` in the innermost scope that has it; null when none does
    template<typename Key>
    [[nodiscard]] JINJA2CPP_ALWAYS_INLINE const InternalValueMap::value_type* FindEntry(const Key& val) const
    {
        return FindEntryFrom(this, m_scopes.size(), m_parentDepth, val);
    }
    // FindEntry from the `count` innermost scopes of `ctx` (this context or a parent it sees
    // `limit` scopes of) outwards
    template<typename Key>
    [[nodiscard]] JINJA2CPP_ALWAYS_INLINE const InternalValueMap::value_type* FindEntryFrom(const RenderContext* ctx, size_t count, size_t limit, const Key& val) const
    {
        auto finder = [&val](const InternalValueMap& map) { return FindIn(map, val); };

        // The scopes of this context, then the ones it sees of its parents. Scopes past the
        // first chunk are rare and searched out of line
        for (; ctx;)
        {
            if (count > ScopeStack::ChunkSize)
            {
                const auto* result = FindInDeepScopes(ctx->m_scopes, count, ToHashedName(val));
                if (result)
                {
                    return result;
                }
                count = ScopeStack::ChunkSize;
            }
            const auto* first = ctx->m_scopes.FirstChunk();
            for (const auto* p = first + count; p != first;)
            {
                --p;
                const auto* result = finder(*p);
                if (result)
                {
                    return result;
                }
            }
            ctx = ctx->m_parent;
            if (ctx)
            {
                count = ctx->VisibleScopesCount(limit);
                limit = std::min(limit, ctx->m_parentDepth);
            }
        }

        // Then the external, global and built-in scopes
        const InternalValueMap* map = m_externalScope;
        for (int idx = 0; map; ++idx)
        {
            const auto* result = finder(*map);
            if (result)
            {
                return result;
            }
            map = idx == 0 ? m_globalScope : (idx == 1 ? m_builtinScope : nullptr);
        }
        return nullptr;
    }

    template<typename Key>
    JINJA2CPP_ALWAYS_INLINE static const InternalValueMap::value_type* FindIn(const InternalValueMap& map, const Key& name)
    {
        // An empty scope (a loop body without `set`) is skipped without hashing the name
        if (map.empty())
        {
            return nullptr;
        }
        auto p = map.find(name);
        return p != map.end() ? &*p : nullptr;
    }
    // The innermost of the scopes this context sees from index `minDepth` up that has `name`;
    // the bound module is not one of them
    [[nodiscard]] LookupResult FindInScopesFrom(const std::string& name, size_t minDepth) const
    {
        LookupResult result;
        const auto hashed = ToHashedName(name);
        VisitScopes(
            *this,
            [&](const InternalValueMap* scope, size_t depth) {
                if (!scope)
                {
                    return false;
                }
                if (depth < minDepth)
                {
                    return true;
                }
                const auto* p = FindIn(*scope, hashed);
                if (p)
                {
                    result = LookupResult(p->second);
                }
                return p != nullptr;
            },
            [&](const FrameView& view, size_t depth) {
                if (depth < minDepth)
                {
                    return true;
                }
                const auto idx = FindInView(view, hashed);
                if (idx == view.names.size())
                {
                    return false;
                }
                result = LookupResult(view.slots[idx]);
                return true;
            });
        return result;
    }
    // `name` is taken by value: a pointer to the caller's copy would make it a stack variable
    // The innermost of the scopes [ScopeStack::ChunkSize, count) of `scopes` that has `name`
    JINJA2CPP_NOINLINE_INLINE static const InternalValueMap::value_type* FindInDeepScopes(const ScopeStack& scopes, size_t count, HashedName name)
    {
        for (; count > ScopeStack::ChunkSize; --count)
        {
            const auto& scope = scopes[count - 1];
            auto p = scope.find(name);
            if (p != scope.end())
            {
                return &*p;
            }
        }
        return nullptr;
    }
    // Calls `onMap` with each scope `ctx` sees, innermost first, and with null where the bound
    // module scope goes, and `onView` with the views of each scope right after its map, until
    // one returns true; returns whether one did. Both also get the scope's index among the
    // scopes `ctx` sees
    template<typename Context, typename OnMap, typename OnView>
    static bool VisitScopes(Context& ctx, const OnMap& onMap, const OnView& onView)
    {
        bool isBoundSeen = !ctx.m_boundScope;
        size_t limit = ctx.GetScopesCount();
        for (auto* cur = &ctx; cur; cur = cur->m_parent)
        {
            // The views of the scopes not visited yet are below this index
            size_t views = cur->m_views.size();
            for (size_t idx = cur->VisibleScopesCount(limit); idx != 0; --idx)
            {
                const size_t depth = cur->m_parentDepth + idx - 1;
                if (!isBoundSeen && depth < ctx.m_boundDepth)
                {
                    isBoundSeen = true;
                    if (onMap(nullptr, depth))
                    {
                        return true;
                    }
                }
                if (onMap(&cur->m_scopes[idx - 1], depth))
                {
                    return true;
                }
                // The views of a scope a child does not see are passed over
                for (; views != 0 && cur->m_views[views - 1].scopeIndex >= idx - 1; --views)
                {
                    const auto& view = cur->m_views[views - 1];
                    if (view.scopeIndex == idx - 1 && onView(view.view, depth))
                    {
                        return true;
                    }
                }
            }
            limit = std::min(limit, cur->m_parentDepth);
        }
        return !isBoundSeen && onMap(nullptr, size_t{ 0 });
    }
    // The index of the bound slot of `view` named `name`; the size of the view when none is
    template<typename Name>
    static size_t FindInView(const FrameView& view, const Name& name)
    {
        for (size_t idx = 0; idx != view.names.size(); ++idx)
        {
            const auto& slotName = view.names[idx];
            if (slotName.hash == name.hash && NameEqual::Equal(view.nodes.Text(slotName.name), name.name) && view.slots[idx].IsBound())
            {
                return idx;
            }
        }
        return view.names.size();
    }
    // FindValue for the contexts the inlined walk does not cover: inside an imported macro
    // its own scopes come first, then its module, then the scopes it was called in
    // (docs/tasks/0038; 0117 P0), and the slots of frame views are searched with the scopes
    // they belong to. Out of line, so that the common path stays short; `val` is taken by
    // value for the same reason as in FindInDeepScopes
    [[nodiscard]] JINJA2CPP_NOINLINE_INLINE LookupResult FindValueFull(HashedName val) const
    {
        LookupResult result;
        if (VisitScopes(
                *this,
                [&](const InternalValueMap* scope, size_t /*depth*/) {
                    const auto* p = FindIn(scope ? *scope : *m_boundScope, val);
                    if (p)
                    {
                        result = LookupResult(p->second);
                    }
                    return p != nullptr;
                },
                [&](const FrameView& view, size_t /*depth*/) {
                    const auto idx = FindInView(view, val);
                    if (idx == view.names.size())
                    {
                        return false;
                    }
                    result = LookupResult(view.slots[idx]);
                    return true;
                }))
        {
            return result;
        }
        const InternalValueMap* map = m_externalScope;
        for (int idx = 0; map; ++idx)
        {
            const auto* p = FindIn(*map, val);
            if (p)
            {
                return LookupResult(p->second);
            }
            map = idx == 0 ? m_globalScope : (idx == 1 ? m_builtinScope : nullptr);
        }
        return {};
    }
    // FindValue for a context that sees frame views and no bound module: each scope with the
    // views attached to it, innermost first, through the parents, then the external, global
    // and built-in scopes
    [[nodiscard]] JINJA2CPP_NOINLINE_INLINE LookupResult FindValueWithViews(HashedName val) const
    {
        size_t count = m_scopes.size();
        size_t limit = m_parentDepth;
        for (const auto* ctx = this; ctx;)
        {
            if (const auto result = ctx->FindInScopesWithViews(count, val))
            {
                return result;
            }
            ctx = ctx->m_parent;
            if (ctx)
            {
                count = ctx->VisibleScopesCount(limit);
                limit = std::min(limit, ctx->m_parentDepth);
            }
        }
        const auto* p = FindEntryFrom(static_cast<const RenderContext*>(nullptr), 0, 0, val);
        return p ? LookupResult(p->second) : LookupResult();
    }
    // The innermost of the `count` innermost scopes of this context, and the views attached to
    // them, that has `val`
    [[nodiscard]] LookupResult FindInScopesWithViews(size_t count, const HashedName& val) const
    {
        // The views of the scopes a child does not see are passed over
        size_t views = m_views.size();
        for (size_t idx = count; idx != 0; --idx)
        {
            if (const auto* p = FindIn(m_scopes[idx - 1], val))
            {
                return LookupResult(p->second);
            }
            for (; views != 0 && m_views[views - 1].scopeIndex >= idx - 1; --views)
            {
                const auto& view = m_views[views - 1];
                const auto slot = view.scopeIndex == idx - 1 ? FindInView(view.view, val) : view.view.names.size();
                if (slot != view.view.names.size())
                {
                    return LookupResult(view.view.slots[slot]);
                }
            }
        }
        return {};
    }
    void UpdateFullWalk() { m_fullWalk = m_boundScope != nullptr || m_inheritsViews || !m_views.empty(); }
    static HashedName ToHashedName(const HashedName& name) { return name; }
    static HashedName ToHashedName(const std::string& name) { return { name, HashedName::Hash(name) }; }

    // How many of the scopes of this context a child seeing `limit` scopes sees
    [[nodiscard]] size_t VisibleScopesCount(size_t limit) const
    {
        if (limit >= m_parentDepth + m_scopes.size())
        {
            return m_scopes.size();
        }
        return limit > m_parentDepth ? limit - m_parentDepth : 0;
    }

    static bool IsEqual(const IRendererCallback* lhs, const IRendererCallback* rhs)
    {
        if (lhs && rhs)
        {
            return lhs->IsEqual(*rhs);
        }
        if ((!lhs && rhs) || (lhs && !rhs))
        {
            return false;
        }
        return true;
    }

    static bool IsEqual(const InternalValueMap* lhs, const InternalValueMap* rhs)
    {
        if (lhs && rhs)
        {
            return *lhs == *rhs;
        }
        if ((!lhs && rhs) || (lhs && !rhs))
        {
            return false;
        }
        return true;
    }

    IRendererCallback* m_rendererCallback{};
    InternalValueMap* m_currentScope{};
    const InternalValueMap* m_externalScope{};
    const InternalValueMap* m_globalScope{};
    const InternalValueMap* m_builtinScope{};
    const InternalValueMap* m_boundScope{};
    TemplateFrame* m_templateFrame{};
    ArenaView m_nodes;
    InternalValueMap m_emptyScope;
    LoopControl m_loopControl = LoopControl::None;
    // The context this one was made from and how many of its scopes this one sees, below
    // its own; null for a context that copies no scopes
    RenderContext* m_parent{};
    size_t m_parentDepth{};
    // How many of the scopes this context sees lie below m_boundScope: the scopes above it
    // belong to the imported macro and are searched before it
    size_t m_boundDepth{};
    ScopeStack m_scopes;
    SlotFrame m_frame;
    // The frame views attached to this context's scopes, by scope index
    boost::container::small_vector<ScopeView, 3> m_views;
    // Whether lookups take FindValueFull: there is a bound module or a frame view in sight
    bool m_fullWalk = false;
    // Whether a parent this context sees scopes of has frame views. Set once, when the
    // context is made: the parent pushes and pops no view while this context is alive
    bool m_inheritsViews = false;
    static constexpr size_t MaxSpareScopeMask = 63;
    // A scope left empty, kept for the next EnterScope; copies do not take it
    std::optional<InternalValueMap> m_spareScope;
    bool m_autoescape{};
    LookupCache* m_lookupCache{};
    uint64_t m_epoch{};
};

template<typename Key>
InternalValue& ScopeRef::operator[](Key&& name)
{
    auto [p, isAdded] = m_map->try_emplace(std::forward<Key>(name));
    if (isAdded)
    {
        m_context->NewEpoch();
    }
    return p->second;
}

inline InternalValue& ScopeRef::ForName(const HashedName& name)
{
    // With the hash the name has: the key's string is made only when the name is new
    auto [p, isAdded] = m_map->try_emplace_transparent(name);
    if (isAdded)
    {
        m_context->NewEpoch();
    }
    return p->second;
}

inline void ScopeRef::Erase(const std::string& name)
{
    if (m_map->erase(name) != 0)
    {
        m_context->NewEpoch();
    }
}

inline void ScopeRef::Clear()
{
    if (!m_map->empty())
    {
        m_map->clear();
        m_context->NewEpoch();
    }
}

// Sets the autoescape mode for a scope and restores the previous one when it ends
class AutoescapeGuard
{
public:
    AutoescapeGuard(RenderContext& context, bool autoescape)
        : m_context(context)
        , m_prev(context.SetAutoescape(autoescape))
    {
    }
    ~AutoescapeGuard() { m_context.SetAutoescape(m_prev); }
    AutoescapeGuard(const AutoescapeGuard&) = delete;
    AutoescapeGuard& operator=(const AutoescapeGuard&) = delete;

private:
    RenderContext& m_context;
    bool m_prev;
};

// Runs another template's code (an include, a parent, a block or macro defined elsewhere):
// resolves handles through that template's tree until the scope ends. Most calls stay in
// their own template, and those leave the context untouched
class ArenaSwitch
{
public:
    ArenaSwitch(RenderContext& context, const ArenaView& nodes)
        : m_context(context)
        , m_prev(context.Nodes())
        , m_switched(m_prev != nodes)
    {
        if (m_switched)
        {
            context.SetNodes(nodes);
        }
    }
    ~ArenaSwitch()
    {
        if (m_switched)
        {
            m_context.SetNodes(m_prev);
        }
    }
    ArenaSwitch(const ArenaSwitch&) = delete;
    ArenaSwitch& operator=(const ArenaSwitch&) = delete;

private:
    RenderContext& m_context;
    ArenaView m_prev;
    bool m_switched;
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDER_CONTEXT_H
