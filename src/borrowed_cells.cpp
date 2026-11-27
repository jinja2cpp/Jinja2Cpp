#include "borrowed_cells.h"

#include "cell_trace.h"
#include "cycle_collector.h"
#include "hash_map.h"
#include "internal_value.h"
#include "intrusive_ptr.h"
#include "render_workspace.h"
#include "small_vector.h"
#include "template_dict.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <memory>
#include <new>
#include <optional>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{

namespace detail
{
std::atomic<uint64_t> CellCounters::s_epoch{ 0 };
std::atomic<uint32_t> CellCounters::s_live{ 0 };

uint64_t CellCounters::NextEpoch() noexcept
{
    // Never 0, the epoch of a new adapter, and never as far as the attached mark: 2^63 moves are
    // out of reach, so the counter never returns to a value an adapter stored (docs/tasks/0342)
    return s_epoch.fetch_add(1, std::memory_order_relaxed) + 1;
}

void CellCounters::AdvanceEpochForTesting(uint64_t count) noexcept
{
    s_epoch.fetch_add(count, std::memory_order_relaxed);
}
} // namespace detail

BorrowedCell::BorrowedCell(InternalValue storage, InternalValue source, const void* identity, bool isList, const BorrowedCells& registry, std::weak_ptr<const void> owner, bool unheldIdentity)
    : m_storage(std::move(storage))
    , m_source(std::move(source))
    , m_owner(std::move(owner))
    , m_identity(identity)
    , m_registry(&registry)
    , m_isList(isList)
    , m_unheldIdentity(unheldIdentity)
{
    Bind();
    // A frozen cell is held by the links of the values that outlived the render: a cycle through
    // its items is the collector's (CellLink::TraverseFrozen)
    SetTraversable();
    // Every cell, also one Adopt drops: its free is in the log too
    JINJA2CPP_CELL_TRACE_EVENT("cellnew", Trace::Ptr("c", Trace::Space::NewCell, this));
}

void BorrowedCell::Traverse(const TraverseFn& fn) const
{
    if (m_frozen)
    {
        TraverseValue(m_storage, fn);
        TraverseValue(m_retired, fn);
    }
}

void BorrowedCell::Bind() const noexcept
{
    if (m_isList)
    {
        auto list = AsList(m_storage);
        m_list = list ? list->GetAccessor() : nullptr;
        m_items = list ? list->GetMutableItems() : nullptr;
        m_object = list ? list->GetOwner() : nullptr;
    }
    else
    {
        auto map = AsMap(m_storage);
        m_map = map ? map->GetAccessor() : nullptr;
        m_object = map ? map->GetOwner() : nullptr;
    }
}

BorrowedCells* BorrowedCell::ExportingCall() const noexcept
{
    // Only in the cell's own module call, which alone writes the cell: a frozen cell is never
    // written, and a render's cells live for one render
    if (m_frozen || !m_registry)
    {
        return nullptr;
    }
    auto* cells = CycleCollector::CurrentCells();
    return cells == m_registry && cells->InCall() ? cells : nullptr;
}

GenericList BorrowedCell::ExportList() const
{
    if (auto* call = ExportingCall())
    {
        auto list = ExportCallList(*call, m_identity, InternalValue());
#ifdef JINJA2CPP_CELL_TRACE
        TraceExport(true);
#endif
        return list;
    }
#ifdef JINJA2CPP_CELL_TRACE
    TraceExport(false);
#endif
    return m_list->CreateGenericList();
}

GenericMap BorrowedCell::ExportMap() const
{
    if (auto* call = ExportingCall())
    {
        auto map = ExportCallMap(*call, m_identity, InternalValue());
#ifdef JINJA2CPP_CELL_TRACE
        TraceExport(true);
#endif
        return map;
    }
#ifdef JINJA2CPP_CELL_TRACE
    TraceExport(false);
#endif
    return m_map->CreateGenericMap();
}

#ifdef JINJA2CPP_CELL_TRACE
void BorrowedCell::TraceExport(bool live) const noexcept
{
    // The call is read only where the export may read it: a frozen cell's registry may be gone
    const auto call = !m_frozen && m_registry && CycleCollector::CurrentCells() == m_registry ? m_registry->m_call : 0;
    JINJA2CPP_CELL_TRACE_EVENT("export", Trace::Ptr("c", Trace::Space::Cell, this), Trace::Ptr("g", Trace::Space::Gen, TraceGen()), Trace::Num("call", call),
                               Trace::Num("live", live ? 1 : 0));
}
#endif

BorrowedCell::Holders BorrowedCell::FindHolders(bool byItems) const noexcept
{
    // The registry's reference alone, for data that lives only as long as its adapters, and the
    // reference of the source's own link. The only other adapters of the data are the anchors of
    // copied items (LifetimeAnchor), which are never read; so when nothing but the cell holds the
    // source, no adapter of the data can be read or made again, and no later call finds the cell.
    // The data of the host, or of a global, can be lent again at any time: its cell stays
    if (m_frozen || !m_links.empty() || m_owner.expired())
    {
        return Holders::Some;
    }
    const auto find = [this, byItems](const auto& source) {
        const auto* accessor = source.GetAccessor();
        const ValueObject* owner = source.GetOwner();
        if (!accessor || !owner)
        {
            return Holders::Some;
        }
        // The owner's count first: a cell of data the module's names hold stops there, without the
        // accessor's two virtual calls (DropUnheld runs at the end of every call, docs/tasks/0519)
        const bool sole = owner->use_count() == 1;
        if (!sole && (!byItems || (m_heldUses != 0 && owner->use_count() >= m_heldSourceUses && use_count() >= m_heldUses && StorageSize() < (2 * m_heldItems) + 8U)))
        {
            return Holders::Some;
        }
        // The host's copy of a generic container can lend it to a later call, which must see this
        // call's writes (0049 P2): its cell stays while the host holds one (docs/tasks/0336)
        if (accessor->MayBeLentAgain())
        {
            return Holders::Some;
        }
        // Every other link owns the cell: only the source's own adapter reads it
        const CellLink* link = accessor->GetCellLink();
        const size_t links = link && link->m_cell == this ? 2U : 1U;
        if (sole && use_count() == links)
        {
            return Holders::None;
        }
        return byItems && IsHeldByItsItems(*owner, links) ? Holders::ItsItems : Holders::Some;
    };
    if (m_isList)
    {
        auto list = AsList(m_source);
        return list ? find(*list) : Holders::Some;
    }
    auto map = AsMap(m_source);
    return map ? find(*map) : Holders::Some;
}

