#include "statements.h"

#include "cycle_collector.h"
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
#include "template_slots.h"
#include "undefined.h"
#include "value_methods.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>

#include <boost/container/small_vector.hpp>
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
#include <string_view>
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
// A loop over a value that is not a list: Python iterates an undefined value as empty
// (StrictUndefined refuses) and fails on anything else (`for x in none`)
void CheckIterable(const InternalValue& value)
{
    if (value.IsUndefined())
    {
        CheckUndefinedUse(value, UndefinedUse::Operator);
        return;
    }
    throw std::runtime_error("'"s + Apply<visitors::PythonTypeNameGetter>(value) + "' object is not iterable");
}


// A target as the tree holds it: its root, then its items
using TargetNodes = boost::span<const TargetNode>;

// The name a target node binds, with its hash, as the tree `nodes` holds it
HashedName NameOf(const TargetNode& target, const ArenaView& nodes)
{
    return { nodes.Text(target.name), target.hash };
}

// Calls fn(item) for each item of the tuple `target`, each with its own items
template<typename Fn>
void ForEachItem(TargetNodes target, const Fn& fn)
{
    std::size_t at = 1;
    for (std::uint32_t idx = 0; idx != target[0].count; ++idx)
    {
        const auto item = target.subspan(at, target[at].size);
        fn(item);
        at += item.size();
    }
}

// A tuple of two plain names (`for k, v in ...`): its items are the two nodes after it
bool IsPairOfNames(TargetNodes target)
{
    return target[0].isTuple && target[0].count == 2 && target[1].IsPlainName() && target[2].IsPlainName();
}

// Where an assignment puts the names it binds: a scope, by name
struct ScopeSink
{
    ScopeRef scope;

    void Set(const TargetNode& target, InternalValue value, RenderContext& values) { scope.ForName(NameOf(target, values.Nodes())) = std::move(value); }
};

// The slots of the names a loop binds, by each target name's place among them; `shift`
// moves past the loop's own slots to those of its filter
struct SlotSink
{
    // The loop's slots, from that of `loop`
    boost::span<Slot> slots;
    std::uint16_t shift = 0;

    void Set(const TargetNode& target, InternalValue value, RenderContext& values) const { values.BindSlot(slots[target.slot.value + shift], target.hash, std::move(value)); }
};

// Python's assignment to a target: a name takes the value; a tuple `a, (b, c)` iterates
// the value, which must yield exactly as many items as the tuple has targets, and assigns
// them in turn, so a dict gives its keys. A host object (a reflected struct or another
// user-provided map, which Python could not unpack) is the exception: a tuple of names
// takes its values by name (`set first, last = person`), as Jinja2C++ always has
template<typename Sink>
void AssignTo(TargetNodes target, InternalValue value, Sink& sink, RenderContext& values)
{
    const auto& root = target[0];
    const auto& nodes = values.Nodes();
    if (!root.attr.empty())
    {
        // `set ns.attr = ...` changes a namespace() object wherever it is defined
        const auto found = values.FindValue(NameOf(root, nodes));
        const auto ns = found ? AsMap(*found) : std::nullopt;
        if (!ns || !ns->IsNamespace())
        {
            throw std::runtime_error("cannot assign attribute on non-namespace object");
        }
        // The store may close a reference cycle, which the render frees when it ends. Unlike
        // the list and dict methods it walks nothing: `set ns.v = [ns.v]` in a loop would make
        // that quadratic (fuzz/regressions/deep-nested-value.j2)
        if (value.Kind() == ValueKind::List || value.Kind() == ValueKind::Map)
        {
            CycleCollector::NoteStore(*found);
        }
        ns->SetValue(std::string(nodes.Text(root.attr)), value);
        return;
    }
    if (!root.isTuple)
    {
        sink.Set(root, std::move(value), values);
        return;
    }

    const std::size_t count = root.count;
    // The items of a tuple of names are the nodes after it
    const bool allNames = root.size == count + 1 && std::all_of(target.begin() + 1, target.end(), [](const TargetNode& t) { return t.IsPlainName(); });
    const auto map = AsMap(value);
    // An object (namespace(), cycler(), joiner()) is neither a mapping nor iterable
    if (map && map->GetObjectClass())
    {
        throw std::runtime_error("cannot unpack non-iterable "s + map->GetObjectClass() + " object");
    }
    if (allNames && map && map->HasAttributes())
    {
        for (const auto& t : target.subspan(1))
        {
            sink.Set(t, Subscript(value, std::string(nodes.Text(t.name)), &values), values);
        }
        return;
    }

    InternalValueList items;
    if (auto* pair = GetIf<KeyValuePair>(&value))
    {
        items.push_back(pair->key);
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
            if (items.size() > count)
            {
                break;
            }
        }
    }

    if (items.size() > count)
    {
        throw std::runtime_error("too many values to unpack (expected " + std::to_string(count) + ")");
    }
    if (items.size() < count)
    {
        throw std::runtime_error("not enough values to unpack (expected " + std::to_string(count) + ", got " + std::to_string(items.size()) + ")");
    }

    std::size_t idx = 0;
    ForEachItem(target, [&items, &idx, &sink, &values](TargetNodes item) { AssignTo(item, std::move(items[idx++]), sink, values); });
}

void AssignTo(TargetNodes target, InternalValue value, ScopeRef scope, RenderContext& values)
{
    if (target[0].IsPlainName())
    {
        scope.ForName(NameOf(target[0], values.Nodes())) = std::move(value);
        return;
    }
    ScopeSink sink{ scope };
    AssignTo(target, std::move(value), sink, values);
}

} // namespace
namespace
{
// Where a loop stores its target names, found on the first item: the map keeps its
// nodes in place, so the slots survive other names being added
struct LoopTargetSlots
{
    InternalValue* single = nullptr;
    std::vector<InternalValue*> items;
};

// The state behind a loop object. The template can keep the object past the loop
// (`set ns.x = loop`), so it is shared with the loop object: the object's value owns it
struct LoopState : ValueObject
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
    // Where its outermost level runs, and so every level (MakeLoopRecursion)
    ClosureSite recursiveSite;
    // The output autoescape setting where the recursive loop is written: its levels escape
    // by it, and their results are Markup by it, wherever loop(...) is called
    bool recursiveAutoescape = false;
    // The arguments of the last loop.changed() call, made on first use
    std::shared_ptr<std::optional<InternalValueList>> lastChanged;
    // index0 for the callables loop.cycle made, kept up to date once one is made. They
    // share this, not the state: a loop object kept past the loop collects the rest of a
    // filtered loop, which a callable that cycles needs not
    std::shared_ptr<size_t> cyclePosition;
    // Whether the item after the current one is fetched, isLast telling if there is one
    bool isPeeked = false;
    // A filtered loop over slots fetches its next item lazily, as Jinja2's LoopContext:
    // when the body is done with the current one, or earlier when the body reads `last`,
    // `nextitem`, `length` or `revindex` (docs/tasks/0091, 0154). So a namespace the body
    // writes reaches the filter of the next item. The filter then runs from wherever the
    // read was made, in the loop's frame and tree found again. Null for other loops,
    // which fetch the next item before the body runs
    const ForStatement* lazyFilter = nullptr;
    RenderContext* lazyContext = nullptr;
    // The scopes of lazyContext the filter sees: those below the body's
    size_t lazyDepth = 0;
    FrameHandle lazyFrame;
    ArenaView lazyNodes;

    InternalValue& Item(size_t idx) { return items[idx % items.size()]; }

    // The cell loop.cycle callables read the position from, made on first use
    std::shared_ptr<size_t> CyclePosition()
    {
        if (!cyclePosition)
        {
            cyclePosition = std::make_shared<size_t>(index0);
        }
        return cyclePosition;
    }

    void SetIndex(size_t idx)
    {
        index0 = idx;
        if (cyclePosition)
        {
            *cyclePosition = idx;
        }
    }

    // Fetches the item after the current one, if not done yet
    void Peek()
    {
        if (!isPeeked)
        {
            PeekNow();
        }
    }
    JINJA2CPP_NOINLINE_INLINE void PeekNow()
    {
        // Peeking from inside the filter would run the filter that is running.
        // Jinja2 has no `loop` in the filter at all
        if (isAdvancing)
        {
            throw std::runtime_error("'loop' is undefined in the loop filter");
        }
        isLast = !FetchNext(Item(index0 + 1));
        isPeeked = true;
    }

