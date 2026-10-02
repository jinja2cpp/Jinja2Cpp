#include "statements.h"

#include "expression_evaluator.h"
#include "markup.h"
#include "template_impl.h"
#include "value_methods.h"
#include "value_visitors.h"

#include <boost/core/null_deleter.hpp>

#include <algorithm>
#include <string>

using namespace std::string_literals;

namespace jinja2
{

void ForStatement::Render(OutStream& os, RenderContext& values)
{
    InternalValue loopVal = m_value->Evaluate(values);

    RenderLoop(loopVal, os, values, 0);
}

// Python's assignment to a target: a name takes the value; a tuple `a, (b, c)` iterates
// the value, which must yield exactly as many items as the tuple has targets, and assigns
// them in turn. A mapping assigned to a tuple of names is the exception: Jinja2C++ has
// always taken its values by name (`set first, last = person`), where Python would
// assign its keys
static void AssignTo(const AssignTarget& target, InternalValue value, InternalValueMap& scope, RenderContext& values)
{
    if (!target.attr.empty())
    {
        // `set ns.attr = ...` changes a namespace() object wherever it is defined
        bool found = false;
        auto p = values.FindValue(target.name, found);
        const auto* ns = found ? GetIf<MapAdapter>(&p->second) : nullptr;
        if (ns == nullptr || !ns->IsNamespace())
            throw std::runtime_error("cannot assign attribute on non-namespace object");
        MapAdapter(*ns).SetValue(target.attr, std::move(value));
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
            scope[t.name] = Subscript(value, t.name, &values);
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
            throw std::runtime_error("cannot unpack non-iterable value");
        // One item past the targets is enough to tell that there are too many
        for (const auto& item : list)
        {
            items.push_back(item);
            if (items.size() > targets.size())
                break;
        }
    }

    if (items.size() > targets.size())
        throw std::runtime_error("too many values to unpack (expected " + std::to_string(targets.size()) + ")");
    if (items.size() < targets.size())
        throw std::runtime_error("not enough values to unpack (expected " + std::to_string(targets.size()) + ", got " + std::to_string(items.size()) + ")");

    for (std::size_t idx = 0; idx != targets.size(); ++idx)
        AssignTo(targets[idx], std::move(items[idx]), scope, values);
}

namespace
{
// The state behind a loop object. The template can keep the object past the loop
// (`set ns.x = loop`), so it is shared, and its own callables see it through weak
// pointers to avoid a reference cycle
struct LoopState
{
    InternalValueMap loopVar;
    ListAdapter indexedList;
    std::optional<ListAccessorEnumeratorPtr> enumerator;
    std::optional<size_t> listSize;
    // The index of the current item
    size_t index0 = 0;
    bool isLast = false;

