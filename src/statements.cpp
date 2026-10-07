#include "statements.h"

#include "expression_evaluator.h"
#include "generic_adapters.h"
#include "internal_value.h"
#include "lookup_result.h"
#include "loop_attr.h"
#include "markup.h"
#include "node_arena.h"
#include "out_stream.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "render_workspace.h"
#include "renderer.h"
#include "slot_frame.h"
#include "template_impl.h"
#include "undefined.h"
#include "value_methods.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>

#include <boost/container/small_vector.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/core/span.hpp>

#include <algorithm>
#include <array>
#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{

void ForStatement::Render(OutStream& os, RenderContext& values)
{
    InternalValue loopVal = values.Nodes()[m_value].Evaluate(values);

    RenderLoop(loopVal, os, values, 0);
}

namespace
{

// Where an assignment puts the names it binds: a scope, by name
struct ScopeSink
{
    ScopeRef scope;

    void Set(const AssignTarget& target, InternalValue value) { scope[target.name] = std::move(value); }
};

// The slots of the names a loop binds, by the slot each target name was given at Load;
// `shift` moves past the loop's own slots to those of its filter
struct SlotSink
{
    RenderContext& values;
    boost::span<Slot> slots;
    std::uint16_t shift = 0;

    void Set(const AssignTarget& target, InternalValue value) { values.BindSlot(slots[target.slot.value + shift], std::move(value)); }
};

// Python's assignment to a target: a name takes the value; a tuple `a, (b, c)` iterates
// the value, which must yield exactly as many items as the tuple has targets, and assigns
// them in turn. A mapping assigned to a tuple of names is the exception: Jinja2C++ has
// always taken its values by name (`set first, last = person`), where Python would
// assign its keys
template<typename Sink>
void AssignTo(const AssignTarget& target, InternalValue value, Sink& sink, RenderContext& values)
{
    if (!target.attr.empty())
    {
        // `set ns.attr = ...` changes a namespace() object wherever it is defined
        const auto found = values.FindValue(target.name);
        const auto* ns = found ? GetIf<MapAdapter>(&*found) : nullptr;
        if (!ns || !ns->IsNamespace())
        {
            throw std::runtime_error("cannot assign attribute on non-namespace object");
        }
        MapAdapter(*ns).SetValue(target.attr, value);
        return;
    }
    if (!target.isTuple)
    {
        sink.Set(target, std::move(value));
        return;
    }

    const auto& targets = target.items;
    auto isName = [](const AssignTarget& t) { return !t.isTuple && t.attr.empty(); };
    if (GetIf<MapAdapter>(&value) && std::all_of(targets.begin(), targets.end(), isName))
    {
        for (const auto& t : targets)
        {
            sink.Set(t, Subscript(value, t.name, &values));
        }
        return;
    }

    InternalValueList items;
    if (auto* pair = GetIf<KeyValuePair>(&value))
    {
        items.emplace_back(TargetString(pair->key));
        items.push_back(pair->value);
    }
    else
    {
        bool isConverted = false;
        auto list = ConvertToList(value, isConverted, false);
        if (!isConverted)
        {
            throw std::runtime_error("cannot unpack non-iterable value");
        }
        // One item past the targets is enough to tell that there are too many
        for (const auto& item : list)
        {
            items.push_back(item);
            if (items.size() > targets.size())
            {
                break;
            }
        }
    }

    if (items.size() > targets.size())
    {
        throw std::runtime_error("too many values to unpack (expected " + std::to_string(targets.size()) + ")");
    }
    if (items.size() < targets.size())
    {
        throw std::runtime_error("not enough values to unpack (expected " + std::to_string(targets.size()) + ", got " + std::to_string(items.size()) + ")");
    }

    for (std::size_t idx = 0; idx != targets.size(); ++idx)
    {
        AssignTo(targets[idx], std::move(items[idx]), sink, values);
    }
}

void AssignTo(const AssignTarget& target, InternalValue value, ScopeRef scope, RenderContext& values)
{
    if (!target.isTuple && target.attr.empty())
    {
        scope[target.name] = std::move(value);
        return;
    }
    ScopeSink sink{ scope };
    AssignTo(target, std::move(value), sink, values);
}

} // namespace
namespace
{
bool IsPlainName(const AssignTarget& target)
{
    return !target.isTuple && target.attr.empty();
}

// Where a loop stores its target names, found on the first item: the map keeps its
// nodes in place, so the slots survive other names being added
struct LoopTargetSlots
{
    InternalValue* single = nullptr;
    std::vector<InternalValue*> items;
};

// The state behind a loop object. The template can keep the object past the loop
// (`set ns.x = loop`), so it is shared with the loop object
struct LoopState : std::enable_shared_from_this<LoopState>
{
    ListAdapter indexedList;
    std::optional<ListAccessorEnumeratorPtr> enumerator;
    std::optional<size_t> listSize;
    // The index of the current item
    size_t index0 = 0;
    bool isLast = false;
    // Set while the loop moves to the next item, which runs the loop filter
    bool isAdvancing = false;
    int level = 0;
    // The previous, current and next items, at index0 % 3 for the current one, so that
    // moving to the next item fetches one value and moves none
    std::array<InternalValue, 3> items;
    // A recursive loop renders its body for each loop(...) call; null otherwise
    ForStatement* recursiveStatement = nullptr;
    // The tree the recursive loop belongs to: loop(...) may be called from another template
    ArenaView recursiveNodes;
    // The arguments of the last loop.changed() call, made on first use
    std::shared_ptr<std::optional<InternalValueList>> lastChanged;

    InternalValue& Item(size_t idx) { return items[idx % items.size()]; }

    // Readies a finished loop's state for the next loop entered on this thread
    // (docs/tasks/0133): lets go of everything the loop held, keeping the enumerator
    // only when the next loop can rebind it
    // Out of line, as are the other steps of entering and leaving a loop, to keep the
    // iteration in RenderLoop small enough to inline what it calls
    JINJA2CPP_NOINLINE_INLINE void Recycle()
    {
        indexedList = ListAdapter();
        if (enumerator && !(*enumerator)->Rebind(nullptr))
        {
            enumerator.reset();
        }
        listSize.reset();
        index0 = 0;
        isLast = false;
        isAdvancing = false;
        level = 0;
        items.fill(InternalValue());
        recursiveStatement = nullptr;
        recursiveNodes = ArenaView();
        lastChanged = nullptr;
    }

