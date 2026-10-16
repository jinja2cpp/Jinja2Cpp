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
class BindUndefinedStatement;
class ForStatement;
class IRendererBase;
class MacroStatement;
class ValueRefExpression;
struct TargetNode;

// Decides at Load which names a render reads from slots (docs/design/0117-name-slots-plan.md,
// phases P1 and P2). The parsers tell it the frames they enter (units, loops, loop filters,
// `with` bodies), the names stored in each, and the name expressions made in a frame that may
// bind names. Once the template has parsed, Resolve gives each macro and call block its
// arguments' slots and each loop its slots in its unit's frame, and each name expression read
// inside the macro or loop that binds it, with no store of the name in between, the slot of
// that binding. Every other name stays a lookup by name.
//
// It also finds the names a frame binds at its entry, as Jinja2's idtracking does
// (docs/tasks/0168): a name the frame stores, outside every branch, and does not read itself
// before that store is the frame's own from the start, so a frame inside it that reads the
// name before the store sees it undefined, not the value outside. A frame's body starts with a
// BindUndefinedStatement for those names (EntryBinding); Resolve drops the ones a frame around
// it reads or binds, which Jinja2 aliases to the outer value.
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
    // The body of `with`, whose targets are stores, or another body run in a scope of its own
    // (a block `set`, `filter`, `autoescape`)
    FrameId PushWith(FrameId parent);
    // Expressions evaluated where no frame of the template is known (macro defaults)
    FrameId PushDynamic(FrameId parent);
    [[nodiscard]] FrameId Parent(FrameId frame) const { return m_frames[frame].parent; }

    // How a frame comes to hold a name
    enum class StoreKind : std::uint8_t
    {
        // `set`, a macro, an import alias, in the frame's own body
        Store,
        // The same inside a branch (SetInBranch), or a name the frame may hold
        // that Jinja2 does not store (an imported name besides its alias): never bound at entry
        Branch,
        // A name the frame binds as it starts: a loop target, a macro's argument, a `with`
        // target (those also hide slots, as a store)
        Param,
        Target
    };
    // `name` is set in a scope of `frame`'s statement (`set`, a macro, an import alias), in a
    // branch of the body when the parser says so (SetInBranch)
    void AddStore(FrameId frame, const std::string& name) { AddStore(frame, name, m_inBranch ? StoreKind::Branch : StoreKind::Store); }
    void AddStore(FrameId frame, const std::string& name, StoreKind kind)
    {
        if (frame != NoFrame)
        {
            DoAddStore(frame, name, kind);
        }
    }
    // The names a `set` or `with` target stores; namespace attributes store none
    void AddStores(FrameId frame, boost::span<const TargetNode> target, const NodeArena& nodes, StoreKind kind);
    void AddStores(FrameId frame, boost::span<const TargetNode> target, const NodeArena& nodes) { AddStores(frame, target, nodes, m_inBranch ? StoreKind::Branch : StoreKind::Store); }

    [[nodiscard]] FrameId Current() const { return m_current; }
    void SetCurrent(FrameId frame) { m_current = frame; }
    // The statements parsed next are in a branch of the current frame's body (`if`, `elif`,
    // `else`, a loop's `else`)
    void SetInBranch(bool inBranch) { m_inBranch = inBranch; }
    // A name expression made in the current frame
    void AddUse(NodeRef<ValueRefExpression> ref)
    {
        if (m_current == NoFrame)
        {
            return;
        }
        m_reads.emplace_back(ref, m_current);
        if (m_frames[m_current].mayBind)
        {
            m_uses.emplace_back(ref, m_current);
        }
    }
    // Called when the body of `frame` (a template, block, macro, call block, loop or `with`)
    // ends: the statement that binds the names the frame holds from its entry, to put first in
    // the body, or none
    NodeRef<BindUndefinedStatement> EntryBinding(FrameId frame, NodeArena& nodes);

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
        // A macro's: the names its body reads from its arguments' slots
        std::uint32_t argReads = 0;
        // The reads and stores made from the frame's start on (m_reads, m_stores)
        std::uint32_t firstRead = 0;
        std::uint32_t firstStore = 0;
        // A Unit frame of a block: it sees the names of the frames around it through the
        // context only, as a Jinja2 block does
        bool isBlock = false;
        // The statement that binds names at the frame's entry (EntryBinding)
        NodeRef<BindUndefinedStatement> entry;
    };
    static_assert(std::is_trivially_destructible_v<Frame>);

    FrameId Push(FrameId parent, Kind kind);
    // Gives a loop its slot range in its unit's frame
    void PlaceLoop(NodeArena& nodes, Frame& frame);
    // Gives a macro's arguments the first slots of its frame
    static void PlaceMacroArgs(NodeArena& nodes, Frame& frame);
    static void SetUnitLayout(NodeArena& nodes, NodeRef<IRendererBase> owner, UnitLayout layout);
    [[nodiscard]] bool IsStored(FrameId frame, std::string_view name) const;
    void DoAddStore(FrameId frame, const std::string& name, StoreKind kind);
    // Gives each entry binding the names no frame around its own reads or binds
    void ResolveEntryBindings(NodeArena& nodes);
    // The slot `ref` reads, Dynamic when it is looked up by name
    [[nodiscard]] std::pair<SlotIndex, UnitId> Find(const NodeArena& nodes, NodeRef<ValueRefExpression> ref, FrameId frame) const;

    // Inline room for a template with a few loops, so that most parses allocate nothing here
    boost::container::small_vector<Frame, 8> m_frames;
    boost::container::small_vector<std::pair<NodeRef<ValueRefExpression>, FrameId>, 16> m_uses;
    struct Store
    {
        FrameId frame = NoFrame;
        StoreKind kind = StoreKind::Store;
        // The reads made before the store (m_reads)
        std::uint32_t readsBefore = 0;
        std::string name;
    };
    // A name a frame binds at its entry, unless a frame around it reads or binds it
    struct EntryName
    {
        FrameId frame = NoFrame;
        std::size_t hash = 0;
        std::string name;
    };
    // Every name expression made in a frame, in parse order
    boost::container::small_vector<std::pair<NodeRef<ValueRefExpression>, FrameId>, 16> m_reads;
    // The names stored in frames, in parse order
    std::vector<Store> m_stores;
    std::vector<EntryName> m_entryNames;
    FrameId m_current = NoFrame;
    bool m_inBranch = false;
    bool m_hasSlots = false;
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_NAME_RESOLVER_H