bool BorrowedCell::OwnsItemsAlone() const noexcept
{
    // A pin shares the storage's object (CallExports::Pin), a value of a render or a copy of the
    // accessor the items
    return m_object != nullptr && m_object->use_count() == 1 && (m_isList ? m_list->OwnsMutableItemsAlone() : m_map->OwnsMutableItemsAlone());
}

bool BorrowedCell::Unshare() const
{
    if (m_frozen)
    {
        JINJA2CPP_CELL_TRACE_EVENT("unshare", Trace::Ptr("c", Trace::Space::Cell, this), Trace::Num("kind", 1), Trace::Num("exp", m_shared ? -1 : 0), Trace::Num("call", 0),
                                   Trace::Ptr("g", Trace::Space::Gen, TraceGen()));
        return false;
    }
    if (OwnsItemsAlone())
    {
        JINJA2CPP_CELL_TRACE_EVENT("unshare", Trace::Ptr("c", Trace::Space::Cell, this), Trace::Num("kind", 3), Trace::Num("exp", m_shared ? -1 : 0),
                                   Trace::Num("call", m_registry->m_call), Trace::Ptr("g", Trace::Space::Gen, TraceGen()));
        // The host let go of what ended calls handed it
        m_shared = false;
        return true;
    }
    // A value of an ended call keeps the items as they were: the host holds a snapshot that
    // nothing writes any more (decision D8, copy on write). The mark stays until the copy is in
    // place, so a copy that fails to allocate is tried again by the next write
    InternalValue copy = m_isList ? InternalValue(ListAdapter::CreateAdapter(InternalValueList(*m_items)))
                                  : InternalValue(CreateMapAdapter(TemplateDict(*m_map->GetMutableItems())));
    JINJA2CPP_CELL_TRACE_EVENT("unshare", Trace::Ptr("c", Trace::Space::Cell, this), Trace::Num("kind", 4), Trace::Num("exp", m_shared ? -1 : 0),
                               Trace::Num("call", m_registry->m_call), Trace::Ptr("g", Trace::Space::Gen, TraceGen()));
    m_retired = std::exchange(m_storage, std::move(copy));
    Bind();
    m_shared = false;
    JINJA2CPP_CELL_TRACE_EVENT("copied", Trace::Ptr("c", Trace::Space::Cell, this), Trace::Ptr("g", Trace::Space::NewGen, TraceGen()));
    return true;
}

bool BorrowedCell::UnshareWith(InternalValue& spare) const noexcept
{
    if (m_frozen)
    {
        JINJA2CPP_CELL_TRACE_EVENT("unshare", Trace::Ptr("c", Trace::Space::Cell, this), Trace::Num("kind", 1), Trace::Num("exp", m_shared ? -1 : 0), Trace::Num("call", 0),
                                   Trace::Ptr("g", Trace::Space::Gen, TraceGen()));
        return false;
    }
    if (!m_shared)
    {
        return true;
    }
    if (OwnsItemsAlone())
    {
        JINJA2CPP_CELL_TRACE_EVENT("unshare", Trace::Ptr("c", Trace::Space::Cell, this), Trace::Num("kind", 3), Trace::Num("exp", -1), Trace::Num("call", m_registry->m_call),
                                   Trace::Ptr("g", Trace::Space::Gen, TraceGen()));
        m_shared = false;
        return true;
    }
    // As the copy of Unshare, with the caller's empty list in its place
    JINJA2CPP_CELL_TRACE_EVENT("unshare", Trace::Ptr("c", Trace::Space::Cell, this), Trace::Num("kind", 4), Trace::Num("exp", -1), Trace::Num("call", m_registry->m_call),
                               Trace::Ptr("g", Trace::Space::Gen, TraceGen()));
    m_retired = std::exchange(m_storage, std::move(spare));
    Bind();
    m_shared = false;
    JINJA2CPP_CELL_TRACE_EVENT("copied", Trace::Ptr("c", Trace::Space::Cell, this), Trace::Ptr("g", Trace::Space::NewGen, TraceGen()));
    return true;
}

const BorrowedCell* CellLink::Find(const IListAccessor& self) const
{
    return Find(self.GetIdentity(), true, self.IsGlobalData(), self.IsInSnapshot());
}

const BorrowedCell* CellLink::Find(const IMapAccessor& self) const
{
    return Find(self.GetIdentity(), false, self.IsGlobalData(), self.IsInSnapshot());
}

