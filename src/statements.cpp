#include "statements.h"

#include "call_stop.h"
#include "contracts.h"
#include "cycle_collector.h"
#include "render_failure.h"
#include "engine_errors.h"
#include "expression_evaluator.h"
#include "generation_tracker.h"
#include "generic_adapters.h"
#include "host_object.h"
#include "internal_value.h"
#include "intrusive_ptr.h"
#include "lookup_result.h"
#include "loop_attr.h"
#include "markup.h"
#include "name_resolver.h"
#include "node_arena.h"
#include "out_stream.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "render_workspace.h"
#include "renderer.h"
#include "slot_frame.h"
#include "small_vector.h"
#include "span.h"
#include "template_dict.h"
#include "template_impl.h"
#include "template_slots.h"
#include "undefined.h"
#include "value_methods.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <array>
#include <atomic>
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
#include <variant>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{
inline namespace v2
{

void ForStatement::Render(OutStream& os, RenderContext& values)
{
    InternalValue loopVal = values.Nodes()[m_value].Evaluate(values);

    if (m_isRecursive)
    {
        // Jinja2 renders a recursive loop into a buffer: loop(x) is a call of a macro
        const detail::GenerationFreeze freeze(values);
        RenderLoop(loopVal, os, values, 0);
        return;
    }
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
    throw TemplateRuntimeError(TemplateRuntimeError::PythonClass::TypeError, "'"s + Apply<visitors::PythonTypeNameGetter>(value) + "' object is not iterable");
}


// A target as the tree holds it: its root, then its items
using TargetNodes = Span<const TargetNode>;

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
    Span<Slot> slots;
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
            throw TemplateRuntimeError("cannot assign attribute on non-namespace object");
        }
        // The store may close a reference cycle, which the render frees when it ends
        methods::NoteContainerStore(*found, value);
        // ... and what it replaces may be one that a template module's earlier call made
        if (auto* items = CycleCollector::InModuleCall() ? ns->GetMutableItems() : nullptr)
        {
            if (auto old = items->FindString(std::string(nodes.Text(root.attr))); old != items->end())
            {
                CycleCollector::NoteDropped(old->second);
            }
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
    // An object (namespace(), cycler(), joiner()) is neither a mapping nor iterable; `loop`
    // is iterable
    if (ListAdapter items; map && map->GetIteration(items))
    {
        AssignTo(target, InternalValue(ListAdapter::CreateAdapter(items.ToValueList())), sink, values);
        return;
    }
    if (map && map->GetObjectClass())
    {
        throw TemplateRuntimeError("cannot unpack non-iterable "s + map->GetObjectClass() + " object");
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
            throw TemplateRuntimeError("cannot unpack non-iterable value");
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
        throw TemplateRuntimeError("too many values to unpack (expected " + std::to_string(count) + ")");
    }
    if (items.size() < count)
    {
        throw TemplateRuntimeError("not enough values to unpack (expected " + std::to_string(count) + ", got " + std::to_string(items.size()) + ")");
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

struct LoopState;

// What the callables loop.cycle and loop.changed made share with their loop: the position
// loop.cycle cycles by and the loop their repr prints, <bound method LoopContext.cycle of
// <LoopContext 1/2>> (docs/tasks/0305). The callables do not keep the loop: one kept past a
// loop collects the rest of a filtered loop, which a callable that cycles needs not. The
// loop is read while it is in its frame, and its last position and length after
struct LoopCycleCell final : IBoundReceiver
{
    // index0 of the loop, kept up to date
    size_t index0 = 0;
    // The loop while its frame holds it; null once the frame is reused or freed
    LoopState* live = nullptr;
    // The length of the loop once it is let go of, if known by then
    std::optional<size_t> length;

    [[nodiscard]] const char* GetClassName() const override { return "LoopContext"; }
    [[nodiscard]] std::string Repr() const override;
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
    // Whether the statement's body names `loop` (ForStatement::BodyReadsLoop)
    bool bodyReadsLoop = false;
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
    // Whether the loop is in volatile code (RenderContext::IsVolatileAutoescape)
    bool recursiveVolatile = false;
    // The arguments of the last loop.changed() call, made on first use: a list [called,
    // *args] its callables hold as their receiver, where the cycle collector sees them
    InternalValue lastChanged;
    // Shared with the callables loop.cycle and loop.changed made, kept up to date once one
    // is made (LoopCycleCell)
    std::shared_ptr<LoopCycleCell> cycleCell;
    // The `loop` this loop runs over (IsLoopObject), whose len() is this loop's length once
    // read. It stays here once read, so that the cycle collector sees it while the
    // enumerator runs over it (docs/tasks/0390)
    InternalValue lengthOf;
    // The statement's list, asked on the first read of the length whether the template can
    // change it (HasLiveSize): then its len() at that read is the length, as Jinja2's
    // LoopContext.length takes len(iterable) then, not listSize from the start
    // (docs/tasks/0390). A view, let go of when the statement ends; asked only then, so that
    // a loop that never reads its length pays no call for it
    std::optional<ListRef> sizeOfList;
    // Whether the item after the current one is fetched, isLast telling if there is one
    bool isPeeked = false;
    // Whether the enumerator can run past the statement: over indexedList, the rest the state
    // collected (CollectRest), or over a `loop` object, which collects its own rest when kept
    // (docs/tasks/0390); not over a list of the statement
    bool keepsEnumerator = false;
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

    // The cell the loop.cycle and loop.changed callables share, made on first use
    const std::shared_ptr<LoopCycleCell>& CycleCell()
    {
        if (!cycleCell)
        {
            cycleCell = std::make_shared<LoopCycleCell>();
            cycleCell->index0 = index0;
            cycleCell->live = this;
        }
        return cycleCell;
    }

    void SetIndex(size_t idx)
    {
        index0 = idx;
        if (cycleCell)
        {
            cycleCell->index0 = idx;
        }
    }

    // Leaves the callables that share the cell the loop's last position and its length, if
    // known: a loop that ran out knows it without collecting the rest
    JINJA2CPP_NOINLINE_INLINE void DetachCycleCell()
    {
        cycleCell->live = nullptr;
        if (listSize)
        {
            cycleCell->length = listSize;
        }
        else if (isPeeked && isLast)
        {
            cycleCell->length = index0 + 1;
        }
        cycleCell = nullptr;
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
            throw TemplateRuntimeError("'loop' is undefined in the loop filter");
        }
        // Most peeks are `loop.last` in an unfiltered loop: its enumerator alone, in this call
        if (!lazyFilter && enumerator)
        {
            auto& current = *enumerator;
            isAdvancing = true;
            isLast = !current->MoveNext();
            isAdvancing = false;
            if (!isLast)
            {
                Item(index0 + 1) = current->GetCurrent();
            }
        }
        else
        {
            isLast = !FetchNext(Item(index0 + 1));
        }
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
        // Before the length is forgotten
        if (cycleCell)
        {
            DetachCycleCell();
        }
        indexedList = ListAdapter();
        if (enumerator && !(*enumerator)->Rebind(nullptr))
        {
            enumerator.reset();
        }
        listSize.reset();
        sizeOfList.reset();
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
        // Most loops never read loop.changed: the reset is out of line, so that its temporary
        // gives this function no stack frame to guard
        if (!lastChanged.IsUndefined())
        {
            DropLastChanged();
        }
        isPeeked = false;
        keepsEnumerator = false;
        lazyFilter = nullptr;
        lazyContext = nullptr;
        lazyNodes = ArenaView();
        if (!lengthOf.IsUndefined())
        {
            DropLengthOf();
        }
    }

    JINJA2CPP_NOINLINE_INLINE void DropLastChanged() { lastChanged = InternalValue(); }
    JINJA2CPP_NOINLINE_INLINE void DropLengthOf() { lengthOf = InternalValue(); }

    // A loop object kept past a statement that ended without collecting its rest (an error
    // thrown in the body) can no longer fetch: its enumerator may run over a list the
    // statement owned, its filter in the render that failed
    void LetGoOfStatement()
    {
        // Nor read the size of its list, which may be gone
        sizeOfList.reset();
        if (!keepsEnumerator)
        {
            enumerator.reset();
        }
        lazyFilter = nullptr;
        lazyContext = nullptr;
        lazyNodes = ArenaView();
    }

    // The length of a filtered loop is known once the rest of the items are collected
    size_t GetLength()
    {
        if (listSize && !sizeOfList)
        {
            return *listSize;
        }
        return LengthOnFirstRead();
    }

    // len() of the list or the `loop` the loop runs over, kept as its length, or the length
    // the rest of the items gives
    JINJA2CPP_NOINLINE_INLINE size_t LengthOnFirstRead()
    {
        if (TakeLengthOf())
        {
            return listSize.value_or(0);
        }
        return CollectRest();
    }

    // Takes len() of the statement's list as the length if the template can change it, and
    // lets go of the list
    JINJA2CPP_NOINLINE_INLINE void TakeListSize()
    {
        if (sizeOfList->HasLiveSize()) // NOLINT(bugprone-unchecked-optional-access): asked only when set
        {
            listSize = sizeOfList->GetSize(); // NOLINT(bugprone-unchecked-optional-access): as above
        }
        sizeOfList.reset();
    }

    // Takes len() of sizeOfList or lengthOf into listSize; false when neither gives one
    JINJA2CPP_NOINLINE_INLINE bool TakeLengthOf()
    {
        if (sizeOfList)
        {
            TakeListSize();
            if (listSize)
            {
                return true;
            }
        }
        if (lengthOf.IsUndefined())
        {
            return false;
        }
        InternalValue length;
        const auto map = AsMap(lengthOf);
        // A host object: its len() when first read, as Jinja2 takes len(iterable) (docs/tasks/0441); counted from its
        // items when it has none
        if (const auto* host = map ? map->GetHostObject() : nullptr)
        {
            if (const auto hostLength = host->Len())
            {
                listSize = hostLength;
                return true;
            }
            lengthOf = InternalValue();
            return false;
        }
        if (map && map->GetLoopAttr(LoopAttr::Length, length))
        {
            if (const auto* count = GetIf<int64_t>(&length))
            {
                listSize = static_cast<size_t>(*count);
                return true;
            }
        }
        lengthOf = InternalValue();
        return false;
    }

    // Collects the items the loop has not fetched yet into a list of its own, which the loop
    // then moves through without the filter. For a filtered loop's length, and for a loop
    // object kept past its statement (`set ns.x = loop`), whose enumerator may run over a
    // list the statement owned: iterating the object goes on after the item it stopped at.
    // Returns the loop's length
    size_t CollectRest()
    {
        // Collecting the rest from inside the filter would replace the enumerator that is
        // running it. Jinja2 has no `loop` in the filter at all
        Peek();
        // A length taken from what the loop runs over stays: Jinja2 keeps the first len()
        if (!listSize || sizeOfList)
        {
            TakeLengthOf();
        }
        // On the last item the enumerator has nothing left to collect
        if (isLast)
        {
            listSize = listSize.value_or(index0 + 1);
            enumerator.reset();
            lazyFilter = nullptr;
            return index0 + 1;
        }

        // The rest after the next item, which is fetched
        InternalValueList rest;
        for (InternalValue item; FetchNext(item);)
        {
            CheckSequenceSize(rest.size() + 1, "a loop");
            rest.push_back(std::move(item));
        }

        const size_t length = listSize.value_or(index0 + rest.size() + 2);
        listSize = length;
        indexedList = ListAdapter::CreateAdapter(std::move(rest));
        enumerator = indexedList.GetEnumerator();
        keepsEnumerator = true;
        lazyFilter = nullptr;
        return length;
    }
};

// As Jinja2's LoopContext.__repr__, which counts the items a running loop has left. A loop
// let go of before its length was known (a `break` out of a filtered one) has no items left
// to count (docs/tasks/0366)
std::string LoopCycleCell::Repr() const
{
    if (live)
    {
        const auto loopLength = live->GetLength();
        return "<LoopContext " + std::to_string(live->index0 + 1) + "/" + std::to_string(loopLength) + ">";
    }
    if (length)
    {
        return "<LoopContext " + std::to_string(index0 + 1) + "/" + std::to_string(*length) + ">";
    }
    return "<LoopContext " + std::to_string(index0 + 1) + "/?>";
}

// Whether the loop runs over `loop`, whose len() is its length as Jinja2's LoopContext.length
// takes len(iterable): the length of the loop iterated, not the number of items it has left
// (docs/tasks/0304). Asked only of a list of unknown size
JINJA2CPP_NOINLINE_INLINE bool IsLoopObject(const InternalValue& loopVal)
{
    const auto map = AsMap(loopVal);
    if (!map || !map->Iterates())
    {
        return false;
    }
    const char* objectClass = map->GetObjectClass();
    return objectClass != nullptr && std::string_view(objectClass) == "LoopContext";
}

// Where the loop's length comes from: the size of the list, known at the start unless the
// template can change it (HasLiveSize), whose len() is taken when the length is first
// read, as Jinja2's LoopContext.length takes len(iterable) (docs/tasks/0390); or len() of
// the `loop` or the host object the loop runs over, read then too
// The length of a loop over items without a size is len() of the `loop` it runs over; a loop over a host object's
// items takes len() of the object, whatever its items give (a list subclass that overrides __iter__), as Jinja2 takes
// len(iterable) (docs/tasks/0390, 0441)
JINJA2CPP_NOINLINE_INLINE void SetLengthOfValue(LoopState& state, const InternalValue& loopVal, bool isHost)
{
    if (isHost)
    {
        state.listSize.reset();
        state.sizeOfList.reset();
        state.lengthOf = loopVal;
        return;
    }
    if (!state.listSize && IsLoopObject(loopVal))
    {
        state.lengthOf = loopVal;
    }
}

JINJA2CPP_ALWAYS_INLINE void SetLengthSource(LoopState& state, const ListAdapter& loopItems, const InternalValue& loopVal)
{
    state.sizeOfList = loopItems.Ref();
    state.listSize = loopItems.GetSize();
    const auto* map = std::get_if<detail::MapHandle>(&detail::InternalValueAccess::Data(loopVal));
    const bool isHost = map != nullptr && map->isHost;
    if (!state.listSize || isHost)
    {
        SetLengthOfValue(state, loopVal, isHost);
    }
}

// A loop object kept past the loop (`set ns.x = loop`) can no longer run the filter, which
// needs the render context, nor its enumerator, whose list the statement may own: the rest of
// the items is collected now. An unfiltered loop over `loop` goes on through the loop it runs
// over, as Jinja2's does: that loop is kept by this one's enumerator, and collects its own rest
// when it ends (docs/tasks/0390). The enumerator runs over the statement's adapter of it, which
// the state takes
JINJA2CPP_NOINLINE_INLINE void KeepLoopState(LoopState& state, bool isFiltered, const InternalValue& loopVal, ListAdapter&& loopItems)
{
    if (!isFiltered && IsLoopObject(loopVal))
    {
        // Moved: a copy of a generator's adapter would be a new generator
        state.indexedList = std::move(loopItems);
        state.keepsEnumerator = true;
        return;
    }
    state.CollectRest();
}

// Fetches the item after `itemIdx` once the body is done with it, through the enumerator
// alone (a loop that is not filtered lazily). True when there is one: the caller goes on to
// it, whose pass marks it not peeked again, so that only the end of the loop is stored
JINJA2CPP_ALWAYS_INLINE bool FetchAfterBody(LoopState& state, size_t itemIdx)
{
    // The loop's enumerator, which only the loop's end lets go of
    auto& enumerator = *state.enumerator; // NOLINT(bugprone-unchecked-optional-access): set while the loop runs
    state.isAdvancing = true;
    const bool hasNext = enumerator->MoveNext();
    state.isAdvancing = false;
    if (!hasNext)
    {
        state.isLast = true;
        state.isPeeked = true;
        return false;
    }
    state.Item(itemIdx + 1) = enumerator->GetCurrent();
    return true;
}

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

// loop.changed(*values): whether the values differ from those of the previous call, kept in
// `state`, the list [called, *args] (LoopState::lastChanged)
InternalValue CallLoopChanged(const InternalValue& state, const CallParams& params, RenderContext&)
{
    if (!params.kwParams.empty())
    {
        throw TemplateRuntimeError("changed() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
    }
    const auto list = AsList(state);
    auto* last = list ? list->GetMutableItems() : nullptr;
    // Emptied only by the cycle collector, once nothing can call this
    if (!last || last->empty())
    {
        return InternalValue();
    }
    auto isEqual = [](const InternalValue& lhs, const InternalValue& rhs) {
        return ConvertToBool(Apply2<visitors::BinaryMathOperation>(lhs, rhs, BinaryExpression::LogicalEq));
    };
    const auto& args = params.posParams;
    if (ConvertToBool(last->front()) && last->size() == args.size() + 1 && std::equal(args.begin(), args.end(), last->begin() + 1, isEqual))
    {
        return false;
    }
    // In place: a loop calling with as many values each time allocates nothing past the first call
    ReserveCounted(*last, args.size() + 1);
    last->resize(args.size() + 1);
    last->front() = true;
    std::copy(args.begin(), args.end(), last->begin() + 1);
    return true;
}

// Out of line, as MakeLoopCycle: inlined, it grows the frame of every loop attribute lookup
JINJA2CPP_NOINLINE_INLINE Callable MakeLoopChanged(LoopState& state)
{
    if (state.lastChanged.IsUndefined())
    {
        state.lastChanged = ListAdapter::CreateAdapter(InternalValueList{ InternalValue(false) });
    }
    const auto& lastChanged = state.lastChanged;
    Callable result(Callable::GlobalFunc, BoundMethodCall{ lastChanged, &CallLoopChanged });
    // Each read of loop.changed is a new callable; reads on one loop compare equal
    if (const auto list = AsList(lastChanged))
    {
        result.SetBoundMethod(list->GetIdentity(), "changed", state.CycleCell());
    }
    return result;
}
// loop.cycle(*values): the value at the loop's position when it is called, as Jinja2's
// bound method, so a callable stored on one item cycles with the loop on later ones, and
// from its last position once the loop is over. Out of line, which keeps the attribute
// lookups that inline LoopAccessor::GetProperty small (Render/for_loop_vars)
JINJA2CPP_NOINLINE_INLINE Callable MakeLoopCycle(LoopState& state)
{
    std::shared_ptr<const LoopCycleCell> cell = state.CycleCell();
    Callable result(Callable::GlobalFunc, [cell](const CallParams& params, RenderContext&) -> InternalValue {
        if (!params.kwParams.empty())
        {
            throw TemplateRuntimeError("LoopContext.cycle() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
        }
        const auto& args = params.posParams;
        if (args.empty())
        {
            throw TemplateRuntimeError("no items for cycling given");
        }
        return args[cell->index0 % args.size()];
    });
    result.SetBoundMethod(cell.get(), "cycle", cell);
    return result;
}

// loop(...): the recursion of a recursive loop; a loop without the recursive marker is
// callable in Jinja2, which then refuses
JINJA2CPP_NOINLINE_INLINE Callable MakeLoopCall(const LoopState& state)
{
    if (!state.recursiveStatement)
    {
        return Callable(Callable::GlobalFunc, [](const CallParams&, RenderContext&) -> InternalValue {
            throw TemplateRuntimeError("The loop must have the 'recursive' marker to be called recursively.");
        });
    }
    return ForStatement::MakeLoopRecursion(state.recursiveStatement, state.recursiveNodes, state.level, state.recursiveSite, state.recursiveAutoescape, state.recursiveVolatile);
}

// loop.__call__: loop(...) as the bound method it is in Jinja2, which prints
// <bound method LoopContext.__call__ of <LoopContext 1/2>> (docs/tasks/0474). The call slot
// stays a plain callable, so that a recursive loop shares no cell it does not need
JINJA2CPP_NOINLINE_INLINE Callable MakeLoopDunderCall(LoopState& state)
{
    auto result = MakeLoopCall(state);
    const auto& cell = state.CycleCell();
    result.SetBoundMethod(cell.get(), "__call__", cell);
    return result;
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
        return GenericMap(std::make_shared<const LoopAccessor>(*this));
    }
    [[nodiscard]] bool ShouldExtendLifetime() const override { return true; }
    // An object, not a mapping: dictsort and items refuse it, pprint prints its repr
    [[nodiscard]] const char* GetObjectClass() const override { return "LoopContext"; }
    // Iterating it takes the rest of the loop's own items, each as (item, loop), so that the
    // running loop goes on after them: Jinja2's LoopContext.__next__ (docs/tasks/0219)
    [[nodiscard]] bool Iterates() const override { return true; }
    // `for a in loop`: (item, loop) pairs, made from the loop's state as it advances
    bool GetIteration(ListAdapter& items) const override;
    [[nodiscard]] const LoopState* State() const { return m_state; }
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

    // The numbers and flags are written straight into `value`, and a `last` already peeked
    // read in place; the rest is out of line (GetOtherLoopAttr), so that the common reads pay
    // no stack frame for the callables and temporaries the others make (docs/tasks/0391)
    bool GetLoopAttr(LoopAttr attr, InternalValue& value) const override
    {
        auto& state = *m_state;
        switch (attr)
        {
        // The neighbouring items share the loop's data, and `changed` makes a callable:
        // the generic lookup handles them
        case LoopAttr::None:
        case LoopAttr::PrevItem:
        case LoopAttr::NextItem:
        case LoopAttr::Changed:
        case LoopAttr::Call:
        case LoopAttr::DunderCall:
            return false;
        case LoopAttr::Index:
            value.SetImmediate(static_cast<int64_t>(state.index0 + 1));
            return true;
        case LoopAttr::Index0:
            value.SetImmediate(static_cast<int64_t>(state.index0));
            return true;
        case LoopAttr::First:
            value.SetImmediate(state.index0 == 0);
            return true;
        case LoopAttr::Last:
            state.Peek();
            value.SetImmediate(state.isLast);
            return true;
        case LoopAttr::Length:
            if (!state.listSize || state.sizeOfList)
            {
                break;
            }
            value.SetImmediate(static_cast<int64_t>(*state.listSize));
            return true;
        case LoopAttr::Depth:
            value.SetImmediate(static_cast<int64_t>(state.level) + 1);
            return true;
        case LoopAttr::Depth0:
            value.SetImmediate(static_cast<int64_t>(state.level));
            return true;
        default:
            break;
        }
        GetOtherLoopAttr(attr, value);
        return true;
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
        if (name == "__call__")
        {
            return Property::DunderCall;
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
            return static_cast<int64_t>(state.GetLength()) - static_cast<int64_t>(state.index0);
        case Property::RevIndex0:
            return static_cast<int64_t>(state.GetLength()) - static_cast<int64_t>(state.index0) - 1;
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
            return MakeLoopChanged(state);
        case Property::Call:
            return MakeLoopCall(state);
        case Property::DunderCall:
            return MakeLoopDunderCall(state);
        case Property::None:
            break;
        }
        return InternalValue();
    }

    JINJA2CPP_NOINLINE_INLINE void GetOtherLoopAttr(LoopAttr attr, InternalValue& value) const { value = GetProperty(attr); }

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
    LoopFrame() noexcept { SetTraversable(); }
    LoopFrame(const LoopFrame&) = delete;
    LoopFrame(LoopFrame&&) = delete;
    LoopFrame& operator=(const LoopFrame&) = delete;
    LoopFrame& operator=(LoopFrame&&) = delete;
    // The callables loop.cycle made may outlive the frame
    ~LoopFrame() override
    {
        if (cycleCell)
        {
            DetachCycleCell();
        }
    }

    // A loop object the template keeps past its loop (`set ns.x = loop`) holds the list and
    // the items, which may hold it back (docs/tasks/0251)
    void Traverse(const TraverseFn& fn) const override
    {
        TraverseValue(lastChanged, fn);
        TraverseValue(lengthOf, fn);
        if (const auto* list = indexedList.Ref().GetOwner())
        {
            fn(list);
        }
        for (const auto& item : items)
        {
            TraverseValue(item, fn);
        }
        for (const auto& entry : loopScope)
        {
            TraverseValue(entry.second, fn);
        }
        for (const auto& entry : bodyScope)
        {
            TraverseValue(entry.second, fn);
        }
    }

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

// It holds the loop object, the frame's own accessor, where the cycle collector sees it: in a
// std::function capture a kept loop over `loop` (`set ns.k = loop`) closed a cycle through the
// outer loop that nothing freed (docs/tasks/0400)
bool LoopAccessor::GetIteration(ListAdapter& items) const
{
    // Every loop state is a frame's (LoopFramePool)
    auto& frame = *static_cast<detail::LoopFrame*>(m_state);
    items = ListAdapter::CreateGenerator(
        MapAdapter(&frame.accessor, ValueObjectPtr(&frame)),
        [state = m_state]() {
            state->Peek();
            if (state->isLast)
            {
                return false;
            }
            state->SetIndex(state->index0 + 1);
            state->isPeeked = false;
            return true;
        },
        [state = m_state](const InternalValue& loop) { return InternalValue(ListAdapter::CreateAdapter(InternalValueList{ state->Item(state->index0), loop }).MarkAsTuple()); });
    return true;
}

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
    static IntrusivePtr<LoopFrame> Take(uint64_t id)
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
    JINJA2CPP_NOINLINE_INLINE static IntrusivePtr<LoopFrame> TakeOther(uint64_t id)
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
    JINJA2CPP_NOINLINE_INLINE static void Give(IntrusivePtr<LoopFrame>& frame)
    {
        if (!frame)
        {
            return;
        }
        // Kept past the loop (ForStatement's collection of the rest, or an error)
        if (frame->use_count() != 1)
        {
            frame->LetGoOfStatement();
            return;
        }
        auto& frames = Frames();
        if (frames.size() >= RenderWorkspace::MaxLoopFrames)
        {
            return;
        }
        frame->Recycle();
        frames.push_back(std::move(frame));
    }

private:
    // At most RenderWorkspace::MaxLoopFrames, deeper than any loop nesting a template has by
    // hand; recursive loops past it allocate
    static std::vector<IntrusivePtr<LoopFrame>>& Frames() { return RenderWorkspace::ForThisThread().LoopFrames(); }
};

// Gives the frame of a loop back to the pool when the loop ends, by any path
struct LoopFrameReturn
{
    explicit LoopFrameReturn(IntrusivePtr<LoopFrame>& loopFrame)
        : frame(loopFrame)
    {
    }
    LoopFrameReturn(const LoopFrameReturn&) = delete;
    LoopFrameReturn(LoopFrameReturn&&) = delete;
    LoopFrameReturn& operator=(const LoopFrameReturn&) = delete;
    LoopFrameReturn& operator=(LoopFrameReturn&&) = delete;
    ~LoopFrameReturn() { LoopFramePool::Give(frame); }

    IntrusivePtr<LoopFrame>& frame;
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
    SlotsGuard(RenderContext& values, Span<Slot> slots, Span<const SlotName> names)
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
    Span<Slot> m_slots;
    Span<const SlotName> m_names;
    size_t m_viewCount;
    bool m_isDone = false;
};
} // namespace

Callable ForStatement::MakeLoopRecursion(ForStatement* statement, const ArenaView& nodes, int level, const ClosureSite& site, bool autoescape, bool isVolatile)
{
    return Callable(Callable::GlobalFunc, [statement, nodes, level, site, autoescape, isVolatile](const CallParams& params, RenderContext& context) -> InternalValue {
        // As Python's LoopContext.__call__(iterable)
        for (const auto& kwParam : params.kwParams)
        {
            if (kwParam.first != "iterable")
            {
                throw TemplateRuntimeError("LoopContext.__call__() got an unexpected keyword argument '" + kwParam.first + "'");
            }
            if (!params.posParams.empty())
            {
                throw TemplateRuntimeError("LoopContext.__call__() got multiple values for argument 'iterable'");
            }
        }
        if (params.posParams.size() > 1)
        {
            throw TemplateRuntimeError("LoopContext.__call__() takes 2 positional arguments but " + std::to_string(params.posParams.size() + 1) + " were given");
        }
        bool isSucceeded = false;
        auto parsedParams = helpers::ParseCallParams({ { "iterable", true } }, params, isSucceeded);
        if (!isSucceeded)
        {
            throw TemplateRuntimeError("LoopContext.__call__() missing 1 required positional argument: 'iterable'");
        }

        auto var = parsedParams["iterable"];
        // Python iterates an undefined as empty, and None not at all
        if (var.IsNone())
        {
            throw TemplateRuntimeError("'NoneType' object is not iterable");
        }
        if (var.IsUndefined())
        {
            InternalValue empty(RenderToString(context.GetRendererCallback(), [](OutStream&) {}));
            empty.SetMarkup(isVolatile ? context.IsAutoescape() : autoescape);
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
        const OutputAutoescapeGuard autoescapeGuard(home, autoescape, isVolatile);
        home.EnterScope();
        if (isLinked)
        {
            home.PushFrameView({}, homeCall.Link());
        }
        // Jinja2's loop(...) returns Markup by the setting the loop is compiled under, the
        // runtime one in volatile code
        InternalValue result(RenderToString(home.GetRendererCallback(), [&](OutStream& stream) { statement->RenderLoop(var, stream, home, level + 1, &site); }));
        result.SetMarkup(home.IsOutputAutoescape());
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
        JINJA2CPP_ASSERT(values.Frame().unit == m_unit);
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
    values.CaptureClosures(FrameEnd::Soft(values.GetScopesCount()));
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
    state->bodyReadsLoop = m_bodyReadsLoop;
    if (m_isRecursive)
    {
        state->recursiveStatement = this;
        state->recursiveNodes = values.Nodes();
        // loop(...) runs every level where the outermost one runs (ClosureSite)
        state->recursiveSite = site ? *site : ClosureSite(values);
        state->recursiveAutoescape = values.IsStaticAutoescape();
        state->recursiveVolatile = values.IsVolatileAutoescape();
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
        SetLengthSource(*state, loopItems, loopVal);
    }

    bool loopRendered = false;
    // The next item is fetched after the body, or when the body peeks (`loop.last`), as
    // Jinja2's LoopContext does: a body that changes the list or iterates `loop` sees the
    // items Jinja2's does (docs/tasks/0304). A filtered loop runs its filter then too
    bool hasItem = state->FetchNext(state->Item(0));
    // One scope for the body, emptied after each pass, so `set` in the body stays local
    // to one iteration without a map being made for each
    auto bodyScope = values.EnterScope(std::move(state->bodyScope));
    auto& targetSlots = state->targetSlots;
    const auto target = values.Nodes()[m_target];
    for (size_t itemIdx = 0; hasItem; ++itemIdx)
    {
        state->SetIndex(itemIdx);
        state->isPeeked = false;
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
        // The body took items by iterating `loop` (LoopAccessor::GetIteration): go on after them
        itemIdx = state->index0;
        if (!state->isPeeked && FetchAfterBody(*state, itemIdx))
        {
            continue;
        }
        hasItem = !state->isLast;
    }
    // A closure defined in the body and kept past the loop sees the frame below it (0373)
    values.CaptureClosures(FrameEnd::Soft(values.GetScopesCount() - 1));
    values.ExitScope(state->bodyScope);

    // A loop object kept past the loop (`set ns.x = loop`) can no longer run the filter,
    // which needs this render context, nor its enumerator, whose list the statement may own:
    // collect the rest of the items now (copies of `loop` and the accessor a GenericMap
    // copies from it all own the state: more owners than this function and the scope mean
    // it was kept)
    if (state->use_count() > 2)
    {
        KeepLoopState(*state, static_cast<bool>(m_ifExpr), loopVal, std::move(loopItems));
    }
    // The list may go with the statement: loop.cycle callables kept past it print its size
    if (state->cycleCell && state->sizeOfList)
    {
        state->TakeListSize();
    }
    state->sizeOfList.reset();

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
    state->bodyReadsLoop = m_bodyReadsLoop;
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
        SetLengthSource(*state, loopItems, loopVal);
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
        // The next item is fetched after the body, or when the body peeks (docs/tasks/0304)
        state->isPeeked = false;
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
        // The body took items by iterating `loop` (LoopAccessor::GetIteration): go on after them
        itemIdx = state->index0;
        if (!state->isPeeked)
        {
            if (state->lazyFilter)
            {
                state->isLast = !fetchNext(state->Item(itemIdx + 1));
                state->isPeeked = true;
            }
            else if (FetchAfterBody(*state, itemIdx))
            {
                continue;
            }
        }
        hasItem = !state->isLast;
    }

    // A loop object kept past the loop (`set ns.x = loop`) can no longer run the filter,
    // which needs this render context, nor its enumerator, whose list the statement may own:
    // collect the rest of the items now (more owners than this function and the slot of
    // `loop` mean it was kept)
    if (state->use_count() > 2)
    {
        KeepLoopState(*state, static_cast<bool>(m_ifExpr), loopVal, std::move(loopItems));
    }
    // The list may go with the statement: loop.cycle callables kept past it print its size
    if (state->cycleCell && state->sizeOfList)
    {
        state->TakeListSize();
    }
    state->sizeOfList.reset();
    state->lazyFilter = nullptr;

    guard.Unbind();
    values.CaptureClosures(FrameEnd::Soft(values.GetScopesCount()));
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
    SmallVector<SlotName, 4> names{ SlotName{ nodes.MakeText("loop"), HashedName::Hash("loop") } };
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

bool ForStatement::FilterInFrame(IListAccessorEnumerator& items, Span<Slot> frameSlots, InternalValue& item, RenderContext& values) const
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
        // The items the filter skips render no body, and a plain name or constant checks nothing: each item is a
        // render step here (Settings::Limits::maxRenderSteps), and reaches a host's poll (RenderOptions::poll) in a
        // template loaded without limits too
        CheckStack();
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
    if (Reexports() && values.HasModuleImports())
    {
        Reexport(values);
    }
}

InternalValue SetBlockStatement::RenderBody(RenderContext& values)
{
    auto innerValues = values.Clone(true);
    TargetString result = RenderToString(values.GetRendererCallback(), [&](OutStream& stream) { values.Nodes()[m_body].Render(stream, innerValues); });
    innerValues.CaptureClosuresAtSoftEnd();
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
    if (Reexports() && values.HasModuleImports())
    {
        Reexport(values);
    }
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
    if (Reexports() && values.HasModuleImports())
    {
        Reexport(values);
    }
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
    // Jinja2 renders a block through a generator: a StopIteration out of it is a RuntimeError, not a stopped call
    try
    {
        block.node.RenderBody(os, blockContext, depth);
    }
    catch (detail::RenderFailure& failure)
    {
        failure.SealStop();
        throw;
    }
}

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
        throw TemplateRuntimeError("BlockReference.__call__() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
    }
    if (!params.posParams.empty())
    {
        throw TemplateRuntimeError("BlockReference.__call__() takes 1 positional argument but " + std::to_string(params.posParams.size() + 1) + " were given");
    }
}

// `callable` keeping `keepAlive` (Callable::SetKeepAlive)
Callable WithKeepAlive(Callable&& callable, std::shared_ptr<const void> keepAlive)
{
    callable.SetKeepAlive(std::move(keepAlive));
    return std::move(callable);
}

// What `kept` holds of the frames a closure was defined in, into `scope`, each closure keeping
// its capture again (StrengthenCapture)
void CopyCaptured(ScopeRef& scope, const ClosureKept& kept)
{
    // An inner frame's names hide those of the frames below it
    for (auto frame = kept.frames.rbegin(); frame != kept.frames.rend(); ++frame)
    {
        for (const auto& [name, value] : **frame)
        {
            scope[name] = MacroStatement::StrengthenCapture(value);
        }
    }
}

// CopyCaptured into the current scope of `context`, when the call `home` of the closure of
// `site` sees what it kept
void CopyCaptured(RenderContext& context, const ClosureSite& site, const ClosureHome& home)
{
    if (home.kept != ClosureHome::Kept::None)
    {
        auto scope = context.GetCurrentScope();
        CopyCaptured(scope, site.Kept(home));
    }
}

// `self` for the blocks of `stack`, Jinja2's TemplateReference printed with the context's
// `name`: a callable per block. A call renders the block in the template `self` came from
// while that template runs (the importer, for `self` passed to an imported macro), or while
// the context of its top level is the template's (a module's, for `self` a module's macro
// stored), else in the template running then. The block sees the names of the template's top
// level, `site`, wherever it is called from, as a block function sees its template's context
// in Jinja2 (docs/tasks/0194). Once the stack has ended, the call renders the blocks it held
// (SelfBlocks), seeing the names `site` still has. `keepsCapture` false: the `self` does not
// keep what the site keeps, which keeps it (the copy of an ended include's names,
// RenderContext::KeepEndedSelf)
InternalValue MakeTemplateSelf(const BlocksStack& stack, const ClosureSite& site, std::string_view templateName, bool keepsCapture = true);

// A block of `self` (MakeTemplateSelf), named `name`: renders it in the template `self` came
// from while that runs, else where the comment of MakeTemplateSelf says
struct SelfBlockCall
{
    // `self` may outlive the stack and the trees its names are in
    std::string name;
    std::shared_ptr<SelfBlocks> owner;
    ClosureSite site;
    std::string_view templateName;

    void operator()(const CallParams& params, OutStream& stream, RenderContext& context) const
    {
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
        // Its template has ended: the block sees what that saw (ClosureCapture, 0323)
        CopyCaptured(blockContext, site, home);
        // The template has ended (an include that stored `self`): its blocks, in a frame
        // of their own
        TemplateFrame endedFrame;
        if (!frame && !owner->live)
        {
            endedFrame.blocks = &owner->ended;
            endedFrame.baseDepth = home.link;
            endedFrame.name = templateName;
            endedFrame.outer = curFrame;
            // Its scopes have ended too: a `self` in the block renders the ended blocks,
            // seeing what this one sees
            endedFrame.self = MakeTemplateSelf(owner->ended, site, templateName);
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
    }
};

InternalValue MakeTemplateSelf(const BlocksStack& stack, const ClosureSite& site, std::string_view templateName, bool keepsCapture)
{
    // What the site keeps (ClosureSite::CaptureOnEnd), while the `self` lives; the caller holds it
    const std::shared_ptr<const void> keepAlive = keepsCapture ? site.Capture() : nullptr;
    InternalValueMap self;
    if (!stack.blocks.empty() && !stack.self)
    {
        stack.self = std::make_shared<SelfBlocks>(&stack);
    }
    for (const auto& block : stack.blocks)
    {
        std::string blockName(block.first);
        self[blockName] = MakeWrapped(WithKeepAlive(Callable(Callable::Macro, SelfBlockCall{ blockName, stack.self, site, templateName }), keepAlive));
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
    // A body that stores `self` before reading it has none of its own: Jinja2 reads it from
    // the context, where a parent finds what its child set
    if (m_selfDepth > frame->baseDepth ? m_selfFromContext : frame->selfFromContext)
    {
        if (auto value = FindValue(name); value || !frame->childSelf)
        {
            return value;
        }
        return LookupResult(*frame->childSelf);
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
        ClosureSite site(*home, depth);
        // An included template's `self` stored past the include sees what the include saw (0323)
        std::shared_ptr<const void> keepAlive;
        if (site.OutlivesItsFrames(*home))
        {
            // Its blocks read any name
            keepAlive = site.CaptureOnEnd(*home, {}, true);
        }
        // The blocks keep it (MakeTemplateSelf)
        return MakeTemplateSelf(*frame->blocks, site, frame->name);
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
        self->ended.blocks = std::move(blocks);
    }
}

ClosureSite::ClosureSite(RenderContext& context, size_t depth)
    : m_home(context.Anchor())
    , m_endsWithCall(context.m_endsWithCall)
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

std::shared_ptr<RenderContext> ClosureSite::LiveHome(const ClosureCapture& capture)
{
    auto home = capture.home.lock();
    if (home && !capture.ended && home->HasScopeBelow(capture.depth, capture.serial))
    {
        return home;
    }
    return nullptr;
}

ClosureHome ClosureSite::ResolveKept(RenderContext& caller) const
{
    const auto capture = m_capture.lock();
    for (const auto* frame = capture.get(); frame; frame = frame->outer.get())
    {
        auto home = LiveHome(*frame);
        if (!home)
        {
            continue;
        }
        ClosureHome result;
        // The caller is alive: no lock
        if (home.get() == &caller)
        {
            result.context = &caller;
        }
        else
        {
            result.context = home.get();
            result.lock = std::move(home);
        }
        result.link = frame->depth;
        result.epoch = ClosureHome::Epoch::Fresh;
        result.kept = ClosureHome::Kept::Frames;
        return result;
    }
    auto ended = Ended(caller);
    ended.kept = ClosureHome::Kept::Frames;
    return ended;
}

ClosureKept ClosureSite::Kept(const ClosureHome& home) const
{
    ClosureKept kept;
    if (home.kept == ClosureHome::Kept::None)
    {
        return kept;
    }
    const auto capture = m_capture.lock();
    // The copies of the ended frames, down to the first one still there (ResolveKept)
    for (const auto* frame = capture.get(); frame; frame = frame->outer.get())
    {
        if (LiveHome(*frame))
        {
            break;
        }
        if (auto names = frame->names.lock())
        {
            kept.frames.push_back(std::move(names));
        }
    }
    return kept;
}

std::shared_ptr<ClosureCapture> ClosureSite::CaptureOnEnd(RenderContext& context, Span<const std::size_t> reads, bool readsAll)
{
    // The closures of one scope share its capture: each sees the others in its copy, and the
    // copy lives while any of them is held outside it
    auto capture = context.CaptureOfScope(m_depth);
    if (capture)
    {
        if (readsAll)
        {
            capture->readsAll = true;
        }
        else
        {
            capture->AddReads(reads);
        }
    }
    m_capture = capture;
    return capture;
}

void ClosureCapture::AddReads(Span<const std::size_t> hashes)
{
    if (readsAll || hashes.empty())
    {
        return;
    }
    // Sorted, so the sentinel comes last
    if (hashes.back() == NameResolver::AllNames)
    {
        readsAll = true;
        reads.clear();
        return;
    }
    if (reads.empty())
    {
        reads.assign(hashes.begin(), hashes.end());
        return;
    }
    // The closures of a scope defined again (in a loop, by each call) read the same names
    if (std::includes(reads.begin(), reads.end(), hashes.begin(), hashes.end()))
    {
        return;
    }
    SmallVector<std::size_t, 8> merged;
    merged.reserve(reads.size() + hashes.size());
    std::set_union(reads.begin(), reads.end(), hashes.begin(), hashes.end(), std::back_inserter(merged));
    reads = std::move(merged);
}

void MacroStatement::SetBodyReads(NodeArena& nodes, Span<const std::size_t> reads)
{
    m_bodyReads = nodes.MakeSpan(reads);
}

std::shared_ptr<ClosureCapture> RenderContext::CaptureOfScope(size_t depth)
{
    if (depth == 0)
    {
        return nullptr;
    }
    // The context whose own scope it is
    auto* owner = this;
    while (depth <= owner->m_parentDepth && owner->m_parent)
    {
        owner = owner->m_parent;
    }
    if (depth <= owner->m_parentDepth)
    {
        return nullptr;
    }
    const uint32_t serial = owner->SerialBelow(depth);
    if (owner->m_captures)
    {
        // A scope holding what a call of a kept closure sees stands for the closure's frames
        for (const auto& kept : owner->m_captures->kept)
        {
            if (kept.depth == depth && kept.serial == serial)
            {
                if (auto capture = kept.capture.lock())
                {
                    return capture;
                }
            }
        }
        const auto& sites = owner->m_captures->sites;
        for (auto site = sites.rbegin(); site != sites.rend(); ++site)
        {
            auto capture = site->lock();
            if (capture && capture->depth == depth && capture->serial == serial && !capture->ended)
            {
                return capture;
            }
        }
    }
    auto capture = std::make_shared<ClosureCapture>();
    capture->home = owner->Anchor();
    capture->depth = static_cast<uint32_t>(depth);
    capture->serial = serial;
    owner->AddClosureCapture(capture);
    return capture;
}

void RenderContext::AddClosureCapture(const std::shared_ptr<ClosureCapture>& capture)
{
    if (!m_captures)
    {
        m_captures = std::make_unique<ClosureCaptures>();
    }
    auto& sites = m_captures->sites;
    if (sites.size() >= m_captures->pruneAt)
    {
        // A macro defined in a loop leaves one per iteration, whose frame is gone
        sites.erase(std::remove_if(sites.begin(), sites.end(),
                                   [this](const std::weak_ptr<ClosureCapture>& site) {
                                       const auto p = site.lock();
                                       return !p || p->ended || !HasScopeBelow(p->depth, p->serial);
                                   }),
                    sites.end());
        auto& kept = m_captures->kept;
        kept.erase(std::remove_if(kept.begin(), kept.end(), [this](const ClosureCaptures::Kept& k) { return k.capture.expired() || !HasScopeBelow(k.depth, k.serial); }),
                   kept.end());
        m_captures->pruneAt = std::max(ClosureCaptures{}.pruneAt, sites.size() * 2);
    }
    sites.push_back(capture);
}

void RenderContext::CaptureClosuresImpl(const FrameEnd& end)
{
    // The frame's scopes: from `count - 1` up, all of this context's own when it ends
    const size_t count = end.count != 0 ? end.count : m_parentDepth + 1;
    // The `self` of an included template that ends, made before its blocks went (KeepEndedSelf)
    const auto endedSelf = end.count == 0 ? std::move(m_captures->endedSelf) : nullptr;
    auto& sites = m_captures->sites;
    // Most frames end with one capture, or a few: no allocation on the macro call path
    SmallVector<std::shared_ptr<ClosureCapture>, 4> ending;
    size_t kept = 0;
    for (auto& site : sites)
    {
        auto capture = site.lock();
        if (!capture || capture->ended || !HasScopeBelow(capture->depth, capture->serial))
        {
            continue;
        }
        if (capture->depth < count)
        {
            sites[kept++] = std::move(site);
            continue;
        }
        ending.push_back(std::move(capture));
    }
    sites.resize(kept);
    // Something outside the frame holds one of its closures, or a capture of a frame above
    // it: those see what the frame held. The others are dropped with the frame
    if (ending.empty() || std::none_of(ending.begin(), ending.end(), [this](const auto& capture) { return CaptureEscapes(capture); }))
    {
        if (sites.empty() && m_captures->kept.empty())
        {
            m_captures.reset();
        }
        return;
    }
    // What the closures of the frame and of the frames above it read there: the copy keeps
    // those, the frame below sees them read too. An included template's top level keeps every
    // name, as Jinja2's context of the include does
    ClosureCapture keeps;
    keeps.readsAll = end.count == 0;
    for (const auto& capture : ending)
    {
        keeps.AddReads(*capture);
    }
    // One copy for the frame, holding its closures weakly
    std::shared_ptr<InternalValueMap> names;
    if (end.copies)
    {
        names = std::make_shared<InternalValueMap>();
        CollectNamesDownTo(*names, end.copyFrom, ending, keeps.readsAll ? nullptr : &keeps);
        // Read as `self` in the scopes over the caller's top level (FindSelf), unless set there
        if (endedSelf)
        {
            names->emplace("self", std::move(*endedSelf));
        }
    }
    // Made before the frame's captures leave the list: the frame below is another scope
    const auto outer = CaptureOfScope(end.continuation);
    if (outer)
    {
        outer->AddReads(keeps);
    }
    for (auto& capture : ending)
    {
        capture->ended = true;
        capture->outer = outer;
        if (!names)
        {
            continue;
        }
        // The ones held outside keep the copy; the others, which only closures in the copy
        // reach, live as long as it does. Outside a render nothing would release the copy:
        // the closures read the frame as one whose names are undefined
        const bool isHeld = CaptureEscapes(capture);
        if (isHeld ? CycleCollector::KeepUntilEnd(names, capture, names.get()) : CycleCollector::KeepUntilEnd(capture, names))
        {
            capture->names = names;
        }
    }
}

bool RenderContext::CaptureEscapes(const std::shared_ptr<ClosureCapture>& capture) const
{
    // The boxes of the closures that keep `capture` in the frame that ends, and how many of the
    // frame's values hold each: a value copied elsewhere shares its box (RecursiveWrapper)
    struct Held
    {
        const ValueObject* box;
        unsigned count;
    };
    std::array<Held, 32> held{};
    size_t boxes = 0;
    bool overflow = false;
    const auto note = [&held, &boxes, &overflow, &capture](const InternalValue& value) {
        const auto* callable = GetIf<Callable>(&value);
        if (!callable || callable->GetKeepAlive() != capture.get())
        {
            return;
        }
        const auto* box = detail::InternalValueAccess::Object(value);
        // data(): std::array's iterator is a class in MSVC's library
        Held* const end = held.data() + boxes;
        if (Held* const found = std::find_if(held.data(), end, [box](const Held& h) { return h.box == box; }); found != end)
        {
            ++found->count;
        }
        else if (boxes != held.size())
        {
            held[boxes++] = Held{ box, 1 };
        }
        else
        {
            overflow = true;
        }
    };
    const size_t frame = capture->depth - 1;
    VisitScopes(
        *this,
        [&note, frame](const InternalValueMap* scope, size_t depth) {
            if (depth < frame)
            {
                return true;
            }
            for (const auto& entry : *scope)
            {
                note(entry.second);
            }
            return false;
        },
        [&note](const FrameView& view, size_t /*depth*/) {
            for (size_t idx = 0; idx != view.names.size(); ++idx)
            {
                if (view.slots[idx].IsBound())
                {
                    note(static_cast<const InternalValue&>(view.slots[idx]));
                }
            }
            return false;
        });
    if (overflow)
    {
        return true;
    }
    // The callables in their boxes hold `capture`, and so does the caller: a count never below
    // the holders, so a higher one (another thread's, say) only makes a copy that was not needed
    if (capture.use_count() > static_cast<long>(boxes) + 1) // use_count: a lower count than the holders is impossible
    {
        return true;
    }
    // use_count: as above, a box is held at least by the frame's values that hold it
    return std::any_of(held.data(), held.data() + boxes, [](const Held& h) { return !h.box || h.box->use_count() > h.count; });
}

namespace
{
// Adds `name` to a copy of frames (RenderContext::CollectNamesDownTo), weakened when it is a
// closure keeping one of `captures`
void AddCollected(InternalValueMap& collected, std::string name, const InternalValue& value, Span<const std::shared_ptr<ClosureCapture>> captures)
{
    if (auto weakened = MacroStatement::WeakenCapture(value, captures))
    {
        collected.emplace(std::move(name), std::move(*weakened));
        return;
    }
    collected.emplace(std::move(name), value);
}
} // namespace

void RenderContext::CollectNamesDownTo(InternalValueMap& collected, size_t lowest, Span<const std::shared_ptr<ClosureCapture>> captures, const ClosureCapture* keeps) const
{
    const size_t entryPassDepth = m_entryPassDepth;
    VisitScopes(
        *this,
        [&collected, lowest, entryPassDepth, captures, keeps](const InternalValueMap* scope, size_t depth) {
            if (depth < lowest)
            {
                return true;
            }
            for (const auto& [name, value] : *scope)
            {
                const auto hash = HashedName::Hash(name);
                if (keeps && !keeps->Reads(hash))
                {
                    continue;
                }
                // The names an include's context passes over (PassOverEntryBindings)
                if (depth < entryPassDepth && IsEntryBinding(value, hash))
                {
                    continue;
                }
                AddCollected(collected, name, value, captures);
            }
            return false;
        },
        [&collected, captures, keeps](const FrameView& view, size_t /*depth*/) {
            for (size_t idx = 0; idx != view.names.size(); ++idx)
            {
                if (!view.slots[idx].IsBound())
                {
                    continue;
                }
                if (keeps && !keeps->Reads(view.names[idx].hash))
                {
                    continue;
                }
                AddCollected(collected, std::string(view.nodes.Text(view.names[idx].name)), static_cast<const InternalValue&>(view.slots[idx]), captures);
            }
            return false;
        });
}

void RenderContext::EnterCapturedScope(const ClosureKept& kept, size_t link, const std::shared_ptr<ClosureCapture>& capture)
{
    auto scope = EnterScope();
    CopyCaptured(scope, kept);
    // A frame of the call that ends sees the closure's frames below it (CaptureOfScope)
    if (capture)
    {
        const auto depth = static_cast<uint32_t>(GetScopesCount());
        const uint32_t serial = SerialBelow(depth);
        if (!m_captures)
        {
            m_captures = std::make_unique<ClosureCaptures>();
        }
        m_captures->kept.push_back({ depth, serial, capture });
    }
    if (link < GetScopesCount() - 1)
    {
        PushFrameView({}, link);
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
        try
        {
            values.Nodes()[m_mainBody].Render(os, innerContext);
        }
        catch (detail::RenderFailure& failure)
        {
            failure.SealStop();
            throw;
        }
        return;
    }

    // A top-level block after `extends` is only a definition: the parent decides where it
    // goes. Jinja2 calls one in a scoped statement where it stands as well (docs/tasks/0389)
    if (frame->parent && !m_inPlace)
    {
        return;
    }

    const auto name = GetName(values.Nodes());
    auto p = frame->blocks->blocks.find(name);
    if (m_isRequired && (p == frame->blocks->blocks.end() || p->second.size() <= 1))
    {
        throw TemplateRuntimeError("Required block '" + std::string(name) + "' not found");
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
                RenderBlockAt(owner->live ? *owner->live : owner->ended, name, depth + 1, stream, superContext);
            });
        }
        else
        {
            parentBlock.emplace(Callable::Macro, [name](const CallParams&, OutStream&, RenderContext&) {
                throw TemplateRuntimeError("there is no parent block called '" + std::string(name) + "'.");
            });
        }
    }
    const size_t prevSelfDepth = values.SetSelfDepth(values.GetScopesCount());
    const bool prevSelfFromContext = values.SetSelfFromContext(m_selfFromContext);
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
    // A closure defined in the block and kept past it sees what the block saw above its
    // template's top level, as Jinja2's block function sees its derived context (0373)
    {
        const size_t count = values.GetScopesCount();
        const size_t root = frame ? std::min(frame->baseDepth, count - 1) : count - 1;
        values.CaptureClosures(FrameEnd{ count, true, root, root });
    }
    values.ExitScope();
    values.SetSelfFromContext(prevSelfFromContext);
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
    // the parent's: in Jinja2 `self` is a local of each template's root function. It is still
    // in the context, for a parent with no `self` of its own, and the module's export when the
    // parent sets none
    auto scope = values.GetCurrentScope();
    std::optional<InternalValue> childSelf;
    if (const auto p = scope.Map().find("self"); p != scope.Map().end())
    {
        childSelf.emplace(p->second);
        scope.Erase("self");
    }
    // Its blocks are on the stack already: `extends` pushed them (ExtendsStatement::Render).
    // A grandparent sees the `self` the first template of the chain set
    const auto* frame = values.GetTemplateFrame();
    const ParentRender asParent{ childSelf ? &*childSelf : (frame ? frame->childSelf : nullptr) };
    RenderBody(os, values, &stack, childName, &asParent);
    if (childSelf && scope.Map().find("self") == scope.Map().end())
    {
        scope["self"] = std::move(*childSelf);
    }
}

void TemplateRenderer::RenderAsModule(OutStream& os, RenderContext& values, BlocksStack& stack, std::string_view name)
{
    PushBlocks(*values.GetRendererCallback(), values.Nodes(), stack);
    RenderBody(os, values, &stack, name);
}

void TemplateRenderer::RenderBody(OutStream& os, RenderContext& values, BlocksStack* stack, std::string_view name, const ParentRender* asParent)
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
    frame.extendable = stack;
    frame.baseDepth = values.GetScopesCount();
    frame.name = name;
    frame.selfFromContext = m_selfFromContext;
    frame.childSelf = asParent ? asParent->childSelf : nullptr;
    TemplateFrameGuard frameGuard(values, &frame);

    // A template renders through a generator in Jinja2: a StopIteration out of an included, imported or extended
    // one is a RuntimeError, not a stopped call (call_stop.h)
    try
    {
        if (!m_hasExtends)
        {
            values.Nodes()[m_body].Render(os, values);
            if (stack && values.KeepsClosures())
            {
                KeepEndedSelf(values, frame);
            }
            return;
        }
        // The output tags after an `extends` are dropped or skipped (OutputCheckStatement); what
        // an include or a call block writes there comes out before the parent's, as in Jinja2
        values.Nodes()[m_body].Render(os, values);
        if (frame.parent)
        {
            const auto parent = values.GetRendererCallback()->Templates().Resolve(frame.parent);
            const ArenaSwitch nodesSwitch(values, parent.nodes);
            // A template that extends another has a stack of its own
            JINJA2CPP_ASSERT(stack);
            parent.node.RenderAsParent(os, values, *stack, name);
        }
        if (values.KeepsClosures())
        {
            KeepEndedSelf(values, frame);
        }
    }
    catch (detail::RenderFailure& failure)
    {
        failure.SealStop();
        throw;
    }
}