    // The length of a filtered loop is known once the rest of the items are collected
    size_t GetLength()
    {
        if (listSize)
        {
            return listSize.value();
        }
        // Collecting the rest from inside the filter would replace the enumerator that is
        // running it. Jinja2 has no `loop` in the filter at all
        if (isAdvancing)
        {
            throw std::runtime_error("'loop' is undefined in the loop filter");
        }
        // On the last item the enumerator has nothing left to collect
        if (isLast || !enumerator)
        {
            listSize = index0 + 1;
            return listSize.value();
        }

        InternalValueList rest;
        do
        {
            rest.push_back((*enumerator)->GetCurrent());
        } while ((*enumerator)->MoveNext());

        listSize = index0 + rest.size() + 1;
        indexedList = ListAdapter::CreateAdapter(std::move(rest));
        enumerator = indexedList.GetEnumerator();
        isLast = !enumerator || !(*enumerator)->MoveNext();
        return listSize.value();
    }
};

// Assigns the current item to the loop target. A plain name is stored straight into its
// slot, made by the first item so that the `else` body of an empty loop does not see the
// name. The map keeps its nodes in place, so the slot survives other names being added
void AssignLoopTarget(const AssignTarget& target, const InternalValue& item, ScopeRef scope, LoopTargetSlots& slots, RenderContext& values)
{
    static_assert(!InternalValueMap::is_flat);
    if (!target.isTuple && target.attr.empty())
    {
        if (!slots.single)
        {
            slots.single = &scope[target.name];
        }
        *slots.single = item;
        return;
    }
    // `for k, v in d|dictsort` (or d.items()): a pair goes straight into the slots of two
    // plain names, without the list of items that AssignTo unpacks through
    const auto* pair = GetIf<KeyValuePair>(&item);
    if (pair && target.isTuple && target.items.size() == 2 && IsPlainName(target.items[0]) && IsPlainName(target.items[1]))
    {
        if (slots.items.empty())
        {
            slots.items = { &scope[target.items[0].name], &scope[target.items[1].name] };
        }
        *slots.items[0] = TargetString(pair->key);
        *slots.items[1] = pair->value;
        return;
    }
    AssignTo(target, item, scope, values);
}

// loop.changed(*values): whether the values differ from those of the previous call
Callable MakeLoopChanged(const std::shared_ptr<std::optional<InternalValueList>>& lastChanged)
{
    return Callable(Callable::GlobalFunc, [lastChanged](const CallParams& params, RenderContext&) -> InternalValue {
        if (!params.kwParams.empty())
        {
            throw std::runtime_error("changed() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
        }
        auto isEqual = [](const InternalValue& lhs, const InternalValue& rhs) {
            return ConvertToBool(Apply2<visitors::BinaryMathOperation>(lhs, rhs, BinaryExpression::LogicalEq));
        };
        auto& last = *lastChanged;
        const auto& args = params.posParams;
        if (last && last->size() == args.size() && std::equal(last->begin(), last->end(), args.begin(), isEqual))
        {
            return false;
        }
        last = params.posParams;
        return true;
    });
}
// The `loop` object. As in Jinja2's LoopContext, its attributes are computed from the
// loop state when they are looked up, so an iteration only advances the state
class LoopAccessor : public MapAccessorImpl<LoopAccessor>
{
public:
    // The accessor of a loop lives in its LoopFrame, which owns it
    explicit LoopAccessor(LoopState* state)
        : m_state(state)
    {
    }
    // A copy (the one a GenericMap keeps) owns the state on its own
    LoopAccessor(const LoopAccessor& other)
        : MapAccessorImpl<LoopAccessor>(other)
        , m_state(other.m_state)
        , m_owner(other.m_state->shared_from_this())
    {
    }
    LoopAccessor(LoopAccessor&&) = delete;
    LoopAccessor& operator=(const LoopAccessor&) = delete;
    LoopAccessor& operator=(LoopAccessor&&) = delete;
    ~LoopAccessor() override = default;

    [[nodiscard]] size_t GetSize() const override { return GetKeys().size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override
    {
        auto prop = FindProperty(name);
        return prop && IsPresent(*prop);
    }
    [[nodiscard]] InternalValue GetItem(const std::string& name) const override
    {
        auto prop = FindProperty(name);
        if (!prop || !IsPresent(*prop))
        {
            return InternalValue();
        }
        return GetProperty(*prop);
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> result;
        for (const auto& [name, prop] : Properties())
        {
            if (IsPresent(prop))
            {
                result.emplace_back(name);
            }
        }
        return result;
    }
    [[nodiscard]] GenericMap CreateGenericMap() const override
    {
        return GenericMap([accessor = *this]() -> const IMapItemAccessor* { return &accessor; });
    }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return true; }
    [[nodiscard]] const void* GetIdentity() const override { return m_state; }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const LoopAccessor*>(&other);
        if (!val)
        {
            return false;
        }
        return m_state == val->m_state;
    }

    bool GetLoopAttr(LoopAttr attr, InternalValue& value) const override
    {
        switch (attr)
        {
        // The neighbouring items share the loop's data, and `changed` makes a callable:
        // the generic lookup handles them
        case LoopAttr::None:
        case LoopAttr::PrevItem:
        case LoopAttr::NextItem:
        case LoopAttr::Changed:
        case LoopAttr::Call:
            return false;
        default:
            value = GetProperty(attr);
            return true;
        }
    }

private:
    using Property = LoopAttr;

    static const std::vector<std::pair<std::string, Property>>& Properties()
    {
        static const std::vector<std::pair<std::string, Property>> properties = {
            { "index", Property::Index },
            { "index0", Property::Index0 },
            { "revindex", Property::RevIndex },
            { "revindex0", Property::RevIndex0 },
            { "first", Property::First },
            { "last", Property::Last },
            { "length", Property::Length },
            { "depth", Property::Depth },
            { "depth0", Property::Depth0 },
            { "previtem", Property::PrevItem },
            { "nextitem", Property::NextItem },
            { "cycle", Property::Cycle },
            { "changed", Property::Changed },
            { "operator()", Property::Call },
        };
        return properties;
    }

    static std::optional<Property> FindProperty(const std::string& name)
    {
        if (name == "operator()")
        {
            return Property::Call;
        }
        const auto attr = FindLoopAttr(name);
        return attr != LoopAttr::None ? std::optional<Property>(attr) : std::nullopt;
    }

    [[nodiscard]] bool IsPresent(Property prop) const
    {
        switch (prop)
        {
        case Property::PrevItem:
            return m_state->index0 != 0;
        case Property::NextItem:
            return !m_state->isLast;
        case Property::Call:
            return m_state->recursiveStatement != nullptr;
        default:
            return true;
        }
    }

    [[nodiscard]] InternalValue GetProperty(Property prop) const
    {
        auto& state = *m_state;
        switch (prop)
        {
        case Property::Index:
            return static_cast<int64_t>(state.index0 + 1);
        case Property::Index0:
            return static_cast<int64_t>(state.index0);
        case Property::RevIndex:
            return static_cast<int64_t>(state.GetLength() - state.index0);
        case Property::RevIndex0:
            return static_cast<int64_t>(state.GetLength() - state.index0 - 1);
        case Property::First:
            return state.index0 == 0;
        case Property::Last:
            return state.isLast;
        case Property::Length:
            return static_cast<int64_t>(state.GetLength());
        case Property::Depth:
            return static_cast<int64_t>(state.level + 1);
        case Property::Depth0:
            return static_cast<int64_t>(state.level);
        case Property::PrevItem:
            return state.Item(state.index0 + 2);
        case Property::NextItem:
            return state.Item(state.index0 + 1);
        case Property::Cycle:
            return static_cast<int64_t>(LoopCycleFn);
        case Property::Changed:
            if (!state.lastChanged)
            {
                state.lastChanged = std::make_shared<std::optional<InternalValueList>>();
            }
            return MakeLoopChanged(state.lastChanged);
        case Property::Call:
            return ForStatement::MakeLoopRecursion(state.recursiveStatement, state.recursiveNodes, state.level);
        case Property::None:
            break;
        }
        return InternalValue();
    }

    LoopState* m_state;
    std::shared_ptr<LoopState> m_owner;
};

} // namespace

namespace detail
{
// A loop's state and its `loop` object, made in one allocation. A frame put back into the
// pool also keeps the maps of the loop's two scopes, so that the same loop entered again
// finds its names and their nodes in place
struct LoopFrame : LoopState
{
    LoopAccessor accessor{ this };
    // The loop whose names the scopes hold (ForStatement::m_loopId); 0 for none
    uint64_t loopId = 0;
    // The scope of `loop` and the loop target, and the body's scope, while the loop is
    // not running: emptied of values, but with their names
    InternalValueMap loopScope;
    InternalValueMap bodyScope;
    InternalValue* loopSlot = nullptr;
    LoopTargetSlots targetSlots;

    // Readies the frame for the loop `id`: the names another loop left would be visible
    // in this one
    JINJA2CPP_NOINLINE_INLINE void Claim(uint64_t id)
    {
        if (loopId == id)
        {
            return;
        }
        loopId = id;
        loopScope.clear();
        bodyScope.clear();
        loopSlot = nullptr;
        targetSlots = LoopTargetSlots();
    }

    // Lets go of the values in the loop's scope once it is left: the user data of the last
    // item, and `loop` itself, which would own its own frame
    void ReleaseScopeValues()
    {
        // The names the loop knows the slots of, unless unpacking added others
        const size_t known = (loopSlot ? 1U : 0U) + (targetSlots.single ? 1U : 0U) + targetSlots.items.size();
        if (loopScope.size() != known)
        {
            ReleaseAllScopeValues();
            return;
        }
        if (loopSlot)
        {
            *loopSlot = InternalValue();
        }
        if (targetSlots.single)
        {
            *targetSlots.single = InternalValue();
        }
        for (auto* slot : targetSlots.items)
        {
            *slot = InternalValue();
        }
    }
    JINJA2CPP_NOINLINE_INLINE void ReleaseAllScopeValues()
    {
        for (auto& entry : loopScope)
        {
            entry.second = InternalValue();
        }
    }
};

} // namespace detail

