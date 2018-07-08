#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include "internal_value.h"

#include <list>

namespace jinja2
{
template<typename CharT>
class TemplateImpl;

struct IRendererCallback
{
    virtual TargetString GetAsTargetString(const InternalValue& val) = 0;
    virtual boost::variant<EmptyValue, std::shared_ptr<TemplateImpl<char>>, std::shared_ptr<TemplateImpl<wchar_t>>> LoadTemplate(const std::string& fileName) const = 0;
};

class RenderContext
{
public:
    RenderContext(const InternalValueMap& extValues, IRendererCallback* rendererCallback)
        : m_rendererCallback(rendererCallback)
    {
        m_externalScope = &extValues;
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
        for (auto p = m_scopes.rbegin(); p != m_scopes.rend(); ++ p)
        {
            auto valP = p->find(val);
            if (valP != p->end())
            {
                found = true;
                return valP;
            }
        }
        auto valP = m_externalScope->find(val);
        if (valP != m_externalScope->end())
        {
            found = true;
            return valP;
        }

        found = false;
        return m_externalScope->end();
    }

    auto& GetCurrentScope() const
    {
        return *m_currentScope;
    }

    auto& GetCurrentScope()
    {
        return *m_currentScope;
    }
    auto GetRendererCallback()
    {
        return m_rendererCallback;
    }
    RenderContext Clone(bool includeCurrentContext) const
    {
        if (!includeCurrentContext)
            return RenderContext(*m_externalScope, m_rendererCallback);

        return RenderContext(*this);
    }
private:
    InternalValueMap* m_currentScope;
    const InternalValueMap* m_externalScope;
    std::list<InternalValueMap> m_scopes;
    IRendererCallback* m_rendererCallback;

};
} // jinja2

#endif // RENDER_CONTEXT_H