void TemplateRenderer::KeepEndedSelf(RenderContext& values, const TemplateFrame& frame)
{
    // An included template whose closures something outside holds: they read `self` once it
    // has ended, as Jinja2's macros read the template's root function's (docs/tasks/0373)
    if (!frame.blocks || frame.blocks->blocks.empty() || frame.baseDepth != values.GetScopesCount())
    {
        return;
    }
    // Held by the copy of the template's names, which its site's capture keeps: it holds the
    // capture weakly, so that the copy does not keep itself
    ClosureSite site(values, frame.baseDepth);
    // The capture of the top level, shared with its closures and held by the frames above it
    // that something outside holds; dropped here when none holds it
    [[maybe_unused]] const auto capture = site.CaptureOnEnd(values, {}, true);
    values.KeepEndedSelf(MakeTemplateSelf(*frame.blocks, site, frame.name, false));
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
        throw TemplateRuntimeError("extended multiple times");
    }

    auto name = values.Nodes()[m_templateExpr].Evaluate(values);
    const auto& tpl = values.GetRendererCallback()->LoadTemplate(name);
    auto& templates = values.GetRendererCallback()->Templates();
    frame->parent = VisitTemplateImpl<TemplateNode<TemplateRenderer>>(tpl, true, [&templates](const auto& tplPtr) {
        return TemplateNode<TemplateRenderer>{ templates.Add(tplPtr->Nodes()), tplPtr->GetRenderer() };
    });
    // Jinja2 adds the parent's blocks to the context when `extends` runs, so that the rest of the
    // child's top level (a `self.name()` call) already finds them below the child's own
    if (frame->extendable)
    {
        const auto parent = templates.Resolve(frame->parent);
        parent.node.PushBlocks(*values.GetRendererCallback(), parent.nodes, *frame->extendable);
    }
}

