#ifndef JINJA2CPP_SRC_BORROWED_CELLS_H
#define JINJA2CPP_SRC_BORROWED_CELLS_H

#include "internal_value.h"
#include "intrusive_ptr.h"
#include "robin_hood.h"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <type_traits>
#include <vector>

namespace jinja2
{
inline namespace v2
{

class BorrowedCell;
class BorrowedCells;

#ifdef _MSC_VER
#define JINJA2CPP_CELL_UNLIKELY(cond) (cond)
#define JINJA2CPP_CELL_NOINLINE __declspec(noinline)
#else
#define JINJA2CPP_CELL_UNLIKELY(cond) __builtin_expect(static_cast<bool>(cond), 0)
#define JINJA2CPP_CELL_NOINLINE __attribute__((noinline))
#endif

namespace detail
{
// The epoch of the borrowed cells: it moves on every promotion and at the end of a render that
// made a cell, in any thread, so that an adapter learns that its data may have a cell with one
// load and a compare (CellLink::Get). Defined in borrowed_cells.cpp
extern std::atomic<uint32_t> g_cellEpoch;
// The cells that exist now, in any thread's registry; with the epoch it tells a block of item
// adapters that none of its slots can have a cell (QuietEpoch)
extern std::atomic<uint32_t> g_liveCells;
} // namespace detail

// The part of an adapter of the host's data (a list or map lent by reference, docs/tasks/0115)
// that a write in a render attaches to that data's cell (docs/tasks/0049). Python passes the
// object around, so `y = l`, the loop variable over a list and an item read twice are names of
// one object, and a write through one is seen through all. The host's data is never written: the
// first write copies it into a cell the render owns, every adapter of the same data finds the
// cell by the data's identity in the render's registry (BorrowedCells), and reads go to the cell
// from then on. The registry detaches its adapters when the render ends, so an adapter that
// outlives the render (the thread's globals) reads the host's data again in the next one.
//
// A copy of the adapter (one handed to the host) starts detached and finds the cell again
// while the render runs. Adapters of the host's data are read by one thread (internal_value.cpp)
//
// The destructor does not detach: the owner of the adapter calls Release first. An adapter on
// its own does that as it goes (AutoCellLink, OwnedAdapter in internal_value.cpp); a block of
// item adapters (ItemAdapters) releases its slots itself, so that freeing an item that never had
// a cell calls nothing
class CellLink
{
public:
    CellLink() = default;
    // A copy is another adapter: detached
    CellLink(const CellLink& /*other*/) noexcept {}
    CellLink(CellLink&& /*other*/) noexcept {}
    // An adapter given other data: its cell is not of that data, so it detaches
    CellLink& operator=(const CellLink& other) noexcept;
    CellLink& operator=(CellLink&& other) noexcept;
    ~CellLink() = default;

    // Detaches the adapter from its cell, before the adapter goes
    inline void Release() const noexcept;

    // The cell of `self`'s data in the running render, or null while the render has not
    // written to it: one relaxed load and a compare while no promotion happened since the last
    // read through this adapter. An attached adapter never matches the epoch, so that its reads
    // take the call (Find), and the reads of the others carry no more than the compare
    template<typename Accessor>
    [[nodiscard]] const BorrowedCell* Get(const Accessor& self) const
    {
        if (JINJA2CPP_CELL_UNLIKELY(MayHaveCell()))
        {
            using Base = std::conditional_t<std::is_base_of_v<IListAccessor, Accessor>, IListAccessor, IMapAccessor>;
            return Find(static_cast<const Base&>(self));
        }
        return nullptr;
    }

    // Whether Get may return a cell. The hottest reads (an item of a list or map) test it and
    // call a JINJA2CPP_CELL_NOINLINE member for the rest, so that their own code stays small
    // enough to keep what they inline (the string view of an item, Render/many_tags)
    [[nodiscard]] bool MayHaveCell() const noexcept { return detail::g_cellEpoch.load(std::memory_order_relaxed) != m_seenEpoch; }
    // The same for an epoch the caller read
    [[nodiscard]] bool MayHaveCell(uint32_t epoch) const noexcept { return epoch != m_seenEpoch; }

    // The cell the adapter is attached to, read without a lookup and without a change: what an
    // export of the adapter to the host reads (docs/tasks/0331)
    [[nodiscard]] const BorrowedCell* Current() const noexcept { return m_cell; }

    // Attaches the adapter to a new cell holding `storage` (a list or dict the template owns,
    // the copy of the adapter's items) for the data `self` names; false outside a render, where
    // the caller keeps the copy instead
    bool Adopt(const InternalValue& self, const void* identity, bool isList, InternalValue storage) const;

private:
    friend class BorrowedCell;
    friend class BorrowedCells;

    // Out of line, in borrowed_cells.cpp: the reads that call them stay small
    const BorrowedCell* Find(const IListAccessor& self) const;
    const BorrowedCell* Find(const IMapAccessor& self) const;
    const BorrowedCell* Find(const void* identity, bool isList) const;
    void Link(const BorrowedCell& cell) const;
    // Below BorrowedCell, with Release
    inline void Unlink() const noexcept;

    // An epoch the counter skips: the value of m_seenEpoch while the adapter is attached
    static constexpr uint32_t Attached = UINT32_MAX;

    // Not owned: the registry keeps each cell until it has detached every link to it
    mutable const BorrowedCell* m_cell = nullptr;
    // The epoch of the last look, or Attached
    mutable uint32_t m_seenEpoch = 0;
    // The position of this link in the cell's list of links
    mutable uint32_t m_linkIndex = 0;
};

// The render's copy of one container of the host's data: the list or dict the template owns
// that every adapter of the data reads from and writes to once one of them was written to
class BorrowedCell final : public ValueObject
{
public:
    BorrowedCell(InternalValue storage, InternalValue source, bool isList);
    BorrowedCell(const BorrowedCell&) = delete;
    BorrowedCell(BorrowedCell&&) = delete;
    BorrowedCell& operator=(const BorrowedCell&) = delete;
    BorrowedCell& operator=(BorrowedCell&&) = delete;
    ~BorrowedCell() override;

    [[nodiscard]] bool IsList() const noexcept { return m_isList; }
    // The list or dict the template owns, and its accessor
    [[nodiscard]] const InternalValue& Storage() const noexcept { return m_storage; }
    [[nodiscard]] const IListAccessor& List() const noexcept { return *m_list; }
    [[nodiscard]] const IMapAccessor& Map() const noexcept { return *m_map; }
    // The items of the list, read without a virtual call on the hot paths of the list adapter
    [[nodiscard]] InternalValueList& Items() const noexcept { return *m_items; }

private:
    friend class CellLink;
    friend class BorrowedCells;

    InternalValue m_storage;
    // The value promoted first: it keeps the host's data alive, for an adapter that owns it
    // (a user callable's result), so that the identity the registry keys the cell by is not
    // reused while the cell exists
    InternalValue m_source;
    // Views into m_storage, which owns them for the cell's life
    const IListAccessor* m_list = nullptr;
    const IMapAccessor* m_map = nullptr;
    InternalValueList* m_items = nullptr;
    bool m_isList;
    // The adapters attached to the cell, which the registry detaches when the render ends
    mutable std::vector<const CellLink*> m_links;
};

// What an enumeration of the host's list knows between two moves of the epoch: whether the list
// has a cell, and its size when it has none. A step then costs one load and a compare, and the
// item it reads needs no check of its own (docs/tasks/0049)
struct CellStep
{
    // Never an epoch: a step of a new enumeration, or of a list with a cell, whose size is live
    static constexpr uint32_t Recheck = UINT32_MAX;

    IntrusivePtr<const BorrowedCell> cell;
    size_t size = 0;
    uint32_t epoch = Recheck;

    [[nodiscard]] bool IsCurrent() const noexcept { return epoch == detail::g_cellEpoch.load(std::memory_order_relaxed); }
};

// The epoch, if no cell exists now, else CellStep::Recheck, which the epoch never is: while the
// epoch keeps this value, no adapter made since can have been attached to a cell. Read in this
// order: a cell is counted before its promotion moves the epoch (CellLink::Adopt)
inline uint32_t QuietEpoch() noexcept
{
    const auto epoch = detail::g_cellEpoch.load(std::memory_order_relaxed);
    return detail::g_liveCells.load(std::memory_order_relaxed) == 0 ? epoch : CellStep::Recheck;
}

// Inline, as Release, which the destructors of the adapters call
inline void CellLink::Unlink() const noexcept
{
    auto& links = m_cell->m_links;
    const auto* last = links.back();
    links[m_linkIndex] = last;
    last->m_linkIndex = m_linkIndex;
    links.pop_back();
    m_cell = nullptr;
    m_seenEpoch = 0;
}

inline CellLink& CellLink::operator=(const CellLink& other) noexcept
{
    if (this != &other && m_cell)
    {
        Unlink();
    }
    return *this;
}

inline CellLink& CellLink::operator=(CellLink&& other) noexcept
{
    if (this != &other && m_cell)
    {
        Unlink();
    }
    return *this;
}

inline void CellLink::Release() const noexcept
{
    if (m_cell)
    {
        Unlink();
    }
}

// The link of an adapter that is never an item in a block (the generic containers' adapters):
// it detaches when the adapter goes
class AutoCellLink : public CellLink
{
public:
    AutoCellLink() = default;
    AutoCellLink(const AutoCellLink&) = default;
    AutoCellLink(AutoCellLink&&) noexcept = default;
    AutoCellLink& operator=(const AutoCellLink&) = default;
    AutoCellLink& operator=(AutoCellLink&&) noexcept = default;
    ~AutoCellLink() { Release(); }
};

// The cells of one render, made on its first write to the host's data (docs/tasks/0049), or of
// one template module, whose values live across its calls. The render's CycleCollector holds the
// registry in use (CycleCollector::CurrentCells), so that finding it costs a render nothing more,
// and releases its own before it walks: what detaching the adapters frees needs no walk. A render
// started inside another one on the same thread (a user callable rendering a template) has its
// own: it sees the outer render's cells only through the values passed to it, as in Python
class BorrowedCells
{
public:
    BorrowedCells() noexcept = default;
    ~BorrowedCells() { Release(); }
    BorrowedCells(const BorrowedCells&) = delete;
    BorrowedCells(BorrowedCells&&) = delete;
    BorrowedCells& operator=(const BorrowedCells&) = delete;
    BorrowedCells& operator=(BorrowedCells&&) = delete;

    // Detaches every adapter from its cell and frees the cells
    void Release() noexcept
    {
        if (m_cells)
        {
            Finish();
        }
    }

private:
    friend class CellLink;

    // The cells by the identity of the host's data (IListAccessor::GetIdentity)
    using Table = robin_hood::unordered_flat_map<const void*, IntrusivePtr<const BorrowedCell>>;

    void Finish() noexcept;

    std::unique_ptr<Table> m_cells;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_BORROWED_CELLS_H
