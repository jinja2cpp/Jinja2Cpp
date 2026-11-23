#ifndef JINJA2CPP_SRC_CYCLE_COLLECTOR_H
#define JINJA2CPP_SRC_CYCLE_COLLECTOR_H

#include "borrowed_cells.h"
#include "internal_value.h"
#include "recursion_guard.h"

#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <utility>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
struct RenderThreadState;
} // namespace detail

// The tables of one cycle walk (docs/tasks/0486): the objects found, in the order found, with the
// references the walk counted to each, an open-addressing index over them and the stack of the
// marking pass. The collector reserves them as it notes candidates, where running out of memory is a
// plain std::bad_alloc, so that the walk at the end of a render has what it needs. A walk that
// finds more objects than that tries to grow them, and when it cannot it saturates: the objects
// not yet found stay untracked, which is safe, as an untracked object's reference to a tracked one
// is not counted and the tracked one then looks held from outside. Only a cycle through an
// untracked object is missed
class WalkTables
{
public:
    struct Node
    {
        const ValueObject* object = nullptr;
        // References from the other nodes (ValueObject::Traverse) and from the walk's roots
        unsigned refs = 0;
        bool reachable = false;
    };

    WalkTables() noexcept = default;
    WalkTables(WalkTables&& other) noexcept;
    WalkTables(const WalkTables&) = delete;
    WalkTables& operator=(const WalkTables&) = delete;
    WalkTables& operator=(WalkTables&&) = delete;
    ~WalkTables() = default;

    [[nodiscard]] size_t Capacity() const noexcept { return m_capacity; }
    // Room for `nodes` nodes at least, at least doubling. Throws std::bad_alloc and then changes
    // nothing
    void Reserve(size_t nodes);
    // The walk saturates once it has found `nodes` objects (none: no limit), as if growing had failed
    void SetLimit(size_t nodes) noexcept { m_limit = nodes; }
    // Whether the walk found objects it did not track
    [[nodiscard]] bool Saturated() const noexcept { return m_saturated; }
    // Empties the tables for a walk, keeping their room
    void Reset() noexcept;
    // Counts a reference to `object`, adding it when new, unless the walk is saturated
    void Count(const ValueObject* object) noexcept;
    [[nodiscard]] size_t Size() const noexcept { return m_size; }
    // The nodes in the order found, which grows while the walk goes on
    [[nodiscard]] Node& At(size_t index) noexcept { return m_nodes[index]; }
    [[nodiscard]] const Node& At(size_t index) const noexcept { return m_nodes[index]; }
    // The index of `object`, or Size() when it is not tracked
    [[nodiscard]] size_t IndexOf(const ValueObject* object) const noexcept;
    // The marking pass's stack: at most one entry for each node
    void Push(size_t index) noexcept { m_pending[m_pendingSize++] = static_cast<uint32_t>(index); }
    [[nodiscard]] bool Pop(size_t& index) noexcept;

private:
    static constexpr size_t MinCapacity = 16;

    [[nodiscard]] size_t Probe(const ValueObject* object) const noexcept;

    std::unique_ptr<Node[]> m_nodes;
    // Node index + 1, 0 for an empty slot; a power of two slots, at least twice the capacity
    std::unique_ptr<uint32_t[]> m_slots;
    std::unique_ptr<uint32_t[]> m_pending;
    size_t m_capacity = 0;
    size_t m_slotMask = 0;
    size_t m_size = 0;
    size_t m_pendingSize = 0;
    size_t m_limit = 0;
    bool m_saturated = false;
};

