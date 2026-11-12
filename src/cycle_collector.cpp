#include "cycle_collector.h"

#include "internal_value.h"
#include "render_workspace.h"

#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>
#include <new>
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

namespace
{
// The walks of CollectExport and of a render that handed the host a container: one at a time,
// since the host's copies may go on any thread
std::mutex& ExportWalks()
{
    static std::mutex walks;
    return walks;
}

// The scope of a walk that ExportWalks orders. The outermost one on a thread takes the lock and,
// before it lets go, walks from the roots of the host's copies that went during its walk: a walk
// frees values, a host's copy among them, whose CollectExport then queues its roots here rather
// than taking the lock again. A render a host's destructor starts during the walk walks inside it,
// without the lock
class ExportWalk
{
public:
    ExportWalk()
    {
        if (!s_held)
        {
            m_lock = std::unique_lock<std::mutex>(ExportWalks());
            s_held = this;
        }
    }
    ExportWalk(const ExportWalk&) = delete;
    ExportWalk(ExportWalk&&) = delete;
    ExportWalk& operator=(const ExportWalk&) = delete;
    ExportWalk& operator=(ExportWalk&&) = delete;
    ~ExportWalk()
    {
        if (s_held != this)
        {
            return;
        }
        while (!m_pending.empty())
        {
            auto roots = std::move(m_pending.back());
            m_pending.pop_back();
            CycleCollector::CollectFrom(std::move(roots));
        }
        s_held = nullptr;
    }

    // The walk this thread holds the lock for, or null. A plain pointer, so that a host's copy
    // freed by another thread_local's destructor at thread exit finds no destroyed state
    static thread_local ExportWalk* s_held;
    std::vector<std::vector<ValueObjectPtr>> m_pending;

private:
    std::unique_lock<std::mutex> m_lock;
};
thread_local ExportWalk* ExportWalk::s_held = nullptr;
} // namespace

void CycleCollector::Finish() noexcept
{
    // First: what only these held is gone before the walk counts the references
    m_notes->kept.Clear();
    m_notes->cells.Release(this);
    if (!m_outer)
    {
        if (m_notes->exportToken)
        {
            // A host's copy that goes from now on walks after this walk, not beside it
            const ExportWalk walk;
            m_notes->exportToken.reset();
            Collect();
            return;
        }
        Collect();
        return;
    }
    try
    {
        for (const auto& candidate : m_notes->candidates)
        {
            m_outer->Add(candidate);
        }
        for (const auto& root : m_notes->roots)
        {
            m_outer->NoteRoot(root);
        }
    }
    catch (...) // NOLINT(bugprone-empty-catch): out of memory; the outer collector misses what is not handed over
    {
        // What the outer one did not take is collected here: the render that made it ended
        Collect();
    }
}

std::weak_ptr<const void> CycleCollector::ExportToken()
{
    auto* collector = s_current;
    if (!collector)
    {
        return {};
    }
    // The outermost render's: a nested one hands what it noted to it
    for (;; collector = collector->m_outer)
    {
        if (collector->m_module)
        {
            return {};
        }
        if (!collector->m_outer)
        {
            break;
        }
    }
    if (!collector->m_notes)
    {
        collector->m_notes = std::make_unique<Notes>();
    }
    if (!collector->m_notes->exportToken)
    {
        collector->m_notes->exportToken = std::make_shared<int>(0);
    }
    return collector->m_notes->exportToken;
}

void CycleCollector::CollectExport(std::vector<ValueObjectPtr> roots) noexcept
{
    if (roots.empty())
    {
        return;
    }
    if (auto* walk = ExportWalk::s_held)
    {
        try
        {
            walk->m_pending.push_back(std::move(roots));
        }
        catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): a cycle these roots close leaks
        {
        }
        return;
    }
    const ExportWalk walk;
    CollectFrom(std::move(roots));
}

bool CycleCollector::KeepUntilEnd(std::shared_ptr<const void> object, std::weak_ptr<const void> owner, const InternalValueMap* names)
{
    auto* collector = s_current;
    if (!collector)
    {
        return false;
    }
    if (collector->m_module)
    {
        collector->m_module->kept.Keep(std::move(object), std::move(owner), names);
        return true;
    }
    if (!collector->m_notes)
    {
        collector->m_notes = std::make_unique<Notes>();
    }
    collector->m_notes->kept.Keep(std::move(object), std::move(owner), names);
    return true;
}

void KeptObjects::Keep(std::shared_ptr<const void> object, std::weak_ptr<const void> owner, const InternalValueMap* names)
{
    if (m_kept.size() >= m_pruneAt)
    {
        m_kept.erase(std::remove_if(m_kept.begin(), m_kept.end(), [](const Entry& entry) { return entry.owner.expired(); }), m_kept.end());
        m_pruneAt = std::max(MinPruneAt, m_kept.size() * 2);
    }
    m_kept.push_back(Entry{ std::move(owner), std::move(object), names });
}

void KeptObjects::NoteNames() const
{
    for (const auto& entry : m_kept)
    {
        if (!entry.names)
        {
            continue;
        }
        for (const auto& [name, value] : *entry.names)
        {
            CycleCollector::NoteStore(value);
        }
    }
}

BorrowedCells* CycleCollector::CellsForWrite()
{
    auto* collector = s_current;
    if (!collector)
    {
        return nullptr;
    }
    if (collector->m_module)
    {
        return &collector->m_module->cells;
    }
    if (!collector->m_notes)
    {
        collector->m_notes = std::make_unique<Notes>();
    }
    return &collector->m_notes->cells;
}

