#ifndef JINJA2CPP_SRC_RENDER_CONTEXT_H
#define JINJA2CPP_SRC_RENDER_CONTEXT_H

#include "internal_value.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>

#include <nonstd/expected.hpp>

#include <algorithm>
#include <array>
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
};

// The slots where names were last found (docs/tasks/0100 idea 7). An entry holds the slot
// a name expression resolved to and the epoch of the context it was looked up in; a context
// takes a new epoch whenever a lookup could resolve elsewhere (a name added to a scope, a
// scope cleared, left or bound) and on each copy, so an entry of an older epoch is never
// used. Epochs never repeat, so one cache serves every render on a thread, and a render
// does not pay for clearing it.
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
        const InternalValue* slot = nullptr;
    };

    uint64_t NewEpoch() { return ++m_lastEpoch; }
    // Drops the entry of a key that is going away: an expression made during a render (the
    // `_` alias builds one per call) may be followed by another at the same address while
    // the epoch is still current
    void Forget(const void* key)
    {
        auto& entry = At(key);
        if (entry.key == key)
        {
            entry.key = nullptr;
        }
    }
    // `key` is the expression that looks the name up; two of them may share an entry
    Entry& At(const void* key)
    {
        const auto hash = static_cast<uint64_t>(reinterpret_cast<uintptr_t>(key)) * 0x9E3779B97F4A7C15ULL;
        return m_entries[static_cast<size_t>(hash >> (64 - SizeBits))];
    }