// What CycleCollector::KeepUntilEnd holds, each object with the owner it is of use to: released
// once its owner has expired, or all at once
class KeptObjects
{
public:
    // `names`: the copy of a frame's names `object` is, or null. Throws std::bad_alloc
    void Keep(std::shared_ptr<const void> object, std::weak_ptr<const void> owner, const InternalValueMap* names = nullptr);
    void Clear() noexcept { m_kept.clear(); }
    // Releases what owners that have expired kept, looking at a few entries in turn: a template module's call that
    // returns, so that a closure a call stored and a later one let go of does not keep its copy, and what that holds,
    // until enough others are kept to prune (docs/tasks/0529). A few, so that a module keeping many live closures
    // does not pay for all of them on every call: an expired one goes within a pass over the entries
    void ReleaseExpired() noexcept
    {
        for (std::size_t looked = 0; looked != ReleaseLooksPerCall && !m_kept.empty(); ++looked)
        {
            if (m_cursor >= m_kept.size())
            {
                m_cursor = 0;
            }
            auto& entry = m_kept[m_cursor];
            if (!entry.owner.expired())
            {
                ++m_cursor;
                continue;
            }
            // The order of the entries does not matter: the last one takes its place
            if (&entry != &m_kept.back())
            {
                entry = std::move(m_kept.back());
            }
            m_kept.pop_back();
        }
    }
    // Releases what every owner that has expired kept: before a tree is refused for want of budget (docs/tasks/0539)
    void ReleaseAllExpired() noexcept;
    // The objects held from outside a walk: ModuleScope::LiveRootsFn's. Throws std::bad_alloc
    using LiveRootsFn = void (*)(const void* state, std::vector<const ValueObject*>& roots);
    // Releases the copies of names that only their own owners keep, as if those had expired: the owner of a copy (a
    // ClosureCapture) is kept alive by the closures that hold it, and when the only closures holding it are inside the
    // copy, such as the blocks of a `self` in a list the copy holds, and nothing outside reaches them, nothing else
    // ever lets go of it (an ephemeron cycle, docs/tasks/0542). A template module's outermost call that returns asks
    // this of the copies kept since the last call and of one older copy in turn, a walk of at most WalkNodeLimit
    // objects and WalkItemLimit items from the copy's values: a copy found live is asked again after 1, 2, 4 ... up
    // to 64 calls. `liveRoots` (with `state`), may be null: what the module's names hold, not walked into. What it
    // empties and lets go of is what only the copy reaches, found as the collector's walk finds garbage (an object
    // whose references all come from the walk), so a value that reads it is held from outside and is left as it is.
    // When anything fails, it keeps the copy
    void ReleaseUnreached(LiveRootsFn liveRoots, const void* state) noexcept;
    // Moves to `other` what is still of use. Throws std::bad_alloc, and then keeps what it did not move
    void MoveInto(KeptObjects& other);
    // Notes to the collector running on this thread the values of the kept copies of names: a
    // template module that goes releases them, and a cycle among their containers that a call
    // made has no note left, as it went with that call's collector (docs/tasks/0407). Throws
    // std::bad_alloc
    void NoteNames() const;

private:
    struct Entry
    {
        std::weak_ptr<const void> owner;
        std::shared_ptr<const void> object;
        const InternalValueMap* names;
        // ReleaseUnreached: the call from which on the entry is due, and the log2 of the calls it waits after a check
        // that found its copy live; NeverChecked for an entry whose copy holds no container
        uint32_t checkAt = 0;
        uint8_t backoff = 0;
    };