    // Moves the enumerator to the next item and stores it in `item`; false at the end
    JINJA2CPP_NOINLINE_INLINE bool FetchNext(InternalValue& item)
    {
        if (!enumerator)
        {
            return false;
        }
        isAdvancing = true;
        try
        {
            const bool hasNext = lazyFilter ? lazyFilter->FetchFiltered(*enumerator, item, *lazyContext, lazyDepth, lazyFrame, lazyNodes) : (*enumerator)->MoveNext();
            if (hasNext && !lazyFilter)
            {
                item = (*enumerator)->GetCurrent();
            }
            isAdvancing = false;
            return hasNext;
        }
        catch (...)
        {
            isAdvancing = false;
            throw;
        }
    }

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
        // Only a recursive loop has a site to let go of
        if (recursiveStatement)
        {
            recursiveSite = ClosureSite();
        }
        recursiveStatement = nullptr;
        recursiveNodes = ArenaView();
        lastChanged = nullptr;
        cyclePosition = nullptr;
        isPeeked = false;
        lazyFilter = nullptr;
        lazyContext = nullptr;
        lazyNodes = ArenaView();
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
        Peek();
        // On the last item the enumerator has nothing left to collect
        if (isLast)
        {
            listSize = index0 + 1;
            return listSize.value();
        }

        // The rest after the next item, which is fetched; the loop then moves through them
        // without the filter
        InternalValueList rest;
        for (InternalValue item; FetchNext(item);)
        {
            rest.push_back(std::move(item));
        }

        listSize = index0 + rest.size() + 2;
        indexedList = ListAdapter::CreateAdapter(std::move(rest));
        enumerator = indexedList.GetEnumerator();
        lazyFilter = nullptr;
        return listSize.value();
    }
};

// Assigns the current item to the loop target. A plain name is stored straight into its
// slot, made by the first item so that the `else` body of an empty loop does not see the
// name. The map keeps its nodes in place, so the slot survives other names being added
void AssignLoopTarget(TargetNodes target, const InternalValue& item, ScopeRef scope, LoopTargetSlots& slots, RenderContext& values)
{
    static_assert(!InternalValueMap::is_flat);
    if (target[0].IsPlainName())
    {
        if (!slots.single)
        {
            slots.single = &scope.ForName(NameOf(target[0], values.Nodes()));
        }
        *slots.single = item;
        return;
    }
    // `for k, v in d|dictsort` (or d.items()): a pair goes straight into the slots of two
    // plain names, without the list of items that AssignTo unpacks through
    const auto* pair = GetIf<KeyValuePair>(&item);
    if (pair && IsPairOfNames(target))
    {
        if (slots.items.empty())
        {
            slots.items = { &scope.ForName(NameOf(target[1], values.Nodes())), &scope.ForName(NameOf(target[2], values.Nodes())) };
        }
        *slots.items[0] = pair->key;
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
// loop.cycle(*values): the value at the loop's position when it is called, as Jinja2's
// bound method, so a callable stored on one item cycles with the loop on later ones, and
// from its last position once the loop is over. Out of line, which keeps the attribute
// lookups that inline LoopAccessor::GetProperty small (Render/for_loop_vars)
JINJA2CPP_NOINLINE_INLINE Callable MakeLoopCycle(LoopState& state)
{
    return Callable(Callable::GlobalFunc, [position = std::shared_ptr<const size_t>(state.CyclePosition())](const CallParams& params, RenderContext&) -> InternalValue {
        if (!params.kwParams.empty())
        {
            throw std::runtime_error("LoopContext.cycle() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
        }
        const auto& args = params.posParams;
        if (args.empty())
        {
            throw std::runtime_error("no items for cycling given");
        }
        return args[*position % args.size()];
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
        , m_owner(other.m_owner ? other.m_owner : ValueObjectPtr(other.m_state))
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
            m_state->Peek();
            return !m_state->isLast;
        default:
            return true;
        }
    }

    // Forced inline: each attribute lookup uses one property, which inlining folds the switch to
    [[nodiscard]] JINJA2CPP_ALWAYS_INLINE InternalValue GetProperty(Property prop) const
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
            state.Peek();
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
            state.Peek();
            return state.Item(state.index0 + 1);
        case Property::Cycle:
            return MakeLoopCycle(state);
        case Property::Changed:
            if (!state.lastChanged)
            {
                state.lastChanged = std::make_shared<std::optional<InternalValueList>>();
            }
            return MakeLoopChanged(state.lastChanged);
        case Property::Call:
            if (!state.recursiveStatement)
            {
                // A loop without the recursive marker is callable in Jinja2, which then refuses
                return Callable(Callable::GlobalFunc, [](const CallParams&, RenderContext&) -> InternalValue {
                    throw std::runtime_error("The loop must have the 'recursive' marker to be called recursively.");
                });
            }
            return ForStatement::MakeLoopRecursion(state.recursiveStatement, state.recursiveNodes, state.level, state.recursiveSite, state.recursiveAutoescape);
        case Property::None:
            break;
        }
        return InternalValue();
    }

    LoopState* m_state;
    // Null for the accessor of a LoopFrame, which the frame owns
    ValueObjectPtr m_owner;
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

// The pool in RenderWorkspace holds frames where LoopFrame is incomplete (render_workspace.h)
void intrusive_ptr_add_ref(const LoopFrame* frame) noexcept
{
    intrusive_ptr_add_ref(static_cast<const ValueObject*>(frame));
}

void intrusive_ptr_release(const LoopFrame* frame) noexcept
{
    intrusive_ptr_release(static_cast<const ValueObject*>(frame));
}
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
    static boost::intrusive_ptr<LoopFrame> Take(uint64_t id)
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
    JINJA2CPP_NOINLINE_INLINE static boost::intrusive_ptr<LoopFrame> TakeOther(uint64_t id)
    {
        auto& frames = Frames();
        if (frames.empty())
        {
            auto frame = MakeRef<LoopFrame>();
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
    JINJA2CPP_NOINLINE_INLINE static void Give(boost::intrusive_ptr<LoopFrame>& frame)
    {
        auto& frames = Frames();
        if (!frame || frame->use_count() != 1 || frames.size() >= RenderWorkspace::MaxLoopFrames)
        {
            return;
        }
        frame->Recycle();
        frames.push_back(std::move(frame));
    }

private:
    // At most RenderWorkspace::MaxLoopFrames, deeper than any loop nesting a template has by
    // hand; recursive loops past it allocate
    static std::vector<boost::intrusive_ptr<LoopFrame>>& Frames() { return RenderWorkspace::ForThisThread().LoopFrames(); }
};

// Gives the frame of a loop back to the pool when the loop ends, by any path
struct LoopFrameReturn
{
    explicit LoopFrameReturn(boost::intrusive_ptr<LoopFrame>& loopFrame)
        : frame(loopFrame)
    {
    }
    LoopFrameReturn(const LoopFrameReturn&) = delete;
    LoopFrameReturn(LoopFrameReturn&&) = delete;
    LoopFrameReturn& operator=(const LoopFrameReturn&) = delete;
    LoopFrameReturn& operator=(LoopFrameReturn&&) = delete;
    ~LoopFrameReturn() { LoopFramePool::Give(frame); }

    boost::intrusive_ptr<LoopFrame>& frame;
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
    // `names` are those of `slots`, one by one
    SlotsGuard(RenderContext& values, boost::span<Slot> slots, boost::span<const SlotName> names)
        : m_values(values)
        , m_slots(slots)
        , m_names(names)
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
            m_values.UnbindSlots(m_slots, m_names);
            m_values.TruncateViews(m_viewCount);
        }
    }

    // Unbinds the slots now, forgetting the cached lookups of their names
    void Unbind()
    {
        m_values.UnbindSlots(m_slots, m_names);
        m_isDone = true;
    }

private:
    RenderContext& m_values;
    boost::span<Slot> m_slots;
    boost::span<const SlotName> m_names;
    size_t m_viewCount;
    bool m_isDone = false;
};
} // namespace

