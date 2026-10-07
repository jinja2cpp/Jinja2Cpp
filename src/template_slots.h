#ifndef JINJA2CPP_SRC_TEMPLATE_SLOTS_H
#define JINJA2CPP_SRC_TEMPLATE_SLOTS_H

#include "node_arena.h"

#include <boost/container/small_vector.hpp>

#include <cstdint>
#include <functional>
#include <limits>
#include <stdexcept>
#include <thread>

namespace jinja2
{

// A template one render runs, by its place in the render's table of templates (0118 P4b).
// The render keeps every template it loads until it ends, so a handle needs no reference
// count; the generation tells the table's own handles from those of another render
struct TemplateHandle
{
    std::uint16_t slot = 0;
    std::uint16_t generation = 0;
};

// A node of a template the render runs, maybe another one than the template running now:
// a block on the inheritance stack, the parent an `extends` names
template<typename T>
struct TemplateNode
{
    TemplateHandle tpl;
    NodeRef<T> node;

    explicit operator bool() const { return static_cast<bool>(node); }
};

// The trees of the templates one render runs: the rendered template, then the ones it
// includes, extends or imports, in the order their blocks or parents are first needed
class TemplateSlots
{
public:
    TemplateSlots() = default;
    TemplateSlots(const TemplateSlots&) = delete;
    TemplateSlots& operator=(const TemplateSlots&) = delete;
    TemplateSlots(TemplateSlots&&) = delete;
    TemplateSlots& operator=(TemplateSlots&&) = delete;
    ~TemplateSlots() = default;

    // The handle of `nodes`, which takes a slot the first time. A render runs a few
    // templates, so the newest-first scan is shorter than a hash
    TemplateHandle Add(const ArenaView& nodes)
    {
        for (auto slot = m_slots.size(); slot != 0; --slot)
        {
            if (m_slots[slot - 1] == nodes)
            {
                return { static_cast<std::uint16_t>(slot - 1), m_generation };
            }
        }
        if (m_slots.size() > std::numeric_limits<std::uint16_t>::max())
        {
            throw std::length_error("too many templates in one render");
        }
        if (m_slots.empty())
        {
            // Taken by the first handle, so that a render that makes none does not pay for it
            m_generation = NextGeneration();
        }
        m_slots.push_back(nodes);
        return { static_cast<std::uint16_t>(m_slots.size() - 1), m_generation };
    }

    // Throws for a handle another render made: its template may be gone. No handle has
    // generation 0, the one of a table that has made none
    [[nodiscard]] const ArenaView& operator[](TemplateHandle handle) const
    {
        if (handle.generation != m_generation || handle.slot >= m_slots.size())
        {
            throw std::logic_error("a template handle used outside the render that made it");
        }
        return m_slots[handle.slot];
    }

private:
    // Per thread: a table never leaves the thread rendering with it, and a shared counter
    // would make every core rendering fetch its cache line again
    static std::uint16_t NextGeneration()
    {
        thread_local auto next = static_cast<std::uint16_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
        // 0 is never one, so that a default handle resolves nowhere
        if (++next == 0)
        {
            ++next;
        }
        return next;
    }

    boost::container::small_vector<ArenaView, 4> m_slots;
    std::uint16_t m_generation = 0;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_SLOTS_H
