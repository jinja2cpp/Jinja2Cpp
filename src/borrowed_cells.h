#ifndef JINJA2CPP_SRC_BORROWED_CELLS_H
#define JINJA2CPP_SRC_BORROWED_CELLS_H

#include "internal_value.h"
#include "intrusive_ptr.h"
#include "robin_hood.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/value.h>

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
class CycleCollector;

#ifdef _MSC_VER
#define JINJA2CPP_CELL_UNLIKELY(cond) (cond)
#define JINJA2CPP_CELL_NOINLINE __declspec(noinline)
#else
#define JINJA2CPP_CELL_UNLIKELY(cond) __builtin_expect(static_cast<bool>(cond), 0)
#define JINJA2CPP_CELL_NOINLINE __attribute__((noinline))
#endif

namespace detail
{
// The counters of the borrowed cells that every thread shares. Only borrowed_cells.cpp changes
// them; readers get a value. The storage is a member defined in borrowed_cells.cpp, not a static
// local of an inline function, so code outside the library that reached it would fail to link
// rather than read a copy of its own, and a hidden symbol is one RIP-relative load in a shared
// build
class CellCounters
{
public:
    // Moves on every promotion and at the end of a render that made a cell, in any thread, so
    // that an adapter learns that its data may have a cell with one load and a compare
    // (CellLink::Get)
    static uint32_t Epoch() noexcept { return s_epoch.load(std::memory_order_relaxed); }
    // The cells that exist now, in any thread's registry; with the epoch it tells a block of item
    // adapters that none of its slots can have a cell (QuietEpoch)
    static uint32_t Live() noexcept { return s_live.load(std::memory_order_relaxed); }

private:
    friend class jinja2::BorrowedCells;
    friend class jinja2::CellLink;

    static uint32_t NextEpoch() noexcept;
    static void AddLive(uint32_t count) noexcept { s_live.fetch_add(count, std::memory_order_relaxed); }
    static void RemoveLive(uint32_t count) noexcept { s_live.fetch_sub(count, std::memory_order_relaxed); }

    static std::atomic<uint32_t> s_epoch;
    static std::atomic<uint32_t> s_live;
};

inline uint32_t CellEpoch() noexcept
{
    return CellCounters::Epoch();
}
} // namespace detail

// The part of an adapter of the host's data (a list or map lent by reference, docs/tasks/0115)
// that a write in a render attaches to that data's cell (docs/tasks/0049). Python passes the
// object around, so `y = l`, the loop variable over a list and an item read twice are names of
// one object, and a write through one is seen through all. The host's data is never written: the
// first write copies it into a cell the render owns, every adapter of the same data finds the
// cell by the data's identity in the render's registry (BorrowedCells), and reads go to the cell
// from then on. When the render ends its registry freezes the cells (docs/tasks/0331): an adapter
// of a global, which outlives the render in the thread's snapshot, detaches and reads the host's
// data again in the next render; any other adapter still attached is one a value outlives the
// render in (a container the host keeps), and it keeps the frozen cell, which nothing writes any
// more, as long as it lives.
//
// A copy of an adapter attached to a frozen cell keeps the cell too; a copy of any other adapter
// starts detached. Adapters of the host's data are read by the thread of their render; a value
// handed to the host reads only the link (Current), never changes it (internal_value.cpp)
//
// The destructor does not detach: the owner of the adapter calls Release first. An adapter on
// its own does that as it goes (AutoCellLink, OwnedAdapter in internal_value.cpp); a block of
// item adapters (ItemAdapters) releases its slots itself, so that freeing an item that never had
// a cell calls nothing
class CellLink
{
public:
    CellLink() = default;
    // A copy is another adapter: detached, unless the original reads a frozen cell
    CellLink(const CellLink& other) noexcept
    {
        if (other.m_cell)
        {
            Inherit(other);
        }
    }
    CellLink(CellLink&& other) noexcept
    {
        if (other.m_cell)
        {
            Inherit(other);
        }
    }
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
    [[nodiscard]] bool MayHaveCell() const noexcept { return detail::CellEpoch() != m_seenEpoch; }
    // The same for an epoch the caller read
    [[nodiscard]] bool MayHaveCell(uint32_t epoch) const noexcept { return epoch != m_seenEpoch; }

    // The cell the adapter is attached to, read without a lookup and without a change: what an
    // export of the adapter to the host reads (docs/tasks/0331)
    [[nodiscard]] const BorrowedCell* Current() const noexcept { return m_cell; }
    // Passes fn the frozen cell the link holds a reference to, if any: the Traverse of the
    // adapter's owner, so that the collector frees a cycle through it (docs/tasks/0331)
    void TraverseFrozen(const TraverseFn& fn) const;

