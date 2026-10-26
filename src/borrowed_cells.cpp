#include "borrowed_cells.h"

#include "internal_value.h"
#include "intrusive_ptr.h"

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
} // namespace detail

namespace
{
uint32_t NextEpoch() noexcept
{
    return detail::g_cellEpoch.fetch_add(1, std::memory_order_relaxed) + 1;
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

// The registry detaches every link before it lets go of its cells, and a link holds its cell
BorrowedCell::~BorrowedCell() = default;

void CellLink::Resync(const void* identity, bool isList) const
{
    // Read first: a promotion after this read moves the epoch again
    m_seenEpoch = detail::g_cellEpoch.load(std::memory_order_relaxed);
    // A cell stays until its render detaches it, also while a render nested in it runs
    if (m_cell)
    {
        return;
    }
    const auto* cells = BorrowedCells::s_current;
    if (!cells || !cells->m_cells)
    {
        return;
    }
    const auto found = cells->m_cells->find(identity);
    if (found != cells->m_cells->end() && found->second->IsList() == isList)
    {
        Link(*found->second);
    }
}

void CellLink::Link(const BorrowedCell& cell) const
{
    // Pushed first: when it cannot be, the adapter stays detached
    cell.m_links.push_back(this);
    m_linkIndex = static_cast<uint32_t>(cell.m_links.size() - 1);
    m_cell = IntrusivePtr<const BorrowedCell>(&cell);
}

void CellLink::Unlink() const noexcept
{
    auto& links = m_cell->m_links;
    const auto* last = links.back();
    links[m_linkIndex] = last;
    last->m_linkIndex = m_linkIndex;
    links.pop_back();
    m_cell = nullptr;
}

bool CellLink::Adopt(const InternalValue& self, const void* identity, bool isList, InternalValue storage) const
{
    auto* cells = BorrowedCells::s_current;
    if (!cells || !identity)
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
    // The other adapters of the same data look the cell up on their next read
    m_seenEpoch = NextEpoch();
    return true;
}

void BorrowedCells::Finish() noexcept
{
    // Detached first, while the table holds every cell: freeing a cell frees values that may
    // hold adapters attached to another one
    for (const auto& [identity, cell] : *m_cells)
    {
        for (const auto* link : cell->m_links)
        {
            link->m_cell = nullptr;
        }
        cell->m_links.clear();
    }
    m_cells.reset();
    // An adapter that read a cell of this render, or found none while it ran, looks again
    NextEpoch();
}

} // namespace v2
} // namespace jinja2
