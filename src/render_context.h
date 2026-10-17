#ifndef JINJA2CPP_SRC_RENDER_CONTEXT_H
#define JINJA2CPP_SRC_RENDER_CONTEXT_H

#include "internal_value.h"
#include "lookup_result.h"
#include "node_arena.h"
#include "robin_hood.h"
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
#include <exception>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <random>
#include <string>
#include <string_view>
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

struct BlocksStack;
class TemplateRenderer;
class ImportedModule;

// One template's code running, in an inheritance chain: Jinja2's root render function
struct TemplateFrame
{
    const BlocksStack* blocks = nullptr;
    // Set by `extends`; from then on this template's top-level output is dropped
    TemplateNode<TemplateRenderer> parent;
    // Scopes visible to the template's top level, and so to unscoped blocks
    size_t baseDepth = 0;
    // The template's `self`, made when a name first asks for it (docs/tasks/0139)
    std::optional<InternalValue> self;
    // The name `self` prints (Jinja2's context name): the template's own, the first of an
    // inheritance chain's for its parents; empty for a template loaded from a string. In the
    // tree of that template, which the render keeps
    std::string_view name;
    // The frame this one was entered from (an importer, for a macro it imported); only
    // compared, so that `self` finds the template it came from
    TemplateFrame* outer = nullptr;
};


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
    using LoadTemplateResult = std::variant<NoneValue,
                                            nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
                                            nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>>;
    // The template `include`, `extends` or `import` names. A render resolves each name once and keeps the result
    // until it ends, so the reference stays valid for the rest of the render (docs/tasks/0105)
    [[nodiscard]] virtual const LoadTemplateResult& LoadTemplate(const std::string& fileName) const = 0;
    [[nodiscard]] virtual const LoadTemplateResult& LoadTemplate(const InternalValue& fileName) const = 0;
    // What the statement `site`, whose template name is a constant, loaded earlier in this
    // render, so that it does not look the name up again (docs/tasks/0154); null before its
    // first use, and always with TemplateLookup::EveryUse, where each use looks it up
    [[nodiscard]] virtual const LoadTemplateResult* FindLoadedBy(const void* /*site*/) const { return nullptr; }
    virtual void SetLoadedBy(const void* /*site*/, const LoadTemplateResult& /*tpl*/) const {}
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
    // Keeps `module`, made by an `import`, until the render ends (docs/tasks/0167)
    virtual ImportedModule& AddModule(std::unique_ptr<ImportedModule> module) = 0;
    // Lets the render's later imports of `tpl` without context share `module`, one of the
    // render's that rendered `tpl` to its end, as Jinja2 shares it (FindModule)
    virtual void ShareModule(const LoadTemplateResult& tpl, ImportedModule& module) = 0;
    // The module of `tpl` imported without context earlier in this render; null when none
    [[nodiscard]] virtual ImportedModule* FindModule(const LoadTemplateResult& tpl) const = 0;
};

// The slots where names were last found (docs/tasks/0100 idea 7). An entry holds the slot
// a name expression resolved to and the epoch of the context it was looked up in; a context
// takes a new epoch whenever any lookup could resolve elsewhere (a module scope bound, a
// scope entered with names in it) and on each copy, so an entry of an older epoch is never
// used. Epochs never repeat, so one cache serves every render on a thread, and a render
// does not pay for clearing it. A change that can move the lookups of some names only (a
// name added to or removed from a scope, a scope with names cleared or left, a slot bound
// for the first time or unbound) drops the entries of those names instead (Forget,
// docs/tasks/0158), so that the other names stay cached.
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
    // The entries, in Buckets groups of Ways by the hash of the name. The entries of a bucket
    // are dropped together, which costs a lookup, never a wrong slot
    static constexpr uint32_t Ways = 4;
    static constexpr uint32_t Buckets = 32;
    static constexpr uint32_t Size = Buckets * Ways;
    // A scope keeps the buckets of its names as one bit each (ScopeStack::Buckets)
    static_assert(Buckets <= 32);

    static LookupCache& ForThisThread()
    {
        thread_local LookupCache cache;
        return cache;
    }

    // An entry is kept as its key, in an array of the keys, and the rest: dropping the
    // entries of a bucket then clears Ways keys side by side
    struct Entry
    {
        uint64_t epoch = 0;
        LookupResult slot;
    };

    uint64_t NewEpoch() { return ++m_lastEpoch; }
    // The entry a new name expression for a name with hash `hash` uses: one of the Ways
    // entries of the name's bucket, so that Forget finds the entries of a name without a
    // search. Expressions of a bucket take its entries in turn, so the names of a template
    // share none until more than Ways of them fall in one bucket. Hashing the address of the
    // expression made two names in one loop share an entry or not depending on where the
    // heap put them, and the work of a loop with it (up to +3.5% instructions, docs/tasks/0139)
    static uint32_t NewSlot(size_t hash)
    {
        static std::array<std::atomic<uint32_t>, Buckets> next{};
        const auto bucket = BucketOf(hash);
        return (bucket * Ways) + (next[bucket].fetch_add(1, std::memory_order_relaxed) % Ways);
    }
    // The bucket of the names with hash `hash`
    static uint32_t BucketOf(size_t hash) { return static_cast<uint32_t>(hash % Buckets); }
    // Drops the entries of the names of `bucket`
    void Forget(uint32_t bucket) { std::fill_n(&m_keys[static_cast<size_t>(bucket) * Ways], Ways, nullptr); }
    // The key of entry `slot`: the expression that looked the name up, null when the entry
    // holds none. `slot` is the expression's NewSlot
    [[nodiscard]] const void* KeyAt(uint32_t slot) const { return m_keys[slot]; }
    Entry& At(uint32_t slot) { return m_entries[slot]; }
    void Store(uint32_t slot, const void* key, const Entry& entry)
    {
        m_keys[slot] = key;
        m_entries[slot] = entry;
    }

private:
    uint64_t m_lastEpoch = 0;
    std::array<const void*, Size> m_keys{};
    std::array<Entry, Size> m_entries{};
};

class RenderContext;
class ClosureSite;
class HomeCall;

// A scope of a RenderContext open for writing. Every write to a scope goes through it, so
// that adding a name forgets the cached lookups of that name and records its bucket for when
// the scope is cleared or left; changing the value of a name already there forgets none, since
// its slot stays in place. Nor does adding a name move the values of the others: a scope keeps
// each value in a node of its own
static_assert(!InternalValueMap::is_flat, "the lookup cache keeps pointers to scope values (docs/tasks/0158)");
class ScopeRef
{
public:
    // `buckets` are those of the names added to `map` (ScopeStack::Buckets)
    ScopeRef(RenderContext& context, InternalValueMap& map, uint32_t& buckets)
        : m_context(&context)
        , m_map(&map)
        , m_buckets(&buckets)
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
    uint32_t* m_buckets;
};

