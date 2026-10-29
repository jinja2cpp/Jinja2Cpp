#include "borrowed_cells.h"

#include "cell_trace.h"
#include "cycle_collector.h"
#include "internal_value.h"
#include "intrusive_ptr.h"
#include "render_workspace.h"
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
std::atomic<uint32_t> CellCounters::s_epoch{ 0 };
std::atomic<uint32_t> CellCounters::s_live{ 0 };

uint32_t CellCounters::NextEpoch() noexcept
{
    // Past the attached mark and the epoch of a new adapter: the counter never holds either,
    // also not for a moment while another thread reads it
    auto epoch = s_epoch.load(std::memory_order_relaxed);
    uint32_t next = 0;
    do
    {
        next = epoch + 1;
        while (next == UINT32_MAX || next == 0)
        {
            ++next;
        }
    } while (!s_epoch.compare_exchange_weak(epoch, next, std::memory_order_relaxed));
    return next;
}
} // namespace detail

BorrowedCell::BorrowedCell(InternalValue storage, InternalValue source, const void* identity, bool isList, const BorrowedCells& registry, std::weak_ptr<const void> owner)
    : m_storage(std::move(storage))
    , m_source(std::move(source))
    , m_owner(std::move(owner))
    , m_identity(identity)
    , m_registry(&registry)
    , m_isList(isList)
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
        const auto& exports = call->Exports();
        exports->AddCell(m_identity, true);
#ifdef JINJA2CPP_CELL_TRACE
        TraceExport(true);
#endif
        return ExportCallList(exports, m_identity, InternalValue());
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
        const auto& exports = call->Exports();
        exports->AddCell(m_identity, false);
#ifdef JINJA2CPP_CELL_TRACE
        TraceExport(true);
#endif
        return ExportCallMap(exports, m_identity, InternalValue());
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

bool BorrowedCell::IsUnheld() const noexcept
{
    // The registry's reference alone, for data that lives only as long as its adapters, and the
    // reference of the source's own link. The only other adapters of the data are the anchors of
    // copied items (LifetimeAnchor), which are never read; so when nothing but the cell holds the
    // source, no adapter of the data can be read or made again, and no later call finds the cell.
    // The data of the host, or of a global, can be lent again at any time: its cell stays
    if (m_frozen || !m_links.empty() || m_owner.expired())
    {
        return false;
    }
    const ValueObject* owner = nullptr;
    const CellLink* link = nullptr;
    if (m_isList)
    {
        if (auto list = AsList(m_source))
        {
            owner = list->GetOwner();
            link = list->GetAccessor() ? list->GetAccessor()->GetCellLink() : nullptr;
        }
    }
    else if (auto map = AsMap(m_source))
    {
        owner = map->GetOwner();
        link = map->GetAccessor() ? map->GetAccessor()->GetCellLink() : nullptr;
    }
    if (!owner || owner->use_count() != 1)
    {
        return false;
    }
    // Every other link owns the cell: only the source's own adapter reads it
    return use_count() == (link && link->m_cell == this ? 2U : 1U);
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

const BorrowedCell* CellLink::Find(const IListAccessor& self) const
{
    return Find(self.GetIdentity(), true, self.IsGlobalData());
}

const BorrowedCell* CellLink::Find(const IMapAccessor& self) const
{
    return Find(self.GetIdentity(), false, self.IsGlobalData());
}

const BorrowedCell* CellLink::Find(const void* identity, bool isList, bool held) const
{
    // A cell stays until its render detaches it, also while a render nested in it runs; a frozen
    // one for as long as the link lives
    if (m_cell)
    {
        JINJA2CPP_CELL_TRACE_EVENT("hit", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, m_cell));
        return m_cell;
    }
    // Read first: a promotion after this read moves the epoch again
    m_seenEpoch = detail::CellEpoch();
    const auto* cells = CycleCollector::CurrentCells();
    if (!cells || !cells->m_cells)
    {
        JINJA2CPP_CELL_TRACE_EVENT("find", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("idn", Trace::Space::Idn, identity), Trace::Num("list", isList ? 1 : 0),
                                   Trace::Num("e", m_seenEpoch), Trace::Num("hit", 0));
        return nullptr;
    }
    const auto found = cells->m_cells->find(identity);
    const BorrowedCell* hit = found != cells->m_cells->end() && found->second->IsList() == isList ? found->second.get() : nullptr;
    JINJA2CPP_CELL_TRACE_EVENT("find", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("idn", Trace::Space::Idn, identity), Trace::Num("list", isList ? 1 : 0),
                               Trace::Num("e", m_seenEpoch), Trace::Ptr("hit", Trace::Space::Cell, hit));
    if (hit)
    {
        Link(*hit, held);
    }
    return m_cell;
}