    // Whether `names` holds a value whose object may hold a closure that keeps the copy's owner: a list or a dict, a
    // namespace, but not a `self`, whose blocks the copy holds keep nothing (docs/tasks/0529)
    [[nodiscard]] static bool MayHoldKeeper(const InternalValueMap& names) noexcept;
    // The items a walk of `object` visits: a list's or dict's size, past WalkItemLimit for one whose size is not known
    // cheaply. Throws std::bad_alloc
    [[nodiscard]] static size_t ItemsOf(const ValueObject& object);
    // The entries that hold one copy, one for each closure record of the frame that keeps it (RenderContext::
    // CaptureClosures), with their owners and the references each has. A frame with more than MaxGroup is kept
    static constexpr size_t MaxGroup = 16;
    struct Group
    {
        std::array<size_t, MaxGroup> entries{};
        std::array<const void*, MaxGroup> owners{};
        std::array<long, MaxGroup> references{};
        size_t size = 0;
    };
    // Whether the copy `names` is held by kept entries only, whose owners are held by closures among what it holds
    // only, none of which is held from outside (m_live, read already): then releases them. Else delays their next
    // check. Throws std::bad_alloc, and then changes nothing
    void CheckCopy(const InternalValueMap* names);
    // CheckCopy's steps: the entries of `names` (false: none, or one to leave as it is), their owners (false: the copy
    // or an owner is held elsewhere), the walk from the copy into m_walk (false: past its limits), whether every
    // reference to the owners comes from a closure in the walk not held from outside, the release, and the delay of a
    // copy found live. Throw std::bad_alloc before Release changes anything
    [[nodiscard]] bool FindGroup(const InternalValueMap* names, Group& group) const;
    [[nodiscard]] bool ReadOwners(Group& group) const;
    [[nodiscard]] bool WalkCopy(const InternalValueMap& names);
    [[nodiscard]] bool OwnersHeldInside(const Group& group) const;
    void Release(const Group& group);
    void Delay(const Group& group) noexcept;

    static constexpr size_t MinPruneAt = 64;
    static constexpr size_t ReleaseLooksPerCall = 4;
    // ReleaseUnreached: the copies kept since the last call it asks about at most, the objects a walk tracks at most,
    // and the most calls a copy found live waits for its next check
    static constexpr size_t YoungChecksPerCall = 16;
    static constexpr size_t WalkNodeLimit = 256;
    static constexpr size_t WalkItemLimit = 4096;
    static constexpr uint8_t MaxBackoff = 6;
    static constexpr uint8_t NeverChecked = 0xFF;
    std::vector<Entry> m_kept;
    size_t m_pruneAt = MinPruneAt;
    // The next entry ReleaseExpired looks at
    size_t m_cursor = 0;
    // ReleaseUnreached's: the calls it ran at, the next entry it looks at, and the copies of names kept since its last
    // call that may hold a keeper (MayHoldKeeper); more are looked at in turn
    uint32_t m_calls = 0;
    size_t m_checkCursor = 0;
    std::array<const InternalValueMap*, YoungChecksPerCall> m_young{};
    size_t m_youngSize = 0;
    // The tables of its walk, made on the first one, and the module's live roots it reads (m_liveRoots)
    std::unique_ptr<WalkTables> m_walk;
    std::vector<const ValueObject*> m_live;
    // The running ReleaseUnreached's: the module's live roots, whether m_live holds them, and whether it runs
    LiveRootsFn m_liveRoots = nullptr;
    const void* m_liveState = nullptr;
    bool m_liveRead = false;
    bool m_checking = false;
};

// The lists, dicts and namespaces a template module's calls made and handed the host, which the
// host let go of while the module lives. Each may hold itself, and then nothing else frees it, as
// a render frees the containers a host's copy shared once it lets go (docs/tasks/0424). Unlike a
// render's, the module's later calls may write it, on another thread, so the host's thread does
// not walk it: the module's next call does, or the module when it goes (docs/tasks/0480). Its own
// lock, so that a host never waits for a call to end
class DroppedExports
{
public:
    // Takes `container` (moves from it), unless the module has gone (Close): false then, and the
    // caller walks it, as nothing writes it any more. When noting it cannot allocate, it is let go of here: a cycle
    // it closes leaks
    bool Hand(InternalValue& container) noexcept;
    // Whether Take may find a container: a call that finds none takes no lock
    [[nodiscard]] bool MayHoldAny() const noexcept { return m_any.load(std::memory_order_acquire); }
    // What the host let go of since the last call
    std::vector<InternalValue> Take() noexcept;
    // The module goes: what is left, and Hand refuses from now on
    std::vector<InternalValue> Close() noexcept;

private:
    std::mutex m_mutex;
    std::vector<InternalValue> m_dropped;
    // Whether m_dropped may hold a container: a call that finds none takes no lock
    std::atomic<bool> m_any{ false };
    bool m_closed = false;
};