namespace
{
using detail::LoopFrame;

// The frames of finished loops on this thread, reused by the loops entered next: an inner
// loop entered once per item of the outer one allocates nothing for its frame and inserts
// no names into its scope (docs/tasks/0133). A frame the template kept
// (`set ns.x = loop`) is never put back
class LoopFramePool
{
public:
    // A frame for the loop `id`, the one it ran in last if there is one
    static std::shared_ptr<LoopFrame> Take(uint64_t id)
    {
        // An inner loop entered again finds its frame on top
        auto& frames = Frames();
        if (!frames.empty() && frames.back()->loopId == id)
        {
            auto frame = std::move(frames.back());
            frames.pop_back();
            return frame;
        }
        return TakeOther(id);
    }

private:
    JINJA2CPP_NOINLINE_INLINE static std::shared_ptr<LoopFrame> TakeOther(uint64_t id)
    {
        auto& frames = Frames();
        if (frames.empty())
        {
            auto frame = std::make_shared<LoopFrame>();
            frame->Claim(id);
            return frame;
        }
        auto found = std::find_if(frames.rbegin(), frames.rend(), [id](const auto& frame) { return frame->loopId == id; });
        auto pos = found == frames.rend() ? frames.end() - 1 : std::prev(found.base());
        auto frame = std::move(*pos);
        frames.erase(pos);
        frame->Claim(id);
        return frame;
    }

public:
    // Takes the frame back when the loop that ran in it was its only owner
    JINJA2CPP_NOINLINE_INLINE static void Give(std::shared_ptr<LoopFrame>& frame)
    {
        auto& frames = Frames();
        if (frame.use_count() != 1 || frames.size() >= RenderWorkspace::MaxLoopFrames)
        {
            return;
        }
        frame->Recycle();
        frames.push_back(std::move(frame));
    }

private:
    // At most RenderWorkspace::MaxLoopFrames, deeper than any loop nesting a template has by
    // hand; recursive loops past it allocate
    static std::vector<std::shared_ptr<LoopFrame>>& Frames() { return RenderWorkspace::ForThisThread().LoopFrames(); }
};

// Gives the frame of a loop back to the pool when the loop ends, by any path
struct LoopFrameReturn
{
    explicit LoopFrameReturn(std::shared_ptr<LoopFrame>& loopFrame)
        : frame(loopFrame)
    {
    }
    LoopFrameReturn(const LoopFrameReturn&) = delete;
    LoopFrameReturn(LoopFrameReturn&&) = delete;
    LoopFrameReturn& operator=(const LoopFrameReturn&) = delete;
    LoopFrameReturn& operator=(LoopFrameReturn&&) = delete;
    ~LoopFrameReturn() { LoopFramePool::Give(frame); }

    std::shared_ptr<LoopFrame>& frame;
};

} // namespace

namespace
{
// The frame of slots of one call of a unit (0117 P1): taken from the thread's workspace and
// installed in the context for the call, then given back, with the frame it replaced
// installed again, on any exit. A unit without slots leaves the frame as it is
class UnitCall
{
public:
    UnitCall(RenderContext& values, UnitLayout layout)
        : m_values(values)
        , m_isActive(layout.size != 0)
    {
        if (m_isActive)
        {
            m_previous = values.InstallFrame(RenderWorkspace::ForThisThread().Take(layout));
        }
    }
    UnitCall(const UnitCall&) = delete;
    UnitCall(UnitCall&&) = delete;
    UnitCall& operator=(const UnitCall&) = delete;
    UnitCall& operator=(UnitCall&&) = delete;
    ~UnitCall()
    {
        if (m_isActive)
        {
            RenderWorkspace::ForThisThread().Release(m_values.InstallFrame(m_previous));
        }
    }

private:
    RenderContext& m_values;
    SlotFrame m_previous;
    bool m_isActive;
};

// The slots a loop or its filter bound and the view it pushed, let go of when it is left by
// an exception; a loop left normally lets go of them itself, in its own order
class SlotsGuard
{
public:
    SlotsGuard(RenderContext& values, boost::span<Slot> slots)
        : m_values(values)
        , m_slots(slots)
        , m_viewCount(values.ViewCount())
    {
    }
    SlotsGuard(const SlotsGuard&) = delete;
    SlotsGuard(SlotsGuard&&) = delete;
    SlotsGuard& operator=(const SlotsGuard&) = delete;
    SlotsGuard& operator=(SlotsGuard&&) = delete;
    ~SlotsGuard() // NOLINT(bugprone-exception-escape): unbinding and dropping views free memory only
    {
        if (!m_isDone)
        {
            m_values.UnbindSlots(m_slots);
            m_values.TruncateViews(m_viewCount);
        }
    }

    // Unbinds the slots now, with one new lookup epoch
    void Unbind()
    {
        m_values.UnbindSlots(m_slots);
        m_isDone = true;
    }

private:
    RenderContext& m_values;
    boost::span<Slot> m_slots;
    size_t m_viewCount;
    bool m_isDone = false;
};
} // namespace

Callable ForStatement::MakeLoopRecursion(ForStatement* statement, const ArenaView& nodes, int level)
{
    return Callable(Callable::GlobalFunc, [statement, nodes, level](const CallParams& params, OutStream& stream, RenderContext& context) {
        bool isSucceeded = false;
        auto parsedParams = helpers::ParseCallParams({ { "var", true } }, params, isSucceeded);
        if (!isSucceeded)
        {
            return;
        }

        auto var = parsedParams["var"];
        if (IsEmpty(var))
        {
            return;
        }

        const RenderDepthGuard depthGuard;
        const ArenaSwitch nodesSwitch(context, nodes);
        statement->RenderLoop(var, stream, context, level + 1);
    });
}

void ForStatement::RenderLoop(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level)
{
    if (HasSlots())
    {
        // Every unit with slotted loops takes its frame when it is called (UnitCall)
        assert(values.Frame().unit == m_unit);
        RenderLoopInSlots(loopVal, os, values);
        return;
    }
    RenderLoopInScopes(loopVal, os, values, level);
}

void ForStatement::RenderLoopInScopes(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level)
{
    auto state = LoopFramePool::Take(m_loopId);
    const LoopFrameReturn frameReturn{ state };
    state->level = level;
    if (m_isRecursive)
    {
        state->recursiveStatement = this;
        state->recursiveNodes = values.Nodes();
    }
    auto context = values.EnterScope(std::move(state->loopScope));
    if (!state->loopSlot)
    {
        state->loopSlot = &context["loop"s];
    }
    *state->loopSlot = MapAdapter(std::shared_ptr<LoopAccessor>(state, &state->accessor));

    bool isConverted = false;
    auto loopItems = ConvertToList(loopVal, isConverted, false);
    ListAdapter filteredList;
    if (!isConverted)
    {
        // The `else` body sees the names outside the loop, as in Jinja2
        values.ExitScope(state->loopScope);
        state->ReleaseScopeValues();
        if (m_elseBody)
        {
            values.Nodes()[m_elseBody].Render(os, values);
        }
        return;
    }

    auto& enumerator = state->enumerator;
    if (m_ifExpr)
    {
        filteredList = CreateFilteredAdapter(loopItems, values);
        enumerator = filteredList.GetEnumerator();
    }
    else
    {
        loopItems.RebindEnumerator(enumerator);
        state->listSize = loopItems.GetSize();
    }

    bool loopRendered = false;
    auto& isLast = state->isLast;
    auto moveNext = [&state, &enumerator]() {
        state->isAdvancing = true;
        const bool hasNext = (*enumerator)->MoveNext();
        state->isAdvancing = false;
        return hasNext;
    };
    isLast = !moveNext();
    // One scope for the body, emptied after each pass, so `set` in the body stays local
    // to one iteration without a map being made for each
    auto bodyScope = values.EnterScope(std::move(state->bodyScope));
    auto& targetSlots = state->targetSlots;
    for (size_t itemIdx = 0; !isLast; ++itemIdx)
    {
        state->index0 = itemIdx;
        if (itemIdx == 0)
        {
            state->Item(0) = (*enumerator)->GetCurrent();
        }
        const auto& curValue = state->Item(itemIdx);

        isLast = !moveNext();
        if (!isLast)
        {
            state->Item(itemIdx + 1) = (*enumerator)->GetCurrent();
        }

        AssignLoopTarget(m_target, curValue, context, targetSlots, values);

        values.Nodes()[m_mainBody].Render(os, values);
        bodyScope.Clear();

        // As in Jinja2, the `else` body is skipped only once a pass through the body has
        // finished without `break` or `continue`
        auto control = values.TakeLoopControl();
        if (control == LoopControl::Break)
        {
            break;
        }
        if (control == LoopControl::None)
        {
            loopRendered = true;
        }
    }
    values.ExitScope(state->bodyScope);

    // A loop object kept past the loop (`set ns.x = loop`) can no longer run the filter,
    // which needs this render context: collect the rest of the items now
    // (copies of `loop` and the accessor a GenericMap copies from it all own the state:
    // more owners than this function and the scope mean it was kept)
    if (!state->listSize && state.use_count() > 2)
    {
        state->GetLength();
    }

    values.ExitScope(state->loopScope);
    state->ReleaseScopeValues();
    if (!loopRendered && m_elseBody)
    {
        values.Nodes()[m_elseBody].Render(os, values);
    }
}