// The scopes of a RenderContext, innermost last, in chunks of ChunkSize. The first chunk
// lives inside the context, so a nested context (an include, a block, a `with`) allocates
// nothing for its scopes (docs/tasks/0129); deeper scopes go to chunks allocated on first
// use and kept until the context ends. A scope stays where it is until it is removed: the
// macro code and ScopeRef hold references to a scope across EnterScope, and lookups keep
// pointers to the values in it (docs/tasks/0088, 0104). A scope a closure is defined in gets
// a serial (Stamp), so that the closure can tell it from another scope pushed later at the
// same index (docs/design/0038-lexical-scoping-plan.md)
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
            // A copy is another stack: its scopes have no serial yet
            Buckets(idx) = other.Buckets(idx);
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
        if (m_size < ChunkSize)
        {
            // One store for the buckets and the serial
            m_first.marks[m_size] = {};
        }
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

    // The lookup cache buckets (LookupCache::BucketOf) of the names added to scope `idx` since it
    // was pushed or last cleared, one bit each: the lookups of these names are forgotten when
    // the scope is cleared or left, without hashing its names (docs/tasks/0158)
    uint32_t& Buckets(size_t idx) { return Marks(idx).buckets; }
    [[nodiscard]] uint32_t Buckets(size_t idx) const { return Marks(idx).buckets; }

    // The serial of scope `idx`, given on first use: never the same for two scopes (until the
    // count wraps after 2^32 stamps), so that a scope recorded by its serial is
    // known to be gone once its index holds another one. 0 for a scope not stamped yet
    uint32_t Stamp(size_t idx)
    {
        auto& serial = Marks(idx).serial;
        if (serial == 0)
        {
            // Rare: once per scope a closure is defined in. One count for every stack, so
            // that a copied stack shares no serial with its source either
            static std::atomic<uint32_t> lastSerial{ 0 };
            serial = lastSerial.fetch_add(1, std::memory_order_relaxed) + 1;
        }
        return serial;
    }
    [[nodiscard]] uint32_t Serial(size_t idx) const { return Marks(idx).serial; }

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
    // What a scope keeps beside its map. No initialisers: Push sets them, so that a chunk is
    // not cleared when made
    struct ScopeMarks
    {
        uint32_t buckets; // NOLINT(cppcoreguidelines-pro-type-member-init)
        uint32_t serial;  // NOLINT(cppcoreguidelines-pro-type-member-init)
    };
    // Raw storage: only the scopes below m_size are constructed
    struct Chunk
    {
        Chunk() noexcept {}
        ~Chunk() {}
        Chunk(const Chunk&) = delete;
        Chunk(Chunk&&) = delete;
        Chunk& operator=(const Chunk&) = delete;
        Chunk& operator=(Chunk&&) = delete;
        union
        {
            InternalValueMap maps[ChunkSize];
        };
        // Set by Push: a context that never goes deep pays for no more
        std::array<ScopeMarks, ChunkSize> marks; // NOLINT(cppcoreguidelines-pro-type-member-init)
    };

    ScopeMarks& Marks(size_t idx) { return idx < ChunkSize ? m_first.marks[idx] : m_more[(idx / ChunkSize) - 1]->marks[idx % ChunkSize]; }
    [[nodiscard]] const ScopeMarks& Marks(size_t idx) const
    {
        return idx < ChunkSize ? m_first.marks[idx] : m_more[(idx / ChunkSize) - 1]->marks[idx % ChunkSize];
    }

    // Where scope m_size goes when it is past the first chunk, with no buckets; allocates the
    // chunk if needed
    JINJA2CPP_NOINLINE_INLINE InternalValueMap* DeepSlot()
    {
        const size_t chunk = m_size / ChunkSize;
        if (chunk > m_more.size())
        {
            m_more.push_back(std::make_unique<Chunk>());
        }
        auto& deep = *m_more[chunk - 1];
        deep.marks[m_size % ChunkSize] = {};
        return &deep.maps[m_size % ChunkSize];
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
        , m_evalContext(NewEvalContextId())
    {
        EnterScope();
    }
    // With the table of `topScope`, an empty map an earlier render left, as the first scope
    // (docs/tasks/0160)
    RenderContext(const InternalValueMap& extValues,
                  const InternalValueMap& globalValues,
                  IRendererCallback* rendererCallback,
                  const InternalValueMap* builtins,
                  InternalValueMap& topScope)
        : m_rendererCallback(rendererCallback)
        , m_externalScope(&extValues)
        , m_globalScope(&globalValues)
        , m_builtinScope(builtins)
    {
        // A render's top context keeps EvalContext id 0, with no atomic count in every
        // render: the other ids are counted from 1
        assert(topScope.empty());
        // A map that never had a table has nothing to lend
        if (topScope.mask() != 0)
        {
            m_scopes.Push(std::move(topScope));
        }
        else
        {
            m_scopes.Push();
        }
        UpdateCurrentScope();
    }

    RenderContext(const RenderContext& other)
        : m_rendererCallback(other.m_rendererCallback)
        , m_externalScope(other.m_externalScope)
        , m_globalScope(other.m_globalScope)
        , m_builtinScope(other.m_builtinScope)
        , m_templateFrame(other.m_templateFrame)
        , m_nodes(other.m_nodes)
        , m_parent(other.m_parent)
        , m_parentDepth(other.m_parentDepth)
        , m_selfDepth(other.m_selfDepth)
        , m_scopes(other.m_scopes)
        , m_frame(other.m_frame)
        , m_views(other.m_views)
        , m_fullWalk(other.m_fullWalk)
        , m_inheritsViews(other.m_inheritsViews)
        , m_hasEntryBindings(other.m_hasEntryBindings)
        , m_entryPassDepth(other.m_entryPassDepth)
        , m_autoescape(other.m_autoescape)
        , m_outputAutoescape(other.m_outputAutoescape)
        , m_evalContext(other.m_evalContext)
        , m_lookupCache(other.m_lookupCache)
    {
        // The closures defined in `other` stay at home there: a copy is another context. Only
        // a context that defined none is copied (Clone returns one before its first use)
        assert(!other.m_anchor);
        UpdateCurrentScope();
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
    // Nothing holds the context's anchor beyond a call nested in its lifetime (HomeCall)
    ~RenderContext() { assert(!m_anchor || m_anchor.use_count() == 1); }

    // A context that sees the first `depth` scopes of `other`, plus a fresh one of its own on
    // top. It refers to those scopes instead of copying them (docs/tasks/0108), so it must
    // not outlive `other`: the child is always rendered inside the call that made it. While
    // it is alive `other` keeps those scopes, and the views attached to them, in place: every
    // scope and view `other` pushes after this point, and pops before the child ends, lies
    // above the child's prefix. That holds because pushes and pops on one context are nested
    // on the C++ stack, whichever context's code makes them: a closure's body runs on its
    // home context (HomeCall), which may be a parent of the running code, and pushes its
    // scope on top there (docs/design/0038-lexical-scoping-plan.md). Debug builds check it
    // at each lookup (CheckPrefix). Names it sets go to its own scopes; a value changed in
    // place (`d.update(...)`) is changed where it is stored, as in Jinja2, where the context
    // is a shallow copy.
    RenderContext(RenderContext& other, size_t depth)
        : m_rendererCallback(other.m_rendererCallback)
        , m_externalScope(other.m_externalScope)
        , m_globalScope(other.m_globalScope)
        , m_builtinScope(other.m_builtinScope)
        , m_templateFrame(other.m_templateFrame)
        , m_nodes(other.m_nodes)
        , m_parent(&other)
        , m_parentDepth(std::min(depth, other.GetScopesCount()))
        , m_selfDepth(std::min(other.m_selfDepth, m_parentDepth))
        , m_frame(other.m_frame)
        , m_inheritsViews(other.m_inheritsViews || !other.m_views.empty())
        , m_hasEntryBindings(other.m_hasEntryBindings)
        , m_entryPassDepth(other.m_entryPassDepth)
        , m_autoescape(other.m_autoescape)
        , m_outputAutoescape(other.m_outputAutoescape)
        , m_evalContext(other.m_evalContext)
        , m_lookupCache(other.m_lookupCache)
    {
        m_fullWalk = m_inheritsViews || m_entryPassDepth != 0;
        // Skip the parents whose own scopes are all hidden, so chains stay short
        while (m_parent->m_parent && m_parentDepth <= m_parent->m_parentDepth)
        {
            m_parent = m_parent->m_parent;
        }
        NewEpoch();
        EnterScope();
    }

    // A frame bound names at its entry in a scope of this context (BindUndefinedStatement)
    void SetHasEntryBindings() { m_hasEntryBindings = true; }
    // Called on a context just made from another one for a block, or for a template included
    // with context: its lookups pass over the names the frames it sees of the other bound at
    // their entry, which Jinja2 hands a block or an include only once stored (docs/tasks/0201).
    // They take FindValueWithViews then; a render that binds no name at entry pays nothing
    void PassOverEntryBindings()
    {
        if (m_hasEntryBindings && m_parentDepth > m_entryPassDepth)
        {
            m_entryPassDepth = m_parentDepth;
            m_fullWalk = true;
        }
    }

    // Lookups in this context and the ones copied from it are cached in `cache`
    void SetLookupCache(LookupCache* cache)
    {
        m_lookupCache = cache;
        NewEpoch();
    }
    // Forgets the cached lookups: a name may now resolve to another slot. The closures defined
    // here forget theirs too, seeing another epoch here (ClosureSite::EpochIn)
    void NewEpoch()
    {
        if (m_lookupCache)
        {
            m_epoch = m_lookupCache->NewEpoch();
        }
    }
    // Forgets the cached lookups of the name with hash `hash`: it may now resolve to another
    // slot, while the other names may not (docs/tasks/0158)
    void ForgetName(size_t hash) { ForgetBucket(LookupCache::BucketOf(hash)); }
    // Forgets the cached lookups of the names of `bucket` (LookupCache::BucketOf)
    void ForgetBucket(uint32_t bucket)
    {
        if (m_lookupCache)
        {
            m_lookupCache->Forget(bucket);
        }
    }
    // Forgets the cached lookups of the names of `buckets` (ScopeStack::Buckets)
    void ForgetBuckets(uint32_t buckets)
    {
        if (!m_lookupCache)
        {
            return;
        }
        for (; buckets != 0; buckets &= buckets - 1)
        {
            m_lookupCache->Forget(static_cast<uint32_t>(ROBIN_HOOD_COUNT_TRAILING_ZEROES(buckets)));
        }
    }

    // An empty scope changes no lookup until a name is added to it
    JINJA2CPP_ALWAYS_INLINE ScopeRef EnterScope()
    {
        if (m_spareScope)
        {
            m_scopes.Push(std::move(*m_spareScope));
            m_spareScope.reset();
        }
        else
        {
            m_scopes.Push();
        }
        UpdateCurrentScope();
        return CurrentScopeRef();
    }

    // Enters a scope made of `map`, names and all: the scope a loop kept from its last run
    // (docs/tasks/0133)
    ScopeRef EnterScope(InternalValueMap&& map)
    {
        auto& scope = m_scopes.Push(std::move(map));
        m_currentScope = &scope;
        if (!scope.empty())
        {
            // Names whose buckets the scope does not know: they are forgotten as names of any
            // bucket when it goes
            NewEpoch();
            CurrentBuckets() = ~uint32_t{ 0 };
        }
        return CurrentScopeRef();
    }

    // Leaves the innermost scope, moving its map with its names and nodes into `map`
    void ExitScope(InternalValueMap& map)
    {
        // A view attached to the scope would attach to the next scope pushed at its index
        assert(m_views.empty() || m_views.back().scopeIndex < m_scopes.size() - 1);
        auto& scope = *m_currentScope;
        if (!scope.empty())
        {
            ForgetBuckets(CurrentBuckets());
        }
        map = std::move(scope);
        m_scopes.pop_back();
        UpdateCurrentScope();
    }

    // Moves the first scope, with its names, into `map` when it is the only one left and has a
    // table: at the end of a render, so that the next one reuses the table (docs/tasks/0160)
    void TakeTopScope(InternalValueMap& map)
    {
        if (m_scopes.size() == 1 && m_views.empty() && m_scopes.back().mask() != 0)
        {
            ExitScope(map);
        }
    }

    void ExitScope()
    {
        // A view attached to the scope would attach to the next scope pushed at its index
        assert(m_views.empty() || m_views.back().scopeIndex < m_scopes.size() - 1);
        auto& scope = *m_currentScope;
        if (!scope.empty())
        {
            ForgetBuckets(CurrentBuckets());
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
        UpdateCurrentScope();
    }

    // The value of the name `val` (a std::string or a HashedName) in the innermost scope that
    // has it; none when no scope does
    template<typename Key>
    [[nodiscard]] JINJA2CPP_ALWAYS_INLINE LookupResult FindValue(const Key& val) const
    {
        CheckPrefix();
        if (m_fullWalk)
        {
            return FindValueWithViews(ToHashedName(val));
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
        // A body called under its caller's epoch reads no name through the cache
        // (HomeCall::Run)
        assert(m_noCachedLookups == 0);
        if (m_lookupCache)
        {
            // Epochs are unique per cache, not across caches: a context is used only on the
            // thread whose cache it holds
            assert(m_lookupCache == &LookupCache::ForThisThread());
            const auto& entry = m_lookupCache->At(slot);
            if (m_lookupCache->KeyAt(slot) == key && entry.epoch == m_epoch)
            {
                assert(FindValue(nameOf()).IsSame(entry.slot));
                return entry.slot;
            }
        }
        const auto result = FindValue(nameOf());
        if (result && m_lookupCache)
        {
            m_lookupCache->Store(slot, key, { m_epoch, result });
        }
        return result;
    }

    // The variable `self` (docs/tasks/0139): a name set in the scopes of the running template
    // wins, else the template itself, made on first use (Jinja2's TemplateReference). Defined
    // with TemplateFrame in statements.cpp
    LookupResult FindSelf(const std::string& name);
    // A block body sets `self` only in its own scopes, from `depth` up: in Jinja2 every block
    // function declares `self`, which hides the template's top-level `set self`. Returns the
    // depth it replaces
    size_t SetSelfDepth(size_t depth)
    {
        std::swap(depth, m_selfDepth);
        return depth;
    }

    // Where the variable `name` is stored, so that a list or dict the template changes in
    // place can be stored back (docs/tasks/0020); none when it is not found or cannot be
    // written. The external and global scopes are copies made for this render, so writing
    // to them never changes the caller's data; the global scope is kept for the next render
    // on the thread, so its renderer is told (docs/tasks/0139). The built-in scope is shared
    // and never written.
    MutableLookupResult FindForWrite(const std::string& name)
    {
        MutableLookupResult result;
        const auto hashed = ToHashedName(name);
        const bool isFound = VisitScopes(
            *this,
            [&](InternalValueMap* scope, size_t depth) {
                auto valP = scope->find(name);
                // A block passes over a name bound at entry (PassOverEntryBindings)
                if (valP == scope->end() || (depth < m_entryPassDepth && IsEntryBinding(valP->second, hashed.hash)))
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
        return CurrentScopeRef();
    }
    // Adds to `names` each name this context sees in its scopes, frame views and external
    // scope (not the global and built-in ones, which every context shares), with the value a
    // lookup here finds: what a module imported "with context" sees (docs/tasks/0167; Jinja2's
    // get_all() and the importer's locals, which leave out `loop`). A shallow copy: a namespace
    // changed later is seen
    void CollectVisibleNames(InternalValueMap& names) const
    {
        VisitScopes(
            *this,
            [&names](const InternalValueMap* scope, size_t /*depth*/) {
                for (const auto& [name, value] : *scope)
                {
                    // Jinja2 hands an import a name bound at entry only once stored (docs/tasks/0201)
                    if (name != "loop" && !IsEntryBinding(value, HashedName::Hash(name)))
                    {
                        names.emplace(name, value);
                    }
                }
                return false;
            },
            [&names](const FrameView& view, size_t /*depth*/) {
                for (size_t idx = 0; idx != view.names.size(); ++idx)
                {
                    if (view.slots[idx].IsBound() && view.nodes.Text(view.names[idx].name) != "loop")
                    {
                        names.emplace(std::string(view.nodes.Text(view.names[idx].name)), static_cast<const InternalValue&>(view.slots[idx]));
                    }
                }
                return false;
            });
        for (const auto& [name, value] : *m_externalScope)
        {
            names.emplace(name, value);
        }
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
    // Settings::userCallablesAsFilters of the template: whether a name that is no filter or test
    // of the template may be a callable among the values
    [[nodiscard]] bool UserCallablesAsFilters() const { return m_rendererCallback ? m_rendererCallback->GetSettings().userCallablesAsFilters : false; }
    // A context for a nested render: with `includeCurrentContext` it sees all of this
    // context's names (see the constructor above for how long it may live), otherwise only
    // the global ones
    [[nodiscard]] RenderContext Clone(bool includeCurrentContext)
    {
        if (!includeCurrentContext)
        {
            return CloneWith(m_emptyScope);
        }

        return { *this, GetScopesCount() };
    }
    // A context of its own, which sees `external` and the global names: an imported module's
    // (docs/tasks/0167). It refers to `external`, which must outlive it, and to nothing else
    // of this context but what the render keeps
    [[nodiscard]] RenderContext CloneWith(const InternalValueMap& external) const
    {
        RenderContext result(external, *m_globalScope, m_rendererCallback, m_builtinScope);
        result.m_templateFrame = m_templateFrame;
        result.m_nodes = m_nodes;
        result.m_autoescape = m_autoescape;
        result.m_outputAutoescape = m_outputAutoescape;
        result.SetLookupCache(m_lookupCache);
        return result;
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
    // Jinja2 decides two things by autoescape (docs/tasks/0179). Whether code escapes its
    // output, `{{ }}`, `~` and `{% filter %}`, is fixed where the code is written: the setting
    // the code is compiled under (IsOutputAutoescape). Whether a result is Markup, of a macro,
    // `caller()`, `{% set %}` block or a filter such as `join`, is the setting in force when it
    // runs, Jinja2's eval_ctx.autoescape (IsAutoescape). `{% autoescape %}` and a new template
    // set both; a macro or block body sets only the first, and runs with its caller's runtime
    // setting (HomeCall). The runtime setting is one per rendered template, as Jinja2's
    // EvalContext is one per Context: an imported macro runs with its module's
    [[nodiscard]] bool IsAutoescape() const { return m_autoescape; }
    bool SetAutoescape(bool autoescape)
    {
        std::swap(autoescape, m_autoescape);
        return autoescape;
    }
    [[nodiscard]] bool IsOutputAutoescape() const { return m_outputAutoescape; }
    bool SetOutputAutoescape(bool autoescape)
    {
        std::swap(autoescape, m_outputAutoescape);
        return autoescape;
    }
    // Makes this context the root of a new runtime autoescape setting: an included
    // template's, which Jinja2 renders in a new Context
    void NewEvalContext() { m_evalContext = NewEvalContextId(); }

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
    // changes no lookup. With `linkBelow` (ScopeView), the first view of a closure body's
    // scope also hides the scopes from that index up to the current one
    void PushFrameView(const FrameView& view, size_t linkBelow = ScopeView::NoLink)
    {
        assert(!m_scopes.empty() && (m_views.empty() || m_views.back().scopeIndex <= m_scopes.size() - 1));
        // A lookup by name reads the slot of each name
        assert(view.slots.size() >= view.names.size());
        // A link hides scopes below its own, and is the first view of its scope
        assert(linkBelow == ScopeView::NoLink || (linkBelow < GetScopesCount() && (m_views.empty() || m_views.back().scopeIndex < m_scopes.size() - 1)));
        m_views.push_back({ view, m_scopes.size() - 1, linkBelow });
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
    // Binds `slot`, whose name has hash `nameHash`. A slot bound for the first time may hide
    // a name found before, so the lookups of its name are forgotten; giving a bound slot
    // another value forgets none
    template<typename Value>
    void BindSlot(Slot& slot, size_t nameHash, Value&& value)
    {
        if (!slot.IsBound())
        {
            ForgetName(nameHash);
        }
        slot.Bind(std::forward<Value>(value));
    }
    // Unbinds `slots`, whose names are `names`, forgetting the lookups of each that was bound
    void UnbindSlots(boost::span<Slot> slots, boost::span<const SlotName> names)
    {
        assert(slots.size() == names.size());
        for (size_t idx = 0; idx != slots.size(); ++idx)
        {
            if (slots[idx].IsBound())
            {
                slots[idx].Unbind();
                ForgetName(names[idx].hash);
            }
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
        if (m_emptyScope != other.m_emptyScope)
        {
            return false;
        }
        if (m_parent != other.m_parent || m_parentDepth != other.m_parentDepth)
        {
            return false;
        }
        if (m_scopes != other.m_scopes || m_views != other.m_views)
        {
            return false;
        }
        return m_autoescape == other.m_autoescape && m_outputAutoescape == other.m_outputAutoescape;
    }

private:
    // A closure records where it was defined, and its calls run there
    friend class ClosureSite;
    friend class HomeCall;

    // The control block of a context's anchor: it owns nothing, the anchor points at the context
    struct ContextAnchor
    {
    };
    // The anchor, made on first use
    const std::shared_ptr<RenderContext>& Anchor()
    {
        if (!m_anchor)
        {
            m_anchor = std::shared_ptr<RenderContext>(std::make_shared<ContextAnchor>(), this);
        }
        return m_anchor;
    }
    // The serial of scope `depth - 1` (ScopeStack::Stamp), when it is one of this context's
    // own; 0 otherwise
    uint32_t SerialBelow(size_t depth)
    {
        return depth > m_parentDepth && depth - m_parentDepth <= m_scopes.size() ? m_scopes.Stamp(depth - 1 - m_parentDepth) : 0;
    }
    // Whether scope `depth - 1` is still the one whose SerialBelow was `serial`. The scopes of
    // the parents this context sees never change while it is alive
    [[nodiscard]] bool HasScopeBelow(size_t depth, uint32_t serial) const
    {
        return depth <= m_parentDepth || (depth - m_parentDepth <= m_scopes.size() && m_scopes.Serial(depth - 1 - m_parentDepth) == serial);
    }

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
    // The innermost of the scopes this context sees from index `minDepth` up that has `name`
    [[nodiscard]] LookupResult FindInScopesFrom(const std::string& name, size_t minDepth) const
    {
        LookupResult result;
        const auto hashed = ToHashedName(name);
        VisitScopes(
            *this,
            [&](const InternalValueMap* scope, size_t depth) {
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
    // Calls `onMap` with each scope `ctx` sees, innermost first, and `onView` with the views of
    // each scope right after its map, until one returns true; returns whether one did. Both
    // also get the scope's index among the scopes `ctx` sees
    // A closure body's link (ScopeView::linkBelow) passes over the scopes between its own and
    // the place the closure was defined
    template<typename Context, typename OnMap, typename OnView>
    static bool VisitScopes(Context& ctx, const OnMap& onMap, const OnView& onView)
    {
        ctx.CheckPrefix();
        size_t limit = ctx.GetScopesCount();
        for (auto* cur = &ctx; cur; cur = cur->m_parent)
        {
            // The views of the scopes not visited yet are below this index
            size_t views = cur->m_views.size();
            for (size_t idx = cur->VisibleScopesCount(limit); idx != 0; --idx)
            {
                const size_t depth = cur->m_parentDepth + idx - 1;
                if (onMap(&cur->m_scopes[idx - 1], depth))
                {
                    return true;
                }
                // The views of a scope a child does not see are passed over
                size_t link = ScopeView::NoLink;
                for (; views != 0 && cur->m_views[views - 1].scopeIndex >= idx - 1; --views)
                {
                    const auto& view = cur->m_views[views - 1];
                    if (view.scopeIndex != idx - 1)
                    {
                        continue;
                    }
                    if (onView(view.view, depth))
                    {
                        return true;
                    }
                    link = std::min(link, view.linkBelow);
                }
                if (link != ScopeView::NoLink)
                {
                    assert(link <= depth);
                    if (link > cur->m_parentDepth)
                    {
                        // On at scope `link - 1` of this context, after the decrement
                        idx = link - cur->m_parentDepth + 1;
                        continue;
                    }
                    limit = std::min(limit, link);
                    break;
                }
            }
            limit = std::min(limit, cur->m_parentDepth);
        }
        return false;
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
    // FindValue for a context that sees frame views: each scope with the views attached to it,
    // innermost first, through the parents, then the external, global and built-in scopes.
    // Out of line, so that the common path stays short; `val` is taken by value for the same
    // reason as in FindInDeepScopes
    [[nodiscard]] JINJA2CPP_NOINLINE_INLINE LookupResult FindValueWithViews(HashedName val) const
    {
        size_t count = m_scopes.size();
        size_t limit = m_parentDepth;
        for (const auto* ctx = this; ctx;)
        {
            size_t link = ScopeView::NoLink;
            if (const auto result = ctx->FindInScopesWithViews(count, val, link, m_entryPassDepth))
            {
                return result;
            }
            // A closure body's link: on below scope `link`, in this context or a parent
            if (link != ScopeView::NoLink)
            {
                if (link > ctx->m_parentDepth)
                {
                    count = link - ctx->m_parentDepth;
                    continue;
                }
                limit = std::min(limit, link);
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
    // them, that has `val`. Stops at a closure body's link, which it stores in `link`. Passes
    // over a name bound at entry in a scope below `entryPassDepth` (PassOverEntryBindings)
    [[nodiscard]] LookupResult FindInScopesWithViews(size_t count, const HashedName& val, size_t& link, size_t entryPassDepth) const
    {
        // The views of the scopes a child does not see are passed over
        size_t views = m_views.size();
        for (size_t idx = count; idx != 0; --idx)
        {
            if (const auto* p = FindIn(m_scopes[idx - 1], val); p && (m_parentDepth + idx > entryPassDepth || !IsEntryBinding(p->second, val.hash)))
            {
                return LookupResult(p->second);
            }
            for (; views != 0 && m_views[views - 1].scopeIndex >= idx - 1; --views)
            {
                const auto& view = m_views[views - 1];
                if (view.scopeIndex != idx - 1)
                {
                    continue;
                }
                const auto slot = FindInView(view.view, val);
                if (slot != view.view.names.size())
                {
                    return LookupResult(view.view.slots[slot]);
                }
                // A link is on the first view of its scope (PushFrameView): the scope is done
                if (view.linkBelow != ScopeView::NoLink)
                {
                    assert(view.linkBelow < m_parentDepth + idx);
                    link = view.linkBelow;
                    return {};
                }
            }
        }
        return {};
    }
    // Whether `value`, found under the name with hash `hash`, is the undefined a frame bound
    // to that name at its entry
    static bool IsEntryBinding(const InternalValue& value, std::size_t hash)
    {
        const auto* undefined = GetIf<UndefinedValue>(&value);
        return undefined != nullptr && undefined->atEntry && undefined->entryHash == static_cast<std::uint32_t>(hash);
    }
    // The scopes a child sees of its parent are still there (see the constructor): the
    // parent pushes and pops only above them
    void CheckPrefix() const { assert(!m_parent || m_parent->GetScopesCount() >= m_parentDepth); }
    // The current scope is the innermost one
    ScopeRef CurrentScopeRef() { return { *this, m_scopes.back(), CurrentBuckets() }; }
    uint32_t& CurrentBuckets() { return m_scopes.Buckets(m_scopes.size() - 1); }
    void UpdateCurrentScope() { m_currentScope = m_scopes.empty() ? nullptr : &m_scopes.back(); }
    void UpdateFullWalk() { m_fullWalk = m_inheritsViews || m_entryPassDepth != 0 || !m_views.empty(); }
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
    TemplateFrame* m_templateFrame{};
    ArenaView m_nodes;
    InternalValueMap m_emptyScope;
    LoopControl m_loopControl = LoopControl::None;
    // The context this one was made from and how many of its scopes this one sees, below
    // its own; null for a context that copies no scopes
    RenderContext* m_parent{};
    size_t m_parentDepth{};
    // The scopes below it do not set `self` (SetSelfDepth)
    size_t m_selfDepth{};
    ScopeStack m_scopes;
    SlotFrame m_frame;
    // The frame views attached to this context's scopes, by scope index
    boost::container::small_vector<ScopeView, 3> m_views;
    // Whether lookups take FindValueWithViews: there is a frame view in sight
    bool m_fullWalk = false;
    // Whether a parent this context sees scopes of has frame views. Set once, when the
    // context is made: while this context is alive the parent pushes and pops views only
    // above the scopes this one sees (see the constructor), which it never searches
    bool m_inheritsViews = false;
    // A scope of this context, or of one it was made from, holds names bound at entry
    bool m_hasEntryBindings = false;
    // Lookups pass over the names bound at entry in the scopes below this depth
    // (PassOverEntryBindings)
    size_t m_entryPassDepth = 0;
    static constexpr size_t MaxSpareScopeMask = 63;
    // A scope left empty, kept for the next EnterScope; copies do not take it
    std::optional<InternalValueMap> m_spareScope;
    bool m_autoescape{};
    bool m_outputAutoescape{};
    // Contexts with the same id share Jinja2's EvalContext (NewEvalContext)
    uint32_t m_evalContext{};
    static uint32_t NewEvalContextId()
    {
        static std::atomic<uint32_t> next{ 0 };
        return next.fetch_add(1, std::memory_order_relaxed) + 1;
    }
    LookupCache* m_lookupCache{};
    uint64_t m_epoch{};
#ifndef NDEBUG
    // Calls running that must not read the lookup cache (HomeCall::NoCachedLookups)
    int m_noCachedLookups = 0;
#endif
    // The only owner of this context's anchor, made when a closure is first defined here: the
    // closures hold weak references to it, and a call locks one for its duration only
    // (docs/design/0038-lexical-scoping-plan.md, section 11). It aliases this context, so
    // that nothing else has to point back at it; copies of the context do not share it
    std::shared_ptr<RenderContext> m_anchor;
};

template<typename Key>
InternalValue& ScopeRef::operator[](Key&& name)
{
    // Hashed once, for the scope and for the lookup cache
    const std::string_view text(name);
    return ForName({ text, HashedName::Hash(text) });
}

inline InternalValue& ScopeRef::ForName(const HashedName& name)
{
    // With the hash the name has: the key's string is made only when the name is new
    auto [p, isAdded] = m_map->try_emplace_transparent(name);
    if (isAdded)
    {
        const auto bucket = LookupCache::BucketOf(name.hash);
        *m_buckets |= uint32_t{ 1 } << bucket;
        m_context->ForgetBucket(bucket);
    }
    return p->second;
}

inline void ScopeRef::Erase(const std::string& name)
{
    if (m_map->erase(name) != 0)
    {
        m_context->ForgetName(HashedName::Hash(name));
    }
}

inline void ScopeRef::Clear()
{
    if (!m_map->empty())
    {
        m_context->ForgetBuckets(std::exchange(*m_buckets, 0));
        m_map->clear();
    }
}

// Sets both autoescape modes (RenderContext::IsAutoescape) for a scope and restores the
// previous ones when it ends: `{% autoescape %}` and a template's top level
class AutoescapeGuard
{
public:
    AutoescapeGuard(RenderContext& context, bool autoescape)
        : m_context(context)
        , m_prev(context.SetAutoescape(autoescape))
        , m_prevOutput(context.SetOutputAutoescape(autoescape))
    {
    }
    ~AutoescapeGuard()
    {
        m_context.SetAutoescape(m_prev);
        m_context.SetOutputAutoescape(m_prevOutput);
    }
    AutoescapeGuard(const AutoescapeGuard&) = delete;
    AutoescapeGuard& operator=(const AutoescapeGuard&) = delete;

private:
    RenderContext& m_context;
    bool m_prev;
    bool m_prevOutput;
};

// Sets the output autoescape mode only, for the body of a macro or a block
class OutputAutoescapeGuard
{
public:
    OutputAutoescapeGuard(RenderContext& context, bool autoescape)
        : m_context(context)
        , m_prev(context.SetOutputAutoescape(autoescape))
    {
    }
    ~OutputAutoescapeGuard() { m_context.SetOutputAutoescape(m_prev); }
    OutputAutoescapeGuard(const OutputAutoescapeGuard&) = delete;
    OutputAutoescapeGuard& operator=(const OutputAutoescapeGuard&) = delete;

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

// Where a call of a closure runs its body (ClosureSite::Resolve)
struct ClosureHome
{
    // The lookup cache epoch the body runs under
    enum class Epoch : uint8_t
    {
        // The home context's: every call sees the same scopes there (a closure defined at
        // its template's top level), or what the caller sees (a macro of an ended module)
        Home,
        // The closure's own, shared by its calls, which all link to the same scope
        Closure,
        // A new one: the call sees scopes the other calls may not
        Fresh
    };

    RenderContext* context = nullptr;
    // Keeps a home other than the caller alive for the call; never stored past it
    std::shared_ptr<RenderContext> lock;
    // The body sees the scopes of `context` below this index (ScopeView::linkBelow), all of
    // them for ScopeView::NoLink
    size_t link = ScopeView::NoLink;
    Epoch epoch = Epoch::Home;
};

// Where a closure (a macro, a call block's caller, a recursive loop's loop(), super()) was
// defined: its body resolves the names it does not bind there, live, as in Jinja2, not
// through whoever calls it (docs/design/0038-lexical-scoping-plan.md, sections 4.2, 10, 11).
// It holds the defining context weakly, through the context's anchor, with the scopes the
// closure saw there. Opaque, so that the representation can change (0117 P4)
class ClosureSite
{
public:
    ClosureSite() = default;
    // The site of a closure defined now in `context`, which sees the scopes `context` sees
    explicit ClosureSite(RenderContext& context)
        : ClosureSite(context, context.GetScopesCount())
    {
    }
    // The site of a closure defined in `context` that sees its first `depth` scopes (a
    // template's top level, for `self`). Defined with TemplateFrame in statements.cpp
    ClosureSite(RenderContext& context, size_t depth);

    // Where a call made from `caller` runs:
    // 1. the home is the caller when it is the defining context (no lock), else the defining
    //    context, locked for the call;
    // 2. alive, with the scope the closure was defined in still there: the body links to it;
    // 3. alive, but that scope has ended (a closure kept past its loop or macro call): the
    //    body links to the template's root, else to what the context sees of its parents,
    //    so that the names of the ended frame read as undefined;
    // 4. ended (the include, `with` or block defining it is over): the body runs on the
    //    caller with every scope hidden, seeing the params, globals and built-ins only. An
    //    imported module's context lives as long as the render (docs/tasks/0167)
    [[nodiscard]] ClosureHome Resolve(RenderContext& caller) const
    {
        ClosureHome home;
        if (IsDefinedIn(caller))
        {
            home.context = &caller;
        }
        else
        {
            home.lock = m_home.lock();
            home.context = home.lock.get();
            if (!home.context)
            {
                return Ended(caller);
            }
        }
        home.link = m_depth;
        if ((m_state & TopLevel) != 0)
        {
            // Called inside another body on the home: the link hides that body's scopes, whose
            // names the home's epoch may have cached
            if (home.context->GetScopesCount() != m_depth)
            {
                home.epoch = ClosureHome::Epoch::Closure;
            }
            return home;
        }
        auto& context = *home.context;
        if (context.HasScopeBelow(m_depth, m_serial))
        {
            home.epoch = ClosureHome::Epoch::Closure;
            return home;
        }
        home.link = context.HasScopeBelow(m_rootDepth, m_rootSerial) ? m_rootDepth : context.m_parentDepth;
        home.epoch = ClosureHome::Epoch::Fresh;
        return home;
    }

private:
    friend class HomeCall;

    // Whether the closure was defined in `caller`: the weak and the strong reference share a
    // control block when they name one anchor
    [[nodiscard]] bool IsDefinedIn(const RenderContext& caller) const { return caller.m_anchor && HasOwner(caller.m_anchor); }
    // IsDefinedIn for a site defined somewhere, whose reference is not empty
    [[nodiscard]] bool HasOwner(const std::shared_ptr<RenderContext>& anchor) const { return !m_home.owner_before(anchor) && !anchor.owner_before(m_home); }
    [[nodiscard]] static ClosureHome Ended(RenderContext& caller)
    {
        ClosureHome home;
        home.context = &caller;
        home.link = 0;
        home.epoch = ClosureHome::Epoch::Fresh;
        return home;
    }
    // The epoch of the closure's own cached lookups in `home`, its defining context, whose
    // epoch is `homeEpoch` at the call: a new one once `home` has forgotten all of its own
    // (RenderContext::NewEpoch, which never gives an epoch twice) since the last call. A
    // call inside another closure's call on `home` sees that closure's epoch, so it takes a
    // new one too, which costs lookups, never a wrong slot. A home without a lookup cache
    // keeps its own epoch, which nothing reads
    uint64_t EpochIn(const RenderContext& home, uint64_t homeEpoch) const
    {
        if (m_homeEpoch != homeEpoch)
        {
            m_epoch = home.m_lookupCache ? home.m_lookupCache->NewEpoch() : homeEpoch;
            m_homeEpoch = homeEpoch;
        }
        return m_epoch;
    }

    std::weak_ptr<RenderContext> m_home;
    // The scopes the closure saw, and the serial of the innermost of them
    uint32_t m_depth = 0;
    uint32_t m_serial = 0;
    // The scopes its template's top level saw (TemplateFrame::baseDepth)
    uint32_t m_rootDepth = 0;
    uint32_t m_rootSerial = 0;
    // Its own lookup cache epoch, taken at the first call, and the home context's epoch then:
    // none before the first call
    mutable uint64_t m_epoch = 0;
    mutable uint64_t m_homeEpoch = std::numeric_limits<uint64_t>::max();
    enum State : uint8_t
    {
        // Defined at its template's top level, whose scope lives as long as the context:
        // every call links there, under the context's own epoch
        TopLevel = 1,
        // A call of the closure is running: a call made inside it (recursion) links to the
        // same scope, but must not find what the outer call cached in its own scope
        Active = 2
    };
    mutable uint8_t m_state = 0;
};

// What a HomeCall that switches the home saves; at namespace scope, as Clang does not take
// member initialisers in a nested class that std::optional constructs
struct HomeCallSaved
{
    // Keeps a home other than the caller alive for the call
    std::shared_ptr<RenderContext> lock;
    // The home's own scopes when the call started
    size_t scopes = 0;
    uint64_t epoch = 0;
    // The epoch installed for the body: another one at the end means the home forgot its
    // cached lookups during the call
    uint64_t bodyEpoch = 0;
    TemplateFrame* templateFrame = nullptr;
    // The frame the body runs in when its template is not its caller's (HomeCall::Enter)
    std::optional<TemplateFrame> callFrame;
    LoopControl loopControl = LoopControl::None;
    bool autoescape = false;
    bool outputAutoescape = false;
};

// Runs a closure's body on its home context (ClosureSite::Resolve) for the duration of one
// call: switches the lookup cache epoch, the loop control, the template frame and the
// autoescape mode, and restores them on any exit, with the scopes and views the body pushed
// there, so that a body left by an exception leaves a home other than the caller as it found
// it. The body's
// code pushes its scope, linked with Link(); its unit frame (UnitCall) and tree
// (ArenaSwitch) are restored by their own guards. The common call, of a closure defined at
// its template's top level made from the context that defined it, switches nothing and
// stays inline (docs/design/0038-lexical-scoping-plan.md, section 6)
class HomeCall
{
public:
    // `isFresh`: under a new epoch whatever the site (a recursive loop's level)
    HomeCall(const ClosureSite& site, RenderContext& caller, bool isFresh = false)
        : m_site(site)
        , m_context(&caller)
        , m_link(site.m_depth)
        , m_state(site.m_state)
    {
        // No lock: the caller is alive. No epoch to switch: a call of a top-level closure made
        // at the top level links to the scopes the caller sees, unless it runs inside
        // another call of itself
        if (isFresh || m_state != ClosureSite::TopLevel || caller.GetScopesCount() != site.m_depth || !site.HasOwner(caller.m_anchor))
        {
            Enter(isFresh);
        }
        site.m_state = static_cast<uint8_t>(m_state | ClosureSite::Active);
#ifndef NDEBUG
        m_debugScopes = m_context->m_scopes.size();
        m_debugExceptions = std::uncaught_exceptions();
#endif
    }
    HomeCall(const HomeCall&) = delete;
    HomeCall(HomeCall&&) = delete;
    HomeCall& operator=(const HomeCall&) = delete;
    HomeCall& operator=(HomeCall&&) = delete;
    ~HomeCall() // NOLINT(bugprone-exception-escape): Leave frees memory only
    {
        m_site.m_state = m_state;
        // A body that ends normally has left its scope and dropped its views. On the caller,
        // one left by an exception leaves them there, as any statement does: the render is over
        assert(m_saved || std::uncaught_exceptions() != m_debugExceptions || m_context->m_scopes.size() == m_debugScopes);
        if (m_saved)
        {
            Leave(*m_context, *m_saved);
        }
    }

    // Where the body runs
    [[nodiscard]] RenderContext& Context() const { return *m_context; }
    // The link of the body's scope, pushed on Context() (RenderContext::PushFrameView):
    // ScopeView::NoLink, or a link that may hide no scope (HidesScopes)
    [[nodiscard]] size_t Link() const { return m_link; }
    // Whether the link hides scopes of Context(), whose scope count is `count` before the
    // body's scope is pushed
    [[nodiscard]] bool HidesScopes(size_t count) const { return m_link < count; }

    // Runs `body(home, link, args...)` as a HomeCall would. The calls that switch nothing (the
    // common one, of a closure defined at its template's top level made at the top level of
    // the context that defined it) only mark the site as running, inline; the others take a
    // HomeCall out of line. The body and the arguments are passed by reference, so that the
    // direct calls make nothing in memory
    // `readsSlotsOnly`: the body makes no cached lookup by name (MacroStatement::ReadsSlotsOnly)
    template<typename Body, typename... Args>
    static void Run(const ClosureSite& site, RenderContext& caller, bool readsSlotsOnly, const Body& body, Args&... args)
    {
        if (site.m_state == ClosureSite::TopLevel && site.HasOwner(caller.m_anchor))
        {
            // Made at the top level, or by a body nothing of which can be a stale cached lookup
            if (readsSlotsOnly || caller.GetScopesCount() == site.m_depth)
            {
                const Direct direct(site, caller, ClosureSite::TopLevel);
                const NoCachedLookups noLookups(caller, readsSlotsOnly);
                body(caller, static_cast<size_t>(site.m_depth), args...);
                return;
            }
            // Made inside another body on the caller: the link hides that body's scopes, so
            // the call takes the closure's own epoch (ClosureSite::Resolve), and switches
            // nothing else
            const Direct direct(site, caller, ClosureSite::TopLevel);
            const ClosureEpoch epoch(site, caller);
            body(caller, static_cast<size_t>(site.m_depth), args...);
            return;
        }
        RunSwitched(site, caller, body, args...);
    }

private:
    // The site of a call that switches nothing, marked as running (ClosureSite::Active) for
    // its duration
    class Direct
    {
    public:
        // `state`: the site's, known to the caller
        Direct(const ClosureSite& site, [[maybe_unused]] const RenderContext& caller, uint8_t state)
            : m_site(site)
            , m_state(state)
#ifndef NDEBUG
            , m_caller(caller)
            , m_scopes(caller.m_scopes.size())
            , m_exceptions(std::uncaught_exceptions())
#endif
        {
            site.m_state = static_cast<uint8_t>(m_state | ClosureSite::Active);
        }
        Direct(const Direct&) = delete;
        Direct(Direct&&) = delete;
        Direct& operator=(const Direct&) = delete;
        Direct& operator=(Direct&&) = delete;
        ~Direct()
        {
            // A body that ends normally has left its scope and dropped its views
            assert(std::uncaught_exceptions() != m_exceptions || m_caller.m_scopes.size() == m_scopes);
            m_site.m_state = m_state;
        }

    private:
        const ClosureSite& m_site;
        uint8_t m_state;
#ifndef NDEBUG
        const RenderContext& m_caller;
        size_t m_scopes;
        int m_exceptions;
#endif
    };

    // Debug: the lookup cache stays unused for the duration of a call (FindValueCached)
    class NoCachedLookups
    {
    public:
        NoCachedLookups([[maybe_unused]] RenderContext& context, [[maybe_unused]] bool isOn)
#ifndef NDEBUG
            : m_context(context)
            , m_isOn(isOn)
        {
            m_context.m_noCachedLookups += m_isOn ? 1 : 0;
        }
#else
        {
        }
#endif
        NoCachedLookups(const NoCachedLookups&) = delete;
        NoCachedLookups(NoCachedLookups&&) = delete;
        NoCachedLookups& operator=(const NoCachedLookups&) = delete;
        NoCachedLookups& operator=(NoCachedLookups&&) = delete;
#ifndef NDEBUG
        ~NoCachedLookups() { m_context.m_noCachedLookups -= m_isOn ? 1 : 0; }

    private:
        RenderContext& m_context;
        bool m_isOn;
#else
        ~NoCachedLookups() = default;
#endif
    };

    // The closure's own epoch on the caller, its home, for the duration of a call
    class ClosureEpoch
    {
    public:
        ClosureEpoch(const ClosureSite& site, RenderContext& home)
            : m_home(home)
            , m_epoch(home.m_epoch)
            , m_bodyEpoch(site.EpochIn(home, m_epoch))
        {
            home.m_epoch = m_bodyEpoch;
        }
        ClosureEpoch(const ClosureEpoch&) = delete;
        ClosureEpoch(ClosureEpoch&&) = delete;
        ClosureEpoch& operator=(const ClosureEpoch&) = delete;
        ClosureEpoch& operator=(ClosureEpoch&&) = delete;
        ~ClosureEpoch()
        {
            // Another epoch at the end: the home forgot its cached lookups during the call
            if (m_home.m_epoch != m_bodyEpoch)
            {
                m_home.NewEpoch();
            }
            else
            {
                m_home.m_epoch = m_epoch;
            }
        }

    private:
        RenderContext& m_home;
        uint64_t m_epoch;
        uint64_t m_bodyEpoch;
    };

    template<typename Body, typename... Args>
    JINJA2CPP_NOINLINE_INLINE static void RunSwitched(const ClosureSite& site, RenderContext& caller, const Body& body, Args&... args)
    {
        const HomeCall homeCall(site, caller);
        body(homeCall.Context(), homeCall.Link(), args...);
    }

    JINJA2CPP_NOINLINE_INLINE void Enter(bool isFresh)
    {
        const auto* caller = m_context;
        auto home = m_site.Resolve(*m_context);
        if (isFresh || (m_state & ClosureSite::Active) != 0)
        {
            home.epoch = ClosureHome::Epoch::Fresh;
        }
        auto& context = *home.context;
        m_link = home.link;
        if (&context == m_context && home.epoch == ClosureHome::Epoch::Home)
        {
            return;
        }
        m_context = &context;
        auto& saved = m_saved.emplace();
        saved.lock = std::move(home.lock);
        saved.scopes = context.m_scopes.size();
        saved.epoch = context.m_epoch;
        saved.templateFrame = context.m_templateFrame;
        // A body of another template than its caller's (an imported macro) runs in a frame of
        // its template entered from the caller's, so that `self` the caller passes still finds
        // the caller's template (MakeTemplateSelf). One per call: the caller's frames may lead
        // back to the template's own
        if (context.m_templateFrame && context.m_templateFrame != caller->m_templateFrame)
        {
            auto& frame = saved.callFrame.emplace(*context.m_templateFrame);
            frame.outer = caller->m_templateFrame;
            context.m_templateFrame = &frame;
        }
        saved.loopControl = context.TakeLoopControl();
        saved.autoescape = context.m_autoescape;
        saved.outputAutoescape = context.m_outputAutoescape;
        // The body runs with its caller's runtime autoescape when they share an EvalContext:
        // a `{% autoescape %}` the call is in may be set on a context nested in the home
        if (context.m_evalContext == caller->m_evalContext)
        {
            context.m_autoescape = caller->m_autoescape;
        }
        if (context.m_lookupCache)
        {
            if (home.epoch == ClosureHome::Epoch::Closure)
            {
                context.m_epoch = m_site.EpochIn(context, saved.epoch);
            }
            else if (home.epoch == ClosureHome::Epoch::Fresh)
            {
                context.m_epoch = context.m_lookupCache->NewEpoch();
            }
        }
        saved.bodyEpoch = context.m_epoch;
    }
    JINJA2CPP_NOINLINE_INLINE static void Leave(RenderContext& context, const HomeCallSaved& saved) // NOLINT(bugprone-exception-escape): leaving scopes and dropping views free memory only
    {
        const size_t scopes = saved.scopes;
        // Left by an exception: the scopes and views the body pushed are still there
        if (context.m_scopes.size() != scopes)
        {
            assert(context.m_scopes.size() > scopes);
            auto views = context.m_views.size();
            while (views != 0 && context.m_views[views - 1].scopeIndex >= scopes)
            {
                --views;
            }
            context.TruncateViews(views);
            while (context.m_scopes.size() > scopes)
            {
                context.ExitScope();
            }
        }
        // The body's own `break` or `continue` never reaches the home's loops
        context.m_loopControl = saved.loopControl;
        context.m_templateFrame = saved.templateFrame;
        context.m_autoescape = saved.autoescape;
        context.m_outputAutoescape = saved.outputAutoescape;
        if (context.m_epoch != saved.bodyEpoch)
        {
            context.NewEpoch();
        }
        else
        {
            context.m_epoch = saved.epoch;
        }
    }

    const ClosureSite& m_site;
    RenderContext* m_context;
    size_t m_link;
    // The site's state before the call
    uint8_t m_state;
    // Set by a call that switched the home: Leave restores it
    std::optional<HomeCallSaved> m_saved;
#ifndef NDEBUG
    size_t m_debugScopes = 0;
    int m_debugExceptions = 0;
#endif
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDER_CONTEXT_H