// What the calls of a template module share with its collector: its cells, what KeepUntilEnd
// holds, and the objects its own names hold (docs/tasks/0049, 0373, 0450)
struct ModuleScope
{
    // Appends to `roots` the objects the module's names hold. They outlive the call, and so does
    // all they reach: the walk at the end of the call takes them as held from outside without
    // walking into them, so that a call storing into a container of the module walks what it made,
    // not the whole container (docs/tasks/0450). Throws std::bad_alloc
    using LiveRootsFn = void (*)(const void* state, std::vector<const ValueObject*>& roots);

    BorrowedCells& cells;
    KeptObjects& kept;
    LiveRootsFn liveRoots = nullptr;
    const void* state = nullptr;
};

// Frees the reference cycles among the values a render makes (docs/tasks/0110, 0251). Lists,
// dicts and namespaces are shared by reference count, as Python's are, so a container that
// holds itself (`{% set ns.a = ns %}`, `l.append(l)`) keeps itself alive, possibly through
// other objects (a loop object, a cycler, a bound method, a namedtuple); Python's garbage
// collector frees such cycles, and this does the same when the render ends.
//
// A write that stores a value in a container names the receiving container (NoteStore).
// When the render ends, the collector walks the objects reachable from those through
// ValueObject::Traverse, as CPython's collector does through tp_traverse: it counts the
// references each gets from the others, and treats one whose count is higher as held from
// outside (by a variable that still lives, a user callable, a cache), with everything it
// reaches. The rest is garbage: it empties the containers among it (ValueObject::Clear),
// which breaks their cycles, since a cycle can only be closed by a write into one. An object
// whose references Traverse does not show (a std::function capture, a host map) counts as
// held from outside, so the collector may miss a cycle but never empties a live one.
//
// One collector lives for each render, made before anything the render keeps values in so
// that it ends after them. A render started inside another one on the same thread (a user
// callable rendering a template) hands what it noted to the outer one.
class CycleCollector
{
public:
    // Inline: every render makes one, and most note nothing (docs/tasks/0138 counts the
    // instructions of a render of plain text)
    CycleCollector() noexcept
    {
        s_current = this;
    }
    // A collector whose writes to the host's data go to the module's cells, and what KeepUntilEnd
    // holds to its kept objects, which outlive it: a template module's, whose values live across
    // its calls (docs/tasks/0049, 0373)
    explicit CycleCollector(const ModuleScope& module) noexcept
        : m_module(&module)
    {
        s_current = this;
    }
    ~CycleCollector()
    {
        s_current = m_outer;
        if (m_notes)
        {
            Finish();
        }
    }
    CycleCollector(const CycleCollector&) = delete;
    CycleCollector(CycleCollector&&) = delete;
    CycleCollector& operator=(const CycleCollector&) = delete;
    CycleCollector& operator=(CycleCollector&&) = delete;