Callable ForStatement::MakeLoopRecursion(ForStatement* statement, const ArenaView& nodes, int level, const ClosureSite& site, bool autoescape)
{
    return Callable(Callable::GlobalFunc, [statement, nodes, level, site, autoescape](const CallParams& params, RenderContext& context) -> InternalValue {
        // As Python's LoopContext.__call__(iterable)
        for (const auto& kwParam : params.kwParams)
        {
            if (kwParam.first != "iterable")
            {
                throw std::runtime_error("LoopContext.__call__() got an unexpected keyword argument '" + kwParam.first + "'");
            }
            if (!params.posParams.empty())
            {
                throw std::runtime_error("LoopContext.__call__() got multiple values for argument 'iterable'");
            }
        }
        if (params.posParams.size() > 1)
        {
            throw std::runtime_error("LoopContext.__call__() takes 2 positional arguments but " + std::to_string(params.posParams.size() + 1) + " were given");
        }
        bool isSucceeded = false;
        auto parsedParams = helpers::ParseCallParams({ { "iterable", true } }, params, isSucceeded);
        if (!isSucceeded)
        {
            throw std::runtime_error("LoopContext.__call__() missing 1 required positional argument: 'iterable'");
        }

        auto var = parsedParams["iterable"];
        if (IsEmpty(var))
        {
            InternalValue empty(RenderToString(context.GetRendererCallback(), [](OutStream&) {}));
            empty.SetMarkup(autoescape);
            return empty;
        }

        const RenderDepthGuard depthGuard;
        // Each level sees the names around the loop, wherever loop(...) is called from (a macro
        // the body calls), and not the ones of the level calling it. Under a fresh epoch:
        // what this level's body caches must not be found by the level that called it
        const HomeCall homeCall(site, context, true);
        auto& home = homeCall.Context();
        const ArenaSwitch nodesSwitch(home, nodes);
        const bool isLinked = homeCall.HidesScopes(home.GetScopesCount());
        const OutputAutoescapeGuard autoescapeGuard(home, autoescape);
        home.EnterScope();
        if (isLinked)
        {
            home.PushFrameView({}, homeCall.Link());
        }
        // Jinja2's loop(...) returns Markup by the setting the loop is compiled under
        InternalValue result(RenderToString(home.GetRendererCallback(), [&](OutStream& stream) { statement->RenderLoop(var, stream, home, level + 1, &site); }));
        result.SetMarkup(autoescape);
        if (isLinked)
        {
            home.PopFrameView();
        }
        home.ExitScope();
        return result;
    });
}

void ForStatement::RenderLoop(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level, const ClosureSite* site)
{
    if (HasSlots())
    {
        // Every unit with slotted loops takes its frame when it is called (UnitCall)
        assert(values.Frame().unit == m_unit);
        RenderLoopInSlots(loopVal, os, values);
        return;
    }
    RenderLoopInScopes(loopVal, os, values, level, site);
}

// The `else` body runs in a scope of its own, as in Jinja2 (docs/tasks/0202)
void ForStatement::RenderElse(OutStream& os, RenderContext& values)
{
    values.EnterScope();
    values.Nodes()[m_elseBody].Render(os, values);
    values.ExitScope();
}

