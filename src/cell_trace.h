#ifndef JINJA2CPP_SRC_CELL_TRACE_H
#define JINJA2CPP_SRC_CELL_TRACE_H

// The events of the borrowed-cell protocol (docs/tasks/0049, 0331), logged for tla/CellTrace.tla
// to check against the protocol's rules (docs/tasks/0338, trace validation). Only in a build
// configured with JINJA2CPP_CELL_TRACE=ON, and there only while the environment variable
// JINJA2CPP_CELL_TRACE names the file to write: any other build compiles every hook to nothing.
//
// One JSON object a line, in the order the events happened: a global mutex orders them. The
// objects are named by small numbers, not addresses: a cell, a storage generation and a registry
// get a new number when they are made, so an address used again is another object; a link (an
// adapter) and an identity keep their number for the address

#ifdef JINJA2CPP_CELL_TRACE

#include <cstddef>
#include <cstdint>
#include <initializer_list>

namespace jinja2
{
inline namespace v2
{
class BorrowedCells;
class CellLink;

namespace detail
{
class CellTrace
{
public:
    // What a field names: a number as it is, or an object, by the number of its address
    enum class Space : uint8_t
    {
        Num,
        Cell,
        Link,
        Reg,
        Idn,
        Gen,
        // The object is made now: it gets a new number
        NewCell,
        NewReg,
        NewGen,
        // The object goes: its address is free for the next one
        GoneCell,
        GoneReg,
        // A list of links
        Links
    };
    struct Field
    {
        const char* name;
        Space space;
        int64_t num = 0;
        const void* ptr = nullptr;
        const CellLink* const* list = nullptr;
        size_t count = 0;
    };

    // Whether events are written: JINJA2CPP_CELL_TRACE names a file
    static bool On() noexcept;
    // Writes one event with the running thread ("t") and the registry of the render or module
    // call running on it ("cur", CycleCollector::CurrentCells) added. An event about a cell the
    // log never named (one Adopt made and dropped) is left out
    static void Event(const char* kind, std::initializer_list<Field> fields) noexcept;
    // A read that skipped the lookup: the adapter saw this epoch (CellLink::MayHaveCell). False,
    // so that the caller returns it
    static bool Skip(const CellLink* link, uint32_t epoch) noexcept;

    static Field Num(const char* name, int64_t value) noexcept { return { name, Space::Num, value }; }
    static Field Ptr(const char* name, Space space, const void* ptr) noexcept { return { name, space, 0, ptr }; }
    static Field Links(const char* name, const CellLink* const* list, size_t count) noexcept { return { name, Space::Links, 0, nullptr, list, count }; }
};
} // namespace detail
} // namespace v2
} // namespace jinja2

#define JINJA2CPP_CELL_TRACE_EVENT(kind, ...) \
    do \
    { \
        if (::jinja2::detail::CellTrace::On()) \
        { \
            using Trace = ::jinja2::detail::CellTrace; \
            ::jinja2::detail::CellTrace::Event(kind, { __VA_ARGS__ }); \
        } \
    } while (false)

#else

#define JINJA2CPP_CELL_TRACE_EVENT(kind, ...) \
    do \
    { \
    } while (false)

#endif

#endif // JINJA2CPP_SRC_CELL_TRACE_H