const BorrowedCell* CellLink::Find(const void* identity, bool isList, bool held, bool listed) const
{
    // A cell stays until its render detaches it, also while a render nested in it runs; a frozen
    // one for as long as the link lives
    if (m_cell)
    {
        JINJA2CPP_CELL_TRACE_EVENT("hit", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, m_cell));
        return m_cell;
    }
    // Read first: a promotion after this read moves the epoch again. Kept only once the lookup
    // is done: a Link that cannot allocate leaves the adapter as it was (docs/tasks/0338, step D)
    const auto epoch = detail::CellEpoch();
    const auto* cells = held ? CycleCollector::GlobalCells() : CycleCollector::CurrentCells();
    if (!cells || !cells->m_cells)
    {
        JINJA2CPP_CELL_TRACE_EVENT("find", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("idn", Trace::Space::Idn, identity), Trace::Num("list", isList ? 1 : 0),
                                   Trace::Num("e", static_cast<int64_t>(epoch)), Trace::Num("hit", 0), Trace::Ptr("reg", Trace::Space::Reg, cells),
                                   Trace::Num("held", held ? 1 : 0));
        m_state = epoch;
        return nullptr;
    }
    const auto found = cells->m_cells->find(identity);
    const BorrowedCell* hit = found != cells->m_cells->end() && found->second->IsList() == isList ? found->second.get() : nullptr;
    JINJA2CPP_CELL_TRACE_EVENT("find", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("idn", Trace::Space::Idn, identity), Trace::Num("list", isList ? 1 : 0),
                               Trace::Num("e", static_cast<int64_t>(epoch)), Trace::Ptr("hit", Trace::Space::Cell, hit), Trace::Ptr("reg", Trace::Space::Reg, cells),
                               Trace::Num("held", held ? 1 : 0));
    if (!hit)
    {
        m_state = epoch;
        return nullptr;
    }
#ifdef JINJA2CPP_CELL_TRACE
    try
    {
        ReserveLink(*hit, held, listed);
    }
    catch (...)
    {
        // The adapter keeps the epoch it had, which the counter has moved past: it looks again
        JINJA2CPP_CELL_TRACE_EVENT("linkfail", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, hit), Trace::Num("e", static_cast<int64_t>(m_state)));
        throw;
    }
#else
    ReserveLink(*hit, held, listed);
#endif
    Link(*hit, held, listed);
    return m_cell;
}

void CellLink::ReserveLink(const BorrowedCell& cell, bool held, bool listed)
{
    if (!held)
    {
        return;
    }
    // The workspace Link marks, made now if this thread has none yet
    static_cast<void>(RenderWorkspace::ForThisThread());
    if (!listed)
    {
        return;
    }
    auto& links = cell.m_links;
    if (links.size() == links.capacity())
    {
        links.reserve(links.empty() ? 4 : 2 * links.size());
    }
}

void CellLink::Link(const BorrowedCell& cell, bool held, bool listed) const noexcept
{
    if (!held || !listed)
    {
        // A reference: the host may let go of the adapter on any thread (docs/tasks/0340), a
        // global's item too, which the thread's snapshot does not keep (0346)
        intrusive_ptr_add_ref(&cell);
        m_cell = &cell;
        m_state = AttachedBit | OwnsBit;
        JINJA2CPP_CELL_TRACE_EVENT("link", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, &cell), Trace::Num("held", 0));
        if (held)
        {
            RenderWorkspace::ForThisThread().GlobalAttached();
        }
        return;
    }
    // ReserveLink made room: the push does not allocate
    cell.m_links.push_back(this);
    m_cell = &cell;
    m_state = AttachedBit | HeldBit | (cell.m_links.size() - 1);
    JINJA2CPP_CELL_TRACE_EVENT("link", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, &cell), Trace::Num("held", 1));
    // The thread keeps the globals' adapters for its next render, which may run inside this one
    RenderWorkspace::ForThisThread().GlobalAttached();
}

void CellLink::Unlink() const noexcept
{
    const auto* cell = m_cell;
    const bool owns = Owns();
    const auto index = m_state & IndexBits;
    JINJA2CPP_CELL_TRACE_EVENT("unlink", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, cell), Trace::Num("owns", owns ? 1 : 0));
    m_cell = nullptr;
    m_state = 0;
    if (owns)
    {
        // Any thread: the reference is atomic, and the cell's list does not have the link
        intrusive_ptr_release(cell);
        return;
    }
    // A listed link, on the thread of the render that made the cell
    Unlist(*cell, index);
}

void CellLink::Unlist(const BorrowedCell& cell, uint64_t index) const noexcept
{
    auto& links = cell.m_links;
    // The last link takes this one's place. When this link is the last, its state is left as the
    // caller sets it: a detached link holds 0 or an epoch it saw, never a list index
    const auto* last = links.back();
    if (last != this)
    {
        links[index] = last;
        last->m_state = (last->m_state & ~IndexBits) | index;
    }
    links.pop_back();
}

void CellLink::LeaveSnapshot() const noexcept
{
    if (!m_cell || Owns())
    {
        return;
    }
    JINJA2CPP_CELL_TRACE_EVENT("leave", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, m_cell));
    Unlist(*m_cell, m_state & IndexBits);
    intrusive_ptr_add_ref(m_cell);
    m_state = AttachedBit | OwnsBit;
}

void CellLink::TraverseFrozen(const TraverseFn& fn) const
{
    if (m_cell && Owns() && m_cell->m_frozen)
    {
        fn(m_cell);
    }
}

void CellLink::Inherit(const CellLink& other) noexcept
{
    if (!other.Owns() || !other.m_cell->m_frozen)
    {
        return;
    }
    intrusive_ptr_add_ref(other.m_cell);
    m_cell = other.m_cell;
    m_state = AttachedBit | OwnsBit;
    JINJA2CPP_CELL_TRACE_EVENT("inherit", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("from", Trace::Space::Link, &other), Trace::Ptr("c", Trace::Space::Cell, m_cell));
}

#ifdef JINJA2CPP_CELL_TRACE
const CellLink* CellLink::TraceSourceLink(const BorrowedCell& cell, bool isList)
{
    if (auto list = isList ? AsList(cell.m_source) : std::nullopt)
    {
        return list->GetAccessor() ? list->GetAccessor()->GetCellLink() : nullptr;
    }
    if (auto map = isList ? std::nullopt : AsMap(cell.m_source))
    {
        return map->GetAccessor() ? map->GetAccessor()->GetCellLink() : nullptr;
    }
    return nullptr;
}
#endif

