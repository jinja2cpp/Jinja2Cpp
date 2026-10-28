#include "cycle_collector.h"

#include "internal_value.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{

namespace
{
struct Node
{
    const ValueObject* object = nullptr;
    // References from the other nodes (ValueObject::Traverse) and from the collector's candidates
    size_t internalRefs = 0;
    bool reachable = false;
};
} // namespace

void CycleCollector::Finish() noexcept
{
    m_notes->cells.Release(this);
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
    catch (...) // NOLINT(bugprone-empty-catch): out of memory; the outer collector misses what is not handed over
    {
        // What the outer one did not take is collected here: the render that made it ended
        Collect();
    }
}

BorrowedCells* CycleCollector::CellsForWrite()
{
    auto* collector = s_current;
    if (!collector)
    {
        return nullptr;
    }
    if (collector->m_external)
    {
        return collector->m_external;
    }
    if (!collector->m_notes)
    {
        collector->m_notes = std::make_unique<Notes>();
    }
    return &collector->m_notes->cells;
}

void CycleCollector::NoteStore(const InternalValue& container)
{
    auto* collector = s_current;
    if (!collector || (container.Kind() != ValueKind::List && container.Kind() != ValueKind::Map))
    {
        return;
    }
    if (auto object = OwnObjectOf(container))
    {
        collector->Add(std::move(object));
    }
}

void CycleCollector::Add(ValueObjectPtr object)
{
    if (!m_notes)
    {
        m_notes = std::make_unique<Notes>();
    }
    if (m_notes->noted.count(object.get()) != 0)
    {
        return;
    }
    // Pushed first, so that a failed insert leaves nothing marked noted that is not a
    // candidate, which a later note of it would skip (docs/tasks/0283)
    auto& candidates = m_notes->candidates;
    candidates.push_back(std::move(object));
    const auto* noted = candidates.back().get();
    try
    {
        m_notes->noted.insert(noted);
    }
    catch (...)
    {
        candidates.pop_back();
        throw;
    }
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
    // An object that only this collector holds is in no cycle: a cycle would hold it too
    auto& candidates = m_notes->candidates;
    auto& noted = m_notes->noted;
    const auto first = std::remove_if(candidates.begin(), candidates.end(), [&noted](const ValueObjectPtr& candidate) {
        if (candidate->use_count() != 1)
        {
            return false;
        }
        noted.erase(candidate.get());
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
    // A walk that runs out of memory frees its tables when it unwinds, so a second one has
    // what the first held. At the end of a render its scopes are gone too, so a single
    // failed allocation (the allocation-failure sweep) does not stop the second walk. When
    // memory stays exhausted the cycles not broken leak (docs/tasks/0283)
    for (int attempt = 0; attempt != 2; ++attempt)
    {
        try
        {
            CollectOnce();
            break;
        }
        catch (...) // NOLINT(bugprone-empty-catch): retried once, then the comment above says what is lost
        {
        }
    }
    m_notes->candidates.clear();
    m_notes->noted.clear();
}

void CycleCollector::CollectOnce()
{
    // Most of what a render noted is garbage by now with no cycle in it (a list built and
    // printed): releasing it frees it, and what only it held, without a walk
    while (DropUnshared())
    {
    }
    auto& candidates = m_notes->candidates;
    if (candidates.empty())
    {
        return;
    }
    std::unordered_map<const ValueObject*, Node> nodes;
    // In the order found: `order` grows while it is walked
    std::vector<Node*> order;
    auto visit = [&nodes, &order](const ValueObject* object) {
        // Strings, numbers and host adapters hold no template value
        if (!object->IsTraversable())
        {
            return;
        }
        auto [p, isNew] = nodes.try_emplace(object);
        if (isNew)
        {
            p->second.object = object;
            order.push_back(&p->second);
        }
        ++p->second.internalRefs;
    };

    for (const auto& candidate : candidates)
    {
        visit(candidate.get());
    }
    // NOLINTNEXTLINE(modernize-loop-convert): Traverse appends to `order`, which a range-for would not see
    for (size_t next = 0; next != order.size(); ++next)
    {
        order[next]->object->Traverse(visit);
    }

    // A node with more references than the walk found is held from outside, and so is
    // every node it reaches
    std::vector<const Node*> pending;
    for (auto* node : order)
    {
        if (node->object->use_count() > node->internalRefs)
        {
            node->reachable = true;
            pending.push_back(node);
        }
    }
    auto markReachable = [&nodes, &pending](const ValueObject* object) {
        if (!object->IsTraversable())
        {
            return;
        }
        auto& child = nodes.at(object);
        if (!child.reachable)
        {
            child.reachable = true;
            pending.push_back(&child);
        }
    };
    while (!pending.empty())
    {
        const auto* node = pending.back();
        pending.pop_back();
        node->object->Traverse(markReachable);
    }

    // The garbage is held here while it is emptied, so that emptying one object frees none
    // that is still to be emptied; letting go of it then frees all of it
    std::vector<ValueObjectPtr> garbage;
    for (const auto* node : order)
    {
        if (!node->reachable)
        {
            garbage.emplace_back(node->object);
        }
    }
    // Nothing below allocates. The tables are not read again: emptying may free what they name
    candidates.clear();
    for (const auto& object : garbage)
    {
        object->Clear();
    }
}

} // namespace v2
} // namespace jinja2
