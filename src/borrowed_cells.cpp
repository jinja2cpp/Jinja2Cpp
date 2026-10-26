#include "borrowed_cells.h"

#include "cycle_collector.h"
#include "internal_value.h"
#include "intrusive_ptr.h"
#include "template_dict.h" // IWYU pragma: keep (the range-for over a TemplateDict)

#include <atomic>
#include <cstdint>
#include <memory>
#include <utility>

namespace jinja2
{
inline namespace v2
{

namespace detail
{
std::atomic<uint32_t> g_cellEpoch{ 0 };
std::atomic<uint32_t> g_liveCells{ 0 };
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

BorrowedCell::BorrowedCell(InternalValue storage, InternalValue source, bool isList)
    : m_storage(std::move(storage))
    , m_source(std::move(source))
    , m_isList(isList)
{
    if (isList)
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

// The registry detaches every link before it lets go of its cells
BorrowedCell::~BorrowedCell() = default;

const BorrowedCell* CellLink::Find(const IListAccessor& self) const
{
    return Find(self.GetIdentity(), true);
}

const BorrowedCell* CellLink::Find(const IMapAccessor& self) const
{
    return Find(self.GetIdentity(), false);
}

const BorrowedCell* CellLink::Find(const void* identity, bool isList) const
{
    // A cell stays until its render detaches it, also while a render nested in it runs
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
        Link(*found->second);
    }
    return m_cell;
}

void CellLink::Link(const BorrowedCell& cell) const
{
    // Pushed first: when it cannot be, the adapter stays detached
    cell.m_links.push_back(this);
    m_linkIndex = static_cast<uint32_t>(cell.m_links.size() - 1);
    m_cell = &cell;
    m_seenEpoch = Attached;
}

bool CellLink::Adopt(const InternalValue& self, const void* identity, bool isList, InternalValue storage) const
{
    auto* cells = identity ? CycleCollector::CellsForWrite() : nullptr;
    if (!cells)
    {
        return false;
    }
    if (!cells->m_cells)
    {
        cells->m_cells = std::make_unique<BorrowedCells::Table>();
    }
    auto cell = MakeRef<BorrowedCell>(std::move(storage), self, isList);
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
        Link(*entry->second);
        return true;
    }
    try
    {
        Link(*cell);
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

namespace
{
// The cell of `item` in `cells`, when it is an adapter of the host's data that has one
template<typename Table>
const BorrowedCell* CellOfItem(const Table& cells, const InternalValue& item)
{
    const void* identity = nullptr;
    bool isList = false;
    if (auto list = AsList(item))
    {
        const auto* accessor = list->GetAccessor();
        if (!accessor || !accessor->IsBorrowed())
        {
            return nullptr;
        }
        identity = accessor->GetIdentity();
        isList = true;
    }
    else if (auto map = AsMap(item))
    {
        const auto* accessor = map->GetAccessor();
        if (!accessor || !accessor->IsBorrowed())
        {
            return nullptr;
        }
        identity = accessor->GetIdentity();
    }
    else
    {
        return nullptr;
    }
    const auto found = cells.find(identity);
    return found != cells.end() && found->second->IsList() == isList ? found->second.get() : nullptr;
}

} // namespace

void BorrowedCells::Relink() noexcept
{
    // One pass over the items the cells hold: an item of an item is a cell's item itself when
    // the render wrote to it, and an adapter whose data has no cell reads the host's data, as
    // the render did
    for (const auto& [identity, cell] : *m_cells)
    {
        try
        {
            // Noted first, as a write notes its receiver: the list may hold itself, given back
            // by a callable (`l.append(id(l))`), or come to through this pass (`l.append(l)`).
            // A write through an adapter noted the adapter, which does not show the cell
            CycleCollector::NoteStore(cell->Storage());
        }
        catch (...) // NOLINT(bugprone-empty-catch): out of memory: its items stay views of the host's data
        {
            continue;
        }
        if (cell->IsList())
        {
            for (auto& item : cell->Items())
            {
                if (const auto* found = CellOfItem(*m_cells, item))
                {
                    item = found->Storage();
                }
            }
        }
        else if (auto* items = cell->Map().GetMutableItems())
        {
            for (auto& entry : *items)
            {
                if (const auto* found = CellOfItem(*m_cells, entry.second))
                {
                    entry.second = found->Storage();
                }
            }
        }
    }
}

void BorrowedCells::Finish() noexcept
{
    // While the cells are attached and the table holds them: a list the host keeps then holds
    // the render's other copies, not adapters that read the host's data again
    if (m_exported)
    {
        m_exported = false;
        Relink();
    }
    // Detached first, while the table holds every cell: freeing a cell frees values that may
    // hold adapters attached to another one
    for (const auto& [identity, cell] : *m_cells)
    {
        for (const auto* link : cell->m_links)
        {
            link->m_cell = nullptr;
            link->m_seenEpoch = 0;
        }
        cell->m_links.clear();
    }
    const auto count = static_cast<uint32_t>(m_cells->size());
    m_cells.reset();
    detail::g_liveCells.fetch_sub(count, std::memory_order_relaxed);
    // An adapter that read a cell of this render, or found none while it ran, looks again
    NextEpoch();
}

} // namespace v2
} // namespace jinja2
