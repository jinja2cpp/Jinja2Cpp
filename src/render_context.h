#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include "internal_value.h"

#include <nonstd/expected.hpp>
#include <jinja2cpp/error_info.h>

#include <list>

namespace jinja2
{
template<typename CharT>
class TemplateImpl;

struct IRendererCallback
{
    virtual TargetString GetAsTargetString(const InternalValue& val) = 0;
    virtual OutStream GetStreamOnString(TargetString& str) = 0;
    virtual nonstd::variant<EmptyValue,
        nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
        nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>> LoadTemplate(const std::string& fileName) const = 0;
    virtual nonstd::variant<EmptyValue,
        nonstd::expected<std::shared_ptr<TemplateImpl<char>>, ErrorInfo>,
        nonstd::expected<std::shared_ptr<TemplateImpl<wchar_t>>, ErrorInfoW>> LoadTemplate(const InternalValue& fileName) const = 0;
    virtual void ThrowRuntimeError(ErrorCode code, ValuesList extraParams) = 0;
};

class RenderContext
{
public:
    RenderContext(const InternalValueMap& extValues, IRendererCallback* rendererCallback)
        : m_rendererCallback(rendererCallback)
    {
        m_externalScope = &extValues;
        EnterScope();
        (*m_currentScope)["self"] = MapAdapter::CreateAdapter(InternalValueMap());
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
        auto finder = [&val, &found](auto& map) mutable
        {
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

        for (auto p = m_scopes.rbegin(); p != m_scopes.rend(); ++ p)
        {
            auto valP = finder(*p);
            if (found)
                return valP;
        }
        return finder(*m_externalScope);
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
    auto GetRendererCallback()
    {
        return m_rendererCallback;
    }
    RenderContext Clone(bool includeCurrentContext) const
    {
        if (!includeCurrentContext)
            return RenderContext(m_emptyScope, m_rendererCallback);

        return RenderContext(*this);
    }

    void BindScope(InternalValueMap* scope)
    {
        m_boundScope = scope;
    }
private:
    InternalValueMap* m_currentScope;
    const InternalValueMap* m_externalScope;
    InternalValueMap m_emptyScope;
    std::list<InternalValueMap> m_scopes;
    IRendererCallback* m_rendererCallback;
    const InternalValueMap* m_boundScope = nullptr;
};
} // jinja2

#endif // RENDER_CONTEXT_H
