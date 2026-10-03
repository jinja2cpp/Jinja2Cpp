#include "statements.h"

#include "expression_evaluator.h"
#include "generic_adapters.h"
#include "internal_value.h"
#include "markup.h"
#include "out_stream.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "renderer.h"
#include "template_impl.h"
#include "undefined.h"
#include "value_methods.h"
#include "value_visitors.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/utils/i_comparable.h>
#include <jinja2cpp/value.h>

#include <boost/core/null_deleter.hpp>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std::string_literals;

namespace jinja2
{

void ForStatement::Render(OutStream& os, RenderContext& values)
{
    InternalValue loopVal = m_value->Evaluate(values);

    RenderLoop(loopVal, os, values, 0);
}

namespace
{

// Python's assignment to a target: a name takes the value; a tuple `a, (b, c)` iterates
// the value, which must yield exactly as many items as the tuple has targets, and assigns
// them in turn. A mapping assigned to a tuple of names is the exception: Jinja2C++ has
// always taken its values by name (`set first, last = person`), where Python would
// assign its keys
void AssignTo(const AssignTarget& target, InternalValue value, InternalValueMap& scope, RenderContext& values)
{
    if (!target.attr.empty())
    {
        // `set ns.attr = ...` changes a namespace() object wherever it is defined
        bool found = false;
        auto p = values.FindValue(target.name, found);
        const auto* ns = found ? GetIf<MapAdapter>(&p->second) : nullptr;
        if (!ns || !ns->IsNamespace())
        {
            throw std::runtime_error("cannot assign attribute on non-namespace object");
        }
        MapAdapter(*ns).SetValue(target.attr, value);
        return;
    }
    if (!target.isTuple)
    {
        scope[target.name] = std::move(value);
        return;
    }

    const auto& targets = target.items;
    auto isName = [](const AssignTarget& t) { return !t.isTuple && t.attr.empty(); };
    if (GetIf<MapAdapter>(&value) && std::all_of(targets.begin(), targets.end(), isName))
    {
        for (const auto& t : targets)
        {
            scope[t.name] = Subscript(value, t.name, &values);
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
        AssignTo(targets[idx], std::move(items[idx]), scope, values);
    }
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
struct LoopState
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
    // The arguments of the last loop.changed() call, made on first use
    std::shared_ptr<std::optional<InternalValueList>> lastChanged;

    InternalValue& Item(size_t idx) { return items[idx % items.size()]; }

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
void AssignLoopTarget(const AssignTarget& target, const InternalValue& item, InternalValueMap& scope, LoopTargetSlots& slots, RenderContext& values)
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
    explicit LoopAccessor(std::shared_ptr<LoopState> state)
        : m_state(std::move(state))
    {
    }

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
    [[nodiscard]] const void* GetIdentity() const override { return m_state.get(); }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const LoopAccessor*>(&other);
        if (!val)
        {
            return false;
        }
        return m_state == val->m_state;
    }

private:
    enum class Property : uint8_t
    {
        Index,
        Index0,
        RevIndex,
        RevIndex0,
        First,
        Last,
        Length,
        Depth,
        Depth0,
        PrevItem,
        NextItem,
        Cycle,
        Changed,
        Call
    };

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
        for (const auto& [propName, prop] : Properties())
        {
            if (propName == name)
            {
                return prop;
            }
        }
        return std::nullopt;
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
            return ForStatement::MakeLoopRecursion(state.recursiveStatement, state.level);
        }
        return InternalValue();
    }

    std::shared_ptr<LoopState> m_state;
};

} // namespace

Callable ForStatement::MakeLoopRecursion(ForStatement* statement, int level)
{
    return Callable(Callable::GlobalFunc, [statement, level](const CallParams& params, OutStream& stream, RenderContext& context) {
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
        statement->RenderLoop(var, stream, context, level + 1);
    });
}