void ForStatement::RenderLoopInSlots(const InternalValue& loopVal, OutStream& os, RenderContext& values)
{
    // Declared first, so that the loop's slots let go of `loop` before the frame is given back
    auto state = LoopFramePool::Take(m_loopId);
    const LoopFrameReturn frameReturn{ state };
    // `loop`, then the targets; the frame's slots never move during the call
    const auto names = values.Nodes()[m_slotNames];
    const auto frameSlots = values.Frame().slots;
    const auto slots = frameSlots.subspan(m_firstSlot.value, names.size());
    SlotsGuard guard(values, slots);
    values.BindSlot(slots[0], MapAdapter(std::shared_ptr<LoopAccessor>(state, &state->accessor)));

    bool isConverted = false;
    auto loopItems = ConvertToList(loopVal, isConverted, false);
    if (!isConverted)
    {
        // The `else` body sees the names outside the loop, as in Jinja2
        guard.Unbind();
        if (m_elseBody)
        {
            values.Nodes()[m_elseBody].Render(os, values);
        }
        return;
    }

    // One scope for the body, emptied after each pass, so `set` in the body stays local to
    // one iteration without a map being made for each. The loop's names are seen by name
    // right below it, from the filter too, which tells that `loop` is not there yet
    auto bodyScope = values.EnterScope(std::move(state->bodyScope));
    values.PushFrameView({ slots, names });

    auto& enumerator = state->enumerator;
    ListAdapter filteredList;
    if (m_ifExpr)
    {
        filteredList = CreateSlottedFilteredAdapter(loopItems, values);
        enumerator = filteredList.GetEnumerator();
    }
    else
    {
        loopItems.RebindEnumerator(enumerator);
        state->listSize = loopItems.GetSize();
    }

    bool loopRendered = false;
    auto& isLast = state->isLast;
    auto moveNext = [&state, &enumerator]() {
        state->isAdvancing = true;
        const bool hasNext = (*enumerator)->MoveNext();
        state->isAdvancing = false;
        return hasNext;
    };
    isLast = !moveNext();
    SlotSink sink{ values, frameSlots };
    const bool isPair = m_target.isTuple && m_target.items.size() == 2 && IsPlainName(m_target.items[0]) && IsPlainName(m_target.items[1]);
    for (size_t itemIdx = 0; !isLast; ++itemIdx)
    {
        state->index0 = itemIdx;
        if (itemIdx == 0)
        {
            state->Item(0) = (*enumerator)->GetCurrent();
        }
        const auto& curValue = state->Item(itemIdx);

        isLast = !moveNext();
        if (!isLast)
        {
            state->Item(itemIdx + 1) = (*enumerator)->GetCurrent();
        }

        if (!m_target.isTuple)
        {
            values.BindSlot(frameSlots[m_target.slot.value], curValue);
        }
        else if (const auto* pair = isPair ? GetIf<KeyValuePair>(&curValue) : nullptr)
        {
            // `for k, v in d|dictsort` (or d.items()): no list of items to unpack through
            values.BindSlot(frameSlots[m_target.items[0].slot.value], TargetString(pair->key));
            values.BindSlot(frameSlots[m_target.items[1].slot.value], pair->value);
        }
        else
        {
            AssignTo(m_target, curValue, sink, values);
        }

        values.Nodes()[m_mainBody].Render(os, values);
        bodyScope.Clear();

        // As in Jinja2, the `else` body is skipped only once a pass through the body has
        // finished without `break` or `continue`
        auto control = values.TakeLoopControl();
        if (control == LoopControl::Break)
        {
            break;
        }
        if (control == LoopControl::None)
        {
            loopRendered = true;
        }
    }

    // A loop object kept past the loop (`set ns.x = loop`) can no longer run the filter,
    // which needs this render context: collect the rest of the items now (more owners than
    // this function and the slot of `loop` mean it was kept)
    if (!state->listSize && state.use_count() > 2)
    {
        state->GetLength();
    }

    guard.Unbind();
    values.PopFrameView();
    values.ExitScope(state->bodyScope);
    if (!loopRendered && m_elseBody)
    {
        values.Nodes()[m_elseBody].Render(os, values);
    }
}

namespace
{
// Gives each target name its index among the names the loop binds
void CollectTargetNames(AssignTarget& target, boost::container::small_vector<SlotName, 4>& names)
{
    if (!target.isTuple)
    {
        const auto hash = HashedName::Hash(target.name);
        const auto found = std::find_if(names.begin(), names.end(), [&target, hash](const SlotName& name) { return name.hash == hash && name.name == target.name; });
        target.slot = SlotIndex{ static_cast<std::uint16_t>(found - names.begin()) };
        if (found == names.end())
        {
            names.push_back({ target.name, hash });
        }
        return;
    }
    for (auto& item : target.items)
    {
        CollectTargetNames(item, names);
    }
}

template<typename Fn>
void ForEachTargetName(const AssignTarget& target, const Fn& fn)
{
    if (!target.isTuple)
    {
        fn(target.name);
        return;
    }
    for (const auto& item : target.items)
    {
        ForEachTargetName(item, fn);
    }
}

void OffsetTargetSlots(AssignTarget& target, SlotIndex first)
{
    if (!target.isTuple)
    {
        target.slot = SlotIndex{ static_cast<std::uint16_t>(first.value + target.slot.value) };
        return;
    }
    for (auto& item : target.items)
    {
        OffsetTargetSlots(item, first);
    }
}
} // namespace

ArenaSpan<SlotName> ForStatement::MakeBinderNames(NodeArena& nodes)
{
    static const SlotName loopName{ "loop", HashedName::Hash("loop") };
    boost::container::small_vector<SlotName, 4> names{ loopName };
    CollectTargetNames(m_target, names);
    m_slotNames = nodes.MakeSpan(names);
    return m_slotNames;
}

void ForStatement::OnRelocated(const ArenaView& nodes) const
{
    const auto names = nodes.Rewrite(m_slotNames);
    if (names.empty())
    {
        return;
    }
    // In the order CollectTargetNames gave them, after `loop`. The old views are not read:
    // the move may have emptied the strings they point into
    std::size_t next = 1;
    ForEachTargetName(m_target, [&names, &next](const std::string& name) {
        if (next < names.size() && names[next].hash == HashedName::Hash(name))
        {
            names[next++].name = name;
        }
    });
    assert(next == names.size());
}

void ForStatement::BindSlots(SlotIndex first, UnitId unit)
{
    m_firstSlot = first;
    m_unit = unit;
    OffsetTargetSlots(m_target, first);
}

uint64_t ForStatement::NewLoopId()
{
    static std::atomic<uint64_t> lastId{ 0 };
    return ++lastId;
}

ListAdapter ForStatement::CreateFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const
{
    // Like the slotted one below, the filter may run while another template's code does
    return ListAdapter::CreateAdapter([eo = loopItems.GetEnumerator(), this, &values, nodes = values.Nodes()]() mutable {
        using ResultType = std::optional<InternalValue>;

        const ArenaSwitch nodesSwitch(values, nodes);
        auto tempContext = values.EnterScope();
        if (!eo.has_value())
        {
            return ResultType();
        }
        auto& e = *eo;
        for (bool finish = !e->MoveNext(); !finish; finish = !e->MoveNext())
        {
            auto curValue = e->GetCurrent();
            try
            {
                AssignTo(m_target, curValue, tempContext, values);
            }
            catch (...)
            {
                values.ExitScope();
                throw;
            }

            if (ConvertToBool(values.Nodes()[m_ifExpr].Evaluate(values)))
            {
                values.ExitScope();
                return ResultType(std::move(curValue));
            }
        }
        values.ExitScope();

        return ResultType();
    });
}