CycleCollector* CycleCollector::SnapshotOwner() noexcept
{
    auto* collector = s_current;
    if (!collector || collector->m_module)
    {
        return nullptr;
    }
    // The chain starts at the running render's own entry: none for code of a module's call, or of
    // a render called back from inside another one (Reentry)
    const auto* running = RenderWorkspace::ForThisThread().Running();
    if (!running || running->collector != collector || !running->owner || !*running->owner)
    {
        return nullptr;
    }
    const auto* snapshot = running->owner->get();
    const auto* owner = running;
    for (const auto* outer = running->outer; outer && outer->owner && outer->owner->get() == snapshot; outer = outer->outer)
    {
        owner = outer;
    }
    return owner != running && owner->collector && !owner->collector->m_module ? owner->collector : nullptr;
}

BorrowedCells* CycleCollector::GlobalCells() noexcept
{
    auto* owner = SnapshotOwner();
    if (!owner)
    {
        return CurrentCells();
    }
    return owner->m_notes ? &owner->m_notes->cells : nullptr;
}

BorrowedCells* CycleCollector::GlobalCellsForWrite()
{
    auto* owner = SnapshotOwner();
    if (!owner)
    {
        return CellsForWrite();
    }
    if (!owner->m_notes)
    {
        owner->m_notes = std::make_unique<Notes>();
    }
    return &owner->m_notes->cells;
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

void CycleCollector::NoteDroppedInCall(const InternalValue& value)
{
    // Strings, numbers and host adapters hold no template value
    if (auto object = OwnObjectOf(value); object && object->IsTraversable())
    {
        Add(std::move(object));
    }
}

void CycleCollector::NoteRoot(ValueObjectPtr cell)
{
    if (!m_notes)
    {
        m_notes = std::make_unique<Notes>();
    }
    m_notes->roots.push_back(std::move(cell));
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
    if (candidates.size() < m_notes->pruneAt || m_notes->walkFromEvery)
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
    m_notes->roots.clear();
}

namespace
{
// Whether `object` is one of `live`, sorted: held by what outlives the walk (ModuleScope::LiveRootsFn)
bool IsLive(const std::vector<const ValueObject*>& live, const ValueObject* object)
{
    return !live.empty() && std::binary_search(live.begin(), live.end(), object);
}

// The walk of CollectOnce from `candidates` and `roots`, which it empties, or, with `keepRoots`,
// leaves with the roots that are not garbage: the garbage among what they reach is emptied, which
// breaks its cycles. The objects in `live` (sorted) are held from outside and are not walked into:
// none of what they reach is garbage, and what the walk finds that only they hold counts as held
// from outside, which it is. Throws std::bad_alloc, and then changes nothing
void WalkAndClear(std::vector<ValueObjectPtr>& candidates, std::vector<ValueObjectPtr>& roots, bool keepRoots = false, const std::vector<const ValueObject*>& live = {})
{
    std::unordered_map<const ValueObject*, Node> nodes;
    // In the order found: `order` grows while it is walked
    std::vector<Node*> order;
    auto visit = [&nodes, &order, &live](const ValueObject* object) {
        // Strings, numbers and host adapters hold no template value
        if (!object->IsTraversable() || IsLive(live, object))
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
    for (const auto& root : roots)
    {
        visit(root.get());
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
    auto markReachable = [&nodes, &pending, &live](const ValueObject* object) {
        if (!object->IsTraversable() || IsLive(live, object))
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
    if (keepRoots)
    {
        // The garbage holds the roots it takes
        roots.erase(std::remove_if(roots.begin(), roots.end(), [&nodes](const ValueObjectPtr& root) {
                        const auto node = nodes.find(root.get());
                        return node != nodes.end() && !node->second.reachable;
                    }),
                    roots.end());
    }
    else
    {
        roots.clear();
    }
    for (const auto& object : garbage)
    {
        object->Clear();
    }
}
} // namespace

void CycleCollector::CollectKeeping(std::vector<ValueObjectPtr>& roots) noexcept
{
    std::vector<ValueObjectPtr> none;
    try
    {
        WalkAndClear(none, roots, true);
    }
    catch (...) // NOLINT(bugprone-empty-catch): the roots stay as they were, for the next walk
    {
    }
}

void CycleCollector::CollectFrom(std::vector<ValueObjectPtr> roots) noexcept
{
    std::vector<ValueObjectPtr> none;
    for (int attempt = 0; attempt != 2 && !roots.empty(); ++attempt)
    {
        try
        {
            WalkAndClear(none, roots);
        }
        catch (...) // NOLINT(bugprone-empty-catch): retried once, then the cycles not broken leak (docs/tasks/0283)
        {
        }
    }
}

void CycleCollector::CollectOnce()
{
    // Most of what a render noted is garbage by now with no cycle in it (a list built and
    // printed): releasing it frees it, and what only it held, without a walk
    // Not when a dropped one may hold a cycle no note names (WalkFromEveryNote)
    while (!m_notes->walkFromEvery && DropUnshared())
    {
    }
    auto& candidates = m_notes->candidates;
    auto& roots = m_notes->roots;
    if (candidates.empty() && roots.empty())
    {
        return;
    }
    std::vector<const ValueObject*> live;
    if (m_module && m_module->liveRoots)
    {
        m_module->liveRoots(m_module->state, live);
        std::sort(live.begin(), live.end());
    }
    WalkAndClear(candidates, roots, false, live);
}

} // namespace v2
} // namespace jinja2