    // Attaches the adapter to a new cell holding `storage` (a list or dict the template owns,
    // the copy of the adapter's items) for the data `self` names; false outside a render, where
    // the caller keeps the copy instead, and for an adapter that reads a frozen cell. `held`: the adapter is a
    // global's (ValueObject::IsGlobalData). `owner`: what owns data that lives only as long as
    // its adapters (a user callable's result, a module call's argument), empty for the host's
    // data
    bool Adopt(const InternalValue& self, const void* identity, bool isList, InternalValue storage, bool held, std::weak_ptr<const void> owner = {}) const;

private:
    friend class BorrowedCell;
    friend class BorrowedCells;

    // Out of line, in borrowed_cells.cpp: the reads that call them stay small
    const BorrowedCell* Find(const IListAccessor& self) const;
    const BorrowedCell* Find(const IMapAccessor& self) const;
    const BorrowedCell* Find(const void* identity, bool isList, bool held) const;
    void Link(const BorrowedCell& cell, bool held) const;
    // Detaches from a live cell, or lets go of a frozen one
    JINJA2CPP_CELL_NOINLINE void Unlink() const noexcept;
    // Takes the frozen cell `other` reads, if it reads one
    JINJA2CPP_CELL_NOINLINE void Inherit(const CellLink& other) noexcept;

    // An epoch the counter skips: the value of m_seenEpoch while the adapter is attached
    static constexpr uint32_t Attached = UINT32_MAX;
    // The bit of m_linkIndex that marks the link of a global's adapter, which the end of the render
    // detaches instead of freezing
    static constexpr uint32_t HeldBit = 0x80000000U;

    // A live cell is not owned: its registry keeps it until the render ends. A frozen one is: the
    // link holds a reference to it
    mutable const BorrowedCell* m_cell = nullptr;
    // The epoch of the last look, or Attached
    mutable uint32_t m_seenEpoch = 0;
    // The position of this link in the live cell's list of links, and HeldBit
    mutable uint32_t m_linkIndex = 0;
};

// The render's copy of one container of the host's data: the list or dict the template owns
// that every adapter of the data reads from and writes to once one of them was written to
class BorrowedCell final : public ValueObject
{
public:
    BorrowedCell(InternalValue storage, InternalValue source, bool isList, const BorrowedCells& registry, std::weak_ptr<const void> owner = {});
    BorrowedCell(const BorrowedCell&) = delete;
    BorrowedCell(BorrowedCell&&) = delete;
    BorrowedCell& operator=(const BorrowedCell&) = delete;
    BorrowedCell& operator=(BorrowedCell&&) = delete;
    ~BorrowedCell() override = default;

    void Traverse(const TraverseFn& fn) const override;

    [[nodiscard]] bool IsList() const noexcept { return m_isList; }
    // The list or dict the template owns, and its accessor
    [[nodiscard]] const InternalValue& Storage() const noexcept { return m_storage; }
    [[nodiscard]] const IListAccessor& List() const noexcept { return *m_list; }
    [[nodiscard]] const IMapAccessor& Map() const noexcept { return *m_map; }
    // The items of the list, read without a virtual call on the hot paths of the list adapter
    [[nodiscard]] InternalValueList& Items() const noexcept { return *m_items; }
    // The items a write changes: null once the cell is frozen, which nothing writes. A write in a
    // template module's call after a value of an earlier call took the items copies them first
    // (Unshare)
    [[nodiscard]] InternalValueList* MutableItems() const
    {
        if (JINJA2CPP_CELL_UNLIKELY(m_frozen || m_exportedIn != 0))
        {
            return Unshare() ? m_items : nullptr;
        }
        return m_items;
    }
    [[nodiscard]] TemplateDict* MutableDict() const
    {
        if (JINJA2CPP_CELL_UNLIKELY(m_frozen || m_exportedIn != 0))
        {
            return Unshare() ? m_map->GetMutableItems() : nullptr;
        }
        return m_map->GetMutableItems();
    }
    // The list or dict a value handed to the host reads (docs/tasks/0331)
    [[nodiscard]] GenericList ExportList() const;
    [[nodiscard]] GenericMap ExportMap() const;
    // Whether the render that made the cell ended: the values that outlived it read the cell, and
    // nothing writes it (docs/tasks/0331)
    [[nodiscard]] bool IsFrozen() const noexcept { return m_frozen; }

private:
    friend class CellLink;
    friend class BorrowedCells;

    // Before a write: false for a frozen cell; else copies the items if a value handed out in an
    // earlier call of the module still shares them
    JINJA2CPP_CELL_NOINLINE bool Unshare() const;
    // Notes that a value handed to the host shares the items
    void NoteExport() const noexcept;

