#ifndef RENDER_CONTEXT_H
#define RENDER_CONTEXT_H

#include "internal_value.h"

#include <list>

namespace jinja2
{
class RenderContext
{
public:
    RenderContext(const InternalValueMap& extValues)
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
private:
    InternalValueMap* m_currentScope;
    const InternalValueMap* m_externalScope;
    std::list<InternalValueMap> m_scopes;

};
} // jinja2

#endif // RENDER_CONTEXT_H
