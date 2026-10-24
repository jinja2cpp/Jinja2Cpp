#ifndef JINJA2CPP_SRC_RENDERER_H
#define JINJA2CPP_SRC_RENDERER_H

#include "engine_errors.h"
#include "expression_evaluator.h"
#include "internal_value.h"
#include "lexertk.h"
#include "markup.h"
#include "node_arena.h"
#include "out_stream.h"
#include "recursion_guard.h"
#include "render_context.h"
#include "render_failure.h"
#include "span.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <optional>
#include <utility>
#include <vector>

namespace jinja2
{
inline namespace v2
{
class IRendererBase : public ArenaNode
{
public:
    static constexpr KindRange Family{ NodeKind::ComposedBody, NodeKind::OutputCheckStmt };
    using FamilyOwner = IRendererBase;

    IRendererBase() = default;
    IRendererBase(const IRendererBase&) = delete;
    IRendererBase& operator=(const IRendererBase&) = delete;
    IRendererBase& operator=(IRendererBase&&) = delete;
    virtual void Render(OutStream& os, RenderContext& values) = 0;
protected:
    // Only the arena moves a node, when it seals the tree into one buffer
    IRendererBase(IRendererBase&&) = default;
    // Nothing destroys a node through its base: the arena destroys only the nodes that own
    // something, each as its own class, and leaves the others (0118 P5b). An imported
    // module's renderer is made with make_shared, which destroys it as its own class
    ~IRendererBase() = default;
};

// A renderer made during a render (an included or parent template); parse-tree renderers are
// NodeRefs into their template's arena
using RendererPtr = std::shared_ptr<IRendererBase>;

// A body: the arena keeps its children right after the node (NodeArena::MakeWithItems), so
// rendering it reads no separate list, and after them where each child's tag starts in the
// template's source, which only an error reads
class ComposedRenderer final : public IRendererBase
{
public:
    static constexpr NodeKind Kind = NodeKind::ComposedBody;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs.All(Children());
    }

    using Child = NodeRef<IRendererBase>;

    explicit ComposedRenderer(std::uint32_t count)
        : m_count(count)
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        // Every statement body: nested blocks recurse through here
        CheckStack();
        const auto& nodes = values.Nodes();
        const auto children = Children();
        const auto* const end = children.data() + children.size();
        const auto* child = children.data();
        // An error gets the location of the statement it came out of here, unless a body in
        // that statement gave it one (docs/api-2.0.md, 2.1 rule 8). Table-based unwinding: the
        // try costs nothing until something throws, and the handler needs nothing the loop
        // does not keep anyway
        bool failed = false;
        try
        {
            for (; child != end; ++child)
            {
                nodes[*child].Render(os, values);
                if (values.HasLoopControl())
                {
                    return;
                }
            }
        }
        catch (...)
        {
            // Thrown on once the handler is left (ThrowRenderFailure). A flag, not the
            // exception_ptr itself: a local that needs destroying would cost every body
            detail::KeepRenderFailure();
            failed = true;
        }
        if (failed)
        {
            detail::ThrowRenderFailure(values.Nodes(), child, OffsetOf(child, end));
        }
    }

    [[nodiscard]] Span<const Child> Children() const
    {
        // NodeArena::MakeWithItems put them one past this node
        return { std::launder(reinterpret_cast<const Child*>(this + 1)), m_count };
    }

private:
    // Where the tag of `child` starts in the source: the offsets follow the children, last
    // child first, so that the end of the children finds them
    static std::uint32_t OffsetOf(const Child* child, const Child* end)
    {
        const auto* offsets = std::launder(reinterpret_cast<const std::uint32_t*>(end));
        return offsets[end - child - 1];
    }

    std::uint32_t m_count;
};

class RawTextRenderer final : public IRendererBase
{
public:
    static constexpr NodeKind Kind = NodeKind::RawText;
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void VisitRefs(detail::RefChecker& /*refs*/) const {}

    // The text lives in the template's source, or in a converted copy the template keeps
    // with it
    RawTextRenderer(const void* ptr, size_t len)
        : m_ptr(ptr)
        , m_length(len)
        , m_isLong(len >= OutStream::LongLength)
    {
    }

    void Render(OutStream& os, RenderContext&) override
    {
        if (m_isLong)
        {
            os.WriteLong(m_ptr, m_length);
        }
        else
        {
            os.WriteBuffer(m_ptr, m_length);
        }
    }
private:
    const void* m_ptr{};
    size_t m_length{};
    bool m_isLong = false;
};

