#ifndef JINJA2CPP_SRC_NAME_RESOLVER_H
#define JINJA2CPP_SRC_NAME_RESOLVER_H

#include "node_arena.h"
#include "slot_frame.h"

#include <boost/container/small_vector.hpp>
#include <boost/core/span.hpp>

#include <cstddef>
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
        // A `with` target: bound as the frame starts (a loop's targets and a macro's arguments
        // are found on their statements, BindsName)
        Target
    };
    // `name` is set in a scope of `frame`'s statement (`set`, a macro, an import alias), in a
    // branch of the body when the parser says so (SetInBranch). The resolver keeps the view:
    // `name` is text of the arena, which outlives it (docs/tasks/0204)
    void AddStore(FrameId frame, std::string_view name) { AddStore(frame, name, m_inBranch ? StoreKind::Branch : StoreKind::Store); }
    void AddStore(FrameId frame, std::string_view name, StoreKind kind)
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
    // An `include` or `import` with context in the current frame: it reads every name of the
    // frames around it from their scopes, which Jinja2 hands it only once stored. A frame
    // binds no name at entry that it stores after one
    void AddContextRead()
    {
        if (m_current != NoFrame)
        {
            m_contextReads.push_back(ContextRead{ m_current, static_cast<std::uint32_t>(m_stores.size()) });
        }
    }
    // Called when the body of `frame` (a template, block, macro, call block, loop or `with`)
    // ends: the statement that binds the names the frame holds from its entry, to put first in
    // the body, or none
    NodeRef<BindUndefinedStatement> EntryBinding(FrameId frame, NodeArena& nodes) { return MayBindAtEntry(frame) ? MakeEntryBinding(frame, nodes) : NodeRef<BindUndefinedStatement>(); }
    // False when EntryBinding of `frame` surely finds none: a name needs a store in the frame
    // and a read in a frame inside it. Most frames have neither, and the check stays inline,
    // as the call costs more than it (docs/tasks/0204)
    [[nodiscard]] bool MayBindAtEntry(FrameId frame) const { return m_frames[frame].firstStore != m_stores.size() && m_frames.size() != std::size_t{ frame } + 1; }

    // Assigns the slots; called once, after a parse without errors. A template with no entry
    // binding and no macro or loop that binds slots has nothing to resolve
    void Resolve(NodeArena& nodes)
    {
        if (!m_entryNames.empty())
        {
            ResolveEntryBindings(nodes);
        }
        if (m_hasSlots)
        {
            ResolveSlots(nodes);
        }
    }
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
    void DoAddStore(FrameId frame, std::string_view name, StoreKind kind);
    // A name a frame may bind at its entry (EntryBinding)
    struct EntryCandidate
    {
        std::size_t hash = 0;
        std::string_view name;
        std::uint32_t readsBefore = 0;
        // The store's place in m_stores
        std::uint32_t order = 0;
        bool isDropped = false;
        bool isReadInside = false;
    };
    struct EntryName;
    void MarkCandidateReads(FrameId frameId, boost::span<EntryCandidate> candidates, const NodeArena& nodes) const;
    // Whether `frame` binds `name` as it starts: a loop target, `loop`, a macro's argument
    [[nodiscard]] bool BindsName(FrameId frame, std::string_view name, const NodeArena& nodes) const;
    [[nodiscard]] bool IsBoundOutside(FrameId frameId, const EntryName& entry, const NodeArena& nodes) const;
    // Gives each entry binding the names no frame around its own reads or binds
    void ResolveEntryBindings(NodeArena& nodes);
    NodeRef<BindUndefinedStatement> MakeEntryBinding(FrameId frameId, NodeArena& nodes);
    void ResolveSlots(NodeArena& nodes);
    // The slot `ref` reads, Dynamic when it is looked up by name
    [[nodiscard]] std::pair<SlotIndex, UnitId> Find(const NodeArena& nodes, NodeRef<ValueRefExpression> ref, FrameId frame) const;

    using Doubling = boost::container::small_vector_options_t<boost::container::growth_factor<boost::container::growth_factor_100>>;
    // Inline room for a template with a few loops, so that most parses allocate nothing here
    boost::container::small_vector<Frame, 8> m_frames;
    boost::container::small_vector<std::pair<NodeRef<ValueRefExpression>, FrameId>, 16> m_uses;
    struct Store
    {
        FrameId frame = NoFrame;
        StoreKind kind = StoreKind::Store;
        // The reads made before the store (m_reads)
        std::uint32_t readsBefore = 0;
        // Text of the arena
        std::string_view name;
    };
    // A name a frame binds at its entry, unless a frame around it reads or binds it
    struct EntryName
    {
        FrameId frame = NoFrame;
        std::size_t hash = 0;
        std::string name;
    };
    // Every name expression made in a frame, in parse order
    // Inline room for a page of expressions, then doubling: most templates read fewer names, and
    // a long one reads thousands (docs/tasks/0204)
    boost::container::small_vector<std::pair<NodeRef<ValueRefExpression>, FrameId>, 64, void, Doubling> m_reads;
    // The names stored in frames, in parse order; most templates store a few
    boost::container::small_vector<Store, 8, void, Doubling> m_stores;
    std::vector<EntryName> m_entryNames;
    struct ContextRead
    {
        FrameId frame = NoFrame;
        // The stores made before it (m_stores)
        std::uint32_t storesBefore = 0;
    };
    // Inline room for a few, so that a template with an `include` allocates nothing here
    boost::container::small_vector<ContextRead, 4> m_contextReads;
    FrameId m_current = NoFrame;
    bool m_inBranch = false;
    bool m_hasSlots = false;
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_NAME_RESOLVER_H