bool CellLink::Adopt(const InternalValue& self, const void* identity, bool isList, InternalValue storage, bool held, bool listed, std::weak_ptr<const void> owner, bool unheldIdentity) const
{
    // An adapter of a value kept after its render: nothing writes the frozen cell, and the caller
    // writes its copy instead (docs/tasks/0331)
    if (m_cell && m_cell->m_frozen)
    {
        JINJA2CPP_CELL_TRACE_EVENT("adoptno", Trace::Ptr("a", Trace::Space::Link, this), Trace::Num("why", 1), Trace::Ptr("c", Trace::Space::Cell, m_cell));
        return false;
    }
    BorrowedCells* cells = nullptr;
    if (identity)
    {
        cells = held ? CycleCollector::GlobalCellsForWrite() : CycleCollector::CellsForWrite();
    }
    if (!cells)
    {
        JINJA2CPP_CELL_TRACE_EVENT("adoptno", Trace::Ptr("a", Trace::Space::Link, this), Trace::Num("why", 2), Trace::Num("c", 0));
        return false;
    }
    // Failure-atomic (docs/tasks/0338, step D): every allocation comes first, the table's insert
    // last, and the commit after it allocates nothing. A step that runs out of memory leaves the
    // registry and the adapter as they were
    std::unique_ptr<BorrowedCells::Table> table = cells->m_cells ? nullptr : std::make_unique<BorrowedCells::Table>();
    auto& target = table ? *table : *cells->m_cells;
    auto cell = MakeRef<BorrowedCell>(std::move(storage), self, identity, isList, *cells, std::move(owner), unheldIdentity);
    if (isList ? !cell->m_items : !cell->m_map)
    {
        JINJA2CPP_CELL_TRACE_EVENT("adoptno", Trace::Ptr("a", Trace::Space::Link, this), Trace::Num("why", 3), Trace::Num("c", 0));
        return false;
    }
    const auto found = target.find(identity);
    const BorrowedCell* existing = found != target.end() ? found->second.get() : nullptr;
    if (!existing)
    {
        ReserveLink(*cell, held, listed);
        target.try_emplace(identity, cell);
        cells->m_hasUnheldIdentity = cells->m_hasUnheldIdentity || unheldIdentity;
    }
    else if (existing->IsList() == isList)
    {
        ReserveLink(*existing, held, listed);
    }
    // The commit
    if (table)
    {
        cells->m_cells = std::move(table);
    }
    if (m_cell)
    {
        Unlink();
    }
    if (existing)
    {
        // Another adapter of the data made one first: this one had not seen it yet
        if (existing->IsList() != isList)
        {
            JINJA2CPP_CELL_TRACE_EVENT("adoptno", Trace::Ptr("a", Trace::Space::Link, this), Trace::Num("why", 4), Trace::Ptr("c", Trace::Space::Cell, existing));
            return false;
        }
        JINJA2CPP_CELL_TRACE_EVENT("adopt", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("idn", Trace::Space::Idn, identity), Trace::Num("list", isList ? 1 : 0),
                                   Trace::Num("held", held ? 1 : 0), Trace::Num("owned", 0), Trace::Ptr("reg", Trace::Space::Reg, cells),
                                   Trace::Ptr("c", Trace::Space::Cell, existing), Trace::Num("new", 0), Trace::Num("g", 0), Trace::Num("src", 0));
        Link(*existing, held, listed);
        return true;
    }
#ifdef JINJA2CPP_CELL_TRACE
    if (detail::CellTrace::On())
    {
        const CellLink* source = TraceSourceLink(*cell, isList);
        JINJA2CPP_CELL_TRACE_EVENT("adopt", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("idn", Trace::Space::Idn, identity), Trace::Num("list", isList ? 1 : 0),
                                   Trace::Num("held", held ? 1 : 0), Trace::Num("owned", cell->m_owner.expired() ? 0 : 1), Trace::Ptr("reg", Trace::Space::Reg, cells),
                                   Trace::Ptr("c", Trace::Space::Cell, cell.get()), Trace::Num("new", 1), Trace::Ptr("g", Trace::Space::NewGen, cell->TraceGen()),
                                   Trace::Ptr("src", Trace::Space::Link, source));
    }
#endif
    Link(*cell, held, listed);
    // Counted before the epoch moves (QuietEpoch)
    detail::CellCounters::AddLive(1);
    // The other adapters of the same data look the cell up on their next read
    [[maybe_unused]] const auto epoch = detail::CellCounters::NextEpoch();
    JINJA2CPP_CELL_TRACE_EVENT("promote", Trace::Ptr("c", Trace::Space::Cell, cell.get()), Trace::Num("e", static_cast<int64_t>(epoch)), Trace::Num("live", detail::CellCounters::Live()));
    return true;
}

void BorrowedCells::DropUnheld(bool walk) noexcept
{
    // A dropped cell lets go of its items, which may hold the only source of another cell (the
    // adapter of a nested list): again until a pass drops none
    while (m_cells && DropUnheldOnce(walk))
    {
    }
}