const LoopAccessor* AsLoopAccessor(const InternalValue& value)
{
    const auto map = AsMap(value);
    return map ? dynamic_cast<const LoopAccessor*>(map->GetAccessor()) : nullptr;
}

bool IsLoopObject(const InternalValue& value)
{
    return AsLoopAccessor(value) != nullptr;
}

// Takes the `loop` of a running for loop out of sight of the template included from it, and puts
// it back when the include is done
class LoopHiding
{
public:
    LoopHiding() = default;
    LoopHiding(const LoopHiding&) = delete;
    LoopHiding& operator=(const LoopHiding&) = delete;
    ~LoopHiding()
    {
        if (m_binding && m_saved)
        {
            *m_binding = std::move(*m_saved);
        }
    }
    // The binding of the loop itself becomes `undefined` in place. The scopes that hold it do not
    // change while the include renders in a scope of its own
    void Hide(InternalValue& binding, InternalValue undefined)
    {
        m_saved.emplace(std::move(binding));
        binding = std::move(undefined);
        m_binding = &binding;
    }

private:
    // Empty unless the loop is hidden: most includes hide nothing, and then cost no more than this
    std::optional<InternalValue> m_saved;
    InternalValue* m_binding = nullptr;
};

// `loop` as the included template of `values` sees it. With a `loop` of the same name outside the
// loop (a render parameter, a global, a top-level set) that one is shadowed in the include's own
// scope; otherwise the loop's binding is hidden for the length of the include
void HideIncludingLoop(RenderContext& values, RenderContext& innerContext, LoopHiding& hidden)
{
    static const HashedName loopName{ "loop", HashedName::Hash("loop") };
    const auto found = values.FindValue(loopName);
    if (!found)
    {
        return;
    }
    // Jinja2 passes the loop to the include only when the body names `loop`
    const auto* loopAccessor = AsLoopAccessor(*found);
    if (!loopAccessor || loopAccessor->State()->bodyReadsLoop)
    {
        return;
    }
    // The `loop` that is visible to Jinja2 there: an enclosing loop whose body names it, or a
    // value of that name set outside the loops
    if (auto outer = values.FindValueWhere("loop"s, [](const InternalValue& value) {
            const auto* accessor = AsLoopAccessor(value);
            return !accessor || accessor->State()->bodyReadsLoop;
        }))
    {
        innerContext.GetCurrentScope()["loop"s] = *outer;
        return;
    }
    if (const auto binding = values.FindForWrite("loop"))
    {
        // A plain undefined costs no allocation; a policy that tells the name apart (Strict, Debug,
        // Chainable) needs the name, as a missing `loop` would carry it
        auto* callback = values.GetRendererCallback();
        InternalValue undefined;
        if (callback && callback->GetSettings().undefinedPolicy != UndefinedPolicy::Default)
        {
            undefined = MakeUndefined(values, "loop");
        }
        hidden.Hide(*binding, std::move(undefined));
    }
}

