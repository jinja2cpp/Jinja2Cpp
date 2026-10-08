#ifndef JINJA2CPP_SRC_NAME_RESOLVER_H
#define JINJA2CPP_SRC_NAME_RESOLVER_H

#include "node_arena.h"
#include "slot_frame.h"

#include <boost/container/small_vector.hpp>
#include <boost/core/span.hpp>

#include <cstdint>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace jinja2
{
class ForStatement;
class IRendererBase;
class MacroStatement;
class ValueRefExpression;
struct AssignTarget;

// Decides at Load which names a render reads from slots (docs/design/0117-name-slots-plan.md,
// phases P1 and P2). The parsers tell it the frames they enter (units, loops, loop filters,
// `with` bodies), the names stored in each, and the name expressions made in a frame that may
// bind names. Once the template has parsed, Resolve gives each macro and call block its
// arguments' slots and each loop its slots in its unit's frame, and each name expression read
// inside the macro or loop that binds it, with no store of the name in between, the slot of
// that binding. Every other name stays a lookup by name.
class NameResolver
{
public:
    using FrameId = std::uint32_t;
    static constexpr FrameId NoFrame = 0xFFFFFFFF;

    // A template body, a macro, a call block's body or a block: a new frame of slots per call
    FrameId PushUnit(FrameId parent, NodeRef<IRendererBase> owner);
    // A macro or a call block's body: a unit that binds its arguments, the special names it
    // catches (caller, kwargs, varargs) among them, in slots of its frame
    FrameId PushMacro(FrameId parent, NodeRef<MacroStatement> macro);
    // The body of `loop`. A recursive loop and everything in it up to the next unit keep
    // their names in scopes: its body runs again for each loop(...) call
    FrameId PushFor(FrameId parent, NodeRef<ForStatement> loop, bool isRecursive);
    // The filter of a loop being parsed: its targets are the loop's, bound to slots of their
    // own while the filter looks ahead; `loop` is not bound there
    FrameId PushFilter(FrameId parent, bool isRecursive);
    void LinkFilter(FrameId filter, FrameId loop);
    // The body of `with`; its targets are stores
    FrameId PushWith(FrameId parent);
    // Expressions evaluated where no frame of the template is known (macro defaults)
    FrameId PushDynamic(FrameId parent);
    [[nodiscard]] FrameId Parent(FrameId frame) const { return m_frames[frame].parent; }

    // `name` is set in a scope of `frame`'s statement (`set`, a macro, an import alias). Only
    // a frame where names may resolve to slots keeps it: everywhere else names are lookups
    void AddStore(FrameId frame, const std::string& name)
    {
        if (frame != NoFrame && m_frames[frame].mayBind)
        {
            DoAddStore(frame, name);
        }
    }
    // The names a `set` or `with` target stores; namespace attributes store none
    void AddStores(FrameId frame, const AssignTarget& target);

    [[nodiscard]] FrameId Current() const { return m_current; }
    void SetCurrent(FrameId frame) { m_current = frame; }
    // A name expression made in the current frame
    void AddUse(NodeRef<ValueRefExpression> ref)
    {
        if (m_current != NoFrame && m_frames[m_current].mayBind)
        {
            m_uses.emplace_back(ref, m_current);
        }
    }

    // Assigns the slots; called once, after a parse without errors
    void Resolve(NodeArena& nodes);
    // The name expressions made where a slot may be bound, in parse order (for the tests)
    [[nodiscard]] boost::span<const std::pair<NodeRef<ValueRefExpression>, FrameId>> Uses() const { return m_uses; }

private:
    enum class Kind : std::uint8_t
    {
        Unit,
        For,
        Filter,
        With,
        Dynamic
    };

    struct Frame
    {
        FrameId parent = NoFrame;
        Kind kind = Kind::Unit;
        // Inside a recursive loop: nothing binds slots
        bool isDynamicRegion = false;
        // Inside a loop of this unit, where a name may resolve to a slot
        bool mayBind = false;
        // The loop of a For frame, the owner of a Unit frame
        NodeRef<IRendererBase> node;
        // A Filter frame's loop
        FrameId loop = NoFrame;
        FrameId unit = NoFrame;
        // A name is stored in the frame (m_stores)
        bool hasStores = false;
        // A Unit frame of a macro: its arguments are bound in slots
        bool isMacro = false;
        // Resolve: a For frame's names, `loop` first, or a macro's arguments, and their slot
        // range (a macro's from the first slot of its frame)
        boost::span<const SlotName> binders;
        std::uint32_t offset = 0;
        std::uint32_t size = 0;
        bool isSlotted = false;
        // Resolve: a Unit frame's number in the template, and the number of slots of its
        // frame: its own names', then its loops'
        UnitId unitId;
        std::uint32_t frameSize = 0;
    };
    static_assert(std::is_trivially_destructible_v<Frame>);

    FrameId Push(FrameId parent, Kind kind);
    // Gives a loop its slot range in its unit's frame
    void PlaceLoop(NodeArena& nodes, Frame& frame);
    // Gives a macro's arguments the first slots of its frame
    static void PlaceMacroArgs(NodeArena& nodes, Frame& frame);
    static void SetUnitLayout(NodeArena& nodes, NodeRef<IRendererBase> owner, UnitLayout layout);
    [[nodiscard]] bool IsStored(FrameId frame, std::string_view name) const;
    void DoAddStore(FrameId frame, const std::string& name);
    // The slot `ref` reads, Dynamic when it is looked up by name
    [[nodiscard]] std::pair<SlotIndex, UnitId> Find(const NodeArena& nodes, NodeRef<ValueRefExpression> ref, FrameId frame) const;

    // Inline room for a template with a few loops, so that most parses allocate nothing here
    boost::container::small_vector<Frame, 8> m_frames;
    boost::container::small_vector<std::pair<NodeRef<ValueRefExpression>, FrameId>, 16> m_uses;
    // The names stored in frames where names may resolve to slots, and their frames
    std::vector<std::pair<FrameId, std::string>> m_stores;
    FrameId m_current = NoFrame;
    bool m_hasSlots = false;
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_NAME_RESOLVER_H
