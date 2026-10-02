#ifndef JINJA2CPP_SRC_RENDER_CONTEXT_H
#define JINJA2CPP_SRC_RENDER_CONTEXT_H

#include "internal_value.h"
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/utils/i_comparable.h>

#include <nonstd/expected.hpp>

#include <algorithm>
#include <list>
#include <deque>

namespace jinja2
{
template<typename CharT>
class TemplateImpl;

struct TemplateFrame;

struct IRendererCallback : IComparable
{
    virtual ~IRendererCallback() {}
    virtual TargetString GetAsTargetString(const InternalValue& val) = 0;
    virtual OutStream GetStreamOnString(TargetString& str) = 0;
    virtual nonstd::variant<EmptyValue,
                            nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
                            nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>>
    LoadTemplate(const std::string& fileName) const = 0;
    virtual nonstd::variant<EmptyValue,
                            nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
                            nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>>
    LoadTemplate(const InternalValue& fileName) const = 0;
    virtual void ThrowRuntimeError(ErrorCode code, ValuesList extraParams) = 0;
    virtual const Settings& GetSettings() const = 0;
};

class RenderContext
{
public:
    RenderContext(const InternalValueMap& extValues, const InternalValueMap& globalValues, IRendererCallback* rendererCallback)
        : m_rendererCallback(rendererCallback)
    {
        m_externalScope = &extValues;
        m_globalScope = &globalValues;
        EnterScope();
    }

    RenderContext(const RenderContext& other)
        : m_rendererCallback(other.m_rendererCallback)
        , m_externalScope(other.m_externalScope)
        , m_globalScope(other.m_globalScope)
        , m_boundScope(other.m_boundScope)
        , m_templateFrame(other.m_templateFrame)
        , m_scopes(other.m_scopes)
        , m_autoescape(other.m_autoescape)
    {
        m_currentScope = &m_scopes.back();
    }

    // A copy that sees only the first `depth` scopes, plus a fresh one on top
    RenderContext(const RenderContext& other, size_t depth)
        : m_rendererCallback(other.m_rendererCallback)
        , m_externalScope(other.m_externalScope)
        , m_globalScope(other.m_globalScope)
        , m_boundScope(other.m_boundScope)
        , m_templateFrame(other.m_templateFrame)
        , m_scopes(other.m_scopes.begin(), other.m_scopes.begin() + static_cast<std::ptrdiff_t>(std::min(depth, other.m_scopes.size())))
        , m_autoescape(other.m_autoescape)
    {
        EnterScope();
    }

    InternalValueMap& EnterScope()
    {
        m_scopes.push_back(InternalValueMap());
        m_currentScope = &m_scopes.back();
        return *m_currentScope;
    }

    void ExitScope()
    {
        m_scopes.pop_back();
        if (!m_scopes.empty())
            m_currentScope = &m_scopes.back();
        else
            m_currentScope = nullptr;
    }

    auto FindValue(const std::string& val, bool& found) const
    {
        auto finder = [&val, &found](auto& map) mutable {
            auto p = map.find(val);
            if (p != map.end())
                found = true;

            return p;
        };

        if (m_boundScope)
        {
            auto valP = finder(*m_boundScope);
            if (found)
                return valP;
        }

        for (auto p = m_scopes.rbegin(); p != m_scopes.rend(); ++p)
        {
            auto valP = finder(*p);
            if (found)
                return valP;
        }

        auto valP = finder(*m_externalScope);
        if (found)
            return valP;

        return finder(*m_globalScope);
    }

    // Where the variable `name` is stored, so that a list or dict the template changes in
    // place can be stored back (docs/tasks/0020); null when it is not found or cannot be
    // written. The external and global scopes are copies made for this render, so writing
    // to them never changes the caller's data.
    InternalValue* FindValueSlot(const std::string& name)
    {
        if (m_boundScope)
        {
            auto p = m_boundScope->find(name);
            if (p != m_boundScope->end())
                return nullptr;
        }
        for (auto p = m_scopes.rbegin(); p != m_scopes.rend(); ++p)
        {
            auto valP = p->find(name);
            if (valP != p->end())
                return &valP->second;
        }
        for (auto* scope : { m_externalScope, m_globalScope })
        {
            auto valP = scope->find(name);
            if (valP != scope->end())
                return const_cast<InternalValue*>(&valP->second);
        }
        return nullptr;
    }

    auto& GetCurrentScope() const
    {
        return *m_currentScope;
    }

    auto& GetCurrentScope()
    {
        return *m_currentScope;
    }
    auto& GetGlobalScope()
    {
        return m_scopes.front();
    }
    size_t GetScopesCount() const
    {
        return m_scopes.size();
    }
    auto GetRendererCallback()
    {
        return m_rendererCallback;
    }
    RenderContext Clone(bool includeCurrentContext) const
    {
        if (!includeCurrentContext)
        {
            RenderContext result(m_emptyScope, *m_globalScope, m_rendererCallback);
            result.m_templateFrame = m_templateFrame;
            result.m_autoescape = m_autoescape;
            return result;
        }

        return RenderContext(*this);
    }

    // The template whose code is running: its blocks and the parent set by `extends`
    TemplateFrame* GetTemplateFrame() const
    {
        return m_templateFrame;
    }
    TemplateFrame* SetTemplateFrame(TemplateFrame* frame)
    {
        std::swap(frame, m_templateFrame);
        return frame;
    }

    // Whether `{{ }}` output is HTML-escaped here (Jinja2's eval_ctx.autoescape)
    bool IsAutoescape() const { return m_autoescape; }
    bool SetAutoescape(bool autoescape)
    {
        std::swap(autoescape, m_autoescape);
        return autoescape;
    }

    void BindScope(InternalValueMap* scope)
    {
        m_boundScope = scope;
    }

    bool IsEqual(const RenderContext& other) const
    {
        if (!IsEqual(m_rendererCallback, other.m_rendererCallback))
            return false;
        if (!IsEqual(this->m_currentScope, other.m_currentScope))
            return false;
        if (!IsEqual(m_externalScope, other.m_externalScope))
            return false;
        if (!IsEqual(m_globalScope, other.m_globalScope))
            return false;
        if (!IsEqual(m_boundScope, other.m_boundScope))
            return false;
        if (m_emptyScope != other.m_emptyScope)
            return false;
        if (m_scopes != other.m_scopes)
            return false;
        return m_autoescape == other.m_autoescape;
    }

private:
    bool IsEqual(const IRendererCallback* lhs, const IRendererCallback* rhs) const
    {
        if (lhs && rhs)
            return lhs->IsEqual(*rhs);
        if ((!lhs && rhs) || (lhs && !rhs))
            return false;
        return true;
    }

    bool IsEqual(const InternalValueMap* lhs, const InternalValueMap* rhs) const
    {
        if (lhs && rhs)
            return *lhs == *rhs;
        if ((!lhs && rhs) || (lhs && !rhs))
            return false;
        return true;
    }

private:
    IRendererCallback* m_rendererCallback{};
    InternalValueMap* m_currentScope{};
    const InternalValueMap* m_externalScope{};
    const InternalValueMap* m_globalScope{};
    const InternalValueMap* m_boundScope{};
    TemplateFrame* m_templateFrame{};
    InternalValueMap m_emptyScope;
    std::deque<InternalValueMap> m_scopes;
    bool m_autoescape{};
};

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