ListAdapter ForStatement::CreateSlottedFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const
{
    // The filter runs whenever the loop moves to its next item, which a macro the body calls
    // can do (`loop.length`) while the macro's frame and tree are installed: it installs the
    // loop's frame again, found by its handle, which fails loudly once the loop's unit call
    // is over, and the loop's tree
    return ListAdapter::CreateAdapter([eo = loopItems.GetEnumerator(), this, &values, handle = values.Frame().handle, nodes = values.Nodes()]() mutable {
        using ResultType = std::optional<InternalValue>;

        if (!eo.has_value())
        {
            return ResultType();
        }
        const ArenaSwitch nodesSwitch(values, nodes);
        const auto frame = RenderWorkspace::ForThisThread().Resolve(handle);
        const auto targets = static_cast<std::uint16_t>(m_slotNames.size() - 1);
        const auto slots = frame.slots.subspan(m_firstSlot.value + 1U + targets, targets);
        // The frame and scope the filter ran in are restored however it ends
        class FilterCall
        {
        public:
            FilterCall(RenderContext& context, const SlotFrame& frame)
                : m_values(context)
                , m_previous(context.InstallFrame(frame))
            {
                m_values.EnterScope();
            }
            FilterCall(const FilterCall&) = delete;
            FilterCall(FilterCall&&) = delete;
            FilterCall& operator=(const FilterCall&) = delete;
            FilterCall& operator=(FilterCall&&) = delete;
            ~FilterCall()
            {
                m_values.ExitScope();
                m_values.InstallFrame(m_previous);
            }

        private:
            RenderContext& m_values;
            SlotFrame m_previous;
        };
        const FilterCall call(values, frame);
        SlotsGuard guard(values, slots);
        values.PushFrameView({ slots, values.Nodes()[m_slotNames].subspan(1) });
        auto leave = [&values, &guard]() {
            guard.Unbind();
            values.PopFrameView();
        };

        SlotSink sink{ values, frame.slots, targets };
        auto& e = *eo;
        for (bool finish = !e->MoveNext(); !finish; finish = !e->MoveNext())
        {
            auto curValue = e->GetCurrent();
            AssignTo(m_target, curValue, sink, values);
            if (ConvertToBool(values.Nodes()[m_ifExpr].Evaluate(values)))
            {
                leave();
                return ResultType(std::move(curValue));
            }
        }
        leave();
        return ResultType();
    });
}

void IfStatement::Render(OutStream& os, RenderContext& values)
{
    InternalValue val = values.Nodes()[m_expr].Evaluate(values);
    bool isTrue = Apply<visitors::BooleanEvaluator>(val);

    if (isTrue)
    {
        values.Nodes()[m_mainBody].Render(os, values);
        return;
    }

    const auto nodes = values.Nodes();
    for (const auto b : nodes[m_elseBranches])
    {
        auto& branch = nodes[b];
        if (branch.ShouldRender(values))
        {
            branch.Render(os, values);
            break;
        }
    }
}

bool ElseBranchStatement::ShouldRender(RenderContext& values) const
{
    if (!m_expr)
    {
        return true;
    }

    return Apply<visitors::BooleanEvaluator>(values.Nodes()[m_expr].Evaluate(values));
}

void ElseBranchStatement::Render(OutStream& os, RenderContext& values)
{
    values.Nodes()[m_mainBody].Render(os, values);
}

void SetLineStatement::Render(OutStream&, RenderContext& values)
{
    if (!m_expr)
    {
        return;
    }
    auto value = values.Nodes()[m_expr].Evaluate(values);
    AssignTo(GetTarget(), std::move(value), values.GetCurrentScope(), values);
}

InternalValue SetBlockStatement::RenderBody(RenderContext& values)
{
    auto innerValues = values.Clone(true);
    TargetString result = RenderToString(values.GetRendererCallback(), [&](OutStream& stream) { values.Nodes()[m_body].Render(stream, innerValues); });
    values.SetLoopControl(innerValues.GetLoopControl());
    return result;
}

void SetRawBlockStatement::Render(OutStream&, RenderContext& values)
{
    // A block set is Markup under autoescape
    auto body = RenderBody(values);
    // A `break` or `continue` in the body leaves the variable unassigned, as in Jinja2
    if (values.HasLoopControl())
    {
        return;
    }
    body.SetMarkup(values.IsAutoescape());
    AssignTo(GetTarget(), std::move(body), values.GetCurrentScope(), values);
}

void SetFilteredBlockStatement::Render(OutStream&, RenderContext& values)
{
    if (!m_expr)
    {
        return;
    }
    auto body = RenderBody(values);
    if (values.HasLoopControl())
    {
        return;
    }
    // Jinja2 wraps the filtered value: Markup(str(result)) under autoescape
    auto result = values.Nodes()[m_expr].Evaluate(body, values);
    if (values.IsAutoescape())
    {
        result = MakeMarkup(result, values.GetRendererCallback());
    }
    AssignTo(GetTarget(), std::move(result), values.GetCurrentScope(), values);
}

namespace
{
bool TemplateAutoescape(RenderContext& values)
{
    auto* callback = values.GetRendererCallback();
    return callback != nullptr && callback->GetSettings().autoescape;
}

// Renders the block at `depth` of the stack for `name` in `blockContext`, the context
// Jinja2 passes to a block function
void RenderBlockAt(const BlocksStack& stack, const std::string& name, size_t depth, OutStream& os, RenderContext& blockContext)
{
    auto p = stack.blocks.find(name);
    if (p == stack.blocks.end() || depth >= p->second.size())
    {
        return;
    }
    const auto& entry = p->second[depth];
    const ArenaSwitch nodesSwitch(blockContext, entry.nodes);
    entry.block->RenderBody(os, blockContext, depth);
}

// Writes to the template's output only until the template extends another one
class TopLevelWriter : public OutStream::StreamWriter
{
public:
    TopLevelWriter(OutStream& os, const TemplateFrame& frame)
        : m_os(os)
        , m_frame(frame)
    {
    }

    void WriteBuffer(const void* ptr, size_t length) override
    {
        if (!m_frame.parent)
        {
            m_os.WriteBuffer(ptr, length);
        }
    }
    void WriteValue(const InternalValue& val) override
    {
        if (!m_frame.parent)
        {
            m_os.WriteValue(val);
        }
    }

private:
    OutStream& m_os;
    const TemplateFrame& m_frame;
};

class TemplateFrameGuard
{
public:
    TemplateFrameGuard(RenderContext& values, TemplateFrame* frame)
        : m_values(values)
        , m_prevFrame(values.SetTemplateFrame(frame))
    {
        frame->outer = m_prevFrame;
    }
    ~TemplateFrameGuard() { m_values.SetTemplateFrame(m_prevFrame); }

    TemplateFrameGuard(const TemplateFrameGuard&) = delete;
    TemplateFrameGuard& operator=(const TemplateFrameGuard&) = delete;

private:
    RenderContext& m_values;
    TemplateFrame* m_prevFrame;
};

// `self` for the blocks of `stack`: a callable per block. A call renders the block in the
// template `self` came from while that template runs (the importer, for `self` passed to an
// imported macro), else in the template running then. The stack is only compared, never read,
// since `self` may outlive it
InternalValue MakeTemplateSelf(const BlocksStack& stack)
{
    InternalValueMap self;
    for (const auto& block : stack.blocks)
    {
        const auto& name = block.first;
        self[name] = MakeWrapped(Callable(Callable::Macro, [name, owner = &stack](const CallParams&, OutStream& stream, RenderContext& context) {
            auto* curFrame = context.GetTemplateFrame();
            if (!curFrame || !curFrame->blocks)
            {
                return;
            }
            auto* frame = curFrame;
            for (auto* f = curFrame; f; f = f->outer)
            {
                if (f->blocks == owner)
                {
                    frame = f;
                    break;
                }
            }
            RenderContext blockContext(context, frame->baseDepth);
            blockContext.SetTemplateFrame(frame);
            RenderBlockAt(*frame->blocks, name, 0, stream, blockContext);
        }));
    }
    return CreateMapAdapter(std::move(self));
}
} // namespace

LookupResult RenderContext::FindSelf(const std::string& name)
{
    auto* frame = m_templateFrame;
    if (!frame || !frame->blocks || frame->baseDepth == 0)
    {
        return FindValue(name);
    }
    // The template's base scope is its own: a `set self` at its top level wins
    if (const auto value = FindInScopesFrom(name, frame->baseDepth - 1))
    {
        return value;
    }
    if (!frame->self)
    {
        frame->self = MakeTemplateSelf(*frame->blocks);
    }
    return LookupResult(*frame->self);
}

void BlockStatement::Render(OutStream& os, RenderContext& values)
{
    auto* frame = values.GetTemplateFrame();
    if (!frame || !frame->blocks)
    {
        RenderContext innerContext = values.Clone(true);
        innerContext.EnterScope();
        const UnitCall unitCall(innerContext, m_unitLayout);
        values.Nodes()[m_mainBody].Render(os, innerContext);
        return;
    }

    // A block after `extends` is only a definition: the parent decides where it goes
    if (frame->parent)
    {
        return;
    }

    auto p = frame->blocks->blocks.find(m_name);
    if (m_isRequired && (p == frame->blocks->blocks.end() || p->second.size() <= 1))
    {
        throw std::runtime_error("Required block '" + m_name + "' not found");
    }

    // An unscoped block sees the template-level names only, not the loop variables or
    // other locals around it
    RenderContext blockContext = m_isScoped ? RenderContext(values, values.GetScopesCount()) : RenderContext(values, frame->baseDepth);
    RenderBlockAt(*frame->blocks, m_name, 0, os, blockContext);
}