    // `container` (a list, dict or namespace the template owns) is about to get a value that
    // may refer back to it. Called before the store, so that a note that fails to allocate
    // fails the store and leaves no cycle unnoted (docs/tasks/0283). Does nothing outside a
    // render.
    static void NoteStore(const InternalValue& container);
    // A container lets go of `value` in a call of a template module: a store that replaces it, a
    // pop, a clear. The walk at the call's end starts from it too, as it may hold a cycle that an
    // earlier call made and stored in a value of the module, whose note went with that call's
    // collector (docs/tasks/0460). Called before the container lets go, so that a note that fails
    // to allocate fails the change. Does nothing outside a module's call: a render's notes last
    // as long as its values. Throws std::bad_alloc
    static void NoteDropped(const InternalValue& value)
    {
        if (InModuleCall())
        {
            s_current->NoteDroppedInCall(value);
        }
    }
    // Whether a call of a template module runs on this thread: NoteDropped notes something
    [[nodiscard]] static bool InModuleCall() noexcept { return JINJA2CPP_CELL_UNLIKELY(s_current && s_current->m_module); }
    // The registry of the borrowed cells of the render running on this thread, or null when
    // the render has none yet (docs/tasks/0049)
    [[nodiscard]] static BorrowedCells* CurrentCells() noexcept
    {
        const auto* collector = s_current;
        if (!collector)
        {
            return nullptr;
        }
        if (collector->m_module)
        {
            return &collector->m_module->cells;
        }
        return collector->m_notes ? &collector->m_notes->cells : nullptr;
    }
    // The registry a promotion adds its cell to, made on the first one; null outside a render
    static BorrowedCells* CellsForWrite();
    // CurrentCells and CellsForWrite for an adapter of a global (ValueObject::IsGlobalData): the
    // registry of the outermost render running on this thread whose snapshot of the globals the
    // running one shares (docs/tasks/0324). A global that a render started inside another one
    // writes first gets its cell there, so the outer render sees the write once the inner one has
    // ended, as Python's renders share the globals' objects (docs/tasks/0463); the cell's links
    // detach when that outer render ends. Else the running render's own
    [[nodiscard]] static BorrowedCells* GlobalCells() noexcept;
    static BorrowedCells* GlobalCellsForWrite();
    // Holds `object` until the render running on this thread ends, and releases it before the
    // walk, so that what only it held is freed rather than counted as held from outside: the
    // names a closure keeps of its ended frames (ClosureCapture), which the closure reaches
    // weakly. In a call of a template module, until the module ends, since the closures the
    // call stores in the module's values live as long (docs/tasks/0373). False outside a
    // render. Throws std::bad_alloc
    // Released earlier once `owner` has expired: the copy is of no use without it, and a loop
    // that makes one per iteration stays in constant memory (pruned as Add prunes candidates)
    // `names`: the copy of a frame's names `object` is, or null (KeptObjects::NoteNames)
    static bool KeepUntilEnd(std::shared_ptr<const void> object, std::weak_ptr<const void> owner, const InternalValueMap* names = nullptr);
    // The collector of the render running on this thread, or null
    [[nodiscard]] static CycleCollector* Current() noexcept { return s_current; }
    // Whether the collector of the render running on this thread holds `object` as a candidate of its walk (NoteStore,
    // NoteDropped): one reference that goes when the render ends (KeptObjects::ReleaseUnreached)
    [[nodiscard]] static bool HoldsCandidate(const ValueObject* object)
    {
        const auto* collector = s_current;
        if (!collector || !collector->m_notes)
        {
            return false;
        }
        return collector->m_notes->noted.count(object) != 0;
    }
    // Whether the render noted anything: a store, a container letting go of a value, a frozen cell (docs/tasks/0312)
    [[nodiscard]] bool HasNotes() const noexcept { return m_notes != nullptr; }
    // Counts a tree (a template, what a render of it loaded) against this render's memory budget for as long as
    // something owns it: a render started inside this one that shares its snapshot of the globals can store a value
    // of its own in a global this render reads afterwards (docs/tasks/0324, 0463), a macro or `caller` that runs in
    // that tree, a `self` or loop that names it, a param it views. The host may destroy the nested template once its
    // render has returned, so each such value owns it (RenderContext::TreeOwner) and this render holds no reference:
    // it notes the tree weakly with the bytes it took, and gives them back once the tree has expired
    // (docs/tasks/0521). A tree is noted once. Its bytes count against this render's memory budget from then on,
    // although another render's limits may be running: past it this render fails at its next step (docs/tasks/0492)
    // KeepNewTree is that for a tree a render nested in this one starts to need (its template, one it loads), before
    // it runs or records it: when the tree does not fit in this render's budget, after giving back what the trees that have
    // expired took, it is not noted, this render fails at its next step, and the nested one fails now with
    // ResourceLimitError, so that a nested render with a loop of its own cannot keep more (docs/tasks/0492). `Tree`
    // has TreeBytes(), computed for a tree not noted yet only. Throws std::bad_alloc
    template<typename Tree>
    void KeepNewTree(const std::shared_ptr<Tree>& tree)
    {
        if (!tree || Keeps(tree.get()))
        {
            return;
        }
        const std::uint64_t bytes = tree->TreeBytes();
        if (bytes > detail::RenderMemoryLeftOf(this))
        {
            // The copies of names that closures nothing holds any more kept, which may own trees (docs/tasks/0539)
            ReleaseExpiredKept();
            GiveBackExpired();
        }
        std::uint64_t taken = 0;
        if (!detail::ChargeRenderMemoryOf(this, bytes, &taken))
        {
            detail::ThrowKeptTreeMemory(bytes);
        }
        try
        {
            Insert(tree.get(), tree, taken);
        }
        catch (...)
        {
            detail::ReleaseRenderMemoryOf(this, taken);
            throw;
        }
    }
    // Gives back to the budget what the noted trees took that nothing owns any more, and forgets them
    void GiveBackExpired() noexcept;
    // Releases what KeepUntilEnd holds for owners that have expired (KeptObjects::ReleaseAllExpired)
    void ReleaseExpiredKept() noexcept;
    // The noted trees something still owns, and all the notes, expired ones too: what a test reads to see that a
    // loop of nested renders keeps neither (docs/tasks/0521)
    [[nodiscard]] size_t KeptTreeCount() const noexcept;
    [[nodiscard]] size_t KeptLedgerSize() const noexcept { return m_notes ? m_notes->trees.size() : 0; }
    // This render shares `keeper`'s snapshot of the globals (KeepTree): when it ends, what it kept until then
    // (KeepUntilEnd) goes to `keeper`, as the closures that keep it may be stored in a global the outer render reads
    // afterwards (docs/tasks/0491)
    // Throws std::bad_alloc
    void KeepForOuter(CycleCollector* keeper) { NotesForKeep().keeper = keeper; }
    // What a container the render running on this thread hands the host keeps, so that the host's
    // copy can tell when it goes whether that render has ended: then nothing walks the container
    // any more, and a cycle it closes is freed only by CollectExport (docs/tasks/0424). Expires when
    // the render's walk starts. Empty outside a render and in a template module, whose values its
    // later calls use, on other threads too, and which walks them when it goes. Throws std::bad_alloc
    static std::weak_ptr<const void> ExportToken();
    // Whether the render `token` (ExportToken) is of has ended: what its values hold may be
    // read without a race, as no render writes it any more. False for an empty token
    [[nodiscard]] static bool HasEnded(const std::weak_ptr<const void>& token) noexcept { return token.expired() && IsToken(token); }
    // Whether `token` is one ExportToken made, rather than empty
    [[nodiscard]] static bool IsToken(const std::weak_ptr<const void>& token) noexcept
    {
        const std::weak_ptr<const void> none;
        return token.owner_before(none) || none.owner_before(token);
    }
    // The host let go of its copy of a container that a render which HasEnded made, whose items
    // hold `roots`: a walk from them empties the garbage among what they reach, the container's
    // own cycle when the template's copy is held by nothing but its items (docs/tasks/0424). One
    // such walk runs at a time, after the walk of the render that made the container
    static void CollectExport(std::vector<ValueObjectPtr> roots) noexcept;
    // Makes `collector`, a render's that is still running further down this thread's stack, the
    // current one for the scope: code of that render called back from inside another one (a
    // template callable lent to a host's call that called a module's macro, docs/tasks/0300)
    // notes its writes where that render's do
    class Reentry
    {
    public:
        explicit Reentry(CycleCollector* collector) noexcept
            : m_saved(std::exchange(s_current, collector))
        {
        }
        Reentry(const Reentry&) = delete;
        Reentry(Reentry&&) = delete;
        Reentry& operator=(const Reentry&) = delete;
        Reentry& operator=(Reentry&&) = delete;
        ~Reentry() { s_current = m_saved; }

