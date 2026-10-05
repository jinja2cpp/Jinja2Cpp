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
#include <deque>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <utility>
#include <variant>

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
    void Clear();
    [[nodiscard]] bool empty() const { return m_map->empty(); }
    [[nodiscard]] const InternalValueMap& Map() const { return *m_map; }

private:
    RenderContext* m_context;
    InternalValueMap* m_map;
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
            m_scopes.push_back(std::move(*m_spareScope));
            m_spareScope.reset();
        }
        else
        {
            m_scopes.emplace_back();
        }
        m_currentScope = &m_scopes.back();
        return { *this, *m_currentScope };
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

    // val is a std::string or a HashedName
    template<typename Key>
    auto FindValue(const Key& val, bool& found) const
    {
        auto finder = [&val, &found](auto& map) mutable {
            // An empty scope (a loop body without `set`) is skipped without hashing the name
            if (map.empty())
            {
                return map.end();
            }
            auto p = map.find(val);
            if (p != map.end())
            {
                found = true;
            }

            return p;
        };

        if (m_boundScope)
        {
            auto valP = finder(*m_boundScope);
            if (found)
            {
                return valP;
            }
        }

        // A plain backward loop: a reverse_iterator re-decrements the deque iterator on
        // every dereference
        for (auto p = m_scopes.end(); p != m_scopes.begin();)
        {
            --p;
            auto valP = finder(*p);
            if (found)
            {
                return valP;
            }
        }
        // Then the scopes this context sees of its parents
        size_t limit = m_parentDepth;
        for (const auto* ctx = m_parent; ctx; ctx = ctx->m_parent)
        {
            for (auto p = ctx->VisibleScopesEnd(limit); p != ctx->m_scopes.begin();)
            {
                --p;
                auto valP = finder(*p);
                if (found)
                {
                    return valP;
                }
            }
            limit = std::min(limit, ctx->m_parentDepth);
        }

        auto valP = finder(*m_externalScope);
        if (found)
        {
            return valP;
        }

        valP = finder(*m_globalScope);
        if (found || !m_builtinScope)
        {
            return valP;
        }

        return finder(*m_builtinScope);
    }

    // FindValue for the name expression `key`, through the lookup cache. Only names that are
    // found are cached: an undefined name is rare and may be defined by the next statement.
    const InternalValue* FindValueCached(const void* key, const HashedName& name)
    {
        LookupCache::Entry* entry = nullptr;
        if (m_lookupCache)
        {
            entry = &m_lookupCache->At(key);
            if (entry->key == key && entry->epoch == m_epoch)
            {
#ifndef NDEBUG
                bool found = false;
                auto p = FindValue(name, found);
                assert(found && &p->second == entry->slot);
#endif
                return entry->slot;
            }
        }
        bool found = false;
        auto p = FindValue(name, found);
        if (!found)
        {
            return nullptr;
        }
        if (entry)
        {
            *entry = { key, m_epoch, &p->second };
        }
        return &p->second;
    }

    // Where the variable `name` is stored, so that a list or dict the template changes in
    // place can be stored back (docs/tasks/0020); null when it is not found or cannot be
    // written. The external and global scopes are copies made for this render, so writing
    // to them never changes the caller's data; the built-in scope is shared and never written.
    InternalValue* FindValueSlot(const std::string& name)
    {
        if (m_boundScope)
        {
            auto p = m_boundScope->find(name);
            if (p != m_boundScope->end())
            {
                return nullptr;
            }
        }
        size_t limit = GetScopesCount();
        for (auto* ctx = this; ctx; ctx = ctx->m_parent)
        {
            for (auto p = ctx->VisibleScopesEnd(limit); p != ctx->m_scopes.begin();)
            {
                --p;
                auto valP = p->find(name);
                if (valP != p->end())
                {
                    return &valP->second;
                }
            }
            limit = std::min(limit, ctx->m_parentDepth);
        }
        for (const auto* scope : { m_externalScope, m_globalScope })
        {
            auto valP = scope->find(name);
            if (valP != scope->end())
            {
                return const_cast<InternalValue*>(&valP->second);
            }
        }
        return nullptr;
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

    void BindScope(InternalValueMap* scope)
    {
        m_boundScope = scope;
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
        if (m_parent != other.m_parent || m_parentDepth != other.m_parentDepth)
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
    // The end of the scopes of this context that a child seeing `limit` scopes sees
    [[nodiscard]] std::deque<InternalValueMap>::const_iterator VisibleScopesEnd(size_t limit) const
    {
        if (limit >= m_parentDepth + m_scopes.size())
        {
            return m_scopes.end();
        }
        return m_scopes.begin() + static_cast<std::ptrdiff_t>(limit > m_parentDepth ? limit - m_parentDepth : 0);
    }
    std::deque<InternalValueMap>::iterator VisibleScopesEnd(size_t limit)
    {
        if (limit >= m_parentDepth + m_scopes.size())
        {
            return m_scopes.end();
        }
        return m_scopes.begin() + static_cast<std::ptrdiff_t>(limit > m_parentDepth ? limit - m_parentDepth : 0);
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
    std::deque<InternalValueMap> m_scopes;
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
