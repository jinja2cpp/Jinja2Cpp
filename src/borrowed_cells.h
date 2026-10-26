#ifndef JINJA2CPP_SRC_BORROWED_CELLS_H
#define JINJA2CPP_SRC_BORROWED_CELLS_H

#include "internal_value.h"
#include "intrusive_ptr.h"
#include "robin_hood.h"

#include <atomic>
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
#else
#define JINJA2CPP_CELL_UNLIKELY(cond) __builtin_expect(static_cast<bool>(cond), 0)
#endif

namespace detail
{
// The epoch of the borrowed cells: it moves on every promotion and at the end of a render that
// made a cell, in any thread, so that an adapter learns that its data may have a cell with one
// load and a compare (CellLink::Get). Defined in borrowed_cells.cpp
extern std::atomic<uint32_t> g_cellEpoch;
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
class CellLink
{
public:
    CellLink() = default;
    // A copy is another adapter: detached
    CellLink(const CellLink& /*other*/) noexcept {}
    CellLink(CellLink&& /*other*/) noexcept {}
    CellLink& operator=(const CellLink& /*other*/) noexcept { return *this; }
    CellLink& operator=(CellLink&& /*other*/) noexcept { return *this; }
    ~CellLink();

    // The cell of `self`'s data in the running render, or null while the render has not
    // written to it. One relaxed load and a compare unless a promotion happened since the last
    // read through this adapter
    template<typename Accessor>
    [[nodiscard]] const BorrowedCell* Get(const Accessor& self) const
    {
        if (JINJA2CPP_CELL_UNLIKELY(detail::g_cellEpoch.load(std::memory_order_relaxed) != m_seenEpoch))
        {
            Resync(self.GetIdentity(), std::is_base_of_v<IListAccessor, Accessor>);
        }
        return m_cell.get();
    }

    // Attaches the adapter to a new cell holding `storage` (a list or dict the template owns,
    // the copy of the adapter's items) for the data `self` names; false outside a render, where
    // the caller keeps the copy instead
    bool Adopt(const InternalValue& self, const void* identity, bool isList, InternalValue storage) const;

private:
    friend class BorrowedCell;
    friend class BorrowedCells;

    void Resync(const void* identity, bool isList) const;
    void Link(const BorrowedCell& cell) const;
    void Unlink() const noexcept;

    mutable IntrusivePtr<const BorrowedCell> m_cell;
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

inline CellLink::~CellLink()
{
    if (m_cell)
    {
        Unlink();
    }
}

// The cells of one render, made on its first write to the host's data (docs/tasks/0049), or of
// one template module, whose values live across its calls. A render started inside another one
// on the same thread (a user callable rendering a template) has its own: it sees the outer
// render's cells only through the values passed to it, as in Python. Made after the
// CycleCollector, so that it ends first: what it frees when it detaches its adapters needs no walk
class BorrowedCells
{
public:
    // The registry that promotions and adapters on this thread use while it lives
    class Use
    {
    public:
        // Inline: every render makes one, and most promote nothing (docs/tasks/0138)
        explicit Use(BorrowedCells& cells) noexcept { s_current = &cells; }
        ~Use() { s_current = m_outer; }
        Use(const Use&) = delete;
        Use(Use&&) = delete;
        Use& operator=(const Use&) = delete;
        Use& operator=(Use&&) = delete;

    private:
        BorrowedCells* m_outer = s_current;
    };

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

    // The registry in use on this thread, or null
    static inline thread_local BorrowedCells* s_current = nullptr;

    std::unique_ptr<Table> m_cells;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_BORROWED_CELLS_H