void BlockStatement::RenderBody(OutStream& os, RenderContext& values, size_t depth) const
{
    const RenderDepthGuard depthGuard;
    const UnitCall unitCall(values, m_unitLayout);
    auto* frame = values.GetTemplateFrame();
    auto baseDepth = values.GetScopesCount();
    auto scope = values.EnterScope();
    if (frame && frame->blocks)
    {
        auto* stack = frame->blocks;
        auto p = stack->blocks.find(m_name);
        if (p != stack->blocks.end() && depth + 1 < p->second.size())
        {
            scope["super"] = Callable(Callable::Macro, [stack, this, depth, baseDepth](const CallParams&, OutStream& stream, RenderContext& context) {
                RenderContext superContext(context, baseDepth);
                RenderBlockAt(*stack, m_name, depth + 1, stream, superContext);
            });
        }
        else
        {
            scope["super"] = Callable(Callable::Macro, [this](const CallParams&, OutStream&, RenderContext&) {
                throw std::runtime_error("there is no parent block called '" + m_name + "'.");
            });
        }
    }
    // A block body escapes as its template does, whatever `{% autoescape %}` surrounds it
    AutoescapeGuard autoescapeGuard(values, TemplateAutoescape(values));
    values.Nodes()[m_mainBody].Render(os, values);
    values.ExitScope();
}

void TemplateRenderer::PushBlocks(const ArenaView& nodes, BlocksStack& stack) const
{
    for (const auto& [name, block] : m_blocks)
    {
        stack.blocks[name].push_back({ nodes, &nodes[block] });
    }
}

void TemplateRenderer::Render(OutStream& os, RenderContext& values)
{
    BlocksStack stack;
    PushBlocks(values.Nodes(), stack);
    RenderBody(os, values, stack);
}

void TemplateRenderer::RenderAsParent(OutStream& os, RenderContext& values)
{
    auto* frame = values.GetTemplateFrame();
    if (!frame || !frame->blocks)
    {
        Render(os, values);
        return;
    }
    // The parent shares the child's top-level scope, but a `self` the child set there is not
    // the parent's: in Jinja2 `self` is a local of each template's root function
    values.GetCurrentScope().Erase("self");
    auto& stack = *frame->blocks;
    PushBlocks(values.Nodes(), stack);
    RenderBody(os, values, stack);
}

void TemplateRenderer::RenderBody(OutStream& os, RenderContext& values, BlocksStack& stack)
{
    const RenderDepthGuard depthGuard;
    const UnitCall unitCall(values, m_unitLayout);
    // Included, imported and parent templates use the environment's autoescape setting
    AutoescapeGuard autoescapeGuard(values, TemplateAutoescape(values));
    TemplateFrame frame;
    frame.blocks = &stack;
    frame.baseDepth = values.GetScopesCount();
    TemplateFrameGuard frameGuard(values, &frame);


    if (!m_hasExtends)
    {
        values.Nodes()[m_body].Render(os, values);
        return;
    }

    TopLevelWriter writer(os, frame);
    OutStream topLevelStream(&writer);
    values.Nodes()[m_body].Render(topLevelStream, values);

    if (frame.parent)
    {
        auto parent = frame.parent;
        stack.parents.push_back(parent);
        parent->Render(os, values);
    }
}

template<typename Result, typename Fn>
struct TemplateImplVisitor
{
    // ExtendsStatement::BlocksCollection* m_blocks;
    const Fn& m_fn;
    bool m_throwError{};

    explicit TemplateImplVisitor(const Fn& fn, bool throwError)
        : m_fn(fn)
        , m_throwError(throwError)
    {
    }

    // By reference: the result lives in the render's table of loaded templates, and copying the pointer would
    // bump a reference count every thread rendering the same template shares
    template<typename CharT>
    Result operator()(const nonstd::expected<std::shared_ptr<TemplateImpl<CharT>>, BasicErrorInfo<CharT>>& tpl) const
    {
        if (!m_throwError && !tpl)
        {
            return Result{};
        }
        if (!tpl)
        {
            throw BasicErrorInfo<CharT>(tpl.error()); // NOLINT(bugprone-exception-copy-constructor-throws)
        }
        return m_fn(tpl.value());
    }

    Result operator()(EmptyValue) const { return Result(); }
};

template<typename Result, typename Fn, typename Arg>
Result VisitTemplateImpl(Arg&& tpl, bool throwError, const Fn& fn)
{
    return visit(TemplateImplVisitor<Result, Fn>(fn, throwError), std::forward<Arg>(tpl));
}

template<template<typename T> class RendererTpl, typename CharT, typename... Args>
auto CreateTemplateRenderer(const std::shared_ptr<TemplateImpl<CharT>>& tpl, Args&&... args)
{
    return std::make_shared<RendererTpl<CharT>>(tpl, std::forward<Args>(args)...);
}

// The template an `extends` names; keeps it alive while it renders
template<typename CharT>
class ParentTemplateRenderer : public IRendererBase
{
public:
    explicit ParentTemplateRenderer(std::shared_ptr<TemplateImpl<CharT>> tpl)
        : m_template(std::move(std::move(tpl)))
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        const auto nodes = m_template->Nodes();
        const ArenaSwitch nodesSwitch(values, nodes);
        nodes[m_template->GetRenderer()].RenderAsParent(os, values);
    }

private:
    std::shared_ptr<TemplateImpl<CharT>> m_template;
};

void ExtendsStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    auto* frame = values.GetTemplateFrame();
    if (!frame)
    {
        return;
    }
    if (frame->parent)
    {
        throw std::runtime_error("extended multiple times");
    }

    auto name = values.Nodes()[m_templateExpr].Evaluate(values);
    const auto& tpl = values.GetRendererCallback()->LoadTemplate(name);
    frame->parent = VisitTemplateImpl<RendererPtr>(tpl, true, [](const auto& tplPtr) { return CreateTemplateRenderer<ParentTemplateRenderer>(tplPtr); });
}

template<typename CharT>
class IncludedTemplateRenderer : public IRendererBase
{
public:
    // `exportNames`: copy the names the template sets at its top level into the caller's
    // current scope. Import collects a module this way; include must not leak them
    IncludedTemplateRenderer(std::shared_ptr<TemplateImpl<CharT>> tpl, bool withContext, bool exportNames)
        : m_template(std::move(tpl))
        , m_withContext(withContext)
        , m_exportNames(exportNames)
    {
    }

    void Render(OutStream& os, RenderContext& values) override { Render(*m_template, m_withContext, m_exportNames, os, values); }

    // Renders `tpl` the way an instance holding it would; `include` calls it directly, without an instance to allocate
    static void Render(const TemplateImpl<CharT>& tpl, bool withContext, bool exportNames, OutStream& os, RenderContext& values)
    {
        RenderContext innerContext = values.Clone(withContext);
        if (withContext)
        {
            innerContext.EnterScope();
        }

        innerContext.SetNodes(tpl.Nodes());
        innerContext.Nodes()[tpl.GetRenderer()].Render(os, innerContext);
        if (withContext && exportNames)
        {
            auto innerScope = innerContext.TakeCurrentScope();
            auto scope = values.GetCurrentScope();
            for (auto& [name, value] : innerScope)
            {
                if (name != "self")
                {
                    scope[name] = std::move(value);
                }
            }
        }
    }

private:
    std::shared_ptr<TemplateImpl<CharT>> m_template;
    bool m_withContext{};
    bool m_exportNames{};
};

void IncludeStatement::Render(OutStream& os, RenderContext& values)
{
    auto templateNames = values.Nodes()[m_expr].Evaluate(values);
    bool isConverted = false;
    ListAdapter list = ConvertToList(templateNames, isConverted);

    auto doRender = [this, &values, &os](auto&& name) -> bool {
        const auto& tpl = values.GetRendererCallback()->LoadTemplate(name);

        try
        {
            return VisitTemplateImpl<bool>(tpl, true, [this, &values, &os](const auto& tplPtr) {
                using CharT = typename std::decay_t<decltype(*tplPtr)>::CharType;
                IncludedTemplateRenderer<CharT>::Render(*tplPtr, m_withContext, false, os, values);
                return true;
            });
        }
        catch (const BasicErrorInfo<char>& err)
        {
            if (err.GetCode() != ErrorCode::FileNotFound)
            {
                throw;
            }
        }
        catch (const BasicErrorInfo<wchar_t>& err)
        {
            if (err.GetCode() != ErrorCode::FileNotFound)
            {
                throw;
            }
        }

        return false;
    };

    bool rendered = false;
    if (isConverted)
    {
        for (const auto& name : list)
        {
            rendered = doRender(name);
            if (rendered)
            {
                break;
            }
        }
    }
    else
    {
        rendered = doRender(templateNames);
    }

    if (!rendered && !m_ignoreMissing)
    {
        InternalValueList files;
        ValuesList extraParams;
        if (isConverted)
        {
            extraParams.push_back(IntValue2Value(templateNames));
        }
        else
        {
            files.push_back(templateNames);
            extraParams.push_back(IntValue2Value(ListAdapter::CreateAdapter(std::move(files))));
        }

        values.GetRendererCallback()->ThrowRuntimeError(ErrorCode::TemplateNotFound, std::move(extraParams));
    }
}