// Renders `tpl` for `include`: the names it sets stay its own
template<typename CharT>
void RenderIncludedTemplate(const TemplateImpl<CharT>& tpl, bool withContext, OutStream& os, RenderContext& values)
{
    RenderContext innerContext = values.Clone(withContext);
    // Its macros and `self` kept past the include see what it saw (0323)
    innerContext.SetEndsBeforeRender();
    if (withContext)
    {
        // Jinja2 renders it in a new Context, with an EvalContext of its own
        innerContext.NewEvalContext();
        innerContext.PassOverEntryBindings();
        innerContext.EnterScope();
        // The names of the including loop are visible, `loop` is not (Jinja2's include context
        // takes the loop's targets, not its LoopContext); a `loop` set outside it is
        if (tpl.MayReadLoop())
        {
            // Declared after the context: the loop is put back once the included template has rendered
            LoopHiding hidden;
            HideIncludingLoop(values, innerContext, hidden);
            innerContext.SetNodes(tpl.Nodes());
            innerContext.Nodes()[tpl.GetRenderer()].RenderNamed(os, innerContext, tpl.SelfName());
            innerContext.CaptureClosures(FrameEnd{ 0, true, 0, 0 });
            return;
        }
    }

    innerContext.SetNodes(tpl.Nodes());
    innerContext.Nodes()[tpl.GetRenderer()].RenderNamed(os, innerContext, tpl.SelfName());
    // What it saw, its parents' names too: Jinja2 renders it with a copy of the context
    innerContext.CaptureClosures(FrameEnd{ 0, true, 0, 0 });
}

