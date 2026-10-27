#include "borrowed_cells.h"

#include "cycle_collector.h"
#include "internal_value.h"
#include "intrusive_ptr.h"
#include "robin_hood.h"
#include "template_dict.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

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

// Breaks the cycles through frozen cells (docs/tasks/0331, decision D7): a written container
// that holds itself (`l.append(l)`), directly or through the template's lists and dicts and
// other cells, and that a value outlives the render in, would be a cycle no collector sees any
// more once the host lets go of it. A depth-first walk over the frozen cells' items and the
// template's containers in them detaches each link back to a cell on the walk's path, which then
// reads the host's data, as on master, where the write had not reached that name. The walk
// enters no host data and no host callable, so it ends (the walk of r6 did not)
class FrozenCycleBreaker
{
public:
    // Walks from each cell of `cells` a link still reads
    template<typename Table>
    void Run(const Table& cells)
    {
        for (const auto& [identity, cell] : cells)
        {
            if (cell->use_count() > 1)
            {
                Enter(Items(*cell));
                Walk();
            }
        }
    }

private:
    // The items of a list or dict on the walk's path, and the next one to look at
    struct Node
    {
        const InternalValueList* list = nullptr;
        const TemplateDict* dict = nullptr;
        size_t next = 0;
        TemplateDict::const_iterator it{};
    };

    static Node Items(const BorrowedCell& cell)
    {
        Node node;
        if (cell.IsList())
        {
            node.list = &cell.Items();
        }
        else
        {
            node.dict = cell.Map().GetMutableItems();
            if (node.dict)
            {
                node.it = node.dict->begin();
            }
        }
        return node;
    }
    static const void* Key(const Node& node) { return node.list ? static_cast<const void*>(node.list) : static_cast<const void*>(node.dict); }

    void Enter(Node node)
    {
        const auto* key = Key(node);
        if (!key || !m_seen.emplace(key, true).second)
        {
            return;
        }
        m_path.push_back(node);
    }

    void Walk()
    {
        while (!m_path.empty())
        {
            auto& node = m_path.back();
            const InternalValue* item = nullptr;
            if (node.list && node.next < node.list->size())
            {
                item = &(*node.list)[node.next++];
            }
            else if (node.dict && node.it != node.dict->end())
            {
                item = &node.it->second;
                ++node.it;
            }
            if (!item)
            {
                m_seen[Key(node)] = false;
                m_path.pop_back();
                continue;
            }
            Visit(*item);
        }
    }

    void Visit(const InternalValue& item)
    {
        Node node;
        const CellLink* link = nullptr;
        if (auto list = AsList(item))
        {
            const auto* accessor = list->GetAccessor();
            link = accessor ? accessor->GetCellLink() : nullptr;
            node.list = accessor && !link ? accessor->GetMutableItems() : nullptr;
        }
        else if (auto map = AsMap(item))
        {
            const auto* accessor = map->GetAccessor();
            link = accessor ? accessor->GetCellLink() : nullptr;
            node.dict = accessor && !link ? accessor->GetMutableItems() : nullptr;
            if (node.dict)
            {
                node.it = node.dict->begin();
            }
        }
        if (link)
        {
            if (!link->m_cell || !link->m_cell->IsFrozen())
            {
                return;
            }
            node = Items(*link->m_cell);
            const auto found = m_seen.find(Key(node));
            if (found != m_seen.end() && found->second)
            {
                // Back to a cell on the path: the table still holds it
                link->Unlink();
                return;
            }
        }
        Enter(node);
    }

    // The lists and dicts the walk entered: true while on its path
    robin_hood::unordered_flat_map<const void*, bool> m_seen;
    std::vector<Node> m_path;
};

void BorrowedCells::Finish() noexcept
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
    try
    {
        FrozenCycleBreaker().Run(*m_cells);
    }
    catch (...)
    {
        // Out of memory: the cycles not broken leak, as the collector's do (docs/tasks/0283)
    }
    // A cell no link reads goes now, and with it what only its items held
    m_cells.reset();
    detail::g_liveCells.fetch_sub(count, std::memory_order_relaxed);
    // An adapter that read a cell of this render, or found none while it ran, looks again
    NextEpoch();
}

} // namespace v2
} // namespace jinja2