class ImportedMacroRenderer : public IRendererBase
{
public:
    // `module` owns the statements behind the imported macros: it must outlive them even
    // when the environment does not cache the template
    ImportedMacroRenderer(InternalValueMap&& map, bool withContext, RendererPtr module, BlocksStack&& moduleBlocks)
        : m_importedContext(std::move(map))
        , m_withContext(withContext)
        , m_module(std::move(module))
        , m_moduleBlocks(std::move(moduleBlocks))
    {
    }

    void Render(OutStream& /*os*/, RenderContext& /*values*/) override {}

    void InvokeMacro(const Callable& callable, const CallParams& params, OutStream& stream, RenderContext& context)
    {
        auto ctx = context.Clone(m_withContext);
        ctx.BindScope(&m_importedContext);
        // The macro runs in the template that defines it: its `self` is that template
        TemplateFrame frame;
        frame.blocks = &m_moduleBlocks;
        frame.baseDepth = ctx.GetScopesCount();
        const TemplateFrameGuard frameGuard(ctx, &frame);
        callable.GetStatementCallable()(params, stream, ctx);
    }

    static void InvokeMacro(const std::string& contextName, const Callable& callable, const CallParams& params, OutStream& stream, RenderContext& context)
    {
        const auto contextVal = context.FindValue(contextName);
        if (!contextVal)
        {
            return;
        }

        const auto* rendererPtr = GetIf<RendererPtr>(&*contextVal);
        if (!rendererPtr)
        {
            return;
        }

        auto* renderer = static_cast<ImportedMacroRenderer*>(rendererPtr->get());
        renderer->InvokeMacro(callable, params, stream, context);
    }

private:
    InternalValueMap m_importedContext;
    bool m_withContext{};
    RendererPtr m_module;
    // The blocks of the module, which `m_module` keeps alive
    BlocksStack m_moduleBlocks;
};

void ImportStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    auto name = values.Nodes()[m_nameExpr].Evaluate(values);

    // Resolved on every render: the name may change between renders or loop iterations
    const auto& tpl = values.GetRendererCallback()->LoadTemplate(name);
    auto renderer =
        VisitTemplateImpl<RendererPtr>(tpl, true, [](const auto& tplPtr) { return CreateTemplateRenderer<IncludedTemplateRenderer>(tplPtr, true, true); });
    if (!renderer)
    {
        return;
    }

    std::string scopeName;
    {
        TargetString tsScopeName = values.GetRendererCallback()->GetAsTargetString(name);
        scopeName = "$$_imported_" + GetAsSameString(scopeName, tsScopeName).value_or(std::string());
    }

    RenderContext newContext = values.Clone(m_withContext);
    newContext.EnterScope();
    // Only the names the template defines are imported, its output is dropped
    RenderToString(values.GetRendererCallback(), [&](OutStream& stream) { renderer->Render(stream, newContext); });
    InternalValueMap importedScope = newContext.TakeCurrentScope();

    BlocksStack moduleBlocks;
    VisitTemplateImpl<bool>(tpl, true, [&moduleBlocks](const auto& tplPtr) {
        tplPtr->Nodes()[tplPtr->GetRenderer()].PushBlocks(tplPtr->Nodes(), moduleBlocks);
        return true;
    });

    ImportNames(values, importedScope, scopeName);
    values.GetCurrentScope()[scopeName] = std::static_pointer_cast<IRendererBase>(
        std::make_shared<ImportedMacroRenderer>(std::move(importedScope), m_withContext, renderer, std::move(moduleBlocks)));
}

// Copies: the module keeps every name, which its own macros read through the bound scope
void ImportStatement::ImportNames(RenderContext& values, const InternalValueMap& importedScope, const std::string& scopeName) const
{
    InternalValueMap importedNs;

    for (const auto& [name, value] : importedScope)
    {
        if (name.empty())
        {
            continue;
        }

        if (name[0] == '_')
        {
            continue;
        }

        auto mappedP = m_namesToImport.find(name);
        if (!m_namespace && mappedP == m_namesToImport.end())
        {
            continue;
        }

        InternalValue imported;
        const auto* callable = GetIf<Callable>(&value);
        if (!callable)
        {
            imported = value;
        }
        else if (callable->GetKind() == Callable::Macro)
        {
            auto attributes = callable->GetAttributes();
            Callable wrapper(Callable::Macro, [fn = *callable, scopeName](const CallParams& params, OutStream& stream, RenderContext& context) {
                ImportedMacroRenderer::InvokeMacro(scopeName, fn, params, stream, context);
            });
            wrapper.SetAttributes(std::move(attributes));
            imported = std::move(wrapper);
        }
        else
        {
            continue;
        }

        if (m_namespace)
        {
            importedNs[name] = std::move(imported);
        }
        else
        {
            values.GetCurrentScope()[mappedP->second] = std::move(imported);
        }
    }

    if (m_namespace)
    {
        values.GetCurrentScope()[m_namespace.value()] = CreateMapAdapter(std::move(importedNs));
    }
}

Callable MacroStatement::MakeCallable(RenderContext& values) const
{
    std::vector<InternalValue> definedDefaults(m_params.size());
    for (std::size_t idx = 0; idx < m_params.size(); ++idx)
    {
        const auto& p = m_params[idx];
        if (p.defaultValue && !p.defaultRefersToArgs)
        {
            definedDefaults[idx] = values.Nodes()[p.defaultValue].Evaluate(values);
        }
    }

    // The body escapes as where the macro is defined; the caller decides whether the result is Markup
    Callable result(Callable::Macro,
                    [this, defaults = std::move(definedDefaults), nodes = values.Nodes(), autoescape = values.IsAutoescape()](
                        const CallParams& callParams, OutStream& stream, RenderContext& context) {
                        // A macro called from another template (imported, or passed as `caller`) runs in its own tree
                        const ArenaSwitch nodesSwitch(context, nodes);
                        AutoescapeGuard autoescapeGuard(context, autoescape);
                        InvokeMacroRenderer(defaults, callParams, stream, context);
                    });
    result.SetAttributes(m_attributes);
    return result;
}

void MacroStatement::Render(OutStream&, RenderContext& values)
{
    values.GetCurrentScope()[m_name] = MakeCallable(values);
}

InternalValue MacroStatement::GetMacroName() const
{
    return InternalValue(m_name);
}

std::string MacroStatement::GetDisplayName() const
{
    return IsEmpty(GetMacroName()) ? "None"s : "'" + m_name + "'";
}

std::shared_ptr<const InternalValueMap> MacroStatement::MakeAttributes() const
{
    InternalValueList arguments;
    for (const auto& p : m_params)
    {
        arguments.emplace_back(p.paramName);
    }

    auto attributes = std::make_shared<InternalValueMap>();
    (*attributes)["name"s] = GetMacroName();
    (*attributes)["arguments"s] = ListAdapter::CreateAdapter(std::move(arguments));
    const auto caught = GetCaughtNames();
    (*attributes)["catch_kwargs"s] = InternalValue((caught & UsesKwargs) != 0);
    (*attributes)["catch_varargs"s] = InternalValue((caught & UsesVarargs) != 0);
    (*attributes)["caller"s] = InternalValue((m_specialNames & UsesCaller) != 0);
    return attributes;
}

unsigned MacroStatement::GetCaughtNames() const
{
    auto names = m_specialNames;
    for (const auto& p : m_params)
    {
        if (p.paramName == "caller")
        {
            names &= ~UsesCaller;
        }
        else if (p.paramName == "varargs")
        {
            names &= ~UsesVarargs;
        }
        else if (p.paramName == "kwargs")
        {
            names &= ~UsesKwargs;
        }
    }
    return names;
}

namespace
{
// The arguments of one macro call, read in place rather than copied first: a keyword
// argument binds a parameter that no positional argument filled, the others are extra (kwargs)
struct MacroArgBinder
{
    const MacroParams& params;
    const CallParams& callParams;
    bool catchCaller = false;

