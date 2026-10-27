#include "borrowed_cells.h"

#include "cycle_collector.h"
#include "internal_value.h"
#include "intrusive_ptr.h"
#include "template_dict.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/value.h>

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>

namespace jinja2
{
inline namespace v2
{

namespace detail
{
std::atomic<uint32_t> g_cellEpoch{ 0 };
std::atomic<uint32_t> g_liveCells{ 0 };
std::atomic<uint32_t> g_frozenCells{ 0 };
} // namespace detail

namespace
{
uint32_t NextEpoch() noexcept
{
    // Past the attached mark and the epoch of a new adapter: the counter never holds either,
    // also not for a moment while another thread reads it
    auto epoch = detail::g_cellEpoch.load(std::memory_order_relaxed);
    uint32_t next = 0;
    do
    {
        next = epoch + 1;
        while (next == UINT32_MAX || next == 0)
        {
            ++next;
        }
    } while (!detail::g_cellEpoch.compare_exchange_weak(epoch, next, std::memory_order_relaxed));
    return next;
}
} // namespace

BorrowedCell::BorrowedCell(InternalValue storage, InternalValue source, bool isList, const BorrowedCells& registry)
    : m_storage(std::move(storage))
    , m_source(std::move(source))
    , m_registry(&registry)
    , m_isList(isList)
{
    Bind();
    // A frozen cell is held by the links of the values that outlived the render: a cycle through
    // its items is the collector's (CellLink::TraverseFrozen)
    SetTraversable();
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
    }
    else
    {
        auto map = AsMap(m_storage);
        m_map = map ? map->GetAccessor() : nullptr;
    }
}

GenericList BorrowedCell::ExportList() const
{
    NoteExport();
    return m_list->CreateGenericList();
}

GenericMap BorrowedCell::ExportMap() const
{
    NoteExport();
    return m_map->CreateGenericMap();
}

void BorrowedCell::NoteExport() const noexcept
{
    // Only in the cell's own render or module call, which alone writes the cell: a frozen cell is
    // never written, and a render's cells live for one call
    if (m_frozen || m_exportedIn != 0 || !m_registry || CycleCollector::CurrentCells() != m_registry || m_registry->m_call == 0)
    {
        return;
    }
    m_exportedIn = m_registry->m_call;
}

bool BorrowedCell::Unshare() const
{
    if (m_frozen)
    {
        return false;
    }
    // The call that handed the items out sees its own later writes, as the host does while it runs
    if (m_exportedIn == m_registry->m_call)
    {
        return true;
    }
    m_exportedIn = 0;
    if (m_isList ? m_list->OwnsMutableItemsAlone() : m_map->OwnsMutableItemsAlone())
    {
        // The host let go of what it was handed
        return true;
    }
    // The value of the earlier call keeps the items as they were: the host holds a snapshot that
    // nothing writes any more (decision D8, copy on write)
    InternalValue copy = m_isList ? InternalValue(ListAdapter::CreateAdapter(InternalValueList(*m_items)))
                                  : InternalValue(CreateMapAdapter(TemplateDict(*m_map->GetMutableItems())));
    m_retired = std::exchange(m_storage, std::move(copy));
    Bind();
    return true;
}

// The registry detaches or freezes every link before it lets go of its cells, and a frozen cell
// goes with the last link that reads it
BorrowedCell::~BorrowedCell()
{
    if (m_frozen)
    {
        detail::g_frozenCells.fetch_sub(1, std::memory_order_relaxed);
    }
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
        return m_cell;
    }
    // Read first: a promotion after this read moves the epoch again
    m_seenEpoch = detail::g_cellEpoch.load(std::memory_order_relaxed);
    const auto* cells = CycleCollector::CurrentCells();
    if (!cells || !cells->m_cells)
    {
        return nullptr;
    }
    const auto found = cells->m_cells->find(identity);
    if (found != cells->m_cells->end() && found->second->IsList() == isList)
    {
        Link(*found->second, held);
    }
    return m_cell;
}

void CellLink::Link(const BorrowedCell& cell, bool held) const
{
    // Pushed first: when it cannot be, the adapter stays detached
    cell.m_links.push_back(this);
    m_linkIndex = static_cast<uint32_t>(cell.m_links.size() - 1) | (held ? HeldBit : 0);
    m_cell = &cell;
    m_seenEpoch = Attached;
}

void CellLink::Unlink() const noexcept
{
    const auto* cell = m_cell;
    m_cell = nullptr;
    m_seenEpoch = 0;
    if (cell->m_frozen)
    {
        // Any thread: the reference is atomic, and a frozen cell's links are not listed
        intrusive_ptr_release(cell);
        return;
    }
    auto& links = cell->m_links;
    const auto index = m_linkIndex & ~HeldBit;
    const auto* last = links.back();
    links[index] = last;
    last->m_linkIndex = (last->m_linkIndex & HeldBit) | index;
    links.pop_back();
}

void CellLink::TraverseFrozen(const TraverseFn& fn) const
{
    if (m_cell && m_cell->m_frozen)
    {
        fn(m_cell);
    }
}

void CellLink::Inherit(const CellLink& other) noexcept
{
    if (!other.m_cell->m_frozen)
    {
        return;
    }
    intrusive_ptr_add_ref(other.m_cell);
    m_cell = other.m_cell;
    m_seenEpoch = Attached;
}

bool CellLink::Adopt(const InternalValue& self, const void* identity, bool isList, InternalValue storage, bool held) const
{
    // An adapter of a value kept after its render: nothing writes the frozen cell, and the caller
    // writes its copy instead (docs/tasks/0331)
    if (m_cell && m_cell->m_frozen)
    {
        return false;
    }
    auto* cells = identity ? CycleCollector::CellsForWrite() : nullptr;
    if (!cells)
    {
        return false;
    }
    if (!cells->m_cells)
    {
        cells->m_cells = std::make_unique<BorrowedCells::Table>();
    }
    auto cell = MakeRef<BorrowedCell>(std::move(storage), self, isList, *cells);
    if (isList ? !cell->m_items : !cell->m_map)
    {
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
            return false;
        }
        Link(*entry->second, held);
        return true;
    }
    try
    {
        Link(*cell, held);
    }
    catch (...)
    {
        cells->m_cells->erase(identity);
        throw;
    }
    // Counted before the epoch moves (QuietEpoch)
    detail::g_liveCells.fetch_add(1, std::memory_order_relaxed);
    // The other adapters of the same data look the cell up on their next read
    NextEpoch();
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
    // Then a global's adapter detaches, as the thread's snapshot outlives the render (0211), and
    // every other link still there is held from outside the render: by a value the host keeps,
    // another cell's items, or a cycle. It keeps the frozen cell
    for (const auto& [identity, cell] : *m_cells)
    {
        cell->m_frozen = true;
        for (const auto* link : cell->m_links)
        {
            if ((link->m_linkIndex & CellLink::HeldBit) != 0)
            {
                link->m_cell = nullptr;
                link->m_seenEpoch = 0;
            }
            else
            {
                intrusive_ptr_add_ref(cell.get());
            }
            link->m_linkIndex = 0;
        }
        cell->m_links.clear();
        cell->m_links.shrink_to_fit();
    }
    const auto count = static_cast<uint32_t>(m_cells->size());
    detail::g_frozenCells.fetch_add(count, std::memory_order_relaxed);
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
    m_cells.reset();
    detail::g_liveCells.fetch_sub(count, std::memory_order_relaxed);
    // An adapter that read a cell of this render, or found none while it ran, looks again
    NextEpoch();
}

} // namespace v2
} // namespace jinja2
