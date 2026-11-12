#ifndef JINJA2CPP_SRC_CYCLE_COLLECTOR_H
#define JINJA2CPP_SRC_CYCLE_COLLECTOR_H

#include "borrowed_cells.h"
#include "internal_value.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
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

// What CycleCollector::KeepUntilEnd holds, each object with the owner it is of use to: released
// once its owner has expired, or all at once
class KeptObjects
{
public:
    // `names`: the copy of a frame's names `object` is, or null. Throws std::bad_alloc
    void Keep(std::shared_ptr<const void> object, std::weak_ptr<const void> owner, const InternalValueMap* names = nullptr);
    void Clear() noexcept { m_kept.clear(); }
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
    };

    static constexpr size_t MinPruneAt = 64;
    std::vector<Entry> m_kept;
    size_t m_pruneAt = MinPruneAt;
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
    // The collector of the render whose registry GlobalCells finds, or null for the running one's
    [[nodiscard]] static CycleCollector* SnapshotOwner() noexcept;
    void Add(ValueObjectPtr object);
    void NoteDroppedInCall(const InternalValue& value);
    // Drops the candidates nothing else holds; false when there were none
    bool DropUnshared();
    void Collect() noexcept;
    // One walk; throws std::bad_alloc when its tables cannot be allocated
    void CollectOnce();

    // The collector of the render running on this thread, or null
    static inline thread_local CycleCollector* s_current = nullptr;

    // What a render noted, made on the first note: most renders store no container in another
    struct Notes
    {
        // The receiving containers, held until the render ends
        std::vector<ValueObjectPtr> candidates;
        std::unordered_set<const void*> noted;
        // Candidates held by nothing else are dropped when there are this many (Add)
        static constexpr size_t MinPruneAt = 64;
        size_t pruneAt = MinPruneAt;
        // WalkFromEveryNote: nothing is dropped before the walk
        bool walkFromEvery = false;
        // NoteRoot: walked, never dropped before the walk
        std::vector<ValueObjectPtr> roots;
        // What KeepUntilEnd holds
        KeptObjects kept;
        // The render's copies of the host's data, released before the walk: detaching them
        // frees what only they held
        BorrowedCells cells;
        // ExportToken: made on the first container the render hands the host
        std::shared_ptr<const void> exportToken;
    };

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
