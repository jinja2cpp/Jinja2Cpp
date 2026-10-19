#include "cycle_collector.h"

#include "internal_value.h"
#include "template_dict.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{

namespace
{
// A container the walk can follow: its accessor is the value's own object and the only
// holder of its items, so emptying it changes nothing that another accessor sees
struct Container
{
    const ValueObject* object = nullptr;
    InternalValueList* list = nullptr;
    TemplateDict* dict = nullptr;
};

Container FindContainer(const InternalValue& val)
{
    Container result;
    if (auto list = AsList(val))
    {
        const auto* accessor = list->GetAccessor();
        if (accessor && static_cast<const ValueObject*>(accessor) == list->GetOwner() && accessor->OwnsMutableItemsAlone())
        {
            result.list = accessor->GetMutableItems();
            result.object = result.list ? list->GetOwner() : nullptr;
        }
    }
    else if (auto map = AsMap(val))
    {
        const auto* accessor = map->GetAccessor();
        if (accessor && static_cast<const ValueObject*>(accessor) == map->GetOwner() && accessor->OwnsMutableItemsAlone())
        {
            result.dict = accessor->GetMutableItems();
            result.object = result.dict ? map->GetOwner() : nullptr;
        }
    }
    return result;
}

const ValueObject* ObjectOf(const InternalValue& val)
{
    if (auto list = AsList(val))
    {
        return list->GetOwner();
    }
    if (auto map = AsMap(val))
    {
        return map->GetOwner();
    }
    return nullptr;
}

struct Node
{
    Container container;
    // References from the items of other nodes and from the collector's candidates
    size_t internalRefs = 0;
    bool reachable = false;
};

template<typename Fn>
void ForEachItem(const Container& container, const Fn& fn)
{
    if (container.list)
    {
        for (const auto& item : *container.list)
        {
            fn(item);
        }
    }
    else
    {
        for (const auto& [key, item] : *container.dict)
        {
            fn(item);
            // A key that is not a string keeps its first spelling, a tuple that can hold a container
            if (container.dict->HasKeyValues())
            {
                fn(container.dict->KeyOf(key));
            }
        }
    }
}
} // namespace

void CycleCollector::Finish() noexcept
{
    if (!m_outer)
    {
        Collect();
        return;
    }
    try
    {
        for (const auto& candidate : m_notes->candidates)
        {
            m_outer->Add(candidate);
        }
    }
    catch (...) // NOLINT(bugprone-empty-catch): out of memory; what is not handed over may leak
    {
    }
}

void CycleCollector::NoteStore(const InternalValue& container)
{
    if (auto* collector = s_current)
    {
        collector->Add(container);
    }
}

void CycleCollector::Add(const InternalValue& container)
{
    const void* object = ObjectOf(container);
    if (!object)
    {
        return;
    }
    if (!m_notes)
    {
        m_notes = std::make_unique<Notes>();
    }
    if (!m_notes->noted.insert(object).second)
    {
        return;
    }
    auto& candidates = m_notes->candidates;
    candidates.push_back(container);
    if (candidates.size() < m_notes->pruneAt)
    {
        return;
    }
    // Dropping the ones nothing else holds now keeps a loop that fills a temporary list in
    // constant memory, as before the collector. The next pass waits until the survivors have
    // doubled, which keeps notes amortized O(1)
    DropUnshared();
    m_notes->pruneAt = std::max(Notes::MinPruneAt, 2 * candidates.size());
}

bool CycleCollector::DropUnshared()
{
    // A container that only this collector holds is in no cycle: a cycle would hold it too
    auto& candidates = m_notes->candidates;
    auto& noted = m_notes->noted;
    const auto first = std::remove_if(candidates.begin(), candidates.end(), [&noted](const InternalValue& candidate) {
        const auto* owner = ObjectOf(candidate);
        if (owner->use_count() != 1)
        {
            return false;
        }
        noted.erase(owner);
        return true;
    });
    if (first == candidates.end())
    {
        return false;
    }
    candidates.erase(first, candidates.end());
    return true;
}

void CycleCollector::Collect() noexcept
{
    // The items of the containers found garbage: released after the walk, when every
    // container they keep alive has been emptied
    std::vector<InternalValueList> releasedLists;
    std::vector<TemplateDict> releasedDicts;
    try
    {
        // Most of what a render noted is garbage by now with no cycle in it (a list built and
        // printed): releasing it frees it, and what only it held, without a walk
        while (DropUnshared())
        {
        }
        std::unordered_map<const ValueObject*, Node> nodes;
        std::vector<Node*> order;
        auto visit = [&nodes, &order](const InternalValue& val) -> Node* {
            const auto container = FindContainer(val);
            if (!container.object)
            {
                return nullptr;
            }
            auto [p, isNew] = nodes.try_emplace(container.object);
            if (isNew)
            {
                p->second.container = container;
                order.push_back(&p->second);
            }
            ++p->second.internalRefs;
            return &p->second;
        };

        for (const auto& candidate : m_notes->candidates)
        {
            visit(candidate);
        }
        // `order` grows while it is walked
        size_t next = 0;
        while (next != order.size())
        {
            ForEachItem(order[next++]->container, visit);
        }

        // A node with more references than the walk found is held from outside, and so is
        // every node it reaches
        std::vector<const Node*> pending;
        for (auto* node : order)
        {
            if (node->container.object->use_count() > node->internalRefs)
            {
                node->reachable = true;
                pending.push_back(node);
            }
        }
        while (!pending.empty())
        {
            const auto* node = pending.back();
            pending.pop_back();
            ForEachItem(node->container, [&nodes, &pending](const InternalValue& item) {
                const auto container = FindContainer(item);
                if (!container.object)
                {
                    return;
                }
                auto& child = nodes.at(container.object);
                if (!child.reachable)
                {
                    child.reachable = true;
                    pending.push_back(&child);
                }
            });
        }

        for (const auto* node : order)
        {
            if (node->reachable)
            {
                continue;
            }
            if (node->container.list)
            {
                releasedLists.push_back(std::move(*node->container.list));
                node->container.list->clear();
            }
            else
            {
                releasedDicts.push_back(std::move(*node->container.dict));
                node->container.dict->Clear();
            }
        }
    }
    catch (...) // NOLINT(bugprone-empty-catch): out of memory; the cycles not broken leak
    {
    }
    m_notes->candidates.clear();
}

} // namespace v2
} // namespace jinja2