void IncludeStatement::Render(OutStream& os, RenderContext& values)
{
    // Only a template that is not found is skipped, as Python's `ignore missing` and select_template
    // skip it: what fails once it is found, while it loads or renders, fails the include
    auto doRender = [this, &values, &os](const IRendererCallback::LoadTemplateResult& tpl) -> bool {
        const bool missing = std::visit(
            [](const auto& loaded) {
                if constexpr (std::is_same_v<std::decay_t<decltype(loaded)>, NoneValue>)
                {
                    return false;
                }
                else
                {
                    return !loaded && loaded.error().GetCode() == ErrorCode::FileNotFound;
                }
            },
            tpl);
        if (missing)
        {
            return false;
        }
        return VisitTemplateImpl<bool>(tpl, true, [this, &values, &os](const auto& tplPtr) {
            RenderIncludedTemplate(*tplPtr, m_withContext, os, values);
            return true;
        });
    };

    // A constant name found in this render is not looked up again
    auto* callback = values.GetRendererCallback();
    if (m_isConstant)
    {
        if (const auto* tpl = callback->FindLoadedBy(this); tpl && doRender(*tpl))
        {
            return;
        }
    }

    RenderByNames(values, doRender);
}

JINJA2CPP_NOINLINE
void IncludeStatement::RenderByNames(RenderContext& values, FunctionRef<bool(const IRendererCallback::LoadTemplateResult&)> doRender) const
{
    auto* callback = values.GetRendererCallback();
    auto templateNames = values.Nodes()[m_expr].Evaluate(values);
    bool isConverted = false;
    ListAdapter list = ConvertToList(templateNames, isConverted);
    // A host sequence is stored as a map and iterates: Python's select_template takes any iterable
    if (!isConverted && AsMap(templateNames))
    {
        list = ConvertToList(templateNames, isConverted, false);
    }

    bool rendered = false;
    // The names tried, for the error: a host iterable may iterate once, and stops at the first found
    // name as Python's select_template does
    InternalValueList tried;
    if (isConverted)
    {
        list.ForEach([&](InternalValue&& name) {
            CheckSequenceSize(tried.size() + 1, "a list");
            const auto& tryName = tried.emplace_back(std::move(name));
            // Python's select_template finds no template by a name that is not a string and tries the next
            if (!GetAsSameString(std::string(), tryName))
            {
                return true;
            }
            rendered = doRender(callback->LoadTemplate(tryName));
            return !rendered;
        });
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
            extraParams.push_back(IntValue2Value(ListAdapter::CreateAdapter(std::move(tried))));
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
    m_context.SetModuleImports(&m_imports);
}

ImportedModule::ImportedModule(InternalValueMap names, const InternalValueMap& globals, IRendererCallback* callback, const InternalValueMap* builtins)
    : m_importerNames(std::move(names))
    , m_context(m_importerNames, globals, callback, builtins)
{
    m_context.SetModuleImports(&m_imports);
    // As CloneWith: the template rendered sets its own (Render)
    m_context.ResetAutoescape(callback->GetSettings().autoescape);
}

template<typename Tpl>
void ImportedModule::Render(const Tpl& tpl)
{
    m_output = InternalValue(RenderToString(m_context.GetRendererCallback(), [this, &tpl](OutStream& stream) { Render(tpl, stream); }));
}

InternalValue ImportedModule::MakeObject(InternalValueMap&& exports) const
{
    if (m_object.IsUndefined())
    {
        m_object = CreateModuleAdapter(std::move(exports), m_frame.name.empty() ? InternalValue(NoneValue()) : InternalValue(std::string(m_frame.name)), m_output);
    }
    return m_object;
}

template<typename Tpl>
void ImportedModule::Render(const Tpl& tpl, OutStream& os)
{
    // The module's macros run in its template, whose blocks their `self` has: its own and,
    // once its top level has run, those of the templates it extends
    m_frame.blocks = &m_blocks;
    m_frame.baseDepth = m_context.GetScopesCount();
    m_frame.name = tpl.SelfName();
    m_context.SetTemplateFrame(&m_frame);
    m_context.SetNodes(tpl.Nodes());
    // The module's context is its template's: its macros run with the template's runtime
    // autoescape setting wherever they are called
    m_context.ResetAutoescape(m_context.Nodes()[tpl.GetRenderer()].IsAutoescape());
    // Only the names the template, and the ones it extends, define are imported, as in Jinja2
    m_context.Nodes()[tpl.GetRenderer()].RenderAsModule(os, m_context, m_blocks, tpl.SelfName());
}

template void ImportedModule::Render(const TemplateImpl<char>& tpl, OutStream& os);
template void ImportedModule::Render(const TemplateImpl<wchar_t>& tpl, OutStream& os);

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
            ImportNames(values, nodes, *shared, name);
            return;
        }
    }
    // Owned by the render, so that its macros find their names wherever they are called
    auto& module = callback.AddModule(std::make_unique<ImportedModule>(values, m_withContext));
    // Made by a host's call of a module's macro, it ends with that call (docs/tasks/0301);
    // one imported without context is shared on for the module's life, and so is what its
    // top level imports
    if (m_withContext)
    {
        if (callback.ModulesEndWithCall())
        {
            module.Context().SetEndsWithCall();
        }
        VisitTemplateImpl<bool>(tpl, true, [&module](const auto& tplPtr) {
            module.Render(*tplPtr);
            return true;
        });
        ImportNames(values, nodes, module, name);
        return;
    }
    class SharedModuleScope
    {
    public:
        explicit SharedModuleScope(IRendererCallback& owner)
            : m_callback(owner)
            , m_token(owner.EnterSharedModule())
        {
        }
        SharedModuleScope(const SharedModuleScope&) = delete;
        SharedModuleScope(SharedModuleScope&&) = delete;
        SharedModuleScope& operator=(const SharedModuleScope&) = delete;
        SharedModuleScope& operator=(SharedModuleScope&&) = delete;
        ~SharedModuleScope() { m_callback.LeaveSharedModule(m_token, m_shared); }
        void Shared() { m_shared = true; }

    private:
        IRendererCallback& m_callback;
        std::size_t m_token;
        bool m_shared = false;
    };
    SharedModuleScope scope(callback);
    VisitTemplateImpl<bool>(tpl, true, [&module](const auto& tplPtr) {
        module.Render(*tplPtr);
        return true;
    });
    // Shared once complete: a module that imports itself recurses, as in Jinja2
    callback.ShareModule(tpl, module);
    scope.Shared();
    ImportNames(values, nodes, module, name);
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

