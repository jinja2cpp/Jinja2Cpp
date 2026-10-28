#include "cell_trace.h"

#ifdef JINJA2CPP_CELL_TRACE

#include "borrowed_cells.h"
#include "cycle_collector.h"

#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <mutex>
#include <unordered_map>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
namespace
{
class TraceLog
{
public:
    TraceLog()
    {
        const char* path = std::getenv("JINJA2CPP_CELL_TRACE"); // NOLINT(concurrency-mt-unsafe): read once, before any event
        if (path && *path)
        {
            m_file = std::fopen(path, "w"); // NOLINT(cppcoreguidelines-owning-memory)
        }
    }

    [[nodiscard]] bool On() const noexcept { return m_file != nullptr; }

    void Write(const char* kind, std::initializer_list<CellTrace::Field> fields) noexcept
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        int64_t values[16] = {};
        size_t at = 0;
        for (const auto& field : fields)
        {
            const auto value = Resolve(field);
            if (value < 0 && (field.space == CellTrace::Space::GoneCell || field.space == CellTrace::Space::Cell))
            {
                // A cell Adopt made and dropped, never in the log
                return;
            }
            values[at++] = value;
        }
        std::fprintf(m_file, "{\"ev\":\"%s\",\"t\":%d,\"cur\":%lld", kind, Thread(), static_cast<long long>(Lookup(m_regs, CycleCollector::CurrentCells())));
        at = 0;
        for (const auto& field : fields)
        {
            if (field.space == CellTrace::Space::Links)
            {
                std::fprintf(m_file, ",\"%s\":[", field.name);
                for (size_t i = 0; i < field.count; ++i)
                {
                    std::fprintf(m_file, "%s%lld", i == 0 ? "" : ",", static_cast<long long>(Number(m_links, m_nextLink, field.list[i])));
                }
                std::fputs("]", m_file);
                ++at;
                continue;
            }
            std::fprintf(m_file, ",\"%s\":%lld", field.name, static_cast<long long>(values[at++]));
        }
        std::fputs("}\n", m_file);
        std::fflush(m_file);
    }

private:
    using Numbers = std::unordered_map<const void*, int64_t>;

    // 0 for null; -1 for a cell the log does not know
    int64_t Resolve(const CellTrace::Field& field)
    {
        switch (field.space)
        {
        case CellTrace::Space::Num:
            return field.num;
        case CellTrace::Space::Cell:
            return field.ptr ? (m_cells.count(field.ptr) ? m_cells[field.ptr] : -1) : 0;
        case CellTrace::Space::Link:
            return Number(m_links, m_nextLink, field.ptr);
        case CellTrace::Space::Reg:
            return Lookup(m_regs, field.ptr);
        case CellTrace::Space::Idn:
            return Number(m_idns, m_nextIdn, field.ptr);
        case CellTrace::Space::Gen:
            return Number(m_gens, m_nextGen, field.ptr);
        case CellTrace::Space::NewCell:
            return Fresh(m_cells, m_nextCell, field.ptr);
        case CellTrace::Space::NewReg:
            return Fresh(m_regs, m_nextReg, field.ptr);
        case CellTrace::Space::NewGen:
            return Fresh(m_gens, m_nextGen, field.ptr);
        case CellTrace::Space::GoneCell:
            return Forget(m_cells, field.ptr);
        case CellTrace::Space::GoneReg:
            return Forget(m_regs, field.ptr);
        case CellTrace::Space::Links:
            return 0;
        }
        return 0;
    }

    static int64_t Number(Numbers& numbers, int64_t& next, const void* ptr)
    {
        if (!ptr)
        {
            return 0;
        }
        auto [entry, isNew] = numbers.try_emplace(ptr, next + 1);
        if (isNew)
        {
            ++next;
        }
        return entry->second;
    }
    static int64_t Fresh(Numbers& numbers, int64_t& next, const void* ptr)
    {
        numbers[ptr] = ++next;
        return next;
    }
    static int64_t Lookup(const Numbers& numbers, const void* ptr)
    {
        const auto found = ptr ? numbers.find(ptr) : numbers.end();
        return found != numbers.end() ? found->second : 0;
    }
    static int64_t Forget(Numbers& numbers, const void* ptr)
    {
        const auto found = numbers.find(ptr);
        if (found == numbers.end())
        {
            return -1;
        }
        const auto number = found->second;
        numbers.erase(found);
        return number;
    }
    static int Thread() noexcept
    {
        static std::atomic<int> s_next{ 0 };
        static thread_local const int s_thread = ++s_next;
        return s_thread;
    }

    std::mutex m_mutex;
    std::FILE* m_file = nullptr;
    Numbers m_cells;
    Numbers m_links;
    Numbers m_regs;
    Numbers m_idns;
    Numbers m_gens;
    int64_t m_nextCell = 0;
    int64_t m_nextLink = 0;
    int64_t m_nextReg = 0;
    int64_t m_nextIdn = 0;
    int64_t m_nextGen = 0;
};

// Never destroyed: the registries of the threads' globals snapshots and of static templates go
// after main returns, and log their Finish
TraceLog& Log()
{
    static auto* const s_log = new TraceLog(); // NOLINT(cppcoreguidelines-owning-memory)
    return *s_log;
}
} // namespace

bool CellTrace::On() noexcept
{
    static const bool s_on = Log().On();
    return s_on;
}

void CellTrace::Event(const char* kind, std::initializer_list<Field> fields) noexcept
{
    Log().Write(kind, fields);
}

bool CellTrace::Skip(const CellLink* link, uint32_t epoch) noexcept
{
    if (On())
    {
        Event("skip", { Ptr("a", Space::Link, link), Num("e", epoch) });
    }
    return false;
}

} // namespace detail
} // namespace v2
} // namespace jinja2

#endif