    private:
        CycleCollector* m_saved;
    };
    // A frozen cell that values outliving the render still read: a candidate of this collector
    // (BorrowedCells::Release, docs/tasks/0331). Throws std::bad_alloc
    void NoteFrozen(ValueObjectPtr cell) { Add(std::move(cell)); }
    // A frozen cell a template module let go of: the walk starts from it although nothing else
    // holds it, as what it holds may be a cycle that an earlier call made, whose note went with
    // that call's collector (BorrowedCells::FreezeCallScoped). Throws std::bad_alloc
    void NoteRoot(ValueObjectPtr cell);
    // Walks from `roots` now, apart from any render's notes, and empties the garbage among what
    // they reach: what the cells a template module's call drops held (BorrowedCells::DropUnheld)
    static void CollectFrom(std::vector<ValueObjectPtr> roots) noexcept;
    // The same, keeping in `roots` the ones that are not garbage: the call-scoped cells a template
    // module keeps, whose cycles no later note may name (BorrowedCells::FreezeCallScoped)
    static void CollectKeeping(std::vector<ValueObjectPtr>& roots) noexcept;
    // The walk starts from every note, also one that nothing else holds any more: a template
    // module's values when it goes, where such a value may hold a cycle that a call made and
    // whose note went with that call's collector (docs/tasks/0212)
    void WalkFromEveryNote() noexcept
    {
        if (m_notes)
        {
            m_notes->walkFromEvery = true;
        }
    }

private:
    // A suspendable render has a collector of its own, apart from the renders it interleaves with
    // (detail::RenderThreadState, docs/tasks/0457)
    friend struct detail::RenderThreadState;

