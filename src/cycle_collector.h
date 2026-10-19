#ifndef JINJA2CPP_SRC_CYCLE_COLLECTOR_H
#define JINJA2CPP_SRC_CYCLE_COLLECTOR_H

#include "internal_value.h"

#include <cstddef>
#include <memory>
#include <unordered_set>
#include <vector>

namespace jinja2
{
inline namespace v2
{

// Frees the reference cycles among the lists, dicts and namespaces a render builds
// (docs/tasks/0110). They are shared by reference count, as Python's are, so a container
// that holds itself (`{% set ns.a = ns %}`, `l.append(l)`) keeps itself alive; Python's
// garbage collector frees such cycles, and this does the same when the render ends.
//
// A write that stores a container in another one names the receiving container
// (NoteStore). When the render ends, the collector walks the containers reachable from
// those, counts the references each gets from the others, and treats one whose count is
// higher as held from outside (by a variable that still lives, a user callable, a cache),
// with everything it reaches. It empties the rest, which breaks their cycles. A container
// the walk cannot follow (one whose items another accessor shares, a bound method) counts
// as held from outside, so the collector may miss a cycle but never empties a live one.
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

    // `container` (a list, dict or namespace the template owns) got a value that may refer
    // back to it. Does nothing outside a render.
    static void NoteStore(const InternalValue& container);

private:
    void Finish() noexcept;
    void Add(const InternalValue& container);
    // Drops the candidates nothing else holds; false when there were none
    bool DropUnshared();
    void Collect() noexcept;

    // The collector of the render running on this thread, or null
    static inline thread_local CycleCollector* s_current = nullptr;

    // What a render noted, made on the first note: most renders store no container in another
    struct Notes
    {
        // The receiving containers, held until the render ends
        std::vector<InternalValue> candidates;
        std::unordered_set<const void*> noted;
        // Candidates held by nothing else are dropped when there are this many (Add)
        static constexpr size_t MinPruneAt = 64;
        size_t pruneAt = MinPruneAt;
    };

    CycleCollector* m_outer = s_current;
    std::unique_ptr<Notes> m_notes;
};

} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_CYCLE_COLLECTOR_H
