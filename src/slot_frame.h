#ifndef JINJA2CPP_SRC_SLOT_FRAME_H
#define JINJA2CPP_SRC_SLOT_FRAME_H

#include "internal_value.h"
#include "node_arena.h"
#include "span.h"

#include <cstddef>
#include <cstdint>
#include <utility>
#include <variant>

// The slots of names resolved at Load (docs/design/0117-name-slots-plan.md, phase P1).
// A unit (a template body, a macro, a call or a block body) gets one frame of slots per
// call, taken from the thread's RenderWorkspace; a name bound in the unit and read there
// is read from its slot by index, every other name keeps the lookup by name. These types
// never depend on JINJA2CPP_CHECK_SLOTS, so that every translation unit agrees on them.

// Debug builds check every slot read against a lookup by name; the sanitizer and fuzz
// jobs turn it on with -DJINJA2CPP_CHECK_SLOTS=ON
#if !defined(NDEBUG) && !defined(JINJA2CPP_CHECK_SLOTS)
#define JINJA2CPP_CHECK_SLOTS
#endif

namespace jinja2
{
inline namespace v2
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

// The name of a slot, for the lookups by name that reach it: a text of the parse tree that
// binds it, read through that tree's view
struct SlotName
{
    ArenaText name;
    std::size_t hash = 0;
};

// A value a unit binds by index. Unbound until the statement that binds it runs, so that a
// lookup by name passes over it; same size as an InternalValue (an unbound slot holds an
// undefined value marked unbound)
class Slot : public InternalValue
{
public:
    Slot() { m_imm = UndefinedValue{ nullptr, true }; }
    Slot(const Slot&) = delete;
    Slot(Slot&&) = delete;
    Slot& operator=(const Slot&) = delete;
    Slot& operator=(Slot&&) = delete;
    ~Slot() = default;

    [[nodiscard]] bool IsBound() const
    {
        const auto* undefined = std::get_if<UndefinedValue>(&m_imm);
        return undefined == nullptr || !undefined->unbound;
    }
    template<typename Value>
    void Bind(Value&& value)
    {
        static_cast<InternalValue&>(*this) = std::forward<Value>(value);
        if (auto* undefined = std::get_if<UndefinedValue>(&m_imm))
        {
            undefined->unbound = false;
        }
    }
    // Lets go of the value, so that what it held is freed with the scope it was bound in
    void Unbind()
    {
        static_cast<InternalValue&>(*this) = InternalValue();
        m_imm = UndefinedValue{ nullptr, true };
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
    Span<Slot> slots;
    FrameHandle handle;
    UnitId unit;
};

// Some slots of a frame and their names, as a lookup by name sees them (a loop's `loop`
// and targets): a view, valid only inside the statement that made it. The names are texts
// of `nodes`, the tree of that statement, which need not be the tree of the running code
struct FrameView
{
    Span<Slot> slots;
    Span<const SlotName> names;
    ArenaView nodes;

    bool operator==(const FrameView& other) const
    {
        return slots.data() == other.slots.data() && slots.size() == other.slots.size() && names.data() == other.names.data() && names.size() == other.names.size();
    }
};

// A view attached to a scope of a RenderContext: a lookup by name searches it right after
// the scope's map, so names set in the scope hide it and it hides the scopes below (all of
// them down to linkBelow, when it has one)
struct ScopeView
{
    // linkBelow of a view that hides no scope
    static constexpr std::size_t NoLink = static_cast<std::size_t>(-1);

    FrameView view;
    // The scope's index among the context's own scopes
    std::size_t scopeIndex = 0;
    // The static link of a closure's body (docs/design/0038-lexical-scoping-plan.md): once a
    // lookup has searched the scope and its views, it goes on below scope `linkBelow`, an index
    // among all the scopes the context sees, passing over the scopes the caller pushed above
    // the place the closure was defined. NoLink for the others
    std::size_t linkBelow = NoLink;

    bool operator==(const ScopeView& other) const { return view == other.view && scopeIndex == other.scopeIndex && linkBelow == other.linkBelow; }
    bool operator!=(const ScopeView& other) const { return !(*this == other); }
};
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_SLOT_FRAME_H
