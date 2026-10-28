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
    // A collector whose writes to the host's data go to `cells`, which outlive it: a template
    // module's, whose values live across its calls (docs/tasks/0049)
    explicit CycleCollector(BorrowedCells& cells) noexcept
        : m_external(&cells)
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
    // The registry of the borrowed cells of the render running on this thread, or null when
    // the render has none yet (docs/tasks/0049)
    [[nodiscard]] static BorrowedCells* CurrentCells() noexcept
    {
        const auto* collector = s_current;
        if (!collector)
        {
            return nullptr;
        }
        if (collector->m_external)
        {
            return collector->m_external;
        }
        return collector->m_notes ? &collector->m_notes->cells : nullptr;
    }
    // The registry a promotion adds its cell to, made on the first one; null outside a render
    static BorrowedCells* CellsForWrite();
    // A frozen cell that values outliving the render still read: a candidate of this collector
    // (BorrowedCells::Release, docs/tasks/0331). Throws std::bad_alloc
    void NoteFrozen(ValueObjectPtr cell) { Add(std::move(cell)); }

private:
    void Finish() noexcept;
    void Add(ValueObjectPtr object);
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
        // The render's copies of the host's data, released before the walk: detaching them
        // frees what only they held
        BorrowedCells cells;
    };

    CycleCollector* m_outer = s_current;
    std::unique_ptr<Notes> m_notes;
    // The cells of a template module, which outlive the call (docs/tasks/0049); null for a render
    BorrowedCells* m_external = nullptr;
};

// A call of a template module: it runs as a new call of the module's cells (BorrowedCells::BeginCall),
// and the call it nested in, made from a host callable, runs again when it ends. Apart from
// CycleCollector, so that a render's collector stays as small as it was (docs/tasks/0138)
class ModuleCallCollector
{
public:
    explicit ModuleCallCollector(BorrowedCells& cells) noexcept
        : m_cells(cells)
        , m_outerCall(cells.BeginCall())
        , m_collector(cells)
    {
    }
    ~ModuleCallCollector()
    {
        // Last in the call: what the call made is gone, and with it the adapters of its data
        m_cells.DropUnheld();
        m_cells.EndCall(m_outerCall);
    }
    ModuleCallCollector(const ModuleCallCollector&) = delete;
    ModuleCallCollector(ModuleCallCollector&&) = delete;
    ModuleCallCollector& operator=(const ModuleCallCollector&) = delete;
    ModuleCallCollector& operator=(ModuleCallCollector&&) = delete;

private:
    BorrowedCells& m_cells;
    uint32_t m_outerCall;
    CycleCollector m_collector;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_CYCLE_COLLECTOR_H
