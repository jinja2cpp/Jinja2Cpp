#include "cycle_collector.h"

#include "internal_value.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <memory>
#include <mutex>
#include <new>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{

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
    // Room for the walk first, so that a note that fails to allocate fails here (docs/tasks/0486)
    m_notes->ReserveWalk();
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
    // Room for the walk first (docs/tasks/0486), then pushed, so that a failed insert leaves nothing marked noted that is not a
    // candidate, which a later note of it would skip (docs/tasks/0283)
    m_notes->ReserveWalk();
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

namespace
{
// Whether `object` is one of `live`, sorted: held by what outlives the walk (ModuleScope::LiveRootsFn)
bool IsLive(const std::vector<const ValueObject*>& live, const ValueObject* object)
{
    return !live.empty() && std::binary_search(live.begin(), live.end(), object);
}

// The walk from `candidates` and `roots`, which it empties, or, with `keepRoots`, leaves with the
// roots that are not garbage: the garbage among what they reach is emptied, which breaks its
// cycles. The objects in `live` (sorted) are held from outside and are not walked into: none of what
// they reach is garbage, and what the walk finds that only they hold counts as held from outside,
// which it is. Allocates only to grow `tables`, and when that fails the walk saturates
// (WalkTables): it frees less, never more
void WalkAndClear(WalkTables& tables, std::vector<ValueObjectPtr>& candidates, std::vector<ValueObjectPtr>& roots, bool keepRoots = false, const std::vector<const ValueObject*>& live = {}) noexcept
{
    tables.Reset();
    auto visit = [&tables, &live](const ValueObject* object) {
        // Strings, numbers and host adapters hold no template value
        if (object->IsTraversable() && !IsLive(live, object))
        {
            tables.Count(object);
        }
    };

    for (const auto& candidate : candidates)
    {
        visit(candidate.get());
    }
    for (const auto& root : roots)
    {
        visit(root.get());
    }
    // The nodes are in the order found, and the walk finds more while it goes. Read by index: growing
    // moves them
    for (size_t next = 0; next != tables.Size(); ++next)
    {
        tables.At(next).object->Traverse(visit);
    }

    // A node with more references than the walk found is held from outside, and so is
    // every node it reaches
    for (size_t index = 0; index != tables.Size(); ++index)
    {
        auto& node = tables.At(index);
        if (node.object->use_count() > node.refs)
        {
            node.reachable = true;
            tables.Push(index);
        }
    }
    auto markReachable = [&tables, &live](const ValueObject* object) {
        if (!object->IsTraversable() || IsLive(live, object))
        {
            return;
        }
        // Not tracked: the walk saturated before it found this object
        const auto index = tables.IndexOf(object);
        if (index == tables.Size())
        {
            return;
        }
        auto& child = tables.At(index);
        if (!child.reachable)
        {
            child.reachable = true;
            tables.Push(index);
        }
    };
    size_t index = 0;
    while (tables.Pop(index))
    {
        tables.At(index).object->Traverse(markReachable);
    }

    // The garbage is held while it is emptied, so that emptying one object frees none that is
    // still to be emptied; letting go of it then frees all of it. A reference taken here needs no
    // storage
    for (size_t i = 0; i != tables.Size(); ++i)
    {
        if (!tables.At(i).reachable)
        {
            intrusive_ptr_add_ref(tables.At(i).object);
        }
    }
    if (keepRoots)
    {
        // The garbage holds the roots it takes
        roots.erase(std::remove_if(roots.begin(), roots.end(), [&tables](const ValueObjectPtr& root) {
                        const auto node = tables.IndexOf(root.get());
                        return node != tables.Size() && !tables.At(node).reachable;
                    }),
                    roots.end());
    }
    else
    {
        roots.clear();
    }
    candidates.clear();
    // The tables are the walk's own, and what they name that is garbage is held: emptying an object
    // may free others and start a render, but nothing else reads the tables
    for (size_t i = 0; i != tables.Size(); ++i)
    {
        if (!tables.At(i).reachable)
        {
            tables.At(i).object->Clear();
        }
    }
    for (size_t i = 0; i != tables.Size(); ++i)
    {
        if (!tables.At(i).reachable)
        {
            // Adopts the reference taken above and lets it go
            const ValueObjectPtr release(tables.At(i).object, false);
        }
    }
}
} // namespace

void CycleCollector::Collect() noexcept
{
    // Most of what a render noted is garbage by now with no cycle in it (a list built and
    // printed): releasing it frees it, and what only it held, without a walk
    // Not when a dropped one may hold a cycle no note names (WalkFromEveryNote)
    while (!m_notes->walkFromEvery && DropUnshared())
    {
    }
    auto& candidates = m_notes->candidates;
    auto& roots = m_notes->roots;
    if (!candidates.empty() || !roots.empty())
    {
        std::vector<const ValueObject*> live;
        bool known = true;
        if (m_module && m_module->liveRoots)
        {
            try
            {
                m_module->liveRoots(m_module->state, live);
                std::sort(live.begin(), live.end());
            }
            catch (...) // NOLINT(bugprone-empty-catch): the comment below says what is lost
            {
                // Out of memory: without the live roots a walk could free what outlives it, so there is none
                // and the cycles not broken leak (docs/tasks/0283)
                known = false;
            }
        }
        if (known)
        {
            // Out of the notes first: a release during the walk can start a render that hands notes here
            WalkTables tables = std::move(m_notes->walk);
            WalkAndClear(tables, candidates, roots, false, live);
        }
    }
    m_notes->candidates.clear();
    m_notes->noted.clear();
    m_notes->roots.clear();
}