void CellLink::Link(const BorrowedCell& cell, bool held) const
{
    if (!held)
    {
        // A reference: the host may let go of the adapter on any thread (docs/tasks/0340)
        intrusive_ptr_add_ref(&cell);
        m_linkIndex = OwnsBit;
        m_cell = &cell;
        m_seenEpoch = Attached;
        JINJA2CPP_CELL_TRACE_EVENT("link", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, &cell), Trace::Num("held", 0));
        return;
    }
    // Pushed first: when it cannot be, the adapter stays detached and, with an epoch the counter
    // never holds, looks for the cell again at its next read
    m_seenEpoch = 0;
#ifdef JINJA2CPP_CELL_TRACE
    try
    {
        cell.m_links.push_back(this);
    }
    catch (...)
    {
        JINJA2CPP_CELL_TRACE_EVENT("linkfail", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, &cell));
        throw;
    }
#else
    cell.m_links.push_back(this);
#endif
    m_linkIndex = static_cast<uint32_t>(cell.m_links.size() - 1) | HeldBit;
    m_cell = &cell;
    m_seenEpoch = Attached;
    JINJA2CPP_CELL_TRACE_EVENT("link", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, &cell), Trace::Num("held", 1));
    // The thread keeps the globals' adapters for its next render, which may run inside this one
    RenderWorkspace::ForThisThread().GlobalAttached();
}

void CellLink::Unlink() const noexcept
{
    const auto* cell = m_cell;
    const bool owns = (m_linkIndex & OwnsBit) != 0;
    JINJA2CPP_CELL_TRACE_EVENT("unlink", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("c", Trace::Space::Cell, cell), Trace::Num("owns", owns ? 1 : 0));
    m_cell = nullptr;
    m_seenEpoch = 0;
    if (owns)
    {
        // Any thread: the reference is atomic, and the cell's list does not have the link
        m_linkIndex = 0;
        intrusive_ptr_release(cell);
        return;
    }
    // A global's link, on the thread of the render that made the cell
    auto& links = cell->m_links;
    const auto index = m_linkIndex & ~HeldBit;
    const auto* last = links.back();
    links[index] = last;
    last->m_linkIndex = (last->m_linkIndex & HeldBit) | index;
    links.pop_back();
}

void CellLink::TraverseFrozen(const TraverseFn& fn) const
{
    if (m_cell && (m_linkIndex & OwnsBit) != 0 && m_cell->m_frozen)
    {
        fn(m_cell);
    }
}

void CellLink::Inherit(const CellLink& other) noexcept
{
    if ((other.m_linkIndex & OwnsBit) == 0 || !other.m_cell->m_frozen)
    {
        return;
    }
    intrusive_ptr_add_ref(other.m_cell);
    m_cell = other.m_cell;
    m_linkIndex = OwnsBit;
    m_seenEpoch = Attached;
    JINJA2CPP_CELL_TRACE_EVENT("inherit", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("from", Trace::Space::Link, &other), Trace::Ptr("c", Trace::Space::Cell, m_cell));
}