void ForStatement::RenderLoop(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level)
{
    auto& context = values.EnterScope();

    auto state = std::make_shared<LoopState>();
    state->level = level;
    if (m_isRecursive)
    {
        state->recursiveStatement = this;
    }
    auto loopAccessor = std::make_shared<LoopAccessor>(state);
    context["loop"s] = MapAdapter(loopAccessor);

    bool isConverted = false;
    auto loopItems = ConvertToList(loopVal, isConverted, false);
    ListAdapter filteredList;
    if (!isConverted)
    {
        // The `else` body sees the names outside the loop, as in Jinja2
        values.ExitScope();
        if (m_elseBody)
        {
            m_elseBody->Render(os, values);
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
        enumerator = loopItems.GetEnumerator();
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
    auto& bodyScope = values.EnterScope();
    LoopTargetSlots targetSlots;
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

        m_mainBody->Render(os, values);
        if (!bodyScope.empty())
        {
            bodyScope.clear();
        }

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
    values.ExitScope();

    // A loop object kept past the loop (`set ns.x = loop`) can no longer run the filter,
    // which needs this render context: collect the rest of the items now
    // (copies of `loop` share its accessor, a GenericMap made from it copies the accessor
    // and shares the state: more owners of either than this function and the scope mean
    // it was kept)
    if (!state->listSize && (loopAccessor.use_count() > 2 || state.use_count() > 2))
    {
        state->GetLength();
    }

    values.ExitScope();
    if (!loopRendered && m_elseBody)
    {
        m_elseBody->Render(os, values);
    }
}

ListAdapter ForStatement::CreateFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const
{
    return ListAdapter::CreateAdapter([eo = loopItems.GetEnumerator(), this, &values]() mutable {
        using ResultType = std::optional<InternalValue>;

        auto& tempContext = values.EnterScope();
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

            if (ConvertToBool(m_ifExpr->Evaluate(values)))
            {
                values.ExitScope();
                return ResultType(std::move(curValue));
            }
        }
        values.ExitScope();

        return ResultType();
    });
}

void IfStatement::Render(OutStream& os, RenderContext& values)
{
    InternalValue val = m_expr->Evaluate(values);
    bool isTrue = Apply<visitors::BooleanEvaluator>(val);

    if (isTrue)
    {
        m_mainBody->Render(os, values);
        return;
    }

    for (auto& b : m_elseBranches)
    {
        if (b->ShouldRender(values))
        {
            b->Render(os, values);
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

    return Apply<visitors::BooleanEvaluator>(m_expr->Evaluate(values));
}

void ElseBranchStatement::Render(OutStream& os, RenderContext& values)
{
    m_mainBody->Render(os, values);
}

void SetStatement::AssignBody(InternalValue body, RenderContext& values)
{
    AssignTo(m_target, std::move(body), values.GetCurrentScope(), values);
}

void SetLineStatement::Render(OutStream&, RenderContext& values)
{
    if (!m_expr)
    {
        return;
    }
    AssignBody(m_expr->Evaluate(values), values);
}

InternalValue SetBlockStatement::RenderBody(RenderContext& values)
{
    TargetString result;
    auto stream = values.GetRendererCallback()->GetStreamOnString(result);
    auto innerValues = values.Clone(true);
    m_body->Render(stream, innerValues);
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
    AssignBody(std::move(body), values);
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
    auto result = m_expr->Evaluate(body, values);
    if (values.IsAutoescape())
    {
        result = MakeMarkup(result, values.GetRendererCallback());
    }
    AssignBody(std::move(result), values);
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
    p->second[depth]->RenderBody(os, blockContext, depth);
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
    }
    ~TemplateFrameGuard() { m_values.SetTemplateFrame(m_prevFrame); }

    TemplateFrameGuard(const TemplateFrameGuard&) = delete;
    TemplateFrameGuard& operator=(const TemplateFrameGuard&) = delete;

private:
    RenderContext& m_values;
    TemplateFrame* m_prevFrame;
};
} // namespace

void BlockStatement::Render(OutStream& os, RenderContext& values)
{
    auto* frame = values.GetTemplateFrame();
    if (!frame || !frame->blocks)
    {
        RenderContext innerContext = values.Clone(true);
        innerContext.EnterScope();
        m_mainBody->Render(os, innerContext);
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
    auto* frame = values.GetTemplateFrame();
    auto baseDepth = values.GetScopesCount();
    auto& scope = values.EnterScope();
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
    m_mainBody->Render(os, values);
    values.ExitScope();
}

void TemplateRenderer::PushBlocks(BlocksStack& stack) const
{
    for (const auto& [name, block] : m_blocks)
    {
        stack.blocks[name].push_back(block.get());
    }
}

void TemplateRenderer::Render(OutStream& os, RenderContext& values)
{
    BlocksStack stack;
    PushBlocks(stack);
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
    auto& stack = *frame->blocks;
    PushBlocks(stack);
    RenderBody(os, values, stack);
}

void TemplateRenderer::RenderBody(OutStream& os, RenderContext& values, BlocksStack& stack)
{
    const RenderDepthGuard depthGuard;
    // Included, imported and parent templates use the environment's autoescape setting
    AutoescapeGuard autoescapeGuard(values, TemplateAutoescape(values));
    TemplateFrame frame;
    frame.blocks = &stack;
    frame.baseDepth = values.GetScopesCount();
    TemplateFrameGuard frameGuard(values, &frame);

    InternalValueMap self;
    for (auto& block : stack.blocks)
    {
        const auto& name = block.first;
        self[name] = MakeWrapped(Callable(Callable::Macro, [name](const CallParams&, OutStream& stream, RenderContext& context) {
            auto* curFrame = context.GetTemplateFrame();
            if (!curFrame || !curFrame->blocks)
            {
                return;
            }
            RenderContext blockContext(context, curFrame->baseDepth);
            RenderBlockAt(*curFrame->blocks, name, 0, stream, blockContext);
        }));
    }
    values.GetCurrentScope()["self"] = CreateMapAdapter(std::move(self));

    if (!m_hasExtends)
    {
        m_body->Render(os, values);
        return;
    }

    TopLevelWriter writer(os, frame);
    OutStream topLevelStream(&writer);
    m_body->Render(topLevelStream, values);

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
            throw tpl.error(); // NOLINT(bugprone-exception-copy-constructor-throws)
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
        static_cast<TemplateRenderer&>(*m_template->GetRenderer()).RenderAsParent(os, values);
    }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const ParentTemplateRenderer*>(&other);
        if (!val)
        {
            return false;
        }
        return m_template == val->m_template;
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

    auto name = m_templateExpr->Evaluate(values);
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

        tpl.GetRenderer()->Render(os, innerContext);
        if (withContext && exportNames)
        {
            auto& innerScope = innerContext.GetCurrentScope();
            auto& scope = values.GetCurrentScope();
            for (auto& [name, value] : innerScope)
            {
                if (name != "self")
                {
                    scope[name] = std::move(value);
                }
            }
        }
    }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const IncludedTemplateRenderer<CharT>*>(&other);
        if (!val)
        {
            return false;
        }
        if (m_template != val->m_template)
        {
            return false;
        }
        if (m_withContext != val->m_withContext)
        {
            return false;
        }
        if (m_exportNames != val->m_exportNames)
        {
            return false;
        }
        return true;
    }