namespace
{
// Python's repr() of a value
std::string ReprOf(const InternalValue& value)
{
    std::string result;
    Apply<visitors::ValueRenderer<char>>(value, result, true);
    return result;
}

// An import at a module's top level hides the name from the module's exports, until a `set`
// binds it again (SetStatement::SetReexports)
void NoteModuleImport(const RenderContext& values, std::string_view name)
{
    auto* imports = values.TopLevelModuleImports();
    if (!imports)
    {
        return;
    }
    if (std::find(imports->begin(), imports->end(), name) == imports->end())
    {
        imports->emplace_back(name);
    }
}
} // namespace

// Copies: the module keeps every name, which its own macros read in its context
void ImportStatement::ImportNames(RenderContext& values, const ArenaView& nodes, const ImportedModule& module, const InternalValue& moduleName) const
{
    const auto& moduleNames = module.Names();
    if (m_namespace.empty())
    {
        auto scope = values.GetCurrentScope();
        for (const auto& name : nodes[m_namesToImport])
        {
            const auto p = moduleNames.find(HashedName{ nodes.Text(name.name), name.nameHash });
            // A name the module does not export (one it imported is not its own) is bound
            // undefined, as Jinja2 binds it, hiding the same name outside. Either way the alias
            // is an import, which this module does not export
            auto& bound = scope.ForName({ nodes.Text(name.alias), name.aliasHash });
            const bool exported = p != moduleNames.end() && IsImported(p->second) && module.Exports(p->first);
            if (exported)
            {
                bound = p->second;
            }
            else
            {
                // Jinja2's message names the module and the import's position
                bound = MakeUndefinedWithHint(values,
                                              "the template " + ReprOf(moduleName) + " (imported on " + std::string(nodes.Text(m_position)) + ") does not export the requested name "
                                                  + ReprOf(InternalValue(std::string(nodes.Text(name.name)))));
            }
            NoteModuleImport(values, nodes.Text(name.alias));
        }
        return;
    }

    InternalValueMap importedNs;
    for (const auto& [name, value] : moduleNames)
    {
        // Jinja2 exports no name that starts with an underline
        if (name.empty() || name[0] == '_' || !IsImported(value) || !module.Exports(name))
        {
            continue;
        }
        importedNs[name] = value;
    }
    auto& bound = values.GetCurrentScope().ForName({ nodes.Text(m_namespace), m_namespaceHash });
    bound = module.MakeObject(std::move(importedNs));
    NoteModuleImport(values, nodes.Text(m_namespace));
}