void ForStatement::RenderLoopInScopes(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level, const ClosureSite* site)
{
    bool isConverted = false;
    auto loopItems = ConvertToList(loopVal, isConverted, false);
    if (!isConverted)
    {
        CheckIterable(loopVal);
        if (m_elseBody)
        {
            RenderElse(os, values);
        }
        return;
    }
    // The filter sees the `loop` around this one, found before this one's hides it
    InternalValue outerLoop;
    if (m_filterReadsLoop)
    {
        const HashedName loopName{ "loop", HashedName::Hash("loop") };
        const auto found = values.FindValue(loopName);
        outerLoop = found ? InternalValue(*found) : MakeUndefined(values, "loop");
    }

    auto state = LoopFramePool::Take(m_loopId);
    const LoopFrameReturn frameReturn{ state };
    state->level = level;
    if (m_isRecursive)
    {
        state->recursiveStatement = this;
        state->recursiveNodes = values.Nodes();
        // loop(...) runs every level where the outermost one runs (ClosureSite)
        state->recursiveSite = site ? *site : ClosureSite(values);
        state->recursiveAutoescape = values.IsOutputAutoescape();
    }
    auto context = values.EnterScope(std::move(state->loopScope));
    if (!state->loopSlot)
    {
        state->loopSlot = &context["loop"s];
    }
    *state->loopSlot = MapAdapter(&state->accessor, ValueObjectPtr(state));

    ListAdapter filteredList;
    auto& enumerator = state->enumerator;
    if (m_ifExpr)
    {
        filteredList = CreateFilteredAdapter(loopItems, values, std::move(outerLoop));
        enumerator = filteredList.GetEnumerator();
    }
    else
    {
        loopItems.RebindEnumerator(enumerator);
        state->listSize = loopItems.GetSize();
    }

    bool loopRendered = false;
    // These loops fetch the next item before the body runs, so the filter runs one item ahead
    bool hasItem = state->FetchNext(state->Item(0));
    // One scope for the body, emptied after each pass, so `set` in the body stays local
    // to one iteration without a map being made for each
    auto bodyScope = values.EnterScope(std::move(state->bodyScope));
    auto& targetSlots = state->targetSlots;
    const auto target = values.Nodes()[m_target];
    for (size_t itemIdx = 0; hasItem; ++itemIdx)
    {
        state->SetIndex(itemIdx);
        state->isAdvancing = true;
        state->isLast = !(*enumerator)->MoveNext();
        state->isAdvancing = false;
        state->isPeeked = true;
        if (!state->isLast)
        {
            state->Item(itemIdx + 1) = (*enumerator)->GetCurrent();
        }
        hasItem = !state->isLast;
        const auto& curValue = state->Item(itemIdx);

        AssignLoopTarget(target, curValue, context, targetSlots, values);

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
    if (!state->listSize && state->use_count() > 2)
    {
        state->GetLength();
    }

    values.ExitScope(state->loopScope);
    state->ReleaseScopeValues();
    if (!loopRendered && m_elseBody)
    {
        RenderElse(os, values);
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
    SlotsGuard guard(values, slots, names);
    values.BindSlot(slots[0], names[0].hash, MapAdapter(&state->accessor, ValueObjectPtr(state)));

    bool isConverted = false;
    auto loopItems = ConvertToList(loopVal, isConverted, false);
    if (!isConverted)
    {
        CheckIterable(loopVal);
        // The `else` body sees the names outside the loop, as in Jinja2
        guard.Unbind();
        if (m_elseBody)
        {
            RenderElse(os, values);
        }
        return;
    }

    // One scope for the body, emptied after each pass, so `set` in the body stays local to
    // one iteration without a map being made for each. The loop's names are seen by name
    // right below it, from the filter too, which tells that `loop` is not there yet
    auto bodyScope = values.EnterScope(std::move(state->bodyScope));
    values.PushFrameView({ slots, names, values.Nodes() });

    auto& enumerator = state->enumerator;
    loopItems.RebindEnumerator(enumerator);
    if (m_ifExpr)
    {
        // Filtered lazily (LoopState::lazyFilter): run in place here, between two passes
        // through the body, and from anywhere when the body peeks
        state->lazyFilter = this;
        state->lazyContext = &values;
        state->lazyDepth = values.GetScopesCount() - 1;
        state->lazyFrame = values.Frame().handle;
        state->lazyNodes = values.Nodes();
    }
    else
    {
        state->listSize = loopItems.GetSize();
    }
    // The next item, filtered in place while the filter is still lazy
    auto fetchNext = [this, &state, &values, frameSlots, slots, names](InternalValue& item) {
        if (!state->lazyFilter || !state->enumerator)
        {
            return state->FetchNext(item);
        }
        state->isAdvancing = true;
        bool hasNext = false;
        if (m_filterReadsLoop)
        {
            // As in Jinja2, `loop` in the filter is the one around this loop: this one's
            // is unbound meanwhile, so a lookup passes over it
            InternalValue loopObject = std::move(static_cast<InternalValue&>(slots[0]));
            values.UnbindSlots(slots.first(1), names.first(1));
            hasNext = FilterInFrame(**state->enumerator, frameSlots, item, values);
            values.BindSlot(slots[0], names[0].hash, std::move(loopObject));
        }
        else
        {
            hasNext = FilterInFrame(**state->enumerator, frameSlots, item, values);
        }
        state->isAdvancing = false;
        return hasNext;
    };

    bool loopRendered = false;
    bool hasItem = fetchNext(state->Item(0));
    SlotSink sink{ slots };
    const auto target = values.Nodes()[m_target];
    const auto& root = target[0];
    const bool isPair = IsPairOfNames(target);
    // Read once: the body may write to anything the arena holds, as far as the compiler knows
    auto* const rootSlot = root.isTuple ? nullptr : &slots[root.slot.value];
    const auto rootHash = root.hash;
    for (size_t itemIdx = 0; hasItem; ++itemIdx)
    {
        state->SetIndex(itemIdx);
        // An unfiltered loop, or one whose rest is collected, fetches the next item first
        state->isPeeked = state->lazyFilter == nullptr;
        if (state->isPeeked)
        {
            state->isAdvancing = true;
            state->isLast = !(*enumerator)->MoveNext();
            state->isAdvancing = false;
            if (!state->isLast)
            {
                state->Item(itemIdx + 1) = (*enumerator)->GetCurrent();
            }
        }
        const auto& curValue = state->Item(itemIdx);

        if (!root.isTuple)
        {
            values.BindSlot(*rootSlot, rootHash, curValue);
        }
        else if (const auto* pair = isPair ? GetIf<KeyValuePair>(&curValue) : nullptr)
        {
            // `for k, v in d|dictsort` (or d.items()): no list of items to unpack through
            values.BindSlot(slots[target[1].slot.value], target[1].hash, pair->key);
            values.BindSlot(slots[target[2].slot.value], target[2].hash, pair->value);
        }
        else
        {
            AssignTo(target, curValue, sink, values);
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
        if (!state->isPeeked)
        {
            state->isLast = !fetchNext(state->Item(itemIdx + 1));
            state->isPeeked = true;
        }
        hasItem = !state->isLast;
    }

    // A loop object kept past the loop (`set ns.x = loop`) can no longer run the filter,
    // which needs this render context: collect the rest of the items now (more owners than
    // this function and the slot of `loop` mean it was kept)
    if (!state->listSize && state->use_count() > 2)
    {
        state->GetLength();
    }
    state->lazyFilter = nullptr;

    guard.Unbind();
    values.PopFrameView();
    values.ExitScope(state->bodyScope);
    if (!loopRendered && m_elseBody)
    {
        RenderElse(os, values);
    }
}

ArenaSpan<SlotName> ForStatement::MakeBinderNames(NodeArena& nodes)
{
    // `loop`, then each target name at the place the parser gave it, met first there
    boost::container::small_vector<SlotName, 4> names{ SlotName{ nodes.MakeText("loop"), HashedName::Hash("loop") } };
    for (const auto& target : nodes[m_target])
    {
        if (target.IsPlainName() && target.slot.value == names.size())
        {
            names.push_back({ target.name, target.hash });
        }
    }
    m_slotNames = nodes.MakeSpan(names);
    return m_slotNames;
}

void ForStatement::BindSlots(SlotIndex first, UnitId unit)
{
    m_firstSlot = first;
    m_unit = unit;
}

uint64_t ForStatement::NewLoopId()
{
    static std::atomic<uint64_t> lastId{ 0 };
    return ++lastId;
}

ListAdapter ForStatement::CreateFilteredAdapter(const ListAdapter& loopItems, RenderContext& values, InternalValue outerLoop) const
{
    // Like the slotted one below, the filter may run while another template's code does
    return ListAdapter::CreateAdapter([eo = loopItems.GetEnumerator(), this, &values, nodes = values.Nodes(), outerLoop = std::move(outerLoop)]() mutable {
        using ResultType = std::optional<InternalValue>;

        const ArenaSwitch nodesSwitch(values, nodes);
        auto tempContext = values.EnterScope();
        if (m_filterReadsLoop)
        {
            tempContext["loop"s] = outerLoop;
        }
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
                AssignTo(values.Nodes()[m_target], curValue, tempContext, values);
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

bool ForStatement::FilterInFrame(IListAccessorEnumerator& items, boost::span<Slot> frameSlots, InternalValue& item, RenderContext& values) const
{
    // The filter's own slots for the target names, seen by name above the loop's
    const auto targets = static_cast<std::uint16_t>(m_slotNames.size() - 1);
    const auto slots = frameSlots.subspan(m_firstSlot.value + 1U + targets, targets);
    const auto names = values.Nodes()[m_slotNames].subspan(1);
    SlotsGuard guard(values, slots, names);
    values.PushFrameView({ slots, names, values.Nodes() });
    SlotSink sink{ frameSlots.subspan(m_firstSlot.value), targets };
    const auto target = values.Nodes()[m_target];
    const auto& root = target[0];
    auto& filter = values.Nodes()[m_ifExpr];
    // Read once: the filter may write to anything the arena holds, as far as the compiler knows
    auto* const rootSlot = root.isTuple ? nullptr : &sink.slots[root.slot.value + sink.shift];
    const auto rootHash = root.hash;
    bool isFound = false;
    while (items.MoveNext())
    {
        // A plain name, the common case, takes the item into its slot and gives it back when
        // it is kept, without a copy
        if (rootSlot)
        {
            auto& slot = *rootSlot;
            values.BindSlot(slot, rootHash, items.GetCurrent());
            if (ConvertToBool(filter.Evaluate(values)))
            {
                item = std::move(static_cast<InternalValue&>(slot));
                isFound = true;
                break;
            }
            continue;
        }
        auto curValue = items.GetCurrent();
        AssignTo(target, curValue, sink, values);
        if (ConvertToBool(filter.Evaluate(values)))
        {
            item = std::move(curValue);
            isFound = true;
            break;
        }
    }
    guard.Unbind();
    values.PopFrameView();
    return isFound;
}

bool ForStatement::FetchFiltered(ListAccessorEnumeratorPtr& items,
                                 InternalValue& item,
                                 RenderContext& values,
                                 size_t depth,
                                 FrameHandle handle,
                                 const ArenaView& nodes) const
{
    // A peek runs the filter wherever the body read `loop`, which a macro the body calls can
    // do while the macro's frame and tree are installed: it installs the loop's frame again,
    // found by its handle, which fails loudly once the loop's unit call is over, and the
    // loop's tree. As in Jinja2, the filter sees the names around the loop, not the ones the
    // body set so far (nor this loop's `loop`): it runs in a context that sees only the
    // scopes below the body's
    const ArenaSwitch nodesSwitch(values, nodes);
    const auto frame = RenderWorkspace::ForThisThread().Resolve(handle);
    RenderContext filterContext(values, depth);
    filterContext.InstallFrame(frame);
    return FilterInFrame(*items, frame.slots, item, filterContext);
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
    AssignTo(values.Nodes()[GetTarget()], std::move(value), values.GetCurrentScope(), values);
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
    AssignTo(values.Nodes()[GetTarget()], std::move(body), values.GetCurrentScope(), values);
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
    // The filter takes the body as Markup under autoescape where the block is written, and
    // Jinja2 wraps the filtered value: Markup(str(result)) under autoescape at run time
    body.SetMarkup(values.IsOutputAutoescape());
    auto result = values.Nodes()[m_expr].Evaluate(body, values);
    if (values.IsAutoescape())
    {
        result = MakeMarkup(result, values.GetRendererCallback());
    }
    AssignTo(values.Nodes()[GetTarget()], std::move(result), values.GetCurrentScope(), values);
}

namespace
{
// Renders the block at `depth` of the stack for `name` in `blockContext`, the context
// Jinja2 passes to a block function
void RenderBlockAt(const BlocksStack& stack, std::string_view name, size_t depth, OutStream& os, RenderContext& blockContext)
{
    auto p = stack.blocks.find(name);
    if (p == stack.blocks.end() || depth >= p->second.size())
    {
        return;
    }
    const auto block = blockContext.GetRendererCallback()->Templates().Resolve(p->second[depth]);
    blockContext.PassOverEntryBindings();
    const ArenaSwitch nodesSwitch(blockContext, block.nodes);
    block.node.RenderBody(os, blockContext, depth);
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

// Jinja2's BlockReference.__call__ takes no arguments
void CheckBlockCallParams(const CallParams& params)
{
    // Python reports a keyword argument first
    if (!params.kwParams.empty())
    {
        throw std::runtime_error("BlockReference.__call__() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
    }
    if (!params.posParams.empty())
    {
        throw std::runtime_error("BlockReference.__call__() takes 1 positional argument but " + std::to_string(params.posParams.size() + 1) + " were given");
    }
}

// `self` for the blocks of `stack`, Jinja2's TemplateReference printed with the context's
// `name`: a callable per block. A call renders the block in the template `self` came from
// while that template runs (the importer, for `self` passed to an imported macro), or while
// the context of its top level is the template's (a module's, for `self` a module's macro
// stored), else in the template running then. The block sees the names of the template's top
// level, `site`, wherever it is called from, as a block function sees its template's context
// in Jinja2 (docs/tasks/0194). Once the stack has ended, the call renders the blocks it held
// (SelfBlocks), seeing the names `site` still has
InternalValue MakeTemplateSelf(const BlocksStack& stack, const ClosureSite& site, std::string_view templateName)
{
    InternalValueMap self;
    if (!stack.blocks.empty() && !stack.self)
    {
        stack.self = std::make_shared<SelfBlocks>(&stack);
    }
    for (const auto& block : stack.blocks)
    {
        // `self` may outlive the stack and the trees its names are in
        std::string blockName(block.first);
        self[blockName] = MakeWrapped(Callable(Callable::Macro, [name = blockName, owner = stack.self, site, templateName](const CallParams& params, OutStream& stream, RenderContext& context) {
            CheckBlockCallParams(params);
            auto* curFrame = context.GetTemplateFrame();
            TemplateFrame* frame = nullptr;
            for (auto* f = curFrame; f && owner->live; f = f->outer)
            {
                if (f->blocks == owner->live)
                {
                    frame = f;
                    break;
                }
            }
            const auto home = site.Resolve(context);
            // A frame of a context that is alive is in place: its guard restores the one before
            if (!frame && owner->live)
            {
                if (auto* homeFrame = home.context->GetTemplateFrame(); homeFrame && homeFrame->blocks == owner->live)
                {
                    frame = homeFrame;
                }
            }
            RenderContext blockContext(*home.context, home.link);
            // The template has ended (an include that stored `self`): its blocks, in a frame
            // of their own
            TemplateFrame endedFrame;
            if (!frame && owner->ended)
            {
                endedFrame.blocks = &*owner->ended;
                endedFrame.baseDepth = home.link;
                endedFrame.name = templateName;
                endedFrame.outer = curFrame;
                // Its scopes have ended too: a `self` in the block renders the ended blocks
                endedFrame.self = MakeTemplateSelf(*owner->ended, ClosureSite(), templateName);
                frame = &endedFrame;
            }
            if (!frame)
            {
                if (!curFrame || !curFrame->blocks)
                {
                    return;
                }
                frame = curFrame;
            }
            blockContext.SetTemplateFrame(frame);
            RenderBlockAt(*frame->blocks, name, 0, stream, blockContext);
        }));
    }
    return CreateObjectAdapter(std::move(self), "TemplateReference", templateName.empty() ? InternalValue(NoneValue()) : InternalValue(std::string(templateName)));
}
} // namespace

LookupResult RenderContext::FindSelf(const std::string& name)
{
    auto* frame = m_templateFrame;
    // A frame with no scopes of its own is an ended template's, whose `self` is made with it
    if (!frame || !frame->blocks || (frame->baseDepth == 0 && !frame->self))
    {
        return FindValue(name);
    }
    // The template's base scope is its own: a `set self` at its top level wins, except in a
    // block, which sets `self` from its own scope up
    if (const auto value = FindInScopesFrom(name, frame->baseDepth == 0 ? m_selfDepth : std::max(frame->baseDepth - 1, m_selfDepth)))
    {
        return value;
    }
    // The context whose scopes the template's top level set, or a scoped block saw, which may
    // run below this one (a `with` or a block): `self` outlives those
    const auto makeSelf = [this, frame](size_t depth) {
        auto* home = this;
        while (home->m_parentDepth >= depth && home->m_parent)
        {
            home = home->m_parent;
        }
        return MakeTemplateSelf(*frame->blocks, ClosureSite(*home, depth), frame->name);
    };
    // In a block that sees more than the template's top level (a scoped one in a loop, or one
    // such a block's `self` renders), `self` passes on what it sees: Jinja2's TemplateReference
    // of the block's derived context. One per block call, kept by its context
    if (m_selfDepth > frame->baseDepth + 1)
    {
        const size_t depth = m_selfDepth - 1;
        auto* block = this;
        while (block->m_parentDepth > depth && block->m_parent)
        {
            block = block->m_parent;
        }
        if (!block->m_blockSelf)
        {
            block->m_blockSelf = std::make_unique<InternalValue>(makeSelf(depth));
        }
        return LookupResult(*block->m_blockSelf);
    }
    if (!frame->self)
    {
        frame->self = makeSelf(frame->baseDepth);
    }
    return LookupResult(*frame->self);
}

BlocksStack::~BlocksStack()
{
    if (self)
    {
        self->live = nullptr;
        self->ended.emplace().blocks = std::move(blocks);
    }
}

ClosureSite::ClosureSite(RenderContext& context, size_t depth)
    : m_home(context.Anchor())
{
    m_depth = static_cast<uint32_t>(depth);
    m_serial = context.SerialBelow(depth);
    // A template's top-level scope lives as long as the context it runs in
    if (const auto* frame = context.GetTemplateFrame())
    {
        const size_t rootDepth = std::min(frame->baseDepth, depth);
        m_rootDepth = static_cast<uint32_t>(rootDepth);
        m_rootSerial = context.SerialBelow(rootDepth);
        if (frame->baseDepth == depth)
        {
            m_state = TopLevel;
        }
    }
}

void BlockStatement::Render(OutStream& os, RenderContext& values)
{
    auto* frame = values.GetTemplateFrame();
    if (!frame || !frame->blocks)
    {
        RenderContext innerContext = values.Clone(true);
        innerContext.PassOverEntryBindings();
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

    const auto name = GetName(values.Nodes());
    auto p = frame->blocks->blocks.find(name);
    if (m_isRequired && (p == frame->blocks->blocks.end() || p->second.size() <= 1))
    {
        throw std::runtime_error("Required block '" + std::string(name) + "' not found");
    }

    // An unscoped block sees the template-level names only, not the loop variables or
    // other locals around it
    RenderContext blockContext = m_isScoped ? RenderContext(values, values.GetScopesCount()) : RenderContext(values, frame->baseDepth);
    RenderBlockAt(*frame->blocks, name, 0, os, blockContext);
}

void BlockStatement::RenderBody(OutStream& os, RenderContext& values, size_t depth) const
{
    const RenderDepthGuard depthGuard;
    const UnitCall unitCall(values, m_unitLayout);
    auto* frame = values.GetTemplateFrame();
    std::optional<Callable> parentBlock;
    if (frame && frame->blocks)
    {
        const auto* stack = frame->blocks;
        // In the block's tree, which the render keeps as long as the block
        const auto name = GetName(values.Nodes());
        auto p = stack->blocks.find(name);
        if (p != stack->blocks.end() && depth + 1 < p->second.size())
        {
            // super() may outlive the stack (stored past an include): it renders what the
            // stack held then, as `self` does
            if (!stack->self)
            {
                stack->self = std::make_shared<SelfBlocks>(stack);
            }
            // The parent block sees the scopes below this block's, wherever super() is called
            // from (a macro the block passes it to): a context on them in the block's own
            parentBlock.emplace(Callable::Macro, [owner = stack->self, name, depth, site = ClosureSite(values)](const CallParams&, OutStream& stream, RenderContext& context) {
                const auto home = site.Resolve(context);
                RenderContext superContext(*home.context, home.link);
                RenderBlockAt(owner->live ? *owner->live : *owner->ended, name, depth + 1, stream, superContext);
            });
        }
        else
        {
            parentBlock.emplace(Callable::Macro, [name](const CallParams&, OutStream&, RenderContext&) {
                throw std::runtime_error("there is no parent block called '" + std::string(name) + "'.");
            });
        }
    }
    const size_t prevSelfDepth = values.SetSelfDepth(values.GetScopesCount());
    auto scope = values.EnterScope();
    if (parentBlock)
    {
        scope["super"] = std::move(*parentBlock);
    }
    // A block body escapes its output as the template it is written in does, whatever
    // `{% autoescape %}` surrounds it; the runtime setting stays the caller's, as Jinja2's
    // derived context shares its EvalContext
    OutputAutoescapeGuard autoescapeGuard(values, m_autoescape);
    values.Nodes()[m_mainBody].Render(os, values);
    values.ExitScope();
    values.SetSelfDepth(prevSelfDepth);
}

void TemplateRenderer::PushBlocks(IRendererCallback& callback, const ArenaView& nodes, BlocksStack& stack) const
{
    if (m_blocks.empty())
    {
        return;
    }
    const auto tpl = callback.Templates().Add(nodes);
    for (const auto block : nodes[m_blocks])
    {
        stack.blocks[nodes[block].GetName(nodes)].push_back({ tpl, block });
    }
}

void TemplateRenderer::RenderNamed(OutStream& os, RenderContext& values, std::string_view name)
{
    // Most included templates define no blocks and extend nothing: their stack stays empty
    if (m_blocks.empty() && !m_hasExtends)
    {
        RenderBody(os, values, nullptr, name);
        return;
    }
    BlocksStack stack;
    PushBlocks(*values.GetRendererCallback(), values.Nodes(), stack);
    RenderBody(os, values, &stack, name);
}

void TemplateRenderer::RenderAsParent(OutStream& os, RenderContext& values, BlocksStack& stack, std::string_view childName)
{
    // The parent shares the child's top-level scope, but a `self` the child set there is not
    // the parent's: in Jinja2 `self` is a local of each template's root function
    values.GetCurrentScope().Erase("self");
    PushBlocks(*values.GetRendererCallback(), values.Nodes(), stack);
    RenderBody(os, values, &stack, childName, true);
}

void TemplateRenderer::RenderAsModule(OutStream& os, RenderContext& values, BlocksStack& stack, std::string_view name)
{
    PushBlocks(*values.GetRendererCallback(), values.Nodes(), stack);
    RenderBody(os, values, &stack, name);
}

void TemplateRenderer::RenderBody(OutStream& os, RenderContext& values, BlocksStack* stack, std::string_view name, bool asParent)
{
    // Nothing adds to the stack of a template that extends nothing, so those share one
    static const BlocksStack noBlocks{};

    const RenderDepthGuard depthGuard;
    const UnitCall unitCall(values, m_unitLayout);
    // A template escapes its output by its own setting (Settings::autoescapeSelector). An
    // included or imported one runs in a context of its own, whose runtime setting is the
    // template's too; a parent runs in its child's and keeps the child's runtime setting, as
    // Jinja2's EvalContext belongs to the context
    AutoescapeGuard autoescapeGuard(values, asParent ? values.IsAutoescape() : m_autoescape, m_autoescape);
    TemplateFrame frame;
    frame.blocks = stack ? stack : &noBlocks;
    frame.baseDepth = values.GetScopesCount();
    frame.name = name;
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
        const auto parent = values.GetRendererCallback()->Templates().Resolve(frame.parent);
        const ArenaSwitch nodesSwitch(values, parent.nodes);
        // A template that extends another has a stack of its own
        assert(stack);
        parent.node.RenderAsParent(os, values, *stack, name);
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

    Result operator()(NoneValue) const { return Result(); }
};

template<typename Result, typename Fn, typename Arg>
Result VisitTemplateImpl(Arg&& tpl, bool throwError, const Fn& fn)
{
    return visit(TemplateImplVisitor<Result, Fn>(fn, throwError), std::forward<Arg>(tpl));
}

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
    auto& templates = values.GetRendererCallback()->Templates();
    frame->parent = VisitTemplateImpl<TemplateNode<TemplateRenderer>>(tpl, true, [&templates](const auto& tplPtr) {
        return TemplateNode<TemplateRenderer>{ templates.Add(tplPtr->Nodes()), tplPtr->GetRenderer() };
    });
}

// Renders `tpl` for `include`: the names it sets stay its own
template<typename CharT>
void RenderIncludedTemplate(const TemplateImpl<CharT>& tpl, bool withContext, OutStream& os, RenderContext& values)
{
    RenderContext innerContext = values.Clone(withContext);
    if (withContext)
    {
        // Jinja2 renders it in a new Context, with an EvalContext of its own
        innerContext.NewEvalContext();
        innerContext.PassOverEntryBindings();
        innerContext.EnterScope();
    }

    innerContext.SetNodes(tpl.Nodes());
    innerContext.Nodes()[tpl.GetRenderer()].RenderNamed(os, innerContext, tpl.SelfName());
}

void IncludeStatement::Render(OutStream& os, RenderContext& values)
{
    auto doRender = [this, &values, &os](const IRendererCallback::LoadTemplateResult& tpl) -> bool {
        try
        {
            return VisitTemplateImpl<bool>(tpl, true, [this, &values, &os](const auto& tplPtr) {
                RenderIncludedTemplate(*tplPtr, m_withContext, os, values);
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

    // A constant name found in this render is not looked up again
    const auto* callback = values.GetRendererCallback();
    if (m_isConstant)
    {
        if (const auto* tpl = callback->FindLoadedBy(this); tpl && doRender(*tpl))
        {
            return;
        }
    }

    auto templateNames = values.Nodes()[m_expr].Evaluate(values);
    bool isConverted = false;
    ListAdapter list = ConvertToList(templateNames, isConverted);

    bool rendered = false;
    if (isConverted)
    {
        for (const auto& name : list)
        {
            // Python's select_template finds no template by a name that is not a string and tries the next
            if (!GetAsSameString(std::string(), name))
            {
                continue;
            }
            rendered = doRender(callback->LoadTemplate(name));
            if (rendered)
            {
                break;
            }
        }
    }
    else
    {
        const auto& tpl = callback->LoadTemplate(templateNames);
        rendered = doRender(tpl);
        if (rendered && m_isConstant)
        {
            callback->SetLoadedBy(this, tpl);
        }
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

namespace
{
// The names an imported module sees: what `importer` sees now, with `withContext`
InternalValueMap ImporterNames(const RenderContext& importer, bool withContext)
{
    InternalValueMap names;
    if (withContext)
    {
        importer.CollectVisibleNames(names);
    }
    return names;
}
} // namespace

ImportedModule::ImportedModule(const RenderContext& importer, bool withContext)
    : m_importerNames(ImporterNames(importer, withContext))
    , m_context(importer.CloneWith(m_importerNames))
{
}

template<typename Tpl>
void ImportedModule::Render(const Tpl& tpl)
{
    auto& callback = *m_context.GetRendererCallback();
    // The module's macros run in its template, whose blocks their `self` has: its own and,
    // once its top level has run, those of the templates it extends
    m_frame.blocks = &m_blocks;
    m_frame.baseDepth = m_context.GetScopesCount();
    m_frame.name = tpl.SelfName();
    m_context.SetTemplateFrame(&m_frame);
    m_context.SetNodes(tpl.Nodes());
    // The module's context is its template's: its macros run with the template's runtime
    // autoescape setting wherever they are called
    const bool autoescape = m_context.Nodes()[tpl.GetRenderer()].IsAutoescape();
    m_context.SetAutoescape(autoescape);
    m_context.SetOutputAutoescape(autoescape);
    // Only the names the template, and the ones it extends, define are imported, as in
    // Jinja2; its output is dropped
    RenderToString(&callback, [this, &tpl](OutStream& stream) { m_context.Nodes()[tpl.GetRenderer()].RenderAsModule(stream, m_context, m_blocks, tpl.SelfName()); });
}

void ImportStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    // The names to import are in this tree, which rendering the module does not change
    const auto& nodes = values.Nodes();
    auto name = nodes[m_nameExpr].Evaluate(values);

    // Resolved on every render: the name may change between renders or loop iterations
    auto& callback = *values.GetRendererCallback();
    const auto& tpl = callback.LoadTemplate(name);
    // Throws if the template failed to load
    if (!VisitTemplateImpl<bool>(tpl, true, [](const auto& /*tplPtr*/) { return true; }))
    {
        return;
    }

    // Without context the module sees nothing of the importer, so the render's imports of the
    // template share it, as in Jinja2: its namespaces count across them
    if (!m_withContext)
    {
        if (const auto* shared = callback.FindModule(tpl))
        {
            ImportNames(values, nodes, shared->Names());
            return;
        }
    }
    // Owned by the render, so that its macros find their names wherever they are called
    auto& module = callback.AddModule(std::make_unique<ImportedModule>(values, m_withContext));
    VisitTemplateImpl<bool>(tpl, true, [&module](const auto& tplPtr) {
        module.Render(*tplPtr);
        return true;
    });
    // Shared once complete: a module that imports itself recurses, as in Jinja2
    if (!m_withContext)
    {
        callback.ShareModule(tpl, module);
    }
    ImportNames(values, nodes, module.Names());
}

namespace
{
// Whether importing `value` gives it: a macro, which runs in its module wherever it is called
// (ClosureSite), and any value but another callable
bool IsImported(const InternalValue& value)
{
    const auto* callable = GetIf<Callable>(&value);
    if (!callable)
    {
        return true;
    }
    return callable->GetKind() == Callable::Macro;
}
} // namespace

// Copies: the module keeps every name, which its own macros read in its context
void ImportStatement::ImportNames(RenderContext& values, const ArenaView& nodes, const InternalValueMap& moduleNames) const
{
    if (m_namespace.empty())
    {
        auto scope = values.GetCurrentScope();
        for (const auto& name : nodes[m_namesToImport])
        {
            const auto p = moduleNames.find(HashedName{ nodes.Text(name.name), name.nameHash });
            if (p == moduleNames.end() || !IsImported(p->second))
            {
                continue;
            }
            scope.ForName({ nodes.Text(name.alias), name.aliasHash }) = p->second;
        }
        return;
    }

    InternalValueMap importedNs;
    for (const auto& [name, value] : moduleNames)
    {
        // Jinja2 exports no name that starts with an underline
        if (name.empty() || name[0] == '_' || !IsImported(value))
        {
            continue;
        }
        importedNs[name] = value;
    }
    values.GetCurrentScope().ForName({ nodes.Text(m_namespace), m_namespaceHash }) = CreateMapAdapter(std::move(importedNs));
}

MacroStatement::MacroStatement(NodeArena& nodes, std::string_view name, const MacroParamsInfo& params)
    : MacroStatement(nodes, nodes.MakeText(name), params)
{
}

MacroStatement::MacroStatement(NodeArena& nodes, ArenaText name, const MacroParamsInfo& params)
    : m_name(name)
    , m_nameHash(HashedName::Hash(nodes.Text(name)))
    , m_attributes(std::make_shared<InternalValueMap>())
{
    boost::container::small_vector<MacroParam, 8> arenaParams;
    arenaParams.reserve(params.size());
    InternalValueList arguments;
    arguments.reserve(params.size());
    for (const auto& p : params)
    {
        arenaParams.push_back({ nodes.MakeText(p.paramName), HashedName::Hash(p.paramName), p.defaultValue, p.defaultRefersToArgs });
        m_hasArgDefaults = m_hasArgDefaults || (p.defaultValue && p.defaultRefersToArgs);
        arguments.emplace_back(p.paramName);
        if (p.paramName == "caller")
        {
            m_declaredNames |= UsesCaller;
            m_callerWithoutDefault = !p.defaultValue;
        }
        else if (p.paramName == "varargs")
        {
            m_declaredNames |= UsesVarargs;
        }
        else if (p.paramName == "kwargs")
        {
            m_declaredNames |= UsesKwargs;
        }
    }
    m_params = nodes.MakeSpan(arenaParams);

    const auto nameText = nodes.Text(name);
    (*m_attributes)["name"s] = nameText.empty() ? InternalValue(NoneValue()) : InternalValue(std::string(nameText));
    auto argumentNames = ListAdapter::CreateAdapter(std::move(arguments));
    argumentNames.MarkAsTuple();
    (*m_attributes)["arguments"s] = std::move(argumentNames);
}

struct MacroStatement::CallableBody
{
    const MacroStatement* macro;
    ArenaView nodes;
    bool autoescape;
    // The body resolves its names where the macro is defined, not through its caller
    // (docs/design/0038-lexical-scoping-plan.md)
    ClosureSite site;
    // MacroStatement::ReadsSlotsOnly
    bool readsSlotsOnly;

    void operator()(const CallParams& callParams, OutStream& stream, RenderContext& context) const
    {
        HomeCall::Run(site, context, readsSlotsOnly, *this, callParams, stream);
    }
    // The call on its home
    void operator()(RenderContext& home, size_t link, const CallParams& callParams, OutStream& stream) const
    {
        // A macro called from another template (imported, or passed as `caller`) runs in its own tree
        const ArenaSwitch nodesSwitch(home, nodes);
        OutputAutoescapeGuard autoescapeGuard(home, autoescape);
        macro->InvokeMacroRenderer(callParams, stream, home, link);
    }
};

Callable MacroStatement::MakeCallable(RenderContext& values) const
{
    // The body escapes its output as where the macro is defined; the runtime setting where it
    // is called decides whether its results are Markup, its own and those of `caller()` and
    // `join` in it (RenderContext::IsAutoescape). The defaults are evaluated by each call
    // (BindMacroDefaults)
    Callable result(Callable::Macro, CallableBody{ this, values.Nodes(), values.IsOutputAutoescape(), ClosureSite(values), m_readsSlotsOnly });
    result.SetAttributes(m_attributes);
    return result;
}

void MacroStatement::Render(OutStream&, RenderContext& values)
{
    values.GetCurrentScope().ForName({ values.Nodes().Text(m_name), m_nameHash }) = MakeCallable(values);
}

void MacroStatement::CompleteAttributes()
{
    (*m_attributes)["catch_kwargs"s] = InternalValue((m_caughtNames & UsesKwargs) != 0);
    (*m_attributes)["catch_varargs"s] = InternalValue((m_caughtNames & UsesVarargs) != 0);
    (*m_attributes)["caller"s] = InternalValue((m_specialNames & UsesCaller) != 0);
}

ArenaSpan<SlotName> MacroStatement::MakeBinderNames(NodeArena& nodes) const
{
    const auto nameOf = [&nodes](std::string_view name) { return SlotName{ nodes.MakeText(name), HashedName::Hash(name) }; };
    boost::container::small_vector<SlotName, 8> names;
    for (const auto& p : nodes[m_params])
    {
        names.push_back({ p.name, p.hash });
    }
    // In the order InvokeMacroRenderer binds them
    const auto caught = m_caughtNames;
    if ((caught & UsesCaller) != 0)
    {
        names.push_back(nameOf("caller"));
    }
    if ((caught & UsesKwargs) != 0)
    {
        names.push_back(nameOf("kwargs"));
    }
    if ((caught & UsesVarargs) != 0)
    {
        names.push_back(nameOf("varargs"));
    }
    return nodes.MakeSpan(names);
}

namespace
{
// The arguments of one macro call, read in place rather than copied first: a keyword
// argument binds a parameter that no positional argument filled, the others are extra (kwargs)
struct MacroArgBinder
{
    MacroArgBinder(const ArenaView& macroNodes, ArenaSpan<MacroParam> macroParams, const CallParams& call, bool isCatchingCaller)
        : nodes(macroNodes)
        , params(macroNodes[macroParams])
        , callParams(call)
        , catchCaller(isCatchingCaller)
    {
        // Each argument's keyword value, found in one pass over the keywords, which also
        // counts the extra ones; none for the positional arguments
        if (!call.kwParams.empty())
        {
            keywordValues.resize(params.size());
            for (const auto& [name, value] : call.kwParams)
            {
                const auto idx = BoundIndex(name);
                if (idx < params.size())
                {
                    keywordValues[idx] = &value;
                }
                else if (!(catchCaller && name == "caller"))
                {
                    ++extraKeywords;
                }
            }
        }
    }

    // The tree of the macro, which binding the defaults may switch the context away from
    ArenaView nodes;
    boost::span<const MacroParam> params;
    const CallParams& callParams;
    bool catchCaller = false;
    boost::container::small_vector<const InternalValue*, 8> keywordValues;
    // The keywords that bind no argument (kwargs), caller left out when the macro catches it
    std::size_t extraKeywords = 0;

    [[nodiscard]] HashedName Name(std::size_t idx) const { return { nodes.Text(params[idx].name), params[idx].hash }; }
    [[nodiscard]] const InternalValue* KeywordValue(std::size_t idx) const { return idx < keywordValues.size() ? keywordValues[idx] : nullptr; }

    // The argument a keyword `name` binds: one no positional argument filled; params.size() for none
    [[nodiscard]] std::size_t BoundIndex(std::string_view name) const
    {
        for (auto idx = callParams.posParams.size(); idx < params.size(); ++idx)
        {
            if (nodes.Text(params[idx].name) == name)
            {
                return idx;
            }
        }
        return params.size();
    }
    [[nodiscard]] bool IsBoundKeyword(std::string_view name) const { return BoundIndex(name) < params.size(); }
    [[nodiscard]] bool IsExtraKeyword(std::string_view name) const { return !IsBoundKeyword(name) && !(catchCaller && name == "caller"); }
    // Keywords or positional arguments the macro does not catch
    [[nodiscard]] bool HasExtraArgs(bool catchKwargs, bool catchVarargs) const
    {
        return (!catchKwargs && extraKeywords != 0) || (!catchVarargs && callParams.posParams.size() > params.size());
    }
    [[nodiscard]] bool IsProvided(std::size_t idx) const { return idx < callParams.posParams.size() || KeywordValue(idx) != nullptr; }
};

// Throws for the extra arguments the macro does not catch, which HasExtraArgs found;
// displayName() names the macro
template<typename DisplayName>
[[noreturn]] void ThrowExtraMacroArgs(const MacroArgBinder& binder, bool catchKwargs, const DisplayName& displayName)
{
    const auto& kwParams = binder.callParams.kwParams;
    const auto argsCount = binder.params.size();
    if (!catchKwargs && binder.extraKeywords != 0)
    {
        for (const auto& [name, value] : kwParams)
        {
            if (!binder.IsExtraKeyword(name))
            {
                continue;
            }
            if (kwParams.find("caller"s) != kwParams.end() && binder.IsExtraKeyword("caller"))
            {
                throw std::runtime_error("macro " + displayName() + " was invoked with two values for the special caller argument. This is most likely a bug.");
            }
            throw std::runtime_error("macro " + displayName() + " takes no keyword argument '" + name + "'");
        }
    }

    throw std::runtime_error("macro " + displayName() + " takes not more than " + std::to_string(argsCount) + " argument(s)");
}

// The slots of the special names: they follow the arguments, in this order, the ones the
// macro does not catch left out
struct SpecialSlots
{
    std::size_t caller = 0;
    std::size_t kwargs = 0;
    std::size_t varargs = 0;

    SpecialSlots(std::size_t params, bool catchCaller, bool catchKwargs)
        : caller(params)
        , kwargs(params + (catchCaller ? 1U : 0U))
        , varargs(kwargs + (catchKwargs ? 1U : 0U))
    {
    }
};

// Binds the given arguments, and the missing ones as undefined unless their default is bound
// later without seeing the other arguments
// Where a call's names go: `bind(slot, name, value)` binds the argument or special name
// `name`, a HashedName, which has slot `slot` when the macro binds its names in slots
template<typename Bind>
void BindMacroArgs(const MacroArgBinder& binder, bool hasArgDefaults, const RenderContext& context, const Bind& bind)
{
    const auto& posParams = binder.callParams.posParams;
    for (std::size_t idx = 0; idx < binder.params.size(); ++idx)
    {
        const auto name = binder.Name(idx);
        if (idx < posParams.size())
        {
            bind(idx, name, posParams[idx]);
            continue;
        }
        if (const auto* value = binder.KeywordValue(idx))
        {
            bind(idx, name, *value);
            continue;
        }
        if (binder.params[idx].defaultValue && !hasArgDefaults)
        {
            continue;
        }
        bind(idx, name, MakeUndefinedWithHint(context, "parameter '" + std::string(name.name) + "' was not provided"));
    }
}

// Binds caller, kwargs and varargs for a macro that uses them
template<typename Bind>
void BindSpecialMacroArgs(const MacroArgBinder& binder, bool catchKwargs, bool catchVarargs, const RenderContext& context, const Bind& bind)
{
    static const HashedName callerName{ "caller", HashedName::Hash("caller") };
    static const HashedName kwargsName{ "kwargs", HashedName::Hash("kwargs") };
    static const HashedName varargsName{ "varargs", HashedName::Hash("varargs") };
    const auto& posParams = binder.callParams.posParams;
    const auto& kwParams = binder.callParams.kwParams;
    const SpecialSlots slots(binder.params.size(), binder.catchCaller, catchKwargs);
    if (binder.catchCaller)
    {
        // Jinja2: a macro called without a call block, or with caller=None, gets a caller that
        // fails when called
        auto p = kwParams.find(callerName.name);
        const bool hasCaller = p != kwParams.end() && !p->second.IsNone();
        bind(slots.caller, callerName, hasCaller ? p->second : MakeUndefinedWithHint(context, "No caller defined"));
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
        bind(slots.kwargs, kwargsName, CreateMapAdapter(std::move(kwArgs)));
    }
    if (catchVarargs)
    {
        InternalValueList varArgs;
        for (auto idx = binder.params.size(); idx < posParams.size(); ++idx)
        {
            varArgs.push_back(posParams[idx]);
        }
        bind(slots.varargs, varargsName, ListAdapter::CreateAdapter(std::move(varArgs)).MarkAsTuple());
    }
}

// Binds the defaults of the arguments that were not provided. Jinja2 evaluates every default
// on each call, in the body's scope where the macro is defined (0038 plan, section 4.3), so a
// default sees the names there as they are now, and `acc=[]` is a new list each time
template<typename Bind>
void BindMacroDefaults(const MacroArgBinder& binder, RenderContext& context, const Bind& bind)
{
    for (std::size_t idx = 0; idx < binder.params.size(); ++idx)
    {
        const auto& p = binder.params[idx];
        if (p.defaultValue && !binder.IsProvided(idx))
        {
            bind(idx, binder.Name(idx), binder.nodes[p.defaultValue].Evaluate(context));
        }
    }
}

template<typename Bind>
void BindMacroCall(const MacroArgBinder& binder, bool catchKwargs, bool catchVarargs, bool hasArgDefaults, RenderContext& context, const Bind& bind)
{
    // Missing arguments and the special ones are bound before the defaults are evaluated, so
    // a default sees them and never an outer variable named like a later argument. When no
    // default refers to the arguments nothing is evaluated in between, and a missing argument
    // with a default gets only the default.
    BindMacroArgs(binder, hasArgDefaults, context, bind);
    if (binder.catchCaller || catchKwargs || catchVarargs)
    {
        BindSpecialMacroArgs(binder, catchKwargs, catchVarargs, context, bind);
    }
    BindMacroDefaults(binder, context, bind);
}
} // namespace

// Binds the call arguments the way Jinja2's Macro.__call__ does
void MacroStatement::InvokeMacroRenderer(const CallParams& callParams, OutStream& stream, RenderContext& context, size_t link) const
{
    const RenderDepthGuard depthGuard;

    const auto caught = m_caughtNames;
    const bool catchCaller = (caught & UsesCaller) != 0;
    const bool catchKwargs = (caught & UsesKwargs) != 0;
    const bool catchVarargs = (caught & UsesVarargs) != 0;

    const MacroArgBinder binder{ context.Nodes(), m_params, callParams, catchCaller };
    if (binder.HasExtraArgs(catchKwargs, catchVarargs))
    {
        ThrowExtraMacroArgs(binder, catchKwargs, [this, &binder]() { return GetDisplayName(binder.nodes); });
    }

    // The body's `set`s go to the macro's scope, searched before its arguments. A scope or
    // view left by an exception is dropped by the HomeCall. A link that hides no scope is
    // harmless on the view of the arguments, which is pushed anyway
    auto scope = context.EnterScope();
    const UnitCall unitCall(context, m_unitLayout);
    if (m_slotNames.empty())
    {
        // Only the link, which hides the scopes between the body's and the definition
        const bool isLinked = link < context.GetScopesCount() - 1;
        if (isLinked)
        {
            context.PushFrameView({}, link);
        }
        BindMacroCall(binder, catchKwargs, catchVarargs, m_hasArgDefaults, context, [&scope](std::size_t /*slot*/, const HashedName& name, auto&& value) {
            scope.ForName(name) = std::forward<decltype(value)>(value);
        });
        context.Nodes()[m_mainBody].Render(stream, context);
        if (isLinked)
        {
            context.PopFrameView();
        }
        context.ExitScope();
        return;
    }

    // The arguments in the first slots of the frame (0117 P2), seen by name below the scope,
    // and the link below them
    const auto names = context.Nodes()[m_slotNames];
    const auto slots = context.Frame().slots.first(names.size());
    SlotsGuard guard(context, slots, names);
    context.PushFrameView({ slots, names, context.Nodes() }, link);
    BindMacroCall(binder, catchKwargs, catchVarargs, m_hasArgDefaults, context, [&context, slots, names](std::size_t slot, const HashedName& /*name*/, auto&& value) {
        context.BindSlot(slots[slot], names[slot].hash, std::forward<decltype(value)>(value));
    });
    context.Nodes()[m_mainBody].Render(stream, context);

    guard.Unbind();
    context.PopFrameView();
    context.ExitScope();
}

void MacroCallStatement::Render(OutStream& os, RenderContext& values)
{
    const auto& call = values.Nodes()[m_call];
    const auto fnVal = values.Nodes()[call.GetValueRef()].Evaluate(values);
    const auto* callable = GetIf<Callable>(&fnVal);
    if (!callable)
    {
        // Calling a name that does not exist is an UndefinedError under every policy
        CheckUndefinedUse(fnVal, UndefinedUse::Call);
        throw std::runtime_error("'"s + Apply<visitors::PythonTypeNameGetter>(fnVal) + "' object is not callable");
    }

    auto callParams = helpers::EvaluateCallParams(call.GetParams(), values);
    callParams.kwParams["caller"s] = MakeCallable(values);
    if (callable->GetType() == Callable::Type::Expression)
    {
        WriteOutput(os, callable->GetExpressionCallable()(callParams, values), values);
        return;
    }
    callable->GetStatementCallable()(callParams, os, values);
}

void DoStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    values.Nodes()[m_expr].Evaluate(values);
}

void WithStatement::Render(OutStream& os, RenderContext& values)
{
    auto innerValues = values.Clone(true);
    auto scope = innerValues.EnterScope();

    const auto& nodes = values.Nodes();
    for (const auto& var : nodes[m_scopeVars])
    {
        scope.ForName({ nodes.Text(var.name), var.hash }) = nodes[var.value].Evaluate(values);
    }

    values.Nodes()[m_mainBody].Render(os, innerValues);

    innerValues.ExitScope();
    values.SetLoopControl(innerValues.GetLoopControl());
}

void TransStatement::Render(OutStream& os, RenderContext& values)
{
    std::vector<InternalValue> evaluated;
    evaluated.reserve(m_variables.size());
    for (const auto var : values.Nodes()[m_variables])
    {
        evaluated.push_back(values.Nodes()[var].Evaluate(values));
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
    // The body is Markup under autoescape where the block is written; the filtered output is
    // written as is
    InternalValue body(std::move(arg));
    body.SetMarkup(values.IsOutputAutoescape());
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

void BindUndefinedStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    const auto& nodes = values.Nodes();
    if (nodes[m_names].empty())
    {
        return;
    }
    values.SetHasEntryBindings();
    auto scope = values.GetCurrentScope();
    for (const auto& name : nodes[m_names])
    {
        const auto text = nodes.Text(name.name);
        auto& value = scope.ForName({ text, name.hash });
        if (value.IsUndefined())
        {
            value = MakeUndefined(values, std::string(text));
            auto* undefined = GetIf<UndefinedValue>(&value);
            undefined->atEntry = true;
            undefined->entryHash = static_cast<std::uint32_t>(name.hash);
        }
    }
}
} // namespace jinja2