private:
    std::shared_ptr<TemplateImpl<CharT>> m_template;
    bool m_withContext{};
    bool m_exportNames{};
};

void IncludeStatement::Render(OutStream& os, RenderContext& values)
{
    auto templateNames = m_expr->Evaluate(values);
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
    ImportedMacroRenderer(InternalValueMap&& map, bool withContext, RendererPtr module)
        : m_importedContext(std::move(map))
        , m_withContext(withContext)
        , m_module(std::move(module))
    {
    }

    void Render(OutStream& /*os*/, RenderContext& /*values*/) override {}

    void InvokeMacro(const Callable& callable, const CallParams& params, OutStream& stream, RenderContext& context)
    {
        auto ctx = context.Clone(m_withContext);
        ctx.BindScope(&m_importedContext);
        callable.GetStatementCallable()(params, stream, ctx);
    }

    static void InvokeMacro(const std::string& contextName, const Callable& callable, const CallParams& params, OutStream& stream, RenderContext& context)
    {
        bool contextValFound = false;
        auto contextVal = context.FindValue(contextName, contextValFound);
        if (!contextValFound)
        {
            return;
        }

        const auto* rendererPtr = GetIf<RendererPtr>(&contextVal->second);
        if (!rendererPtr)
        {
            return;
        }

        auto* renderer = static_cast<ImportedMacroRenderer*>(rendererPtr->get());
        renderer->InvokeMacro(callable, params, stream, context);
    }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ImportedMacroRenderer*>(&other);
        if (!val)
        {
            return false;
        }
        if (m_importedContext != val->m_importedContext)
        {
            return false;
        }
        if (m_withContext != val->m_withContext)
        {
            return false;
        }
        return true;
    }