private:
    static constexpr int SizeBits = 7;
    uint64_t m_lastEpoch = 0;
    std::array<Entry, size_t{ 1 } << SizeBits> m_entries{};
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
        , m_parent(other.m_parent)
        , m_parentDepth(other.m_parentDepth)
        , m_boundDepth(other.m_boundDepth)
        , m_scopes(other.m_scopes)
        , m_autoescape(other.m_autoescape)
        , m_lookupCache(other.m_lookupCache)
    {
        m_currentScope = &m_scopes.back();
        NewEpoch();
    }
    // A move is the copy above: m_currentScope must point into this object's m_scopes.
    // NOLINTNEXTLINE(performance-noexcept-move-constructor): copying the scopes can throw
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
        , m_parent(&other)
        , m_parentDepth(std::min(depth, other.GetScopesCount()))
        , m_boundDepth(std::min(other.m_boundDepth, m_parentDepth))
        , m_autoescape(other.m_autoescape)
        , m_lookupCache(other.m_lookupCache)
    {
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

    // The entry of the name `val` (a std::string or a HashedName) in the innermost scope that
    // has it; null when none does
    template<typename Key>
    JINJA2CPP_ALWAYS_INLINE const InternalValueMap::value_type* FindValue(const Key& val, bool& found) const
    {
        auto finder = [&val](const InternalValueMap& map) { return FindIn(map, val); };

        if (m_boundScope)
        {
            const auto* result = FindValueInMacroOfModule(ToHashedName(val));
            found = result != nullptr;
            return result;
        }

        // The scopes of this context, then the ones it sees of its parents. Scopes past the
        // first chunk are rare and searched out of line
        size_t count = m_scopes.size();
        size_t limit = m_parentDepth;
        for (const auto* ctx = this; ctx;)
        {
            if (count > ScopeStack::ChunkSize)
            {
                const auto* result = FindInDeepScopes(ctx->m_scopes, count, ToHashedName(val));
                if (result)
                {
                    found = true;
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
                    found = true;
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
                found = true;
                return result;
            }
            map = idx == 0 ? m_globalScope : (idx == 1 ? m_builtinScope : nullptr);
        }
        return nullptr;
    }

    // FindValue for the name expression `key`, through the lookup cache. Only names that are
    // found are cached: an undefined name is rare and may be defined by the next statement.
    JINJA2CPP_ALWAYS_INLINE const InternalValue* FindValueCached(const void* key, const HashedName& name)
    {
        LookupCache::Entry* entry = nullptr;
        if (m_lookupCache)
        {
            entry = &m_lookupCache->At(key);
            if (entry->key == key && entry->epoch == m_epoch)
            {
#ifndef NDEBUG
                bool found = false;
                const auto* p = FindValue(name, found);
                assert(found && &p->second == entry->slot);
#endif
                return entry->slot;
            }
        }
        bool found = false;
        const auto* p = FindValue(name, found);
        if (!p)
        {
            return nullptr;
        }
        if (entry)
        {
            *entry = { key, m_epoch, &p->second };
        }
        return &p->second;
    }

    // The variable `self` (docs/tasks/0139): a name set in the scopes of the running template
    // wins, else the template itself, made on first use (Jinja2's TemplateReference). Defined
    // with TemplateFrame in statements.cpp
    const InternalValue* FindSelf(const std::string& name);

    // Where the variable `name` is stored, so that a list or dict the template changes in
    // place can be stored back (docs/tasks/0020); null when it is not found or cannot be
    // written. The external and global scopes are copies made for this render, so writing
    // to them never changes the caller's data; the global scope is kept for the next render
    // on the thread, so its renderer is told (docs/tasks/0139). The built-in scope is shared
    // and never written. A name of the bound module is not writable.
    InternalValue* FindValueSlot(const std::string& name)
    {
        InternalValue* result = nullptr;
        const bool isFound = VisitScopes(*this, [&](InternalValueMap* scope) {
            if (!scope)
            {
                return m_boundScope->find(name) != m_boundScope->end();
            }
            auto valP = scope->find(name);
            if (valP == scope->end())
            {
                return false;
            }
            result = &valP->second;
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
            return &valP->second;
        }
        auto* global = const_cast<InternalValueMap*>(m_globalScope);
        auto globalP = global->find(name);
        if (globalP == global->end())
        {
            return nullptr;
        }
        if (m_rendererCallback)
        {
            m_rendererCallback->GlobalScopeWritten();
        }
        return &globalP->second;
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
        NewEpoch();
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
        if (m_scopes != other.m_scopes)
        {
            return false;
        }
        return m_autoescape == other.m_autoescape;
    }

private:
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
    [[nodiscard]] const InternalValue* FindInScopesFrom(const std::string& name, size_t minDepth) const
    {
        size_t limit = GetScopesCount();
        for (const auto* ctx = this; ctx; ctx = ctx->m_parent)
        {
            for (size_t idx = ctx->VisibleScopesCount(limit); idx != 0; --idx)
            {
                if (ctx->m_parentDepth + idx <= minDepth)
                {
                    return nullptr;
                }
                const auto* p = FindIn(ctx->m_scopes[idx - 1], name);
                if (p)
                {
                    return &p->second;
                }
            }
            limit = std::min(limit, ctx->m_parentDepth);
        }
        return nullptr;
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
    // Calls `visit` with each scope `ctx` sees, innermost first, and with null where the bound
    // module scope goes, until `visit` returns true; returns whether it did
    template<typename Context, typename Visit>
    static bool VisitScopes(Context& ctx, const Visit& visit)
    {
        bool isBoundSeen = !ctx.m_boundScope;
        size_t limit = ctx.GetScopesCount();
        for (auto* cur = &ctx; cur; cur = cur->m_parent)
        {
            for (size_t idx = cur->VisibleScopesCount(limit); idx != 0; --idx)
            {
                if (!isBoundSeen && cur->m_parentDepth + idx <= ctx.m_boundDepth)
                {
                    isBoundSeen = true;
                    if (visit(nullptr))
                    {
                        return true;
                    }
                }
                if (visit(&cur->m_scopes[idx - 1]))
                {
                    return true;
                }
            }
            limit = std::min(limit, cur->m_parentDepth);
        }
        return !isBoundSeen && visit(nullptr);
    }
    // FindValue inside an imported macro: its own scopes, then its module, then the scopes
    // it was called in (docs/tasks/0038; 0117 P0). Out of line, so that the common path stays short;
    // `val` is taken by value for the same reason as in FindInDeepScopes
    [[nodiscard]] JINJA2CPP_NOINLINE_INLINE const InternalValueMap::value_type* FindValueInMacroOfModule(HashedName val) const
    {
        const InternalValueMap::value_type* result = nullptr;
        if (VisitScopes(*this, [&](const InternalValueMap* scope) {
                result = FindIn(scope ? *scope : *m_boundScope, val);
                return result != nullptr;
            }))
        {
            return result;
        }
        const InternalValueMap* map = m_externalScope;
        for (int idx = 0; map; ++idx)
        {
            result = FindIn(*map, val);
            if (result)
            {
                return result;
            }
            map = idx == 0 ? m_globalScope : (idx == 1 ? m_builtinScope : nullptr);
        }
        return nullptr;
    }
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
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDER_CONTEXT_H
