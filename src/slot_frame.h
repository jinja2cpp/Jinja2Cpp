#ifndef JINJA2CPP_SRC_SLOT_FRAME_H
#define JINJA2CPP_SRC_SLOT_FRAME_H

#include "internal_value.h"

#include <boost/core/span.hpp>

#include <cstddef>
#include <cstdint>
#include <utility>

// The slots of names resolved at Load (docs/design/0117-name-slots-plan.md, phase P1).
// A unit (a template body, a macro, a call or a block body) gets one frame of slots per
// call, taken from the thread's RenderWorkspace; a name bound in the unit and read there
// is read from its slot by index, every other name keeps the lookup by name. These types
// never depend on JINJA2CPP_CHECK_SLOTS, so that every translation unit agrees on them.

namespace jinja2
{
// A name's slot in the frame of its unit; Dynamic for a name looked up by name
struct SlotIndex
{
    static constexpr std::uint16_t Dynamic = 0xFFFF;
    std::uint16_t value = Dynamic;

    [[nodiscard]] bool IsDynamic() const { return value == Dynamic; }
    bool operator==(const SlotIndex& other) const { return value == other.value; }
    bool operator!=(const SlotIndex& other) const { return value != other.value; }
};

// A unit of a template, numbered at Load; None outside any
struct UnitId
{
    static constexpr std::uint16_t None = 0xFFFF;
    std::uint16_t value = None;

    bool operator==(const UnitId& other) const { return value == other.value; }
    bool operator!=(const UnitId& other) const { return value != other.value; }
};

// What a unit needs from the workspace for each call
struct UnitLayout
{
    UnitId id;
    std::uint16_t size = 0;
};

// The name of a slot, for the lookups by name that reach it: views into the parse tree's
// strings, which outlive every render
using SlotName = HashedName;

// A value a unit binds by index. Unbound until the statement that binds it runs, so that a
// lookup by name passes over it; same size as an InternalValue (the flag is in its padding)
class Slot : public InternalValue
{
public:
    Slot() { m_isUnbound = true; }
    Slot(const Slot&) = delete;
    Slot(Slot&&) = delete;
    Slot& operator=(const Slot&) = delete;
    Slot& operator=(Slot&&) = delete;
    ~Slot() = default;

    [[nodiscard]] bool IsBound() const { return !m_isUnbound; }
    template<typename Value>
    void Bind(Value&& value)
    {
        static_cast<InternalValue&>(*this) = std::forward<Value>(value);
        m_isUnbound = false;
    }
    // Lets go of the value, so that what it held is freed with the scope it was bound in
    void Unbind()
    {
        static_cast<InternalValue&>(*this) = InternalValue();
        m_isUnbound = true;
    }
};
static_assert(sizeof(Slot) == sizeof(InternalValue));

// A frame named from outside the statement that uses it (the loop filter, which runs while
// the frame may have moved up the workspace's stack). Resolving it checks the generation, so
// a handle to a frame given back fails loudly instead of reading another call's slots
struct FrameHandle
{
    std::uint32_t depth = 0;
    std::uint32_t generation = 0;

    bool operator==(const FrameHandle& other) const { return depth == other.depth && generation == other.generation; }
    bool operator!=(const FrameHandle& other) const { return !(*this == other); }
};

// The slots of one unit call: a view, valid only inside the call that took it
struct SlotFrame
{
    boost::span<Slot> slots;
    FrameHandle handle;
    UnitId unit;
};

// Some slots of a frame and their names, as a lookup by name sees them (a loop's `loop`
// and targets): a view, valid only inside the statement that made it
struct FrameView
{
    boost::span<Slot> slots;
    boost::span<const SlotName> names;

    bool operator==(const FrameView& other) const
    {
        return slots.data() == other.slots.data() && slots.size() == other.slots.size() && names.data() == other.names.data() && names.size() == other.names.size();
    }
};

// A view attached to a scope of a RenderContext: a lookup by name searches it right after
// the scope's map, so names set in the scope hide it and it hides the scopes below
struct ScopeView
{
    FrameView view;
    // The scope's index among the context's own scopes
    std::size_t scopeIndex = 0;

    bool operator==(const ScopeView& other) const { return view == other.view && scopeIndex == other.scopeIndex; }
    bool operator!=(const ScopeView& other) const { return !(*this == other); }
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_SLOT_FRAME_H