private:
    InternalValueMap m_importedContext;
    bool m_withContext{};
    RendererPtr m_module;
};

void ImportStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    auto name = m_nameExpr->Evaluate(values);

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

    TargetString str;
    auto tmpStream = values.GetRendererCallback()->GetStreamOnString(str);

    RenderContext newContext = values.Clone(m_withContext);
    InternalValueMap importedScope;
    {
        auto& intImportedScope = newContext.EnterScope();
        renderer->Render(tmpStream, newContext);
        importedScope = std::move(intImportedScope);
    }

    ImportNames(values, importedScope, scopeName);
    values.GetCurrentScope()[scopeName] =
        std::static_pointer_cast<IRendererBase>(std::make_shared<ImportedMacroRenderer>(std::move(importedScope), m_withContext, renderer));
}

void ImportStatement::ImportNames(RenderContext& values, InternalValueMap& importedScope, const std::string& scopeName) const
{
    InternalValueMap importedNs;

    for (auto& [name, value] : importedScope)
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
        auto* callable = GetIf<Callable>(&value);
        if (!callable)
        {
            imported = std::move(value);
        }
        else if (callable->GetKind() == Callable::Macro)
        {
            auto attributes = callable->GetAttributes();
            Callable wrapper(Callable::Macro, [fn = std::move(*callable), scopeName](const CallParams& params, OutStream& stream, RenderContext& context) {
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
            definedDefaults[idx] = p.defaultValue->Evaluate(values);
        }
    }

    // The body escapes as where the macro is defined; the caller decides whether the result is Markup
    Callable result(Callable::Macro,
                    [this, defaults = std::move(definedDefaults), autoescape = values.IsAutoescape()](
                        const CallParams& callParams, OutStream& stream, RenderContext& context) {
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

// Binds the call arguments the way Jinja2's Macro.__call__ does
// NOLINTNEXTLINE(readability-function-cognitive-complexity): score 32, split in docs/tasks/0061
void MacroStatement::InvokeMacroRenderer(const std::vector<InternalValue>& definedDefaults,
                                         const CallParams& callParams,
                                         OutStream& stream,
                                         RenderContext& context) const
{
    const RenderDepthGuard depthGuard;
    const auto& posParams = callParams.posParams;
    auto kwParams = callParams.kwParams;
    const auto argsCount = m_params.size();

    std::vector<InternalValue> args(argsCount);
    std::vector<bool> isProvided(argsCount, false);
    for (std::size_t idx = 0; idx < argsCount; ++idx)
    {
        const auto& name = m_params[idx].paramName;
        if (idx < posParams.size())
        {
            args[idx] = posParams[idx];
            isProvided[idx] = true;
            continue;
        }

        auto p = kwParams.find(name);
        if (p == kwParams.end())
        {
            continue;
        }

        args[idx] = std::move(p->second);
        isProvided[idx] = true;
        kwParams.erase(p);
    }

    const auto caught = GetCaughtNames();
    const bool catchCaller = (caught & UsesCaller) != 0;
    InternalValue caller;
    if (catchCaller)
    {
        auto p = kwParams.find("caller");
        if (p != kwParams.end())
        {
            caller = std::move(p->second);
            kwParams.erase(p);
        }
    }

    const bool catchKwargs = (caught & UsesKwargs) != 0;
    if (!catchKwargs && !kwParams.empty())
    {
        if (kwParams.count("caller") != 0)
        {
            throw std::runtime_error("macro " + GetDisplayName() + " was invoked with two values for the special caller argument. This is most likely a bug.");
        }
        throw std::runtime_error("macro " + GetDisplayName() + " takes no keyword argument '" + kwParams.begin()->first + "'");
    }

    const bool catchVarargs = (caught & UsesVarargs) != 0;
    if (!catchVarargs && posParams.size() > argsCount)
    {
        throw std::runtime_error("macro " + GetDisplayName() + " takes not more than " + std::to_string(argsCount) + " argument(s)");
    }

    // Missing arguments and the special ones are bound before the defaults are evaluated, so
    // a default sees them and never an outer variable named like a later argument
    auto& scope = context.EnterScope();
    for (std::size_t idx = 0; idx < argsCount; ++idx)
    {
        const auto& name = m_params[idx].paramName;
        scope[name] = isProvided[idx] ? std::move(args[idx]) : MakeUndefinedWithHint(context, "parameter '" + name + "' was not provided");
    }

    if (catchCaller)
    {
        scope["caller"s] = std::move(caller);
    }
    if (catchKwargs)
    {
        InternalDict kwArgs;
        for (auto& [name, value] : kwParams)
        {
            kwArgs[name] = std::move(value);
        }
        scope["kwargs"s] = CreateMapAdapter(std::move(kwArgs));
    }
    if (catchVarargs)
    {
        InternalValueList varArgs;
        for (auto idx = argsCount; idx < posParams.size(); ++idx)
        {
            varArgs.push_back(posParams[idx]);
        }
        scope["varargs"s] = ListAdapter::CreateAdapter(std::move(varArgs)).MarkAsTuple();
    }

    for (std::size_t idx = 0; idx < argsCount; ++idx)
    {
        const auto& p = m_params[idx];
        if (isProvided[idx] || !p.defaultValue)
        {
            continue;
        }

        auto value = p.defaultRefersToArgs ? p.defaultValue->Evaluate(context) : definedDefaults[idx];
        // Jinja2 evaluates defaults on every call, so acc=[] is a new list each time; the
        // template's lists and dicts are shared, so the stored one is copied
        if (methods::IsMutable(value))
        {
            value = methods::CopyContainer(value);
        }
        scope[p.paramName] = std::move(value);
    }

    m_mainBody->Render(stream, context);

    context.ExitScope();
}

void MacroCallStatement::Render(OutStream& os, RenderContext& values)
{
    bool isMacroFound = false;
    auto macroPtr = values.FindValue(m_macroName, isMacroFound);
    if (!isMacroFound)
    {
        return;
    }

    const auto& fnVal = macroPtr->second;
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
    m_expr->Evaluate(values);
}

void WithStatement::Render(OutStream& os, RenderContext& values)
{
    auto innerValues = values.Clone(true);
    auto& scope = innerValues.EnterScope();

    for (auto& [name, expr] : m_scopeVars)
    {
        scope[name] = expr->Evaluate(values);
    }

    m_mainBody->Render(os, innerValues);

    innerValues.ExitScope();
    values.SetLoopControl(innerValues.GetLoopControl());
}

void TransStatement::Render(OutStream& os, RenderContext& values)
{
    std::vector<InternalValue> evaluated;
    evaluated.reserve(m_variables.size());
    for (auto& var : m_variables)
    {
        evaluated.push_back(var.second->Evaluate(values));
    }

    auto& scope = values.EnterScope();
    for (size_t idx = 0; idx < evaluated.size(); ++idx)
    {
        scope[VariableSlot(idx)] = std::move(evaluated[idx]);
    }
    m_output->Render(os, values);
    values.ExitScope();
}

void FilterStatement::Render(OutStream& os, RenderContext& values)
{
    TargetString arg;
    auto argStream = values.GetRendererCallback()->GetStreamOnString(arg);
    auto innerValues = values.Clone(true);
    m_body->Render(argStream, innerValues);
    // A `break` or `continue` in the body drops its output, as in Jinja2
    values.SetLoopControl(innerValues.GetLoopControl());
    if (values.HasLoopControl())
    {
        return;
    }
    // The body is Markup under autoescape; the filtered output is written as is
    InternalValue body(std::move(arg));
    body.SetMarkup(values.IsAutoescape());
    const auto result = m_expr->Evaluate(body, values);
    os.WriteValue(result);
}

void AutoescapeStatement::Render(OutStream& os, RenderContext& values)
{
    AutoescapeGuard autoescapeGuard(values, ConvertToBool(m_expr->Evaluate(values)));
    values.EnterScope();
    m_body->Render(os, values);
    values.ExitScope();
}
} // namespace jinja2
