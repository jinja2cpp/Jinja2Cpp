#include "render_context.h"

#include "internal_value.h"

#include <cstddef>
#include <memory>

namespace jinja2
{
InternalValueMap* ScopeStack::DeepSlot()
{
    const size_t chunk = m_size / ChunkSize;
    if (chunk > m_more.size())
    {
        m_more.push_back(std::make_unique<Chunk>());
    }
    return &m_more[chunk - 1]->maps[m_size % ChunkSize];
}

const InternalValueMap::value_type* RenderContext::FindInDeepScopes(const ScopeStack& scopes, size_t count, HashedName name)
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
} // namespace jinja2