bool BorrowedCells::DropUnheldOnce(bool walk) noexcept
{
    size_t count = 0;
    for (const auto& [identity, cell] : *m_cells)
    {
        count += cell->FindHolders(walk) != BorrowedCell::Holders::Some ? 1U : 0U;
    }
    if (count == 0)
    {
        return false;
    }
    std::vector<IntrusivePtr<const BorrowedCell>> dropped;
    try
    {
        dropped.reserve(count);
    }
    catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment says what is lost
    {
        // Out of memory: the cells stay until the module goes, as before the call ended
        return false;
    }
    // Out of the registry first, while the data the identity names is alive: the identity is
    // not reused before its cell is gone from the table
    // The host may let go of a generic container's last copy on another thread meanwhile
    // (docs/tasks/0336): a cell that became unheld since the count waits for the next pass
    for (auto entry = m_cells->begin(); entry != m_cells->end();)
    {
        const auto holders = dropped.size() != count ? entry->second->FindHolders(walk) : BorrowedCell::Holders::Some;
        if (holders != BorrowedCell::Holders::Some)
        {
            // Only the walk below reaches a cell its own items hold, through the links of its items'
            // adapters, which pass the collector a frozen cell alone (CellLink::TraverseFrozen); nothing
            // else reads it, nor writes it again
            if (holders == BorrowedCell::Holders::ItsItems)
            {
                entry->second->m_frozen = true;
            }
            JINJA2CPP_CELL_TRACE_EVENT("drop", Trace::Ptr("reg", Trace::Space::Reg, this), Trace::Ptr("c", Trace::Space::Cell, entry->second.get()),
                                       Trace::Links("links", entry->second->m_links.data(), entry->second->m_links.size()),
                                       Trace::Num("items", holders == BorrowedCell::Holders::ItsItems ? 1 : 0));
            dropped.push_back(entry->second);
            entry = m_cells->erase(entry);
        }
        else
        {
            ++entry;
        }
    }
    for (const auto& cell : dropped)
    {
        // The source's adapter detaches from the cell, which is still alive
        cell->m_source = InternalValue();
    }
    const auto removed = static_cast<uint32_t>(dropped.size());
    // What the cells' items hold may be a cycle an earlier call of a template module made, which
    // no note names any more (Codex review of #778): a walk from their storage frees it now, and
    // with it the sources of the nested cells, which the next pass drops
    std::vector<ValueObjectPtr> storages;
    if (walk)
    {
        try
        {
            storages.reserve(2 * dropped.size());
            for (const auto& cell : dropped)
            {
                for (const auto* storage : { &cell->m_storage, &cell->m_retired })
                {
                    if (auto object = OwnObjectOf(*storage))
                    {
                        storages.push_back(std::move(object));
                    }
                }
            }
        }
        catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment says what is lost
        {
            // Out of memory: a cycle through the storages not walked leaks (docs/tasks/0283)
        }
    }
    dropped.clear();
    CycleCollector::CollectFrom(std::move(storages));
    detail::CellCounters::RemoveLive(removed);
    JINJA2CPP_CELL_TRACE_EVENT("dropped", Trace::Ptr("reg", Trace::Space::Reg, this), Trace::Num("n", removed), Trace::Num("live", detail::CellCounters::Live()));
    return true;
}

void BorrowedCells::Finish(CycleCollector* collector) noexcept
{
    // The sources first: what they alone held may hold adapters attached to a cell, which detach
    // while every cell is live
    for (const auto& [identity, cell] : *m_cells)
    {
        cell->m_source = InternalValue();
    }
    // Then a global's adapter detaches, as the thread's snapshot outlives the render (0211). Every
    // other link owns its cell and keeps it frozen: a value the host keeps holds it, another
    // cell's items, or a cycle
    for (const auto& [identity, cell] : *m_cells)
    {
        JINJA2CPP_CELL_TRACE_EVENT("freeze", Trace::Ptr("reg", Trace::Space::Reg, this), Trace::Ptr("c", Trace::Space::Cell, cell.get()),
                                   Trace::Links("held", cell->m_links.data(), cell->m_links.size()));
        cell->m_frozen = true;
        for (const auto* link : cell->m_links)
        {
            link->m_cell = nullptr;
            link->m_state = 0;
        }
        cell->m_links.clear();
        cell->m_links.shrink_to_fit();
    }
    const auto count = static_cast<uint32_t>(m_cells->size());
    // A cell a link still reads may be in a cycle the render's end left (`l.append(l)`, or the
    // items of a copy, which keep the value they were copied from): the collector walks from it
    if (collector)
    {
        try
        {
            for (const auto& [identity, cell] : *m_cells)
            {
                if (cell->use_count() > 1)
                {
                    collector->NoteFrozen(ValueObjectPtr(cell));
                }
            }
        }
        catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment says what is lost
        {
            // Out of memory: the cycles through the cells not noted leak (docs/tasks/0283)
        }
    }
    // A cell no link reads goes now, and with it what only its items held
    JINJA2CPP_CELL_TRACE_EVENT("finish", Trace::Ptr("reg", Trace::Space::Reg, this), Trace::Num("n", count));
    m_cells.reset();
    detail::CellCounters::RemoveLive(count);
    // An adapter that read a cell of this render, or found none while it ran, looks again
    [[maybe_unused]] const auto epoch = detail::CellCounters::NextEpoch();
    JINJA2CPP_CELL_TRACE_EVENT("finished", Trace::Ptr("reg", Trace::Space::Reg, this), Trace::Num("e", static_cast<int64_t>(epoch)), Trace::Num("live", detail::CellCounters::Live()));
}

void BorrowedCells::HandOverReleasedCallFrozen(CycleCollector* collector) noexcept
{
    // The kept cells nothing reads any more go to the call's collector, which walks from them: what
    // they hold may be a cycle an earlier call made, which no note names any more (Codex review of
    // #778). One it cannot take stays until the next call or the module's end
    if (collector)
    {
        try
        {
            for (auto it = m_callFrozen.begin(); it != m_callFrozen.end();)
            {
                if ((*it)->use_count() != 1)
                {
                    ++it;
                    continue;
                }
                collector->NoteRoot(*it);
                it = m_callFrozen.erase(it);
            }
        }
        catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment says what is lost
        {
            // Out of memory: the cells not handed over stay in the list
        }
    }
    // A cell in a cycle is never the list's alone: a walk frees the generations nothing outside
    // the cycle reads, as the list's reference counts as one of the walk's
    if (m_callFrozen.size() >= m_callFrozenWalkAt)
    {
        CycleCollector::CollectKeeping(m_callFrozen);
        m_callFrozenWalkAt = std::max(MinCallFrozenWalk, 2 * m_callFrozen.size());
    }
}