void SetStatement::Reexport(RenderContext& values) const
{
    auto* imports = values.TopLevelModuleImports();
    if (!imports)
    {
        return;
    }
    const auto& nodes = values.Nodes();
    for (const auto& node : nodes[m_target])
    {
        if (node.IsPlainName())
        {
            const auto text = nodes.Text(node.name);
            imports->erase(std::remove(imports->begin(), imports->end(), text), imports->end());
        }
    }
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
    SmallVector<MacroParam, 8> arenaParams;
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
    // Defined in volatile code: the body's output follows the runtime setting where it is
    // called (RenderContext::IsVolatileAutoescape)
    bool isVolatile;
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
        const OutputAutoescapeGuard autoescapeGuard(home, autoescape, isVolatile);
        macro->InvokeMacroRenderer(callParams, stream, home, link);
    }
};

Callable MacroStatement::MakeCallable(RenderContext& values) const
{
    // The body escapes its output as where the macro is defined; the runtime setting where it
    // is called decides whether its results are Markup, its own and those of `caller()` and
    // `join` in it (RenderContext::IsAutoescape). The defaults are evaluated by each call
    // (BindMacroDefaults)
    ClosureSite site(values);
    // Defined in a frame that may end before it is called: it keeps what it sees there (0326)
    std::shared_ptr<const void> capture;
    if ((m_bodyNameRefs != 0 || m_bodyReachesContext) && site.OutlivesItsFrames(values))
    {
        capture = site.CaptureOnEnd(values, values.Nodes()[m_bodyReads], false);
    }
    Callable result(Callable::Macro, CallableBody{ this, values.Nodes(), values.IsStaticAutoescape(), values.IsVolatileAutoescape(), std::move(site), m_readsSlotsOnly });
    result.SetAttributes(m_attributes);
    result.SetKeepAlive(std::move(capture));
    return result;
}