class ExpressionRenderer : public IRendererBase
{
public:
    static constexpr NodeKind Kind = NodeKind::ExprRenderer;
    static bool MatchesKind(NodeKind kind) { return kind == NodeKind::ExprRenderer || kind == NodeKind::FinalizedExprRenderer || kind == NodeKind::StaticExprRenderer; }
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expression);
    }

    explicit ExpressionRenderer(NodeRef<Expression> expr)
        : m_expression(expr)
    {
    }
    ExpressionRenderer(const ExpressionRenderer&) = delete;
    // For the arena, which moves the nodes when it seals the tree
    ExpressionRenderer(ExpressionRenderer&&) = default;
    ExpressionRenderer& operator=(const ExpressionRenderer&) = delete;
    ExpressionRenderer& operator=(ExpressionRenderer&&) = delete;

    void Render(OutStream& os, RenderContext& values) override { values.Nodes()[m_expression].Render(os, values); }

protected:
    // FinalizedExpressionRenderer derives from it
    ~ExpressionRenderer() = default;

    NodeRef<Expression> m_expression;
};

// `{{ ... }}` when Settings::finalize is set: only such templates pay for the callable.
// Jinja2 writes escape(finalize(value)), escaped by the output setting, for a value it
// evaluates at run time, and str(finalize(escape(value))), escaped by the static setting, for
// a value it folds (CodeGenerator._output_child_to_const, docs/tasks/0288)
class FinalizedExpressionRenderer final : public ExpressionRenderer
{
public:
    static constexpr NodeKind Kind = NodeKind::FinalizedExprRenderer;
    static bool MatchesKind(NodeKind kind) { return kind == Kind; }

    // finalize: Settings::finalize as a callable. isFolded: Jinja2 folds the value
    FinalizedExpressionRenderer(NodeRef<Expression> expr, InternalValue finalize, bool isFolded)
        : ExpressionRenderer(expr)
        , m_finalize(std::move(finalize))
        , m_isFolded(isFolded)
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        auto value = values.Nodes()[m_expression].Evaluate(values);
        if (m_isFolded && values.IsStaticAutoescape())
        {
            // Jinja2 evaluates the value at run time when finalizing its escaped form raises
            // (CodeGenerator.visit_Output, docs/tasks/0292)
            if (auto folded = TryFinalize(MarkupEscape(value, values.GetRendererCallback()), values))
            {
                os.WriteValue(*folded);
                return;
            }
        }
        else if (m_isFolded)
        {
            os.WriteValue(Finalize(std::move(value), values));
            return;
        }
        WriteOutput(os, Finalize(std::move(value), values), values);
    }

private:
    InternalValue Finalize(InternalValue value, RenderContext& values) const
    {
        CallParams params;
        params.posParams.push_back(std::move(value));
        return GetIf<Callable>(&m_finalize)->GetExpressionCallable()(params, values);
    }

    // Finalize, or nothing when the callable raises an error of its own or of its call
    std::optional<InternalValue> TryFinalize(InternalValue value, RenderContext& values) const
    {
        try
        {
            return Finalize(std::move(value), values);
        }
        catch (const detail::HostCallableFailure&)
        {
            return std::nullopt;
        }
        catch (const TemplateRuntimeError&)
        {
            return std::nullopt;
        }
    }

    InternalValue m_finalize;
    bool m_isFolded;
};

// `{{ ... }}` of a value Jinja2 folds, in volatile code (RenderContext::IsVolatileAutoescape):
// Jinja2 escapes the folded value by the setting the code is compiled under, not by the
// runtime one the rest of the output follows (docs/tasks/0216)
class StaticOutputRenderer final : public ExpressionRenderer
{
public:
    static constexpr NodeKind Kind = NodeKind::StaticExprRenderer;
    static bool MatchesKind(NodeKind kind) { return kind == Kind; }

    using ExpressionRenderer::ExpressionRenderer;

    void Render(OutStream& os, RenderContext& values) override
    {
        const OutputAutoescapeGuard autoescapeGuard(values, values.IsStaticAutoescape());
        values.Nodes()[m_expression].Render(os, values);
    }
};

// finalize: Settings::finalize as a callable, or undefined. isVolatile: in volatile code
// (RenderContext::IsVolatileAutoescape), where a folded value is a StaticOutputRenderer
inline NodeRef<IRendererBase> MakeExpressionRenderer(NodeArena& nodes, NodeRef<Expression> expr, const InternalValue& finalize, bool isVolatile = false)
{
    if (GetIf<Callable>(&finalize))
    {
        return nodes.Make<FinalizedExpressionRenderer>(expr, finalize, nodes[expr].IsFoldable(nodes));
    }
    if (isVolatile && nodes[expr].IsFoldable(nodes))
    {
        return nodes.Make<StaticOutputRenderer>(expr);
    }
    return nodes.Make<ExpressionRenderer>(expr);
}
} // namespace v2
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDERER_H
