#include "name_resolver.h"

#include "expression_evaluator.h"
#include "internal_value.h"
#include "node_arena.h"
#include "renderer.h"
#include "slot_frame.h"
#include "statements.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
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
    frame.firstRead = static_cast<std::uint32_t>(m_reads.size());
    frame.firstStore = static_cast<std::uint32_t>(m_stores.size());
    return id;
}

NameResolver::FrameId NameResolver::PushUnit(FrameId parent, NodeRef<IRendererBase> owner)
{
    const auto id = Push(parent, Kind::Unit);
    m_frames[id].node = owner;
    // The template's own body has no frame around it; any other unit that is not a macro is a block
    m_frames[id].isBlock = parent != NoFrame;
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

void NameResolver::DoAddStore(FrameId frame, std::string_view name, StoreKind kind)
{
    m_frames[frame].hasStores = true;
    m_stores.push_back(Store{ frame, kind, static_cast<std::uint32_t>(m_reads.size()), name });
}

void NameResolver::AddStores(FrameId frame, boost::span<const TargetNode> target, const NodeArena& nodes, StoreKind kind)
{
    for (const auto& node : target)
    {
        if (node.IsPlainName())
        {
            AddStore(frame, nodes.Text(node.name), kind);
        }
    }
}

namespace
{
// Whether frame `inner` is `outer` or inside it, and whether the way out passes a block,
// which sees the frames around it through the context only
template<typename Frames>
std::pair<bool, bool> IsInside(const Frames& frames, std::uint32_t inner, std::uint32_t outer, std::uint32_t none)
{
    bool viaBlock = false;
    for (auto id = inner; id != none; id = frames[id].parent)
    {
        if (id == outer)
        {
            return { true, viaBlock };
        }
        viaBlock = viaBlock || frames[id].isBlock;
    }
    return { false, false };
}
} // namespace

NodeRef<BindUndefinedStatement> NameResolver::MakeEntryBinding(FrameId frameId, NodeArena& nodes)
{
    const auto& frame = m_frames[frameId];
    // Frames are numbered as they start, so every frame after this one is inside it, and a
    // read inside comes at or after the first of them starts. A store with no read before it
    // from there on has nothing to bind; most frames have only such stores (docs/tasks/0205)
    const auto innerReads = m_frames[std::size_t{ frameId } + 1].firstRead;
    const auto mayBeReadInside = [frameId, innerReads](const Store& store) { return store.frame == frameId && store.kind == StoreKind::Store && store.readsBefore > innerReads; };
    if (std::none_of(m_stores.begin() + frame.firstStore, m_stores.end(), mayBeReadInside))
    {
        return {};
    }
    // The names the frame stores, first store first: those it stores first outside a branch
    // are the candidates. A name the frame binds as it starts or stores first in a branch is
    // never one: Jinja2 resolves it outward
    boost::container::small_vector<EntryCandidate, 8> stores;
    bool hasCandidate = false;
    for (auto idx = static_cast<std::size_t>(frame.firstStore); idx < m_stores.size(); ++idx)
    {
        const auto& store = m_stores[idx];
        if (store.frame == frameId)
        {
            const bool isCandidate = store.kind == StoreKind::Store;
            stores.push_back(EntryCandidate{ HashedName::Hash(store.name), store.name, store.readsBefore, static_cast<std::uint32_t>(idx), !isCandidate, false });
            hasCandidate = hasCandidate || isCandidate;
        }
    }
    if (!hasCandidate)
    {
        return {};
    }
    std::sort(stores.begin(), stores.end(), [](const EntryCandidate& lhs, const EntryCandidate& rhs) { return std::tie(lhs.hash, lhs.order) < std::tie(rhs.hash, rhs.order); });
    boost::container::small_vector<EntryCandidate, 4> candidates;
    for (auto pos = stores.begin(); pos != stores.end(); ++pos)
    {
        auto run = pos;
        while (run != stores.begin() && std::prev(run)->hash == pos->hash)
        {
            --run;
        }
        const auto isFirst = std::none_of(run, pos, [pos](const EntryCandidate& earlier) { return earlier.name == pos->name; });
        if (isFirst && !pos->isDropped && !BindsName(frameId, pos->name, nodes))
        {
            candidates.push_back(*pos);
        }
    }
    MarkCandidateReads(frameId, candidates, nodes);

    const auto before = m_entryNames.size();
    for (const auto& candidate : candidates)
    {
        if (!candidate.isDropped && candidate.isReadInside)
        {
            m_entryNames.push_back(EntryName{ frameId, candidate.hash, std::string(candidate.name) });
        }
    }
    if (m_entryNames.size() == before)
    {
        return {};
    }
    const auto statement = nodes.Make<BindUndefinedStatement>();
    m_frames[frameId].entry = statement;
    return statement;
}

// A candidate the frame reads before its store is resolved outward there (Jinja2 loads it
// first). One no frame inside reads before the store needs no binding: the body runs in
// order, so a frame that comes after the store, or a closure defined after it, runs after it.
// A read in a block inside does not count: the block reads the context, passing over the
// binding at render (RenderContext::PassOverEntryBindings), as does an include or import with
// context (docs/tasks/0201)
void NameResolver::MarkCandidateReads(FrameId frameId, boost::span<EntryCandidate> candidates, const NodeArena& nodes) const
{
    std::uint32_t lastStore = 0;
    for (const auto& candidate : candidates)
    {
        lastStore = std::max(lastStore, candidate.readsBefore);
    }
    const auto byHash = [](const EntryCandidate& candidate, std::size_t hash) { return candidate.hash < hash; };
    for (auto idx = m_frames[frameId].firstRead; idx < lastStore; ++idx)
    {
        const auto& [ref, readFrame] = m_reads[idx];
        const auto name = nodes[ref].GetHashedName(nodes);
        for (auto* pos = std::lower_bound(candidates.begin(), candidates.end(), name.hash, byHash); pos != candidates.end() && pos->hash == name.hash; ++pos)
        {
            auto& candidate = *pos;
            if (candidate.isDropped || idx >= candidate.readsBefore || candidate.name != name.name)
            {
                continue;
            }
            if (readFrame == frameId)
            {
                candidate.isDropped = true;
                continue;
            }
            const auto [isInside, viaBlock] = IsInside(m_frames, readFrame, frameId, NoFrame);
            candidate.isReadInside = candidate.isReadInside || (isInside && !viaBlock);
        }
    }
}

bool NameResolver::BindsName(FrameId frameId, std::string_view name, const NodeArena& nodes) const
{
    const auto& frame = m_frames[frameId];
    if (frame.kind == Kind::For)
    {
        const auto target = nodes[nodes.Get<ForStatement>(frame.node).GetTarget()];
        return name == "loop" || std::any_of(target.begin(), target.end(), [name, &nodes](const TargetNode& node) { return node.IsPlainName() && nodes.Text(node.name) == name; });
    }
    if (frame.isMacro)
    {
        const auto params = nodes[nodes.Get<MacroStatement>(frame.node).GetParams()];
        return name == "caller" || name == "varargs" || name == "kwargs" || std::any_of(params.begin(), params.end(), [name, &nodes](const MacroParam& param) { return nodes.Text(param.name) == name; });
    }
    return false;
}

// Whether a frame around `frameId`, up to a block, reads, stores or binds `entry`: Jinja2
// aliases the frame's name to that outer one, which a lookup finds anyway
bool NameResolver::IsBoundOutside(FrameId frameId, const EntryName& entry, const NodeArena& nodes) const
{
    // A block sees no frame around it
    if (m_frames[frameId].isBlock)
    {
        return false;
    }
    for (auto outer = m_frames[frameId].parent; outer != NoFrame; outer = m_frames[outer].parent)
    {
        const auto& frame = m_frames[outer];
        const auto isStore = [outer, &entry](const Store& store) { return store.frame == outer && store.name == entry.name; };
        if (std::any_of(m_stores.begin() + frame.firstStore, m_stores.end(), isStore) || BindsName(outer, entry.name, nodes))
        {
            return true;
        }
        const auto isRead = [outer, &entry, &nodes](const std::pair<NodeRef<ValueRefExpression>, FrameId>& read) {
            if (read.second != outer)
            {
                return false;
            }
            const auto name = nodes[read.first].GetHashedName(nodes);
            return name.hash == entry.hash && name.name == entry.name;
        };
        if (std::any_of(m_reads.begin() + frame.firstRead, m_reads.end(), isRead))
        {
            return true;
        }
        // A block sees no frame around it
        if (frame.isBlock)
        {
            return false;
        }
    }
    return false;
}

void NameResolver::ResolveEntryBindings(NodeArena& nodes)
{
    std::vector<SlotName> names;
    for (auto first = m_entryNames.begin(); first != m_entryNames.end();)
    {
        const auto frameId = first->frame;
        const auto last = std::find_if(first, m_entryNames.end(), [frameId](const EntryName& entry) { return entry.frame != frameId; });
        names.clear();
        for (auto entry = first; entry != last; ++entry)
        {
            if (!IsBoundOutside(frameId, *entry, nodes))
            {
                names.push_back(SlotName{ nodes.MakeText(entry->name), entry->hash });
            }
        }
        nodes[m_frames[frameId].entry].SetNames(nodes.MakeSpan(boost::span<const SlotName>(names.data(), names.size())));
        first = last;
    }
}

// Only with a macro or a loop that binds slots: without one every name is a lookup, and no
// unit needs a frame
void NameResolver::ResolveSlots(NodeArena& nodes)
{
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

    // The names each macro's body reads, itself and not from a frame inside it, from the
    // slots of its own arguments, which every call binds (MacroStatement::SetReadsSlotsOnly)
    for (const auto& [ref, frameId] : m_uses)
    {
        const auto [slot, unit] = Find(nodes, ref, frameId);
        if (!slot.IsDynamic())
        {
            nodes[ref].SetSlot(slot, unit);
            auto& frame = m_frames[frameId];
            if (frame.isMacro && unit == frame.unitId && slot.value < frame.binders.size())
            {
                ++frame.argReads;
            }
        }
    }
    for (const auto& frame : m_frames)
    {
        if (frame.isMacro)
        {
            nodes.Get<MacroStatement>(frame.node).SetReadsSlotsOnly(frame.argReads);
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
    const auto& stores = m_frames[frame];
    return stores.hasStores && std::any_of(m_stores.begin() + stores.firstStore, m_stores.end(), [frame, name](const Store& store) { return store.frame == frame && store.name == name; });
}

// From the frame the name is read in out to its unit: a store of the name on the way makes
// it a lookup, since the scope the store writes hides the slot; so does a frame where no
// slot can be bound. The first loop or macro that binds the name has its slot
std::pair<SlotIndex, UnitId> NameResolver::Find(const NodeArena& nodes, NodeRef<ValueRefExpression> ref, FrameId frameId) const
{
    const auto name = nodes[ref].GetHashedName(nodes);
    const auto isName = [&name, &nodes](const SlotName& binder) { return binder.hash == name.hash && nodes.Text(binder.name) == name.name; };
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

bool NameResolver::DeclaresAtEntry(FrameId frameId, std::string_view name, const DeclarationIndex& index, const NodeArena& nodes) const
{
    const auto& frame = m_frames[frameId];
    if (frame.kind == Kind::For || frame.isMacro)
    {
        return BindsName(frameId, name, nodes);
    }
    // A loop's filter binds the loop's targets, not `loop`
    if (frame.kind == Kind::Filter)
    {
        return frame.loop != NoFrame && name != "loop" && BindsName(frame.loop, name, nodes);
    }
    // A macro's defaults are evaluated in its frame, next after theirs (ParseMacroParams),
    // where its arguments are bound, and `caller`, `varargs` and `kwargs` only when its body
    // reads them, as Jinja2 declares them for the macro then
    const auto next = std::size_t{ frameId } + 1;
    if (frame.kind != Kind::Dynamic || next == m_frames.size() || !m_frames[next].isMacro || m_frames[next].parent != frame.parent)
    {
        return false;
    }
    const auto macro = static_cast<FrameId>(next);
    if (name == "caller" || name == "varargs" || name == "kwargs")
    {
        return index.specialReads.count({ macro, name }) != 0;
    }
    return BindsName(macro, name, nodes);
}

bool NameResolver::IsDeclared(FrameId frameId, std::string_view name, std::size_t storeEnd, const DeclarationIndex& index, const NodeArena& nodes) const
{
    // The template's body and every block have `self`
    if (name == "self")
    {
        return true;
    }
    for (auto id = frameId;;)
    {
        const auto& frame = m_frames[id];
        // The frame's first store of the name: before the read in the read's own frame,
        // anywhere in a frame around it
        if (frame.hasStores)
        {
            const auto first = index.firstStores.find({ id, name });
            if (first != index.firstStores.end() && (id != frameId || first->second < storeEnd))
            {
                return true;
            }
        }
        if (DeclaresAtEntry(id, name, index, nodes))
        {
            return true;
        }
        // A block sees the names around it through the context only
        if (frame.isBlock)
        {
            return name == "super";
        }
        if (frame.parent == NoFrame)
        {
            return false;
        }
        id = frame.parent;
    }
}

std::size_t NameResolver::FrameNameHash::operator()(const FrameName& key) const noexcept
{
    return std::hash<std::string_view>()(key.second) ^ (std::size_t{ key.first } * 0x9E3779B97F4A7C15ULL);
}

NameResolver::DeclarationIndex NameResolver::MakeDeclarationIndex(const NodeArena& nodes) const
{
    DeclarationIndex index;
    for (std::size_t idx = 0; idx < m_stores.size(); ++idx)
    {
        const auto& store = m_stores[idx];
        if (store.kind != StoreKind::Hidden)
        {
            index.firstStores.emplace(FrameName{ store.frame, store.name }, idx);
        }
    }
    // The frames that read `caller`, `varargs` or `kwargs`, themselves or in a frame inside
    // them; Jinja2 looks into the macros, loops and call blocks inside a body, not its blocks
    for (const auto& [ref, frame] : m_reads)
    {
        const auto name = nodes[ref].GetName(nodes);
        if (name != "caller" && name != "varargs" && name != "kwargs")
        {
            continue;
        }
        for (auto id = frame; id != NoFrame; id = m_frames[id].parent)
        {
            if (!index.specialReads.insert({ id, name }).second || m_frames[id].isBlock)
            {
                break;
            }
        }
    }
    return index;
}

std::vector<std::string_view> NameResolver::UndeclaredNames(const NodeArena& nodes) const
{
    std::vector<std::string_view> names;
    const auto index = MakeDeclarationIndex(nodes);
    // The stores before a read are those made before it: m_stores is in parse order
    auto storeEnd = m_stores.begin();
    for (std::size_t idx = 0; idx < m_reads.size(); ++idx)
    {
        storeEnd = std::find_if(storeEnd, m_stores.end(), [idx](const Store& store) { return store.readsBefore > idx; });
        const auto& [ref, frame] = m_reads[idx];
        const auto name = nodes[ref].GetName(nodes);
        if (!IsDeclared(frame, name, static_cast<std::size_t>(storeEnd - m_stores.begin()), index, nodes))
        {
            names.push_back(name);
        }
    }
    // Jinja2 resolves a name stored in some branches only from the context when no frame
    // around declares it (Symbols.branch_update), and counts `if` without `else` as such
    for (std::size_t idx = 0; idx < m_stores.size(); ++idx)
    {
        const auto& store = m_stores[idx];
        if (store.kind == StoreKind::Branch && !IsDeclared(store.frame, store.name, idx, index, nodes))
        {
            names.push_back(store.name);
        }
    }
    return names;
}
} // namespace v2
} // namespace jinja2