namespace
{
// The body of `value` when it is a macro (MacroStatement::MakeCallable), else null
template<typename Body>
const Body* MacroBodyOf(const InternalValue& value)
{
    const auto* callable = GetIf<Callable>(&value);
    if (!callable || callable->GetKind() != Callable::Macro || callable->GetType() != Callable::Type::Statement)
    {
        return nullptr;
    }
    return callable->GetStatementCallable().target<Body>();
}
} // namespace

std::optional<InternalValue> MacroStatement::WeakenCapture(const InternalValue& value, Span<const std::shared_ptr<ClosureCapture>> captures)
{
    const auto* body = MacroBodyOf<CallableBody>(value);
    if (!body)
    {
        return std::nullopt;
    }
    const auto& callable = *GetIf<Callable>(&value);
    const void* keepAlive = callable.GetKeepAlive();
    if (!keepAlive || std::none_of(captures.begin(), captures.end(), [keepAlive](const auto& capture) { return capture.get() == keepAlive; }))
    {
        return std::nullopt;
    }
    // A copy of the same callable: `sameas` it (Callable::IsSame)
    Callable weakened = callable;
    weakened.SetKeepAlive(nullptr);
    return InternalValue(std::move(weakened));
}

InternalValue MacroStatement::StrengthenCapture(const InternalValue& value)
{
    const auto* body = MacroBodyOf<CallableBody>(value);
    if (!body)
    {
        return value;
    }
    const auto& callable = *GetIf<Callable>(&value);
    if (callable.GetKeepAlive())
    {
        return value;
    }
    auto capture = body->site.Capture();
    if (!capture)
    {
        return value;
    }
    Callable strong = callable;
    strong.SetKeepAlive(std::move(capture));
    return InternalValue(std::move(strong));
}

void MacroStatement::Render(OutStream&, RenderContext& values)
{
    values.GetCurrentScope().ForName({ values.Nodes().Text(m_name), m_nameHash }) = MakeCallable(values);
    if (m_reexports && values.HasModuleImports())
    {
        if (auto* imports = values.TopLevelModuleImports())
        {
            const auto text = values.Nodes().Text(m_name);
            imports->erase(std::remove(imports->begin(), imports->end(), text), imports->end());
        }
    }
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
    SmallVector<SlotName, 8> names;
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
    Span<const MacroParam> params;
    const CallParams& callParams;
    bool catchCaller = false;
    SmallVector<const InternalValue*, 8> keywordValues;
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
                throw TemplateRuntimeError(TemplateRuntimeError::PythonClass::TypeError, "macro " + displayName() + " was invoked with two values for the special caller argument. This is most likely a bug.");
            }
            throw TemplateRuntimeError("macro " + displayName() + " takes no keyword argument '" + name + "'");
        }
    }

    throw TemplateRuntimeError("macro " + displayName() + " takes not more than " + std::to_string(argsCount) + " argument(s)");
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
    const OutStream::PieceHold pieceHold(stream);

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
        // A macro defined in the body and kept past the call sees what the call had (0326)
        context.CaptureClosures(FrameEnd::Call(context.GetScopesCount(), link));
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
    context.CaptureClosures(FrameEnd::Call(context.GetScopesCount(), link));

    guard.Unbind();
    context.PopFrameView();
    context.ExitScope();
}

namespace
{
// A call block whose call stopped (docs/tasks/0479): Jinja2 yields the undefined value the call gives, which the
// output cannot join (docs/tasks/0362). A failure that is no stop goes on
[[noreturn]] void ThrowStoppedCallBlock(RenderContext& values, HomeCall::Snapshot& snapshot, bool stopped)
{
    if (!stopped)
    {
        detail::RethrowKept();
    }
    detail::DropKept();
    snapshot.Undo();
    const auto undefined = detail::MakeStopUndefined(values);
    throw TemplateRuntimeError("expected str instance, "s + Apply<visitors::PythonTypeNameGetter>(undefined) + " found");
}
} // namespace

void MacroCallStatement::Render(OutStream& os, RenderContext& values)
{
    const auto& call = values.Nodes()[m_call];
    auto fnVal = values.Nodes()[call.GetValueRef()].Evaluate(values);
    // An object of the host language that is callable takes caller= as any call, and so does
    // an engine object with a call operator (joiner()), which refuses it as Python's does
    if (const auto* host = GetHostObject(fnVal))
    {
        fnVal = host::MakeCall(fnVal, *host);
    }
    else if (!GetIf<Callable>(&fnVal))
    {
        if (auto callOperator = Subscript(fnVal, "operator()"s, nullptr); GetIf<Callable>(&callOperator))
        {
            fnVal = std::move(callOperator);
        }
    }
    const auto* callable = GetIf<Callable>(&fnVal);
    if (!callable)
    {
        // Calling a name that does not exist is an UndefinedError under every policy
        CheckUndefinedUse(fnVal, UndefinedUse::Call);
        throw TemplateRuntimeError("'"s + Apply<visitors::PythonTypeNameGetter>(fnVal) + "' object is not callable");
    }

    auto callParams = helpers::EvaluateCallParams(call.GetParams(), values);
    helpers::AddCallerParam(callParams, MakeCallable(values));
    if (callable->GetType() == Callable::Type::Expression)
    {
        // Jinja2 yields the result into the output as it is: only a string joins it, anything
        // else is the TypeError of joining the output (docs/tasks/0362). A plain string is
        // escaped under autoescape, where Jinja2 writes it raw: a deliberate divergence that
        // keeps a callable's data escaped (Ruslan, 2026-11-02); Markup, as a macro's, is not
        InternalValue result;
        bool failed = false;
        bool stopped = false;
        HomeCall::Snapshot snapshot(values);
        try
        {
            result = callable->GetExpressionCallable()(callParams, values);
        }
        catch (...)
        {
            failed = true;
            stopped = detail::KeepFailure();
        }
        if (failed)
        {
            ThrowStoppedCallBlock(values, snapshot, stopped);
        }
        if (!IsStringValue(result))
        {
            throw TemplateRuntimeError("expected str instance, "s + Apply<visitors::PythonTypeNameGetter>(result) + " found");
        }
        WriteOutput(os, result, values);
        return;
    }
    // A call block's caller runs into a buffer in Jinja2
    const detail::GenerationFreeze freeze(values);
    const auto mark = os.GetMark(); // NOLINT(clang-analyzer-deadcode.DeadStores): read on the failure path
    HomeCall::Snapshot snapshot(values);
    bool stopped = false;
    try
    {
        callable->GetStatementCallable()(callParams, os, values);
        return;
    }
    catch (...)
    {
        stopped = detail::KeepFailure();
    }
    if (!stopped || !os.RollbackTo(mark))
    {
        detail::RethrowKept();
    }
    freeze.Discard();
    ThrowStoppedCallBlock(values, snapshot, true);
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

    innerValues.CaptureClosuresAtSoftEnd();
    innerValues.ExitScope();
    values.SetLoopControl(innerValues.GetLoopControl());
}

void GenerationStatement::Render(OutStream& os, RenderContext& values)
{
    auto innerValues = values.Clone(true);
    static_cast<void>(innerValues.EnterScope());

    auto* tracker = values.GetRendererCallback()->GetGenerationTracker();
    if (!tracker)
    {
        values.Nodes()[m_mainBody].Render(os, innerValues);
    }
    else
    {
        // A nested block is frozen at the start of the outer one's body: transformers measures what the generator has
        // yielded, and the outer block yields nothing until it ends
        const size_t start = tracker->StartPos();
        const size_t before = os.Written();
        {
            const detail::GenerationFreeze freeze(values);
            values.Nodes()[m_mainBody].Render(os, innerValues);
        }
        tracker->Push(start, start + (os.Written() - before));
    }

    innerValues.CaptureClosuresAtSoftEnd();
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
    innerValues.CaptureClosuresAtSoftEnd();
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
    const auto autoescapeGuard = AutoescapeGuard::Statement(values, ConvertToBool(values.Nodes()[m_expr].Evaluate(values)), m_isVolatile);
    values.EnterScope();
    values.Nodes()[m_body].Render(os, values);
    values.CaptureClosures(FrameEnd::Soft(values.GetScopesCount()));
    values.ExitScope();
}

void OutputCheckStatement::Render(OutStream& os, RenderContext& values)
{
    if (const auto* frame = values.GetTemplateFrame(); frame && frame->parent)
    {
        return;
    }
    values.Nodes()[m_output].Render(os, values);
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
} // namespace v2
} // namespace jinja2