bool CellLink::Adopt(const InternalValue& self, const void* identity, bool isList, InternalValue storage, bool held, std::weak_ptr<const void> owner) const
{
    // An adapter of a value kept after its render: nothing writes the frozen cell, and the caller
    // writes its copy instead (docs/tasks/0331)
    if (m_cell && m_cell->m_frozen)
    {
        JINJA2CPP_CELL_TRACE_EVENT("adoptno", Trace::Ptr("a", Trace::Space::Link, this), Trace::Num("why", 1), Trace::Ptr("c", Trace::Space::Cell, m_cell));
        return false;
    }
    auto* cells = identity ? CycleCollector::CellsForWrite() : nullptr;
    if (!cells)
    {
        JINJA2CPP_CELL_TRACE_EVENT("adoptno", Trace::Ptr("a", Trace::Space::Link, this), Trace::Num("why", 2), Trace::Num("c", 0));
        return false;
    }
    if (!cells->m_cells)
    {
        cells->m_cells = std::make_unique<BorrowedCells::Table>();
    }
    auto cell = MakeRef<BorrowedCell>(std::move(storage), self, identity, isList, *cells, std::move(owner));
    if (isList ? !cell->m_items : !cell->m_map)
    {
        JINJA2CPP_CELL_TRACE_EVENT("adoptno", Trace::Ptr("a", Trace::Space::Link, this), Trace::Num("why", 3), Trace::Num("c", 0));
        return false;
    }
    if (m_cell)
    {
        Unlink();
    }
    auto [entry, isNew] = cells->m_cells->try_emplace(identity, cell);
    if (!isNew)
    {
        // Another adapter of the data made one first: this one had not seen it yet
        if (entry->second->IsList() != isList)
        {
            JINJA2CPP_CELL_TRACE_EVENT("adoptno", Trace::Ptr("a", Trace::Space::Link, this), Trace::Num("why", 4), Trace::Ptr("c", Trace::Space::Cell, entry->second.get()));
            return false;
        }
        JINJA2CPP_CELL_TRACE_EVENT("adopt", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("idn", Trace::Space::Idn, identity), Trace::Num("list", isList ? 1 : 0),
                                   Trace::Num("held", held ? 1 : 0), Trace::Num("owned", 0), Trace::Ptr("reg", Trace::Space::Reg, cells),
                                   Trace::Ptr("c", Trace::Space::Cell, entry->second.get()), Trace::Num("new", 0), Trace::Num("g", 0), Trace::Num("src", 0));
        Link(*entry->second, held);
        return true;
    }
#ifdef JINJA2CPP_CELL_TRACE
    if (detail::CellTrace::On())
    {
        // The link IsUnheld compares the cell's links with
        const CellLink* source = nullptr;
        if (auto list = isList ? AsList(cell->m_source) : std::nullopt)
        {
            source = list->GetAccessor() ? list->GetAccessor()->GetCellLink() : nullptr;
        }
        else if (auto map = isList ? std::nullopt : AsMap(cell->m_source))
        {
            source = map->GetAccessor() ? map->GetAccessor()->GetCellLink() : nullptr;
        }
        JINJA2CPP_CELL_TRACE_EVENT("adopt", Trace::Ptr("a", Trace::Space::Link, this), Trace::Ptr("idn", Trace::Space::Idn, identity), Trace::Num("list", isList ? 1 : 0),
                                   Trace::Num("held", held ? 1 : 0), Trace::Num("owned", cell->m_owner.expired() ? 0 : 1), Trace::Ptr("reg", Trace::Space::Reg, cells),
                                   Trace::Ptr("c", Trace::Space::Cell, cell.get()), Trace::Num("new", 1), Trace::Ptr("g", Trace::Space::NewGen, cell->TraceGen()),
                                   Trace::Ptr("src", Trace::Space::Link, source));
    }
#endif
    try
    {
        Link(*cell, held);
    }
    catch (...)
    {
        JINJA2CPP_CELL_TRACE_EVENT("adoptundo", Trace::Ptr("c", Trace::Space::Cell, cell.get()), Trace::Ptr("reg", Trace::Space::Reg, cells));
        cells->m_cells->erase(identity);
        throw;
    }
    // Counted before the epoch moves (QuietEpoch)
    detail::CellCounters::AddLive(1);
    // The other adapters of the same data look the cell up on their next read
    [[maybe_unused]] const auto epoch = detail::CellCounters::NextEpoch();
    JINJA2CPP_CELL_TRACE_EVENT("promote", Trace::Ptr("c", Trace::Space::Cell, cell.get()), Trace::Num("e", epoch), Trace::Num("live", detail::CellCounters::Live()));
    return true;
}

void BorrowedCells::DropUnheld() noexcept
{
    // A dropped cell lets go of its items, which may hold the only source of another cell (the
    // adapter of a nested list): again until a pass drops none
    while (m_cells && DropUnheldOnce())
    {
    }
}