void BorrowedCells::FreezeCallScopedSlow(CycleCollector* collector) noexcept
{
    HandOverReleasedCallFrozen(collector);
    if (!m_cells || !m_hasUnheldIdentity)
    {
        return;
    }
    // Every such cell leaves the table below
    m_hasUnheldIdentity = false;
    // As Finish, for these cells only: the sources first, while every cell is live
    uint32_t count = 0;
    for (const auto& [identity, cell] : *m_cells)
    {
        if (cell->m_unheldIdentity)
        {
            cell->m_source = InternalValue();
            ++count;
        }
    }
    if (count == 0)
    {
        return;
    }
    for (auto entry = m_cells->begin(); entry != m_cells->end();)
    {
        if (!entry->second->m_unheldIdentity)
        {
            ++entry;
            continue;
        }
        const IntrusivePtr<const BorrowedCell> cell = entry->second;
        // The model's call-scoped freeze (tla/CellProtocol.tla, EndModCall; docs/tasks/0402)
        JINJA2CPP_CELL_TRACE_EVENT("cfreeze", Trace::Ptr("reg", Trace::Space::Reg, this), Trace::Ptr("c", Trace::Space::Cell, cell.get()),
                                   Trace::Links("held", cell->m_links.data(), cell->m_links.size()));
        cell->m_frozen = true;
        for (const auto* link : cell->m_links)
        {
            link->m_cell = nullptr;
            link->m_state = 0;
        }
        cell->m_links.clear();
        cell->m_links.shrink_to_fit();
        entry = m_cells->erase(entry);
        // A value that still reads the cell may hold a cycle through it: the call's collector walks
        // from it now, and the module's when it goes, as the cycle can form in a later call
        if (cell->use_count() > 1)
        {
            try
            {
                if (collector)
                {
                    collector->NoteFrozen(ValueObjectPtr(cell));
                }
                m_callFrozen.emplace_back(cell);
            }
            catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment says what is lost
            {
                // Out of memory: a cycle through the cell leaks (docs/tasks/0283)
            }
        }
    }
    detail::CellCounters::RemoveLive(count);
    // An adapter that read one of these cells, or found none while the call ran, looks again
    [[maybe_unused]] const auto epoch = detail::CellCounters::NextEpoch();
    JINJA2CPP_CELL_TRACE_EVENT("cfrozen", Trace::Ptr("reg", Trace::Space::Reg, this), Trace::Num("e", static_cast<int64_t>(epoch)), Trace::Num("live", detail::CellCounters::Live()));
}

void BorrowedCells::ReleaseCallFrozen(CycleCollector* collector) noexcept
{
    if (collector)
    {
        try
        {
            // Also one nothing else reads: what it holds may be a cycle an earlier call made
            for (const auto& cell : m_callFrozen)
            {
                collector->NoteRoot(cell);
            }
        }
        catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): best effort, the comment says what is lost
        {
            // Out of memory: the cycles through the cells not noted leak (docs/tasks/0283)
        }
    }
    m_callFrozen.clear();
    m_callFrozen.shrink_to_fit();
}

void BorrowedCells::PinExports()
{
    if (!m_exports)
    {
        return;
    }
    auto exports = std::move(m_exports);
    try
    {
        if (m_cells)
        {
            exports->Record(*m_cells);
        }
    }
    catch (...)
    {
        // Failure-atomic (docs/tasks/0338, step D): no storage recorded and no cell marked, so
        // the values read the host's data, which nothing writes; never a mix of the two
        exports->KeepHostSources(m_cells.get());
        exports->Seal();
        throw;
    }
    exports->Commit();
    exports->Seal();
}

void BorrowedCells::PinExportsOrSkip() noexcept
{
    try
    {
        PinExports();
    }
    catch (const std::exception&) // NOLINT(bugprone-empty-catch): the call failed already; its values read the host's data
    {
    }
}

BorrowedCells* RunningModuleCall()
{
    auto* cells = CycleCollector::CurrentCells();
    return cells && cells->InCall() ? cells : nullptr;
}

const InternalValue* CallExports::Storage(const void* identity, bool isList) const noexcept
{
    if (m_registry)
    {
        // The call runs: its thread reads the module's cell as it is now
        if (!m_registry->m_cells)
        {
            return nullptr;
        }
        const auto found = m_registry->m_cells->find(identity);
        if (found == m_registry->m_cells->end() || found->second->IsList() != isList || found->second->IsFrozen())
        {
            return nullptr;
        }
        return &found->second->Storage();
    }
    const auto* pin = m_failed ? nullptr : Find(identity, isList);
    return pin ? &pin->storage : nullptr;
}

const InternalValue* CallExports::HostSource(const void* identity, bool isList) const noexcept
{
    const auto* pin = m_failed && !m_registry ? Find(identity, isList) : nullptr;
    return pin ? &pin->storage : nullptr;
}

const CallExports::Pinned* CallExports::Find(const void* identity, bool isList) const noexcept
{
    const auto* const found = std::lower_bound(m_pins.begin(), m_pins.end(), identity, [](const Pinned& pin, const void* key) { return std::less<>()(pin.identity, key); });
    return found != m_pins.end() && found->identity == identity && found->isList == isList ? found : nullptr;
}

