#include "cycle_collector.h"

#include "internal_value.h"
#include "recursion_guard.h"
#include "render_workspace.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
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
    // Out of line: inlined, GCC's -Wdangling-pointer cannot see that the destructor clears s_held
    JINJA2CPP_NOINLINE ExportWalk()
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
    // What the closures stored in the outer render's globals still need lives as long as that render (KeepForOuter)
    if (auto* keeper = m_notes->keeper)
    {
        try
        {
            auto& target = keeper->m_module ? keeper->m_module->kept : keeper->NotesForKeep().kept;
            m_notes->kept.MoveInto(target);
            // A closure an earlier nested render stored and this one let go of keeps its copy, and the tree that
            // owns, no longer than a few such renders (docs/tasks/0539)
            target.ReleaseExpired();
        }
        catch (...) // NOLINT(bugprone-empty-catch): out of memory; the closures read what was not moved as ended
        {
        }
    }
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
    // The values this render noted trees for may outlive it in the outer render too. What it cannot note there only
    // goes uncounted: the values own their trees (docs/tasks/0521)
    for (const auto& kept : m_notes->trees)
    {
        if (kept.tree.expired())
        {
            continue;
        }
        try
        {
            m_outer->KeepTree(kept);
        }
        catch (...) // NOLINT(bugprone-empty-catch): out of memory; the tree stays alive, and stays uncounted
        {
        }
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

void CycleCollector::KeepTree(const KeptTree& kept)
{
    if (auto* note = Insert(kept.address, kept.tree, 0))
    {
        // Past the budget the tree is noted all the same, as its values own it: this render fails at its next step
        detail::ChargeRenderMemoryOf(this, kept.bytes, &note->bytes);
    }
}

void CycleCollector::GiveBack(std::uint64_t bytes) noexcept
{
    if (bytes != 0)
    {
        detail::ReleaseRenderMemoryOf(this, bytes);
    }
}

bool CycleCollector::Keeps(const void* tree) noexcept
{
    if (!m_notes)
    {
        return false;
    }
    auto& notes = *m_notes;
    const auto found = notes.treeIndex.find(tree);
    if (found == notes.treeIndex.end())
    {
        return false;
    }
    auto& note = notes.trees[found->second];
    if (!note.tree.expired())
    {
        return true;
    }
    // Gone: what it took is back, once, and the slot is the next tree's at this address
    GiveBack(note.bytes);
    note.bytes = 0;
    return false;
}

void CycleCollector::ReleaseExpiredKept() noexcept
{
    if (m_module)
    {
        m_module->kept.ReleaseAllExpired();
    }
    else if (m_notes)
    {
        m_notes->kept.ReleaseAllExpired();
    }
}

void CycleCollector::GiveBackExpired() noexcept
{
    if (!m_notes)
    {
        return;
    }
    auto& notes = *m_notes;
    size_t kept = 0;
    for (size_t i = 0; i != notes.trees.size(); ++i)
    {
        auto& note = notes.trees[i];
        if (note.tree.expired())
        {
            GiveBack(note.bytes);
            notes.treeIndex.erase(note.address);
            continue;
        }
        if (kept != i)
        {
            notes.trees[kept] = std::move(note);
        }
        notes.treeIndex[notes.trees[kept].address] = kept;
        ++kept;
    }
    notes.trees.erase(notes.trees.begin() + static_cast<std::ptrdiff_t>(kept), notes.trees.end());
    notes.treePruneAt = std::max(Notes::MinPruneAt, 2 * kept);
}

size_t CycleCollector::KeptTreeCount() const noexcept
{
    if (!m_notes)
    {
        return 0;
    }
    return static_cast<size_t>(std::count_if(m_notes->trees.begin(), m_notes->trees.end(), [](const KeptTree& note) { return !note.tree.expired(); }));
}

CycleCollector::KeptTree* CycleCollector::Insert(const void* address, std::weak_ptr<const void> tree, std::uint64_t bytes)
{
    if (!m_notes)
    {
        m_notes = std::make_unique<Notes>();
    }
    auto& notes = *m_notes;
    // The expired notes first, so that a loop of trees that come and go stays at its live size
    if (notes.trees.size() >= notes.treePruneAt)
    {
        GiveBackExpired();
    }
    const auto found = notes.treeIndex.find(address);
    if (found != notes.treeIndex.end())
    {
        auto& note = notes.trees[found->second];
        if (!note.tree.expired())
        {
            return nullptr;
        }
        // A new tree at the address of an expired one
        GiveBack(note.bytes);
        note.tree = std::move(tree);
        note.bytes = bytes;
        return &note;
    }
    // Pushed first, so that the vector grows geometrically (a reserve of one more per tree made the loop of 0521
    // quadratic), and taken back if the index cannot take its entry
    notes.trees.push_back({ std::move(tree), address, bytes });
    try
    {
        notes.treeIndex.emplace(address, notes.trees.size() - 1);
    }
    catch (...)
    {
        notes.trees.pop_back();
        throw;
    }
    return &notes.trees.back();
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

bool DroppedExports::Hand(InternalValue& container) noexcept
{
    const std::scoped_lock lock(m_mutex);
    if (m_closed)
    {
        return false;
    }
    try
    {
        m_dropped.push_back(std::move(container));
        m_any.store(true, std::memory_order_release);
    }
    catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): the container goes with the caller's reference; a cycle it closes leaks
    {
    }
    return true;
}

std::vector<InternalValue> DroppedExports::Take() noexcept
{
    std::vector<InternalValue> taken;
    // A container handed after this load waits for the next call
    if (!m_any.load(std::memory_order_acquire))
    {
        return taken;
    }
    const std::scoped_lock lock(m_mutex);
    m_any.store(false, std::memory_order_relaxed);
    taken.swap(m_dropped);
    return taken;
}

std::vector<InternalValue> DroppedExports::Close() noexcept
{
    std::vector<InternalValue> taken;
    const std::scoped_lock lock(m_mutex);
    m_closed = true;
    taken.swap(m_dropped);
    return taken;
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
    collector->NotesForKeep().kept.Keep(std::move(object), std::move(owner), names);
    return true;
}

CycleCollector::Notes& CycleCollector::NotesForKeep()
{
    if (!m_notes)
    {
        m_notes = std::make_unique<Notes>();
    }
    return *m_notes;
}

void KeptObjects::Keep(std::shared_ptr<const void> object, std::weak_ptr<const void> owner, const InternalValueMap* names)
{
    if (m_kept.size() >= m_pruneAt)
    {
        m_kept.erase(std::remove_if(m_kept.begin(), m_kept.end(), [](const Entry& entry) { return entry.owner.expired(); }), m_kept.end());
        m_pruneAt = std::max(MinPruneAt, m_kept.size() * 2);
    }
    m_kept.push_back(Entry{ std::move(owner), std::move(object), names });
    if (!names || !m_unreached)
    {
        return;
    }
    auto& u = *m_unreached;
    // The entries of one frame's copy come one after another: the copy is asked about once
    if (!MayHoldKeeper(*names))
    {
        m_kept.back().backoff = NeverChecked;
    }
    else if (u.youngSize != u.young.size() && (u.youngSize == 0 || u.young[u.youngSize - 1].names != names))
    {
        u.young[u.youngSize++] = Young{ names, m_kept.size() - 1 };
    }
}

void KeptObjects::ReleaseAllExpired() noexcept
{
    // The released objects go once the list is whole again: what they hold may keep in turn as it goes
    std::vector<Entry> released;
    size_t kept = 0;
    for (auto& entry : m_kept)
    {
        if (entry.owner.expired())
        {
            try
            {
                released.push_back(std::move(entry));
                continue;
            }
            catch (...) // NOLINT(bugprone-empty-catch): out of memory; the entry stays until the next prune
            {
            }
        }
        if (&m_kept[kept] != &entry)
        {
            m_kept[kept] = std::move(entry);
        }
        ++kept;
    }
    m_kept.resize(kept);
    m_cursor = 0;
}

void KeptObjects::MoveInto(KeptObjects& other)
{
    for (auto& entry : m_kept)
    {
        if (entry.object && !entry.owner.expired())
        {
            other.Keep(std::move(entry.object), std::move(entry.owner), entry.names);
        }
    }
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
    // The outermost render that shares the snapshot, past renders of other environments between them, as
    // SharedSnapshotKeeper finds it: that render reads the write once the ones between have ended (docs/tasks/0491)
    const auto* snapshot = running->owner->get();
    const auto* owner = running;
    for (const auto* outer = running->outer; outer; outer = outer->outer)
    {
        if (outer->owner && outer->owner->get() == snapshot)
        {
            owner = outer;
        }
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

// Marks the nodes held from outside the walk, and every node they reach: a node with more references than the
// walk counted is held from outside. Untracked objects (a saturated walk) are skipped
void MarkHeldFromOutside(WalkTables& tables, const std::vector<const ValueObject*>& live) noexcept
{
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
}

// Empties the garbage (the nodes not marked) while it is held, then lets go of it
void ClearGarbage(WalkTables& tables) noexcept
{
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

    MarkHeldFromOutside(tables, live);

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
    ClearGarbage(tables);
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

size_t KeptObjects::ItemsOf(const ValueObject& object)
{
    if (const auto* list = dynamic_cast<const IListAccessor*>(&object))
    {
        // An iterator or generator is not asked: its size may cost what the walk saves
        if (object.ClonesOnCopy() || object.GetIteratorKind() != IteratorKind::None)
        {
            return WalkItemLimit + 1;
        }
        const auto size = list->GetSize();
        return size ? *size : WalkItemLimit + 1;
    }
    if (const auto* map = dynamic_cast<const IMapAccessor*>(&object))
    {
        return map->GetSize();
    }
    return 1;
}

bool KeptObjects::MayHoldKeeper(const InternalValueMap& names) noexcept
{
    return std::any_of(names.begin(), names.end(), [](const auto& entry) {
        const auto& value = entry.second;
        const auto* object = detail::InternalValueAccess::Object(value);
        if (!object || !object->IsTraversable())
        {
            return false;
        }
        if (AsList(value))
        {
            return true;
        }
        const auto map = AsMap(value);
        if (!map)
        {
            return false;
        }
        const char* objectClass = map->GetObjectClass();
        return !objectClass || std::strcmp(objectClass, "TemplateReference") != 0;
    });
}

void KeptObjects::ReleaseUnreached(LiveRootsFn liveRoots, const void* state) noexcept
{
    // A call that starts while a release empties what it found (a host's object that calls the module as it goes)
    // leaves the check to the next one: the walk's tables are in use
    if (!m_unreached || m_unreached->checking)
    {
        return;
    }
    auto& u = *m_unreached;
    ++u.calls;
    u.checking = true;
    u.liveRoots = liveRoots;
    u.liveState = state;
    u.liveRead = false;
    try
    {
        // A copy CheckCopy releases may keep others as it goes, which come next time. The newest first: a release
        // moves the entries after it, not those before
        const auto young = u.young;
        const size_t youngSize = std::exchange(u.youngSize, 0);
        for (size_t i = youngSize; i != 0; --i)
        {
            CheckCopy(young[i - 1].names, young[i - 1].index);
        }
        // Then one older copy that is due, looking at a few entries in turn, as ReleaseExpired does
        for (size_t looked = 0; looked != ReleaseLooksPerCall && !m_kept.empty(); ++looked)
        {
            if (u.checkCursor >= m_kept.size())
            {
                u.checkCursor = 0;
            }
            const size_t index = u.checkCursor++;
            const auto& entry = m_kept[index];
            // Due: checkAt is not after the calls, in a counter that wraps
            if (!entry.names || entry.backoff == NeverChecked || u.calls - entry.checkAt >= 0x80000000U)
            {
                continue;
            }
            CheckCopy(entry.names, index);
            break;
        }
    }
    catch (...) // NOLINT(bugprone-empty-catch): out of memory; the copies stay until their owners expire or a later call
    {
    }
    u.checking = false;
}

bool KeptObjects::FindGroup(const InternalValueMap* names, size_t hint, Group& group) const
{
    // Keep adds the entries of one copy one after another: the run around `hint`, the index of one of them, without a
    // scan of the others. An entry another has moved since (a prune, the swap of ReleaseExpired) is not found, and the
    // copy is then held by more than its group (ReadOwners): kept
    group.size = 0;
    if (hint >= m_kept.size() || m_kept[hint].names != names)
    {
        return false;
    }
    size_t first = hint;
    while (first != 0 && m_kept[first - 1].names == names)
    {
        if (hint - first == MaxGroup)
        {
            return false;
        }
        --first;
    }
    for (size_t i = first; i != m_kept.size() && m_kept[i].names == names; ++i)
    {
        const auto& entry = m_kept[i];
        // An owner that expired: ReleaseExpired's. A copy that holds no container, at an address a released one had
        if (group.size == MaxGroup || !entry.object || entry.owner.expired() || entry.backoff == NeverChecked)
        {
            return false;
        }
        group.entries[group.size++] = i;
    }
    return true;
}

void KeptObjects::Delay(const Group& group) noexcept
{
    const auto& u = *m_unreached;
    for (size_t i = 0; i != group.size; ++i)
    {
        auto& entry = m_kept[group.entries[i]];
        entry.checkAt = u.calls + (1U << std::min(entry.backoff, MaxBackoff));
        entry.backoff = static_cast<uint8_t>(std::min<unsigned>(entry.backoff + 1U, MaxBackoff));
    }
}

bool KeptObjects::ReadOwners(Group& group) const
{
    // Held by something else than the entries: a call that reads it
    if (m_kept[group.entries[0]].object.use_count() != static_cast<long>(group.size)) // use_count: read under the module's lock, which every reader of a kept copy holds
    {
        return false;
    }
    // The owners, and the references each has: the closures that keep it, and whatever else holds it. A reference a
    // host's thread lets go of has its writes ordered before what follows, as IsSoleOwner orders them (docs/tasks/0339)
    for (size_t i = 0; i != group.size; ++i)
    {
        const auto owner = m_kept[group.entries[i]].owner.lock();
        if (!owner)
        {
            return false;
        }
        group.owners[i] = owner.get();
    }
    for (size_t i = 0; i != group.size; ++i)
    {
        group.references[i] = m_kept[group.entries[i]].owner.use_count(); // use_count: ordered by the fence below; read high, it keeps
    }
    std::atomic_thread_fence(std::memory_order_acquire);
    return true;
}

bool KeptObjects::WalkCopy(const InternalValueMap& names)
{
    auto& u = *m_unreached;
    auto& tables = u.walk;
    if (tables.Capacity() == 0)
    {
        tables.Reserve(WalkNodeLimit / 4);
    }
    // The module's live roots, read once a call, by the first walk
    if (!u.liveRead)
    {
        u.live.clear();
        if (u.liveRoots)
        {
            u.liveRoots(u.liveState, u.live);
            std::sort(u.live.begin(), u.live.end());
        }
        u.liveRead = true;
    }
    tables.Reset();
    tables.SetLimit(WalkNodeLimit);
    const auto& live = u.live;
    auto visit = [&tables, &live](const ValueObject* object) {
        if (object->IsTraversable() && !IsLive(live, object))
        {
            tables.Count(object);
        }
    };
    for (const auto& [name, value] : names)
    {
        TraverseValue(value, visit);
    }
    // At most WalkItemLimit items in all: a copy holding a long list is kept rather than walked on every call
    size_t items = 0;
    for (size_t next = 0; next != tables.Size(); ++next)
    {
        const auto* object = tables.At(next).object;
        items += ItemsOf(*object);
        if (items > WalkItemLimit)
        {
            return false;
        }
        object->Traverse(visit);
    }
    // An object it did not track may hold a closure that keeps an owner, or reach one
    if (tables.Saturated())
    {
        return false;
    }
    // The call's collector holds what the call stored into (NoteStore) until its walk, which runs after this: not a
    // reference from outside
    for (size_t index = 0; index != tables.Size(); ++index)
    {
        if (CycleCollector::HoldsCandidate(tables.At(index).object))
        {
            ++tables.At(index).refs;
        }
    }
    MarkHeldFromOutside(tables, live);
    return true;
}

bool KeptObjects::OwnersHeldInside(const Group& group) const
{
    // The closures among what the copy holds that keep an owner (Callable::SetKeepAlive): all of an owner's
    // references, none held from outside
    const auto& tables = m_unreached->walk;
    std::array<long, MaxGroup> found{};
    const auto* const owners = group.owners.data();
    const auto* const ownersEnd = owners + group.size;
    for (size_t index = 0; index != tables.Size(); ++index)
    {
        const auto& node = tables.At(index);
        const auto* box = dynamic_cast<const ValueBox<Callable>*>(node.object);
        if (!box)
        {
            continue;
        }
        const auto* owner = std::find(owners, ownersEnd, box->Get().GetKeepAlive());
        if (owner == ownersEnd)
        {
            continue;
        }
        if (node.reachable)
        {
            return false;
        }
        ++found[static_cast<size_t>(owner - owners)];
    }
    return std::equal(found.begin(), found.begin() + static_cast<std::ptrdiff_t>(group.size), group.references.begin());
}

void KeptObjects::Release(const Group& group)
{
    // As if the owners had expired. What the walk did not mark is held while the copy goes, then emptied and let go
    // of, as WalkAndClear does: what only the copy reached goes now, with the owners, before the call prunes the
    // trees they held, and a cycle among its containers is broken (`l.append(l)`)
    auto& tables = m_unreached->walk;
    std::vector<Entry> released;
    released.reserve(group.size);
    for (size_t i = 0; i != tables.Size(); ++i)
    {
        if (!tables.At(i).reachable)
        {
            intrusive_ptr_add_ref(tables.At(i).object);
        }
    }
    for (size_t i = 0; i != group.size; ++i)
    {
        released.push_back(std::move(m_kept[group.entries[i]]));
    }
    // The last entries take their places, as in ReleaseExpired: the order does not matter, and no other entry moves
    // that ReleaseUnreached is still to look at (it looks at the newest copies first). From the last of the group, so
    // that one of the group never takes another's place
    for (size_t i = group.size; i != 0; --i)
    {
        const size_t index = group.entries[i - 1];
        if (index != m_kept.size() - 1)
        {
            m_kept[index] = std::move(m_kept.back());
        }
        m_kept.pop_back();
    }
    // Once the list is whole again: what they hold may keep in turn as it goes
    released.clear();
    ClearGarbage(tables);
}

void KeptObjects::CheckCopy(const InternalValueMap* names, size_t hint)
{
    Group group;
    if (!FindGroup(names, hint, group))
    {
        return;
    }
    if (ReadOwners(group) && WalkCopy(*names) && OwnersHeldInside(group))
    {
        Release(group);
        return;
    }
    Delay(group);
}

WalkTables::WalkTables(WalkTables&& other) noexcept
    : m_nodes(std::move(other.m_nodes))
    , m_slots(std::move(other.m_slots))
    , m_pending(std::move(other.m_pending))
    , m_capacity(std::exchange(other.m_capacity, 0))
    , m_slotMask(std::exchange(other.m_slotMask, 0))
    , m_size(std::exchange(other.m_size, 0))
    , m_pendingSize(std::exchange(other.m_pendingSize, 0))
    , m_limit(std::exchange(other.m_limit, 0))
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
    if (m_limit != 0 && m_size == m_limit)
    {
        m_saturated = true;
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