    // The length of a filtered loop is known once the rest of the items are collected
    size_t GetLength()
    {
        if (listSize)
            return listSize.value();
        // On the last item the enumerator has nothing left to collect
        if (isLast)
        {
            listSize = index0 + 1;
            return listSize.value();
        }

        InternalValueList items;
        do
        {
            items.push_back((*enumerator)->GetCurrent());
        } while ((*enumerator)->MoveNext());

        listSize = index0 + items.size() + 1;
        indexedList = ListAdapter::CreateAdapter(std::move(items));
        enumerator = indexedList.GetEnumerator();
        isLast = !(*enumerator)->MoveNext();
        return listSize.value();
    }
};

template<typename Fn>
InternalValue MakeLoopProperty(const std::shared_ptr<LoopState>& state, Fn fn)
{
    return MakeDynamicProperty([weakState = std::weak_ptr<LoopState>(state), fn](const CallParams&, RenderContext&) -> InternalValue {
        auto locked = weakState.lock();
        if (!locked)
            return InternalValue();
        return fn(*locked);
    });
}
} // namespace

void ForStatement::RenderLoop(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level)
{
    auto& context = values.EnterScope();

    auto state = std::make_shared<LoopState>();
    auto& loopVar = state->loopVar;
    context["loop"s] = CreateMapAdapter(std::shared_ptr<InternalValueMap>(state, &state->loopVar));
    if (m_isRecursive)
    {
        loopVar["operator()"s] = Callable(Callable::GlobalFunc, [this, level](const CallParams& params, OutStream& stream, RenderContext& context) {
            bool isSucceeded = false;
            auto parsedParams = helpers::ParseCallParams({ { "var", true } }, params, isSucceeded);
            if (!isSucceeded)
                return;

            auto var = parsedParams["var"];
            if (IsEmpty(var))
                return;

            RenderLoop(var, stream, context, level + 1);
        });
    }
    loopVar["depth"s] = static_cast<int64_t>(level + 1);
    loopVar["depth0"s] = static_cast<int64_t>(level);

    bool isConverted = false;
    auto loopItems = ConvertToList(loopVal, isConverted, false);
    ListAdapter filteredList;
    if (!isConverted)
    {
        if (m_elseBody)
            m_elseBody->Render(os, values);
        values.ExitScope();
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

    if (state->listSize)
    {
        loopVar["length"s] = static_cast<int64_t>(state->listSize.value());
    }
    else
    {
        loopVar["length"s] = MakeLoopProperty(state, [](LoopState& s) { return static_cast<int64_t>(s.GetLength()); });
        // The reverse indices need the length too, so they are computed on first use
        loopVar["revindex"s] = MakeLoopProperty(state, [](LoopState& s) { return static_cast<int64_t>(s.GetLength() - s.index0); });
        loopVar["revindex0"s] = MakeLoopProperty(state, [](LoopState& s) { return static_cast<int64_t>(s.GetLength() - s.index0 - 1); });
    }
    // loop.changed(*values): whether the values differ from those of the previous call
    auto lastChanged = std::make_shared<std::optional<InternalValueList>>();
    loopVar["changed"s] = Callable(Callable::GlobalFunc, [lastChanged](const CallParams& params, RenderContext&) -> InternalValue {
        if (!params.kwParams.empty())
            throw std::runtime_error("changed() got an unexpected keyword argument '" + params.kwParams.begin()->first + "'");
        auto isEqual = [](const InternalValue& lhs, const InternalValue& rhs) {
            return ConvertToBool(Apply2<visitors::BinaryMathOperation>(lhs, rhs, BinaryExpression::LogicalEq));
        };
        auto& last = *lastChanged;
        const auto& args = params.posParams;
        if (last && last->size() == args.size() && std::equal(last->begin(), last->end(), args.begin(), isEqual))
            return false;
        last = params.posParams;
        return true;
    });
    bool loopRendered = false;
    auto& isLast = state->isLast;
    isLast = !(*enumerator)->MoveNext();
    InternalValue prevValue;
    InternalValue curValue;
    InternalValue nextValue;
    loopVar["cycle"s] = static_cast<int64_t>(LoopCycleFn);
    for (size_t itemIdx = 0; !isLast; ++itemIdx)
    {
        state->index0 = itemIdx;
        prevValue = std::move(curValue);
        if (itemIdx != 0)
        {
            std::swap(curValue, nextValue);
            loopVar["previtem"s] = prevValue;
        }
        else
            curValue = (*enumerator)->GetCurrent();

        isLast = !(*enumerator)->MoveNext();
        if (!isLast)
        {
            nextValue = (*enumerator)->GetCurrent();
            loopVar["nextitem"s] = nextValue;
        }
        else
        {
            loopVar.erase("nextitem"s);
        }

        loopVar["index"s] = static_cast<int64_t>(itemIdx + 1);
        loopVar["index0"s] = static_cast<int64_t>(itemIdx);
        loopVar["first"s] = itemIdx == 0;
        loopVar["last"s] = isLast;
        if (state->listSize)
        {
            loopVar["revindex"s] = static_cast<int64_t>(state->listSize.value() - itemIdx);
            loopVar["revindex0"s] = static_cast<int64_t>(state->listSize.value() - itemIdx - 1);
        }

        AssignTo(m_target, curValue, context, values);

        values.EnterScope();
        m_mainBody->Render(os, values);
        values.ExitScope();

        // As in Jinja2, the `else` body is skipped only once a pass through the body has
        // finished without `break` or `continue`
        auto control = values.TakeLoopControl();
        if (control == LoopControl::Break)
            break;
        if (control == LoopControl::None)
            loopRendered = true;
    }

    // A loop object kept past the loop (`set ns.x = loop`) can no longer run the filter,
    // which needs this render context: collect the rest of the items now
    if (!state->listSize && state.use_count() > 2)
        state->GetLength();

    if (!loopRendered && m_elseBody)
        m_elseBody->Render(os, values);

    values.ExitScope();
}

ListAdapter ForStatement::CreateFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const
{
    return ListAdapter::CreateAdapter([eo = loopItems.GetEnumerator(), this, &values]() mutable {
        using ResultType = std::optional<InternalValue>;

        auto& tempContext = values.EnterScope();
        if (!eo.has_value())
            return ResultType();
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
        return true;

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
        return;
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
        return;
    body.SetMarkup(values.IsAutoescape());
    AssignBody(std::move(body), values);
}

void SetFilteredBlockStatement::Render(OutStream&, RenderContext& values)
{
    if (!m_expr)
        return;
    auto body = RenderBody(values);
    if (values.HasLoopControl())
        return;
    // Jinja2 wraps the filtered value: Markup(str(result)) under autoescape
    auto result = m_expr->Evaluate(std::move(body), values);
    if (values.IsAutoescape())
        result = MakeMarkup(result, values.GetRendererCallback());
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
        return;
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
            m_os.WriteBuffer(ptr, length);
    }
    void WriteValue(const InternalValue& val) override
    {
        if (!m_frame.parent)
            m_os.WriteValue(val);
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
        return;

    auto p = frame->blocks->blocks.find(m_name);
    if (m_isRequired && (p == frame->blocks->blocks.end() || p->second.size() <= 1))
        throw std::runtime_error("Required block '" + m_name + "' not found");

    // An unscoped block sees the template-level names only, not the loop variables or
    // other locals around it
    RenderContext blockContext = m_isScoped ? RenderContext(values, values.GetScopesCount()) : RenderContext(values, frame->baseDepth);
    RenderBlockAt(*frame->blocks, m_name, 0, os, blockContext);
}

void BlockStatement::RenderBody(OutStream& os, RenderContext& values, size_t depth) const
{
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
    for (const auto& block : m_blocks)
        stack.blocks[block.first].push_back(block.second.get());
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
                return;
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
    OutStream topLevelStream([&writer]() -> OutStream::StreamWriter* { return &writer; });
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

    template<typename CharT>
    Result operator()(nonstd::expected<std::shared_ptr<TemplateImpl<CharT>>, BasicErrorInfo<CharT>> tpl) const
    {
        if (!m_throwError && !tpl)
        {
            return Result{};
        }
        else if (!tpl)
        {
            throw tpl.error();
        }
        return m_fn(tpl.value());
    }

    Result operator()(EmptyValue) const { return Result(); }
};

template<typename Result, typename Fn, typename Arg>
Result VisitTemplateImpl(Arg&& tpl, bool throwError, Fn&& fn)
{
    return visit(TemplateImplVisitor<Result, Fn>(fn, throwError), tpl);
}

template<template<typename T> class RendererTpl, typename CharT, typename... Args>
auto CreateTemplateRenderer(std::shared_ptr<TemplateImpl<CharT>> tpl, Args&&... args)
{
    return std::make_shared<RendererTpl<CharT>>(tpl, std::forward<Args>(args)...);
}

// The template an `extends` names; keeps it alive while it renders
template<typename CharT>
class ParentTemplateRenderer : public IRendererBase
{
public:
    explicit ParentTemplateRenderer(std::shared_ptr<TemplateImpl<CharT>> tpl)
        : m_template(tpl)
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        auto renderer = std::static_pointer_cast<TemplateRenderer>(m_template->GetRenderer());
        renderer->RenderAsParent(os, values);
    }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const ParentTemplateRenderer*>(&other);
        if (!val)
            return false;
        return m_template == val->m_template;
    }

private:
    std::shared_ptr<TemplateImpl<CharT>> m_template;
};

void ExtendsStatement::Render(OutStream& /*os*/, RenderContext& values)
{
    auto* frame = values.GetTemplateFrame();
    if (!frame)
        return;
    if (frame->parent)
        throw std::runtime_error("extended multiple times");

    auto name = m_templateExpr->Evaluate(values);
    auto tpl = values.GetRendererCallback()->LoadTemplate(name);
    frame->parent = VisitTemplateImpl<RendererPtr>(tpl, true, [](auto tplPtr) { return CreateTemplateRenderer<ParentTemplateRenderer>(tplPtr); });
}

template<typename CharT>
class IncludedTemplateRenderer : public IRendererBase
{
public:
    // `exportNames`: copy the names the template sets at its top level into the caller's
    // current scope. Import collects a module this way; include must not leak them
    IncludedTemplateRenderer(std::shared_ptr<TemplateImpl<CharT>> tpl, bool withContext, bool exportNames)
        : m_template(tpl)
        , m_withContext(withContext)
        , m_exportNames(exportNames)
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        RenderContext innerContext = values.Clone(m_withContext);
        if (m_withContext)
            innerContext.EnterScope();

        m_template->GetRenderer()->Render(os, innerContext);
        if (m_withContext && m_exportNames)
        {
            auto& innerScope = innerContext.GetCurrentScope();
            auto& scope = values.GetCurrentScope();
            for (auto& v : innerScope)
            {
                if (v.first != "self")
                    scope[v.first] = std::move(v.second);
            }
        }
    }

    bool IsEqual(const IComparable& other) const override
    {
        auto* val = dynamic_cast<const IncludedTemplateRenderer<CharT>*>(&other);
        if (!val)
            return false;
        if (m_template != val->m_template)
            return false;
        if (m_withContext != val->m_withContext)
            return false;
        if (m_exportNames != val->m_exportNames)
            return false;
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
        auto tpl = values.GetRendererCallback()->LoadTemplate(name);

        try
        {
            auto renderer = VisitTemplateImpl<RendererPtr>(
                tpl, true, [this](auto tplPtr) { return CreateTemplateRenderer<IncludedTemplateRenderer>(tplPtr, m_withContext, false); });

            if (renderer)
            {
                renderer->Render(os, values);
                return true;
            }
        }
        catch (const BasicErrorInfo<char>& err)
        {
            if (err.GetCode() != ErrorCode::FileNotFound)
                throw;
        }
        catch (const BasicErrorInfo<wchar_t>& err)
        {
            if (err.GetCode() != ErrorCode::FileNotFound)
                throw;
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
                break;
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
            return;

        const auto* rendererPtr = GetIf<RendererPtr>(&contextVal->second);
        if (!rendererPtr)
            return;

        auto* renderer = static_cast<ImportedMacroRenderer*>(rendererPtr->get());
        renderer->InvokeMacro(callable, params, stream, context);
    }

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ImportedMacroRenderer*>(&other);
        if (!val)
            return false;
        if (m_importedContext != val->m_importedContext)
            return false;
        if (m_withContext != val->m_withContext)
            return false;
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

    // Loaded on every render: the name may change between renders or loop iterations
    auto tpl = values.GetRendererCallback()->LoadTemplate(name);
    auto renderer =
        VisitTemplateImpl<RendererPtr>(tpl, true, [](auto tplPtr) { return CreateTemplateRenderer<IncludedTemplateRenderer>(tplPtr, true, true); });
    if (!renderer)
        return;

    std::string scopeName;
    {
        TargetString tsScopeName = values.GetRendererCallback()->GetAsTargetString(name);
        scopeName = "$$_imported_" + GetAsSameString(scopeName, tsScopeName).value();
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

    for (auto& var : importedScope)
    {
        if (var.first.empty())
            continue;

        if (var.first[0] == '_')
            continue;

        auto mappedP = m_namesToImport.find(var.first);
        if (!m_namespace && mappedP == m_namesToImport.end())
            continue;

        InternalValue imported;
        auto* callable = GetIf<Callable>(&var.second);
        if (!callable)
        {
            imported = std::move(var.second);
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
            importedNs[var.first] = std::move(imported);
        else
            values.GetCurrentScope()[mappedP->second] = std::move(imported);
    }

    if (m_namespace)
        values.GetCurrentScope()[m_namespace.value()] = CreateMapAdapter(std::move(importedNs));
}

Callable MacroStatement::MakeCallable(RenderContext& values) const
{
    std::vector<InternalValue> definedDefaults(m_params.size());
    for (std::size_t idx = 0; idx < m_params.size(); ++idx)
    {
        const auto& p = m_params[idx];
        if (p.defaultValue && !p.defaultRefersToArgs)
            definedDefaults[idx] = p.defaultValue->Evaluate(values);
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
        arguments.emplace_back(p.paramName);

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
            names &= ~UsesCaller;
        else if (p.paramName == "varargs")
            names &= ~UsesVarargs;
        else if (p.paramName == "kwargs")
            names &= ~UsesKwargs;
    }
    return names;
}

// Binds the call arguments the way Jinja2's Macro.__call__ does
void MacroStatement::InvokeMacroRenderer(const std::vector<InternalValue>& definedDefaults,
                                         const CallParams& callParams,
                                         OutStream& stream,
                                         RenderContext& context) const
{
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
            continue;

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
        if (kwParams.count("caller"))
            throw std::runtime_error("macro " + GetDisplayName() + " was invoked with two values for the special caller argument. This is most likely a bug.");
        throw std::runtime_error("macro " + GetDisplayName() + " takes no keyword argument '" + kwParams.begin()->first + "'");
    }

    const bool catchVarargs = (caught & UsesVarargs) != 0;
    if (!catchVarargs && posParams.size() > argsCount)
        throw std::runtime_error("macro " + GetDisplayName() + " takes not more than " + std::to_string(argsCount) + " argument(s)");

    // Missing arguments and the special ones are bound before the defaults are evaluated, so
    // a default sees them and never an outer variable named like a later argument
    auto& scope = context.EnterScope();
    for (std::size_t idx = 0; idx < argsCount; ++idx)
    {
        const auto& name = m_params[idx].paramName;
        scope[name] = isProvided[idx] ? std::move(args[idx]) : MakeUndefinedWithHint(context, "parameter '" + name + "' was not provided");
    }

    if (catchCaller)
        scope["caller"s] = std::move(caller);
    if (catchKwargs)
    {
        InternalDict kwArgs;
        for (auto& kw : kwParams)
            kwArgs[kw.first] = std::move(kw.second);
        scope["kwargs"s] = CreateMapAdapter(std::move(kwArgs));
    }
    if (catchVarargs)
    {
        InternalValueList varArgs;
        for (auto idx = argsCount; idx < posParams.size(); ++idx)
            varArgs.push_back(posParams[idx]);
        scope["varargs"s] = ListAdapter::CreateAdapter(std::move(varArgs)).MarkAsTuple();
    }

    for (std::size_t idx = 0; idx < argsCount; ++idx)
    {
        const auto& p = m_params[idx];
        if (isProvided[idx] || !p.defaultValue)
            continue;

        auto value = p.defaultRefersToArgs ? p.defaultValue->Evaluate(context) : definedDefaults[idx];
        // Jinja2 evaluates defaults on every call, so acc=[] is a new list each time; the
        // template's lists and dicts are shared, so the stored one is copied
        if (methods::IsMutable(value))
            value = methods::CopyContainer(value);
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
        return;

    const auto& fnVal = macroPtr->second;
    const Callable* callable = GetIf<Callable>(&fnVal);
    if (callable == nullptr || callable->GetType() == Callable::Type::Expression)
        return;

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

    for (auto& var : m_scopeVars)
        scope[var.first] = var.second->Evaluate(values);

    m_mainBody->Render(os, innerValues);

    innerValues.ExitScope();
    values.SetLoopControl(innerValues.GetLoopControl());
}

void TransStatement::Render(OutStream& os, RenderContext& values)
{
    std::vector<InternalValue> evaluated;
    evaluated.reserve(m_variables.size());
    for (auto& var : m_variables)
        evaluated.push_back(var.second->Evaluate(values));

    auto& scope = values.EnterScope();
    for (size_t idx = 0; idx < evaluated.size(); ++idx)
        scope[VariableSlot(idx)] = std::move(evaluated[idx]);
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
        return;
    // The body is Markup under autoescape; the filtered output is written as is
    InternalValue body(std::move(arg));
    body.SetMarkup(values.IsAutoescape());
    const auto result = m_expr->Evaluate(std::move(body), values);
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