void CallExports::KeepHostSources(const CellTable* table) noexcept
{
    // Nothing is marked: the host's data is never written, and a source only keeps it alive. Every
    // cell, as a view of nested data the call made reads one the values did not hand out; when
    // even that allocation fails, the cells handed out, as many as the room the pins have already
    // holds (one at least), so that a call that ran out of memory needs no more for this
    m_failed = true;
    m_pins.clear();
    if (!table)
    {
        return;
    }
    // NOLINTNEXTLINE(bugprone-exception-escape): push_back below capacity copies an InternalValue, which only adds a reference
    const auto keep = [this](const void* identity, const BorrowedCell& cell) noexcept {
        if (!cell.IsFrozen() && m_pins.size() != m_pins.capacity())
        {
            m_pins.push_back(Pinned{ identity, cell.IsList(), cell.m_source, nullptr });
        }
    };
    try
    {
        m_pins.reserve(table->size());
    }
    catch (const std::exception&)
    {
        for (const auto& [identity, isList] : m_cells)
        {
            const auto found = table->find(identity);
            const bool kept = std::any_of(m_pins.begin(), m_pins.end(), [id = identity](const Pinned& pin) { return pin.identity == id; });
            if (found != table->end() && found->second->IsList() == isList && !kept)
            {
                keep(identity, *found->second);
            }
        }
        return;
    }
    for (const auto& [identity, cell] : *table)
    {
        keep(identity, *cell);
    }
}

void CallExports::PinCell(const BorrowedCell& cell)
{
    // Shares the storage's object, so that the next write copies the items (BorrowedCell::OwnsItemsAlone)
    m_pins.push_back(Pinned{ cell.m_identity, cell.IsList(), cell.Storage(), &cell });
}

void CallExports::Commit() noexcept
{
    for (const auto& pin : m_pins)
    {
        pin.cell->m_shared = true;
        JINJA2CPP_CELL_TRACE_EVENT("pin", Trace::Ptr("reg", Trace::Space::Reg, m_registry), Trace::Ptr("c", Trace::Space::Cell, pin.cell), Trace::Ptr("g", Trace::Space::Gen, pin.cell->TraceGen()),
                                   Trace::Num("call", m_registry->m_call));
    }
}

namespace
{
// The cell of the module's data `item` names, if `item` is an adapter of data with a cell in
// `registry`: by the identity of the data, never by the adapter's link
const BorrowedCell* CellOfItem(const BorrowedCells::Table& table, const InternalValue& item)
{
    const void* identity = nullptr;
    bool isList = true;
    if (auto list = AsList(item))
    {
        const auto* accessor = list->GetAccessor();
        if (!accessor || !accessor->GetCellLink() || accessor->IsGlobalData())
        {
            return nullptr;
        }
        identity = accessor->GetIdentity();
    }
    else if (auto map = AsMap(item))
    {
        const auto* accessor = map->GetAccessor();
        if (!accessor || !accessor->GetCellLink() || accessor->IsGlobalData())
        {
            return nullptr;
        }
        identity = accessor->GetIdentity();
        isList = false;
    }
    else
    {
        return nullptr;
    }
    const auto found = table.find(identity);
    return found != table.end() && found->second->IsList() == isList && !found->second->IsFrozen() ? found->second.get() : nullptr;
}

// The identity of a list or dict the template owns, null for any other value
const void* ContainerIdentity(const InternalValue& item)
{
    if (auto list = AsList(item))
    {
        const auto* accessor = list->GetAccessor();
        return accessor && !accessor->GetCellLink() && accessor->GetMutableItems() ? accessor->GetIdentity() : nullptr;
    }
    if (auto map = AsMap(item))
    {
        const auto* accessor = map->GetAccessor();
        return accessor && !accessor->GetCellLink() && accessor->GetMutableItems() ? accessor->GetIdentity() : nullptr;
    }
    return nullptr;
}

template<typename Fn>
void ForEachItem(const InternalValue& container, const Fn& fn)
{
    if (auto list = AsList(container))
    {
        if (const auto* items = list->GetMutableItems())
        {
            for (const auto& item : *items)
            {
                fn(item);
            }
        }
    }
    else if (auto map = AsMap(container))
    {
        if (const auto* dict = map->GetMutableItems())
        {
            for (const auto& entry : *dict)
            {
                fn(entry.second);
            }
        }
    }
}
} // namespace

namespace
{
// What holds the adapters of a cell's items (BorrowedCell::IsHeldByItsItems): the references of the
// source's adapter the items hold, and the adapters attached to the cell, with the items that hold each
// and their links (an adapter holds one; a block of item adapters holds one for each slot)
class ItemHolders
{
public:
    ItemHolders(const BorrowedCell& cell, const ValueObject& source) noexcept
        : m_cell(&cell)
        , m_source(&source)
    {
    }

    // Throws std::bad_alloc
    void Add(const InternalValue& item)
    {
        const auto [owner, link] = AdapterOf(item);
        if (owner == m_source)
        {
            ++m_sourceUses;
            return;
        }
        if (!owner || !link || link->Current() != m_cell)
        {
            return;
        }
        auto* const known = std::find_if(m_owners.begin(), m_owners.end(), [owner = owner](const Attached& entry) { return entry.owner == owner; });
        if (known == m_owners.end())
        {
            // Many adapters of one cell's data in its own items: the cell stays, rather than a walk of each
            // item over all of them
            if (m_owners.size() == MaxAttached)
            {
                throw std::bad_alloc();
            }
            m_owners.push_back(Attached{ owner, 1 });
        }
        else
        {
            ++known->uses;
        }
        if (std::find(m_links.begin(), m_links.end(), link) == m_links.end())
        {
            m_links.push_back(link);
        }
    }