void CycleCollector::CollectKeeping(std::vector<ValueObjectPtr>& roots) noexcept
{
    std::vector<ValueObjectPtr> none;
    WalkTables tables;
    WalkAndClear(tables, none, roots, true);
}

void CycleCollector::CollectFrom(std::vector<ValueObjectPtr> roots) noexcept
{
    if (roots.empty())
    {
        return;
    }
    std::vector<ValueObjectPtr> none;
    WalkTables tables;
    WalkAndClear(tables, none, roots);
}

WalkTables::WalkTables(WalkTables&& other) noexcept
    : m_nodes(std::move(other.m_nodes))
    , m_slots(std::move(other.m_slots))
    , m_pending(std::move(other.m_pending))
    , m_capacity(std::exchange(other.m_capacity, 0))
    , m_slotMask(std::exchange(other.m_slotMask, 0))
    , m_size(std::exchange(other.m_size, 0))
    , m_pendingSize(std::exchange(other.m_pendingSize, 0))
    , m_saturated(std::exchange(other.m_saturated, false))
{
}

size_t WalkTables::Probe(const ValueObject* object) const noexcept
{
    // Fibonacci hashing: pointers are aligned, so their low bits say little
    const uint64_t hash = static_cast<uint64_t>(std::hash<const ValueObject*>()(object)) * 0x9E3779B97F4A7C15ULL;
    size_t slot = static_cast<size_t>(hash >> 32) & m_slotMask;
    // At most half the slots are used, so the probe ends
    while (m_slots[slot] != 0 && m_nodes[m_slots[slot] - 1].object != object)
    {
        slot = (slot + 1) & m_slotMask;
    }
    return slot;
}

void WalkTables::Reserve(size_t nodes)
{
    if (nodes <= m_capacity)
    {
        return;
    }
    const size_t capacity = std::max({ nodes, 2 * m_capacity, MinCapacity });
    // Indices are 32 bits, and the slots twice the nodes
    if (capacity > std::numeric_limits<uint32_t>::max() / 4)
    {
        throw std::bad_alloc();
    }
    size_t slotCount = 2 * MinCapacity;
    while (slotCount < 2 * capacity)
    {
        slotCount *= 2;
    }
    // Everything allocated before anything changes
    auto nodeTable = std::make_unique<Node[]>(capacity);
    auto slots = std::make_unique<uint32_t[]>(slotCount);
    auto pending = std::make_unique<uint32_t[]>(capacity);
    std::copy_n(m_nodes.get(), m_size, nodeTable.get());
    std::copy_n(m_pending.get(), m_pendingSize, pending.get());
    m_nodes = std::move(nodeTable);
    m_slots = std::move(slots);
    m_pending = std::move(pending);
    m_capacity = capacity;
    m_slotMask = slotCount - 1;
    for (size_t index = 0; index != m_size; ++index)
    {
        m_slots[Probe(m_nodes[index].object)] = static_cast<uint32_t>(index + 1);
    }
}

void WalkTables::Reset() noexcept
{
    std::fill_n(m_slots.get(), m_capacity ? m_slotMask + 1 : 0, 0U);
    m_size = 0;
    m_pendingSize = 0;
    m_saturated = false;
}

void WalkTables::Count(const ValueObject* object) noexcept
{
    if (m_capacity != 0)
    {
        if (const auto slot = Probe(object); m_slots[slot] != 0)
        {
            ++m_nodes[m_slots[slot] - 1].refs;
            return;
        }
    }
    if (m_saturated)
    {
        return;
    }
    if (m_size == m_capacity)
    {
        try
        {
            Reserve(m_size + 1);
        }
        catch (const std::bad_alloc&)
        {
            m_saturated = true;
            return;
        }
    }
    m_nodes[m_size] = Node{ object, 1, false };
    m_slots[Probe(object)] = static_cast<uint32_t>(m_size + 1);
    ++m_size;
}

size_t WalkTables::IndexOf(const ValueObject* object) const noexcept
{
    if (m_capacity == 0)
    {
        return m_size;
    }
    const auto slot = Probe(object);
    return m_slots[slot] != 0 ? m_slots[slot] - 1 : m_size;
}

bool WalkTables::Pop(size_t& index) noexcept
{
    if (m_pendingSize == 0)
    {
        return false;
    }
    index = m_pending[--m_pendingSize];
    return true;
}

} // namespace v2
} // namespace jinja2