bool BorrowedCells::DropUnheldOnce() noexcept
{
    size_t count = 0;
    for (const auto& [identity, cell] : *m_cells)
    {
        count += cell->IsUnheld() ? 1U : 0U;
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
    for (auto entry = m_cells->begin(); entry != m_cells->end();)
    {
        if (entry->second->IsUnheld())
        {
            JINJA2CPP_CELL_TRACE_EVENT("drop", Trace::Ptr("reg", Trace::Space::Reg, this), Trace::Ptr("c", Trace::Space::Cell, entry->second.get()),
                                       Trace::Links("links", entry->second->m_links.data(), entry->second->m_links.size()));
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
    dropped.clear();
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
            link->m_seenEpoch = 0;
            link->m_linkIndex = 0;
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
    JINJA2CPP_CELL_TRACE_EVENT("finished", Trace::Ptr("reg", Trace::Space::Reg, this), Trace::Num("e", epoch), Trace::Num("live", detail::CellCounters::Live()));
}

const IntrusivePtr<CallExports>& BorrowedCells::Exports()
{
    if (!m_exports)
    {
        m_exports = MakeRef<CallExports>(*this);
    }
    return m_exports;
}

void BorrowedCells::PinExports() noexcept
{
    if (m_exports)
    {
        m_exports->Pin();
        m_exports.reset();
    }
}

IntrusivePtr<const CallExports> ExportOfContainer()
{
    auto* cells = CycleCollector::CurrentCells();
    if (!cells || !cells->InCall())
    {
        return {};
    }
    const auto& exports = cells->Exports();
    exports->m_containers = true;
    return exports;
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
    const auto* const found = std::lower_bound(m_pins.begin(), m_pins.end(), identity, [](const Pinned& pin, const void* key) { return std::less<>()(pin.identity, key); });
    return found != m_pins.end() && found->identity == identity && found->isList == isList ? &found->storage : nullptr;
}

void CallExports::AddCell(const void* identity, bool isList)
{
    // A call that hands out the same data again (a loop, or the next call of Module/keep) notes it once
    for (const auto& [noted, list] : m_cells)
    {
        if (noted == identity && list == isList)
        {
            return;
        }
    }
    m_cells.emplace_back(identity, isList);
    // The pin of the cells handed out needs no allocation of its own (Pin)
    m_pins.reserve(m_cells.size());
}

bool CallExports::PinCell(const BorrowedCell& cell)
{
    for (const auto& pin : m_pins)
    {
        if (pin.identity == cell.m_identity && pin.isList == cell.IsList())
        {
            return false;
        }
    }
    // Shares the storage's object, so that the next write copies the items (BorrowedCell::OwnsItemsAlone)
    m_pins.push_back(Pinned{ cell.m_identity, cell.IsList(), cell.Storage() });
    cell.m_shared = true;
    JINJA2CPP_CELL_TRACE_EVENT("pin", Trace::Ptr("reg", Trace::Space::Reg, m_registry), Trace::Ptr("c", Trace::Space::Cell, &cell), Trace::Ptr("g", Trace::Space::Gen, cell.TraceGen()),
                               Trace::Num("call", m_registry->m_call));
    return true;
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

void CallExports::PinHandedOut(const BorrowedCells::Table& table)
{
    // AddCell reserved their room, so that these need no allocation
    for (const auto& [identity, isList] : m_cells)
    {
        const auto found = table.find(identity);
        if (found != table.end() && found->second->IsList() == isList && !found->second->IsFrozen())
        {
            PinCell(*found->second);
        }
    }
    if (m_containers && m_pins.size() < table.size())
    {
        // A list or dict the template owns may hold any of the module's data
        m_pins.reserve(table.size());
        for (const auto& [identity, cell] : table)
        {
            if (!cell->IsFrozen())
            {
                PinCell(*cell);
            }
        }
    }
}

void CallExports::PinNested(const BorrowedCells::Table& table)
{
    std::vector<InternalValue> work;
    work.reserve(m_pins.size());
    for (const auto& pin : m_pins)
    {
        work.push_back(pin.storage);
    }
    std::vector<const void*> seen;
    const auto visit = [this, &table, &work, &seen](const InternalValue& item) {
        if (const auto* cell = CellOfItem(table, item))
        {
            if (PinCell(*cell))
            {
                work.push_back(cell->Storage());
            }
            return;
        }
        const auto* identity = ContainerIdentity(item);
        if (identity && std::find(seen.begin(), seen.end(), identity) == seen.end())
        {
            seen.push_back(identity);
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

void CallExports::Pin() noexcept
{
    const auto* registry = m_registry;
    if (!registry || !registry->m_cells)
    {
        m_registry = nullptr;
        m_cells.clear();
        return;
    }
    const auto& table = *registry->m_cells;
    try
    {
        PinHandedOut(table);
        // Then the items of what the values read, for the cells of nested data: none to find
        // when the values read every cell of the module already (Module/keep, or a list the
        // template owns)
        if (m_pins.size() < table.size())
        {
            PinNested(table);
        }
    }
    catch (const std::exception&) // NOLINT(bugprone-empty-catch): best effort, the comment says what is lost
    {
        // Out of memory (bad_alloc, or length_error from SmallVector): the cells handed out are pinned, and nested data whose cell could not be
        // recorded reads the host's data
    }
    if (m_pins.size() > 1)
    {
        std::sort(m_pins.begin(), m_pins.end(), [](const Pinned& lhs, const Pinned& rhs) { return std::less<>()(lhs.identity, rhs.identity); });
    }
    m_registry = nullptr;
    m_cells.clear();
}

} // namespace v2
} // namespace jinja2