    // The references to the source's adapter, counting the cell's own
    [[nodiscard]] size_t SourceUses() const noexcept { return m_sourceUses; }
    // The links of the items' adapters attached to the cell
    [[nodiscard]] size_t Links() const noexcept { return m_links.size(); }
    // Whether the items alone hold the adapters attached to the cell
    [[nodiscard]] bool OwnersHeldByItems() const noexcept
    {
        return std::all_of(m_owners.begin(), m_owners.end(), [](const Attached& entry) { return entry.owner->use_count() == entry.uses; });
    }

private:
    struct Attached
    {
        const ValueObject* owner;
        size_t uses;
    };

    static std::pair<const ValueObject*, const CellLink*> AdapterOf(const InternalValue& item) noexcept
    {
        if (auto list = AsList(item))
        {
            return { list->GetOwner(), list->GetAccessor() ? list->GetAccessor()->GetCellLink() : nullptr };
        }
        if (auto map = AsMap(item))
        {
            return { map->GetOwner(), map->GetAccessor() ? map->GetAccessor()->GetCellLink() : nullptr };
        }
        return { nullptr, nullptr };
    }

    static constexpr size_t MaxAttached = 16;

    const BorrowedCell* m_cell;
    const ValueObject* m_source;
    size_t m_sourceUses = 1;
    SmallVector<Attached, 4> m_owners;
    SmallVector<const CellLink*, 4> m_links;
};
} // namespace

bool BorrowedCell::IsHeldByItsItems(const ValueObject& source, size_t links) const noexcept
{
    // A value the host keeps that shares the storage (CallExports::Pin) reads the items: they are not the
    // cell's alone
    const auto* retired = detail::InternalValueAccess::Object(m_retired);
    if (!m_object || m_object->use_count() != 1 || (retired && retired->use_count() != 1))
    {
        return false;
    }
    bool alone = false;
    try
    {
        ItemHolders holders(*this, source);
        const auto add = [&holders](const InternalValue& item) { holders.Add(item); };
        ForEachItem(m_storage, add);
        ForEachItem(m_retired, add);
        // Every attached link owns the cell: no listed one (FindHolders asks for none)
        alone = source.use_count() == holders.SourceUses() && use_count() == links + holders.Links() && holders.OwnersHeldByItems();
    }
    catch (...) // NOLINT(bugprone-empty-catch): out of memory or too many adapters, the cell stays as a cell something else holds does
    {
    }
    if (!alone)
    {
        // Not again until a count falls or the storage doubles (FindHolders): only a holder that lets go can leave
        // the items alone, and a write that adds an item of the cell's own data raises a count. A module that keeps
        // many values walks none of them at each call's end, and one whose kept list grows, by itself too
        // (`l.append(l)`), walks it a logarithmic number of times (verifier of 0573). A holder outside the items
        // that lets go of an attached item's adapter, or in the same call as the items gain a reference, lowers
        // neither count: such a cell waits for the next walk, or the module's end
        m_heldSourceUses = static_cast<uint32_t>(source.use_count());
        m_heldUses = use_count();
        m_heldItems = StorageSize();
    }
    return alone;
}

uint32_t BorrowedCell::StorageSize() const noexcept
{
    const TemplateDict* dict = m_isList ? nullptr : m_map->GetMutableItems();
    const size_t size = m_isList ? m_items->size() : (dict ? dict->size() : 0U);
    return static_cast<uint32_t>(size);
}

void CallExports::Record(const CellTable& table)
{
    if (m_containers)
    {
        // A list or dict the template owns may hold any of the module's data: every live cell
        m_pins.reserve(table.size());
        for (const auto& [identity, cell] : table)
        {
            if (!cell->IsFrozen())
            {
                PinCell(*cell);
            }
        }
        return;
    }
    if (m_cells.size() > 1)
    {
        std::sort(m_cells.begin(), m_cells.end(), [](const auto& lhs, const auto& rhs) { return std::less<>()(lhs.first, rhs.first) || (lhs.first == rhs.first && lhs.second < rhs.second); });
        m_cells.erase(std::unique(m_cells.begin(), m_cells.end()), m_cells.end());
    }
    m_pins.reserve(m_cells.size());
    for (const auto& [identity, isList] : m_cells)
    {
        const auto found = table.find(identity);
        if (found != table.end() && found->second->IsList() == isList && !found->second->IsFrozen())
        {
            PinCell(*found->second);
        }
    }
    // Then the items of what the values read, for the cells of nested data: none to find when
    // the values read every cell of the module already (Module/keep)
    if (m_pins.size() < table.size())
    {
        PinNested(table);
    }
}

void CallExports::PinNested(const CellTable& table)
{
    // The table's keys are the identities, so a cell is pinned at most once by its identity
    detail::FlatSet<const void*> pinned;
    pinned.reserve(m_pins.size());
    std::vector<InternalValue> work;
    work.reserve(m_pins.size());
    for (const auto& pin : m_pins)
    {
        pinned.insert(pin.identity);
        work.push_back(pin.storage);
    }
    detail::FlatSet<const void*> seen;
    const auto visit = [this, &table, &work, &pinned, &seen](const InternalValue& item) {
        if (const auto* cell = CellOfItem(table, item))
        {
            if (pinned.insert(cell->m_identity).second)
            {
                PinCell(*cell);
                work.push_back(cell->Storage());
            }
            return;
        }
        const auto* identity = ContainerIdentity(item);
        if (identity && seen.insert(identity).second)
        {
            work.push_back(item);
        }
    };
    while (!work.empty() && m_pins.size() < table.size())
    {
        const auto container = std::move(work.back());
        work.pop_back();
        ForEachItem(container, visit);
    }
}

void CallExports::Seal() noexcept
{
    if (m_pins.size() > 1)
    {
        std::sort(m_pins.begin(), m_pins.end(), [](const Pinned& lhs, const Pinned& rhs) { return std::less<>()(lhs.identity, rhs.identity); });
    }
    m_registry = nullptr;
    m_cells.clear();
}

} // namespace v2
} // namespace jinja2
