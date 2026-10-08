#include "name_resolver.h"

#include "expression_evaluator.h"
#include "node_arena.h"
#include "renderer.h"
#include "slot_frame.h"
#include "statements.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace jinja2
{
NameResolver::FrameId NameResolver::Push(FrameId parent, Kind kind)
{
    const auto id = static_cast<FrameId>(m_frames.size());
    FrameId unit = id;
    bool isDynamicRegion = false;
    bool mayBind = false;
    if (parent != NoFrame && kind != Kind::Unit)
    {
        const auto& outer = m_frames[parent];
        unit = outer.unit;
        isDynamicRegion = outer.isDynamicRegion;
        mayBind = outer.mayBind;
    }
    auto& frame = m_frames.emplace_back();
    frame.parent = parent;
    frame.kind = kind;
    frame.unit = unit;
    frame.isDynamicRegion = isDynamicRegion;
    frame.mayBind = mayBind;
    return id;
}

NameResolver::FrameId NameResolver::PushUnit(FrameId parent, NodeRef<IRendererBase> owner)
{
    const auto id = Push(parent, Kind::Unit);
    m_frames[id].node = owner;
    return id;
}

NameResolver::FrameId NameResolver::PushMacro(FrameId parent, NodeRef<MacroStatement> macro)
{
    const auto id = Push(parent, Kind::Unit);
    auto& frame = m_frames[id];
    frame.node = macro;
    frame.isMacro = true;
    frame.mayBind = true;
    m_hasSlots = true;
    return id;
}

NameResolver::FrameId NameResolver::PushFor(FrameId parent, NodeRef<ForStatement> loop, bool isRecursive)
{
    const auto id = Push(parent, Kind::For);
    auto& frame = m_frames[id];
    frame.node = loop;
    frame.isDynamicRegion = frame.isDynamicRegion || isRecursive;
    frame.mayBind = !frame.isDynamicRegion;
    m_hasSlots = m_hasSlots || frame.mayBind;
    return id;
}

NameResolver::FrameId NameResolver::PushFilter(FrameId parent, bool isRecursive)
{
    const auto id = Push(parent, Kind::Filter);
    auto& frame = m_frames[id];
    frame.isDynamicRegion = frame.isDynamicRegion || isRecursive;
    frame.mayBind = !frame.isDynamicRegion;
    return id;
}

void NameResolver::LinkFilter(FrameId filter, FrameId loop)
{
    m_frames[filter].loop = loop;
}

NameResolver::FrameId NameResolver::PushWith(FrameId parent)
{
    return Push(parent, Kind::With);
}

NameResolver::FrameId NameResolver::PushDynamic(FrameId parent)
{
    const auto id = Push(parent, Kind::Dynamic);
    m_frames[id].mayBind = false;
    return id;
}

void NameResolver::DoAddStore(FrameId frame, const std::string& name)
{
    m_frames[frame].hasStores = true;
    m_stores.emplace_back(frame, name);
}

void NameResolver::AddStores(FrameId frame, const AssignTarget& target)
{
    if (!target.isTuple)
    {
        if (target.attr.empty())
        {
            AddStore(frame, target.name);
        }
        return;
    }
    for (const auto& item : target.items)
    {
        AddStores(frame, item);
    }
}

void NameResolver::Resolve(NodeArena& nodes)
{
    // Without a macro or a loop that binds slots every name is a lookup, and no unit needs a
    // frame
    if (!m_hasSlots)
    {
        return;
    }

    // The units, numbered in parse order, with their arguments' slots, and the slot ranges of
    // their loops: a unit comes before the frames inside it
    std::uint16_t unitCount = 0;
    for (auto& frame : m_frames)
    {
        if (frame.kind == Kind::Unit)
        {
            frame.unitId = UnitId{ unitCount++ };
            if (frame.isMacro)
            {
                PlaceMacroArgs(nodes, frame);
            }
        }
        else if (frame.kind == Kind::For && !frame.isDynamicRegion)
        {
            PlaceLoop(nodes, frame);
        }
    }

    for (const auto& [ref, frameId] : m_uses)
    {
        const auto [slot, unit] = Find(nodes, ref, frameId);
        if (!slot.IsDynamic())
        {
            nodes[ref].SetSlot(slot, unit);
        }
    }

    for (const auto& frame : m_frames)
    {
        // A unit with no slots takes no frame: its default layout says so
        if (frame.kind == Kind::Unit && frame.frameSize != 0 && frame.node)
        {
            SetUnitLayout(nodes, frame.node, UnitLayout{ frame.unitId, static_cast<std::uint16_t>(frame.frameSize) });
        }
    }
}

void NameResolver::PlaceMacroArgs(NodeArena& nodes, Frame& frame)
{
    auto& macro = nodes.Get<MacroStatement>(frame.node);
    const auto names = macro.MakeBinderNames(nodes);
    // A frame indexes its slots with 16 bits; a macro past that keeps its arguments in scopes
    if (names.size() >= SlotIndex::Dynamic)
    {
        return;
    }
    frame.binders = nodes[macro.BindSlots(names)];
    frame.size = static_cast<std::uint32_t>(frame.binders.size());
    frame.frameSize = frame.size;
    frame.isSlotted = true;
}

// A loop's range starts where the range of the loop around it in the same unit ends, or
// after its macro's arguments, so sibling loops share slots
void NameResolver::PlaceLoop(NodeArena& nodes, Frame& frame)
{
    auto& loop = nodes.Get<ForStatement>(frame.node);
    frame.binders = nodes[loop.MakeBinderNames(nodes)];
    const auto targets = static_cast<std::uint32_t>(frame.binders.size() - 1);
    frame.size = 1 + targets + (loop.HasFilter() ? targets : 0);
    for (auto outer = frame.parent; outer != NoFrame; outer = m_frames[outer].parent)
    {
        if (m_frames[outer].isSlotted)
        {
            frame.offset = m_frames[outer].offset + m_frames[outer].size;
            break;
        }
        if (m_frames[outer].kind == Kind::Unit)
        {
            break;
        }
    }
    // A frame indexes its slots with 16 bits; a loop past that keeps its names in scopes
    if (frame.offset + frame.size >= SlotIndex::Dynamic)
    {
        return;
    }
    frame.isSlotted = true;
    auto& unit = m_frames[frame.unit];
    unit.frameSize = std::max(unit.frameSize, frame.offset + frame.size);
    loop.BindSlots(SlotIndex{ static_cast<std::uint16_t>(frame.offset) }, unit.unitId);
}

void NameResolver::SetUnitLayout(NodeArena& nodes, NodeRef<IRendererBase> owner, UnitLayout layout)
{
    if (const auto root = nodes.As<TemplateRenderer>(owner))
    {
        nodes[root].SetUnitLayout(layout);
    }
    else if (const auto macro = nodes.As<MacroStatement>(owner))
    {
        nodes[macro].SetUnitLayout(layout);
    }
    else if (const auto block = nodes.As<BlockStatement>(owner))
    {
        nodes[block].SetUnitLayout(layout);
    }
}

bool NameResolver::IsStored(FrameId frame, std::string_view name) const
{
    return m_frames[frame].hasStores && std::any_of(m_stores.begin(), m_stores.end(), [frame, name](const auto& store) { return store.first == frame && store.second == name; });
}

// From the frame the name is read in out to its unit: a store of the name on the way makes
// it a lookup, since the scope the store writes hides the slot; so does a frame where no
// slot can be bound. The first loop or macro that binds the name has its slot
std::pair<SlotIndex, UnitId> NameResolver::Find(const NodeArena& nodes, NodeRef<ValueRefExpression> ref, FrameId frameId) const
{
    const auto name = nodes[ref].GetHashedName();
    const auto isName = [&name](const SlotName& binder) { return binder.hash == name.hash && binder.name == name.name; };
    const std::pair<SlotIndex, UnitId> dynamic{ SlotIndex{}, UnitId{} };
    for (auto id = frameId; id != NoFrame; id = m_frames[id].parent)
    {
        const auto& frame = m_frames[id];
        const bool isFilter = frame.kind == Kind::Filter;
        if (frame.kind == Kind::Unit && frame.isMacro)
        {
            // A macro's arguments are its frame's first slots; a name it does not bind is
            // read from another unit, and one its body stores from the macro's scope
            const auto* const pos = std::find_if(frame.binders.begin(), frame.binders.end(), isName);
            if (pos == frame.binders.end() || IsStored(id, name.name))
            {
                return dynamic;
            }
            return { SlotIndex{ static_cast<std::uint16_t>(pos - frame.binders.begin()) }, frame.unitId };
        }
        if (frame.kind == Kind::Unit || frame.kind == Kind::Dynamic || frame.isDynamicRegion || IsStored(id, name.name) || (isFilter && frame.loop == NoFrame))
        {
            return dynamic;
        }
        // The filter binds its loop's targets, at their slots past the loop's own, and sees
        // every other name by lookup only
        const auto& loop = isFilter ? m_frames[frame.loop] : frame;
        if (!isFilter && frame.kind != Kind::For)
        {
            continue;
        }
        const auto* const first = isFilter ? loop.binders.begin() + 1 : loop.binders.begin();
        const auto* const pos = std::find_if(first, loop.binders.end(), isName);
        if (pos != loop.binders.end())
        {
            if (!loop.isSlotted)
            {
                return dynamic;
            }
            const auto index = loop.offset + (isFilter ? loop.binders.size() - 1 : 0) + static_cast<std::size_t>(pos - loop.binders.begin());
            return { SlotIndex{ static_cast<std::uint16_t>(index) }, m_frames[loop.unit].unitId };
        }
        if (isFilter)
        {
            return dynamic;
        }
    }
    return dynamic;
}
} // namespace jinja2
