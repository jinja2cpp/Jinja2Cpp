#ifndef JINJA2CPP_SRC_TEMPLATE_SLOTS_H
#define JINJA2CPP_SRC_TEMPLATE_SLOTS_H

#include "node_arena.h"

#include <boost/container/small_vector.hpp>

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>
#include <thread>
#include <unordered_map>

namespace jinja2
{

// A template one render runs, by its place in the render's table of templates (0118 P4b).
// The render keeps every template it loads until it ends, so a handle needs no reference
// count; the generation tells the table's own handles from those of most other renders
class TemplateHandle
{
public:
    // A render may reload a template on every use (TemplateLookup::EveryUse without a cache)
    static constexpr std::uint32_t MaxSlots = std::uint32_t{ 1 } << 24;

    TemplateHandle() = default;
    TemplateHandle(std::uint32_t slot, std::uint8_t generation)
        : m_value((slot << 8) | generation)
    {
        assert(slot < MaxSlots);
    }

    [[nodiscard]] std::uint32_t Slot() const { return m_value >> 8; }
    [[nodiscard]] std::uint8_t Generation() const { return static_cast<std::uint8_t>(m_value & 0xffU); }

private:
    std::uint32_t m_value = 0;
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

// The trees of the templates one render runs, in the order their blocks or parents are
// first needed
class TemplateSlots
{
public:
    TemplateSlots() = default;
    TemplateSlots(const TemplateSlots&) = delete;
    TemplateSlots& operator=(const TemplateSlots&) = delete;
    TemplateSlots(TemplateSlots&&) = delete;
    TemplateSlots& operator=(TemplateSlots&&) = delete;
    ~TemplateSlots() = default;

    // The handle of `nodes`, which takes a slot the first time
    TemplateHandle Add(const ArenaView& nodes)
    {
        if (m_slots.empty())
        {
            // Taken by the first handle, so that a render that makes none does not pay for it
            m_generation = NextGeneration();
        }
        // A render runs a few templates: a scan is shorter than a hash until it runs many
        if (m_bySlot.empty())
        {
            for (std::size_t slot = 0; slot != m_slots.size(); ++slot)
            {
                if (m_slots[slot] == nodes)
                {
                    return { static_cast<std::uint32_t>(slot), m_generation };
                }
            }
            if (m_slots.size() < ScanLimit)
            {
                return Append(nodes);
            }
            for (std::size_t slot = 0; slot != m_slots.size(); ++slot)
            {
                m_bySlot.emplace(m_slots[slot].Id(), static_cast<std::uint32_t>(slot));
            }
        }
        const auto [p, added] = m_bySlot.emplace(nodes.Id(), static_cast<std::uint32_t>(m_slots.size()));
        if (!added)
        {
            return { p->second, m_generation };
        }
        return Append(nodes);
    }

    // Throws for a handle another render made: its template may be gone. No handle has
    // generation 0, the one of a table that has made none
    [[nodiscard]] const ArenaView& operator[](TemplateHandle handle) const
    {
        if (handle.Generation() != m_generation || handle.Slot() >= m_slots.size())
        {
            throw std::logic_error("a template handle used outside the render that made it");
        }
        return m_slots[handle.Slot()];
    }

private:
    static constexpr std::size_t ScanLimit = 8;

    TemplateHandle Append(const ArenaView& nodes)
    {
        if (m_slots.size() == TemplateHandle::MaxSlots)
        {
            throw std::length_error("too many templates in one render");
        }
        m_slots.push_back(nodes);
        return { static_cast<std::uint32_t>(m_slots.size() - 1), m_generation };
    }

    // Per thread: a table never leaves the thread rendering with it, and a shared counter
    // would make every core rendering fetch its cache line again
    static std::uint8_t NextGeneration()
    {
        thread_local auto next = static_cast<std::uint8_t>(std::hash<std::thread::id>()(std::this_thread::get_id()));
        // 0 is never one, so that a default handle resolves nowhere
        if (++next == 0)
        {
            ++next;
        }
        return next;
    }

    boost::container::small_vector<ArenaView, 4> m_slots;
    // The slots by tree, once there are more than a scan should look through
    std::unordered_map<const void*, std::uint32_t> m_bySlot;
    std::uint8_t m_generation = 0;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_TEMPLATE_SLOTS_H