    void Finish() noexcept;
    // Whether `tree` is noted and something owns it still. A note of an expired tree gives back what it took, once
    // (an address may be reused by a new tree: Insert then takes the note)
    [[nodiscard]] bool Keeps(const void* tree) noexcept;
    // What KeepTree notes of a tree (docs/tasks/0521)
    struct KeptTree
    {
        std::weak_ptr<const void> tree;
        const void* address = nullptr;
        // What it took of the budget of the render that notes it, given back with it (docs/tasks/0492)
        std::uint64_t bytes = 0;
    };
    // Notes `tree`, at `address`, with the bytes it took; null when it was noted already. The note, whose bytes the
    // caller may set. Throws std::bad_alloc, and then notes nothing
    KeptTree* Insert(const void* address, std::weak_ptr<const void> tree, std::uint64_t bytes);
    // Notes what a render that ends handed over (Finish), charged here. Throws std::bad_alloc, and then notes nothing
    void KeepTree(const KeptTree& kept);
    // Gives back `bytes` a note took
    void GiveBack(std::uint64_t bytes) noexcept;
    // The collector of the render whose registry GlobalCells finds, or null for the running one's
    [[nodiscard]] static CycleCollector* SnapshotOwner() noexcept;
    void Add(ValueObjectPtr object);
    void NoteDroppedInCall(const InternalValue& value);
    // Drops the candidates nothing else holds; false when there were none
    bool DropUnshared();
    // The walk at the end of the render. It allocates nothing that can fail but the module's live
    // roots (ModuleScope::LiveRootsFn): when those cannot be made the walk is skipped and the cycles
    // not broken leak (docs/tasks/0283)
    void Collect() noexcept;

    // The collector of the render running on this thread, or null
    static inline thread_local CycleCollector* s_current = nullptr;