    // Never a call (BorrowedCells::BeginCall): the mark once values handed out in two calls share
    // the items, so that every write asks whether one still holds them (docs/tasks/0337)
    static constexpr uint32_t SharedExport = UINT32_MAX;
    // Whether nothing can read the cell any more but the registry: no value holds it, its
    // source alone holds its data and no other adapter is attached to it (BorrowedCells::DropUnheld)
    [[nodiscard]] bool IsUnheld() const noexcept;
    void Bind() const noexcept;

    mutable InternalValue m_storage;
    // The storage before the last Unshare. No code holds a cell's items across a call into the
    // template or the host: an enumeration finds the cell again at each step, sort() after its
    // key calls. One slot, so that a template looping over calls that copy does not keep a copy
    // for each
    mutable InternalValue m_retired;
    // The value promoted first: it keeps the host's data alive, for an adapter that owns it
    // (a user callable's result), so that the identity the registry keys the cell by is not
    // reused while the registry has the cell. Dropped when the cell freezes
    mutable InternalValue m_source;
    // The owner of data that lives only as long as its adapters, empty for the host's data
    // (CellLink::Adopt)
    std::weak_ptr<const void> m_owner;
    // Views into m_storage, which owns them
    mutable const IListAccessor* m_list = nullptr;
    mutable const IMapAccessor* m_map = nullptr;
    mutable InternalValueList* m_items = nullptr;
    // The registry that made the cell; not dereferenced once the cell is frozen
    const BorrowedCells* m_registry;
    // The call of a template module in which a value handed to the host first shared the items, 0
    // when none does (docs/tasks/0331, decision D8), SharedExport once values of two calls share
    // them (docs/tasks/0337)
    mutable uint32_t m_exportedIn = 0;
    bool m_isList;
    // Set when the render ends (BorrowedCells::Finish)
    mutable bool m_frozen = false;
    // The adapters attached to the live cell, which the registry detaches or freezes when the
    // render ends
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

    [[nodiscard]] bool IsCurrent() const noexcept { return epoch == detail::CellEpoch(); }
};

// The epoch, if no cell exists now, else CellStep::Recheck, which the epoch never is: while the
// epoch keeps this value, no adapter made since can have been attached to a cell. Read in this
// order: a cell is counted before its promotion moves the epoch (CellLink::Adopt)
inline uint32_t QuietEpoch() noexcept
{
    const auto epoch = detail::CellEpoch();
    return detail::CellCounters::Live() == 0 ? epoch : CellStep::Recheck;
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

// Inline, as the destructors of the adapters call it: the call is out of line
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

    // Freezes the cells: a global's adapters detach, the others keep the frozen cell they read, and
    // the registry lets go of the cells, so that a cell no value reads any more goes. `collector`
    // gets the frozen cells that values still read, so that it frees the cycles through them
    void Release(CycleCollector* collector = nullptr) noexcept
    {
        if (m_cells)
        {
            Finish(collector);
        }
    }

    // A call of a template module starts: a write in it to a container a value of an earlier call
    // shares copies it first (BorrowedCell::Unshare). A render's registry has one call, 0. A call
    // made from a host callable inside another call of the same module nests in it; returns the
    // call to go back to at EndCall
    [[nodiscard]] uint32_t BeginCall() noexcept
    {
        const auto outer = m_call;
        // 0 is a render's call and SharedExport no call's (BorrowedCell::NoteExport)
        ++m_lastCall;
        if (m_lastCall == 0 || m_lastCall == BorrowedCell::SharedExport)
        {
            m_lastCall = 1;
        }
        m_call = m_lastCall;
        return outer;
    }
    // The call BeginCall started ends: the call it nested in runs again
    void EndCall(uint32_t outer) noexcept { m_call = outer; }
    // Drops the cells nothing reads any more and nothing can find again: the cells of data that
    // lived for one call of a template module (an argument, a user callable's result), which
    // the module's cells would otherwise keep until the module goes. A cell of the host's data
    // stays, as a later call may write it again
    void DropUnheld() noexcept;

private:
    friend class CellLink;
    friend class BorrowedCell;

    // The cells by the identity of the host's data (IListAccessor::GetIdentity)
    using Table = robin_hood::unordered_flat_map<const void*, IntrusivePtr<const BorrowedCell>>;

    void Finish(CycleCollector* collector) noexcept;
    // One pass of DropUnheld; false when it dropped none
    bool DropUnheldOnce() noexcept;

    std::unique_ptr<Table> m_cells;
    // The template module's call that runs, or 0 for a render's registry
    uint32_t m_call = 0;
    // The last call number given out
    uint32_t m_lastCall = 0;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_BORROWED_CELLS_H