    [[nodiscard]] bool IsBoundKeyword(const std::string& name) const
    {
        for (auto idx = callParams.posParams.size(); idx < params.size(); ++idx)
        {
            if (params[idx].paramName == name)
            {
                return true;
            }
        }
        return false;
    }
    [[nodiscard]] bool IsExtraKeyword(const std::string& name) const { return !IsBoundKeyword(name) && !(catchCaller && name == "caller"); }
    [[nodiscard]] bool IsProvided(std::size_t idx) const
    {
        return idx < callParams.posParams.size() || callParams.kwParams.find(params[idx].paramName) != callParams.kwParams.end();
    }
};

// Throws for extra arguments the macro does not catch; displayName() names the macro
template<typename DisplayName>
void CheckMacroCallArgs(const MacroArgBinder& binder, bool catchKwargs, bool catchVarargs, const DisplayName& displayName)
{
    const auto& posParams = binder.callParams.posParams;
    const auto& kwParams = binder.callParams.kwParams;
    const auto argsCount = binder.params.size();
    if (!catchKwargs)
    {
        for (const auto& [name, value] : kwParams)
        {
            if (!binder.IsExtraKeyword(name))
            {
                continue;
            }
            if (kwParams.find("caller"s) != kwParams.end() && binder.IsExtraKeyword("caller"s))
            {
                throw std::runtime_error("macro " + displayName() + " was invoked with two values for the special caller argument. This is most likely a bug.");
            }
            throw std::runtime_error("macro " + displayName() + " takes no keyword argument '" + name + "'");
        }
    }

    if (!catchVarargs && posParams.size() > argsCount)
    {
        throw std::runtime_error("macro " + displayName() + " takes not more than " + std::to_string(argsCount) + " argument(s)");
    }
}

// Binds the given arguments, and the missing ones as undefined unless their default is bound
// later without seeing the other arguments
void BindMacroArgs(const MacroArgBinder& binder, bool hasArgDefaults, const RenderContext& context, ScopeRef& scope)
{
    const auto& posParams = binder.callParams.posParams;
    const auto& kwParams = binder.callParams.kwParams;
    for (std::size_t idx = 0; idx < binder.params.size(); ++idx)
    {
        const auto& name = binder.params[idx].paramName;
        if (idx < posParams.size())
        {
            scope[name] = posParams[idx];
            continue;
        }
        auto p = kwParams.find(name);
        if (p != kwParams.end())
        {
            scope[name] = p->second;
            continue;
        }
        if (binder.params[idx].defaultValue && !hasArgDefaults)
        {
            continue;
        }
        scope[name] = MakeUndefinedWithHint(context, "parameter '" + name + "' was not provided");
    }
}

// Binds caller, kwargs and varargs for a macro that uses them
void BindSpecialMacroArgs(const MacroArgBinder& binder, bool catchKwargs, bool catchVarargs, ScopeRef& scope)
{
    const auto& posParams = binder.callParams.posParams;
    const auto& kwParams = binder.callParams.kwParams;
    if (binder.catchCaller)
    {
        auto p = kwParams.find("caller"s);
        scope["caller"s] = p != kwParams.end() ? p->second : InternalValue();
    }
    if (catchKwargs)
    {
        InternalDict kwArgs;
        for (const auto& [name, value] : kwParams)
        {
            if (binder.IsExtraKeyword(name))
            {
                kwArgs[name] = value;
            }
        }
        scope["kwargs"s] = CreateMapAdapter(std::move(kwArgs));
    }
    if (catchVarargs)
    {
        InternalValueList varArgs;
        for (auto idx = binder.params.size(); idx < posParams.size(); ++idx)
        {
            varArgs.push_back(posParams[idx]);
        }
        scope["varargs"s] = ListAdapter::CreateAdapter(std::move(varArgs)).MarkAsTuple();
    }
}

// Binds the defaults of the arguments that were not provided
void BindMacroDefaults(const MacroArgBinder& binder, const std::vector<InternalValue>& definedDefaults, RenderContext& context, ScopeRef& scope)
{
    for (std::size_t idx = 0; idx < binder.params.size(); ++idx)
    {
        const auto& p = binder.params[idx];
        if (!p.defaultValue || binder.IsProvided(idx))
        {
            continue;
        }

        auto value = p.defaultRefersToArgs ? context.Nodes()[p.defaultValue].Evaluate(context) : definedDefaults[idx];
        // Jinja2 evaluates defaults on every call, so acc=[] is a new list each time; the
        // template's lists and dicts are shared, so the stored one is copied
        if (methods::IsMutable(value))
        {
            value = methods::CopyContainer(value);
        }
        scope[p.paramName] = std::move(value);
    }
}
} // namespace

// Binds the call arguments the way Jinja2's Macro.__call__ does
void MacroStatement::InvokeMacroRenderer(const std::vector<InternalValue>& definedDefaults,
                                         const CallParams& callParams,
                                         OutStream& stream,
                                         RenderContext& context) const
{
    const RenderDepthGuard depthGuard;

    const auto caught = GetCaughtNames();
    const bool catchCaller = (caught & UsesCaller) != 0;
    const bool catchKwargs = (caught & UsesKwargs) != 0;
    const bool catchVarargs = (caught & UsesVarargs) != 0;

    const MacroArgBinder binder{ m_params, callParams, catchCaller };
    CheckMacroCallArgs(binder, catchKwargs, catchVarargs, [this]() { return GetDisplayName(); });

    // Missing arguments and the special ones are bound before the defaults are evaluated, so
    // a default sees them and never an outer variable named like a later argument. When no
    // default refers to the arguments nothing is evaluated in between, and a missing argument
    // with a default gets only the default.
    const bool hasArgDefaults = std::any_of(m_params.begin(), m_params.end(), [](const auto& p) { return p.defaultValue && p.defaultRefersToArgs; });
    auto scope = context.EnterScope();
    BindMacroArgs(binder, hasArgDefaults, context, scope);
    BindSpecialMacroArgs(binder, catchKwargs, catchVarargs, scope);
    BindMacroDefaults(binder, definedDefaults, context, scope);

    const UnitCall unitCall(context, m_unitLayout);
    context.Nodes()[m_mainBody].Render(stream, context);

    context.ExitScope();
}

void MacroCallStatement::Render(OutStream& os, RenderContext& values)
{
    const auto macroVal = values.FindValue(m_macroName);
    if (!macroVal)
    {
        return;
    }

    const auto& fnVal = *macroVal;
    const auto* callable = GetIf<Callable>(&fnVal);
    if (!callable || callable->GetType() == Callable::Type::Expression)
    {
        return;
    }

    auto callParams = helpers::EvaluateCallParams(m_callParams, values);
    callParams.kwParams["caller"s] = MakeCallable(values);
    callable->GetStatementCallable()(callParams, os, values);
}

InternalValue MacroCallStatement::GetMacroName() const
{
    return InternalValue();
}

void DoStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    values.Nodes()[m_expr].Evaluate(values);
}

void WithStatement::Render(OutStream& os, RenderContext& values)
{
    auto innerValues = values.Clone(true);
    auto scope = innerValues.EnterScope();

    for (auto& [name, expr] : m_scopeVars)
    {
        scope[name] = values.Nodes()[expr].Evaluate(values);
    }

    values.Nodes()[m_mainBody].Render(os, innerValues);

    innerValues.ExitScope();
    values.SetLoopControl(innerValues.GetLoopControl());
}

void TransStatement::Render(OutStream& os, RenderContext& values)
{
    std::vector<InternalValue> evaluated;
    evaluated.reserve(m_variables.size());
    for (auto& var : m_variables)
    {
        evaluated.push_back(values.Nodes()[var.second].Evaluate(values));
    }

    auto scope = values.EnterScope();
    for (size_t idx = 0; idx < evaluated.size(); ++idx)
    {
        scope[VariableSlot(idx)] = std::move(evaluated[idx]);
    }
    values.Nodes()[m_output].Render(os, values);
    values.ExitScope();
}

void FilterStatement::Render(OutStream& os, RenderContext& values)
{
    auto innerValues = values.Clone(true);
    TargetString arg = RenderToString(values.GetRendererCallback(), [&](OutStream& stream) { values.Nodes()[m_body].Render(stream, innerValues); });
    // A `break` or `continue` in the body drops its output, as in Jinja2
    values.SetLoopControl(innerValues.GetLoopControl());
    if (values.HasLoopControl())
    {
        return;
    }
    // The body is Markup under autoescape; the filtered output is written as is
    InternalValue body(std::move(arg));
    body.SetMarkup(values.IsAutoescape());
    const auto result = values.Nodes()[m_expr].Evaluate(body, values);
    os.WriteValue(result);
}

void AutoescapeStatement::Render(OutStream& os, RenderContext& values)
{
    AutoescapeGuard autoescapeGuard(values, ConvertToBool(values.Nodes()[m_expr].Evaluate(values)));
    values.EnterScope();
    values.Nodes()[m_body].Render(os, values);
    values.ExitScope();
}
} // namespace jinja2