    // What a render noted, made on the first note: most renders store no container in another
    struct Notes
    {
        // Notes of trees and candidates held by nothing else are dropped when there are this many (Insert, Add)
        static constexpr size_t MinPruneAt = 64;
        // KeepTree's: noted weakly, as what owns a tree is the values that name it (docs/tasks/0521)
        std::vector<KeptTree> trees;
        // The index in `trees` of each address noted, expired or not
        std::unordered_map<const void*, size_t> treeIndex;
        // The expired notes are given back when there are this many (Insert)
        size_t treePruneAt = MinPruneAt;
        // The receiving containers, held until the render ends
        std::vector<ValueObjectPtr> candidates;
        std::unordered_set<const void*> noted;
        // Candidates held by nothing else are dropped when there are this many (Add)
        size_t pruneAt = MinPruneAt;
        // WalkFromEveryNote: nothing is dropped before the walk
        bool walkFromEvery = false;
        // NoteRoot: walked, never dropped before the walk
        std::vector<ValueObjectPtr> roots;
        // The tables of the walk, reserved as candidates and roots are noted (WalkTables)
        WalkTables walk;
        // Room in `walk` for one more candidate or root. Throws std::bad_alloc
        void ReserveWalk()
        {
            const size_t nodes = candidates.size() + roots.size() + 1;
            if (nodes > walk.Capacity())
            {
                walk.Reserve(nodes);
            }
        }
        // What KeepUntilEnd holds
        KeptObjects kept;
        // The render's copies of the host's data, released before the walk: detaching them
        // frees what only they held
        BorrowedCells cells;
        // ExportToken: made on the first container the render hands the host
        std::shared_ptr<const void> exportToken;
        // KeepForOuter's; null when this render shares no outer render's globals
        CycleCollector* keeper = nullptr;
    };

    // The notes, made now if there are none. Throws std::bad_alloc
    Notes& NotesForKeep();

    CycleCollector* m_outer = s_current;
    std::unique_ptr<Notes> m_notes;
    // The cells and kept objects of a template module, which outlive the call (docs/tasks/0049,
    // 0373); null for a render
    const ModuleScope* m_module = nullptr;
};

// A call of a template module: it runs as a new call of the module's cells (BorrowedCells::BeginCall),
// and the call it nested in, made from a host callable, runs again when it ends. Apart from
// CycleCollector, so that a render's collector stays as small as it was (docs/tasks/0138)
class ModuleCallCollector
{
public:
    // `liveRoots` and `state`: ModuleScope's
    ModuleCallCollector(BorrowedCells& cells, KeptObjects& kept, ModuleScope::LiveRootsFn liveRoots = nullptr, const void* state = nullptr) noexcept
        : m_scope{ cells, kept, liveRoots, state }
        , m_outerCall(cells.BeginCall())
        , m_collector(m_scope)
    {
    }
    ~ModuleCallCollector()
    {
        // Last in the call: what the call made is gone, and with it the adapters of its data. The
        // values the call handed out keep what they read now, before a cell of theirs is dropped
        // (Complete did that already, unless the call failed)
        m_scope.cells.PinExportsOrSkip();
        // The cells of borrowed JSON values live for the outermost call (docs/tasks/0212)
        if (m_outerCall.call == 0)
        {
            m_scope.cells.FreezeCallScoped(&m_collector);
        }
        m_scope.cells.DropUnheld(true);
        m_scope.cells.EndCall(std::move(m_outerCall));
    }
    // The call succeeded: its values keep what they read now, or the call fails with
    // std::bad_alloc rather than hand out values that read the host's data
    void Complete() { m_scope.cells.PinExports(); }

    ModuleCallCollector(const ModuleCallCollector&) = delete;
    ModuleCallCollector(ModuleCallCollector&&) = delete;
    ModuleCallCollector& operator=(const ModuleCallCollector&) = delete;
    ModuleCallCollector& operator=(ModuleCallCollector&&) = delete;

private:
    ModuleScope m_scope;
    BorrowedCells::CallFrame m_outerCall;
    CycleCollector m_collector;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_CYCLE_COLLECTOR_H
