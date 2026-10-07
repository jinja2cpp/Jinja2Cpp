#ifndef JINJA2CPP_SRC_RENDERER_H
#define JINJA2CPP_SRC_RENDERER_H

#include "expression_evaluator.h"
#include "internal_value.h"
#include "lexertk.h"
#include "node_arena.h"
#include "out_stream.h"
#include "recursion_guard.h"
#include "render_context.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <new>
#include <utility>
#include <vector>

namespace jinja2
{
class IRendererBase : public ArenaNode
{
public:
    IRendererBase() = default;
    IRendererBase(const IRendererBase&) = delete;
    IRendererBase& operator=(const IRendererBase&) = delete;
    IRendererBase& operator=(IRendererBase&&) = delete;
    virtual ~IRendererBase() = default;
    virtual void Render(OutStream& os, RenderContext& values) = 0;
protected:
    // Only the arena moves a node, when it seals the tree into one buffer
    IRendererBase(IRendererBase&&) = default;
};

// A renderer made during a render (an included or parent template); parse-tree renderers are
// NodeRefs into their template's arena
using RendererPtr = std::shared_ptr<IRendererBase>;

// A body: the arena keeps its children right after the node (NodeArena::MakeWithItems), so
// rendering it reads no separate list
class ComposedRenderer final : public IRendererBase
{
public:
    static constexpr NodeKind Kind = NodeKind::ComposedBody;

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
        for (auto r : Children())
        {
            nodes[r].Render(os, values);
            if (values.HasLoopControl())
            {
                return;
            }
        }
    }

    [[nodiscard]] boost::span<const Child> Children() const
    {
        // NodeArena::MakeWithItems put them one past this node
        return { std::launder(reinterpret_cast<const Child*>(this + 1)), m_count };
    }

private:
    std::uint32_t m_count;
};

class RawTextRenderer : public IRendererBase
{
public:
    static constexpr NodeKind Kind = NodeKind::RawText;

    RawTextRenderer(const void* ptr, size_t len, std::shared_ptr<const void> holder = {})
        : m_ptr(ptr)
        , m_length(len)
        , m_isLong(len >= OutStream::LongLength)
        , m_holder(std::move(holder))
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
    std::shared_ptr<const void> m_holder; // owns the text when it is not a part of the template source
};

class ExpressionRenderer : public IRendererBase
{
public:
    static constexpr NodeKind Kind = NodeKind::ExprRenderer;
    static bool MatchesKind(NodeKind kind) { return kind == NodeKind::ExprRenderer || kind == NodeKind::FinalizedExprRenderer; }

    explicit ExpressionRenderer(NodeRef<Expression> expr)
        : m_expression(expr)
    {
    }

    void Render(OutStream& os, RenderContext& values) override { values.Nodes()[m_expression].Render(os, values); }

protected:
    NodeRef<Expression> m_expression;
};

// `{{ ... }}` when Settings::finalize is set: only such templates pay for the callable
class FinalizedExpressionRenderer : public ExpressionRenderer
{
public:
    static constexpr NodeKind Kind = NodeKind::FinalizedExprRenderer;
    static bool MatchesKind(NodeKind kind) { return kind == Kind; }

    // finalize: Settings::finalize as a callable
    FinalizedExpressionRenderer(NodeRef<Expression> expr, InternalValue finalize)
        : ExpressionRenderer(expr)
        , m_finalize(std::move(finalize))
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        CallParams params;
        params.posParams.push_back(values.Nodes()[m_expression].Evaluate(values));
        os.WriteValue(GetIf<Callable>(&m_finalize)->GetExpressionCallable()(params, values));
    }

private:
    InternalValue m_finalize;
};

// finalize: Settings::finalize as a callable, or undefined
inline NodeRef<IRendererBase> MakeExpressionRenderer(NodeArena& nodes, NodeRef<Expression> expr, const InternalValue& finalize)
{
    if (GetIf<Callable>(&finalize))
    {
        return nodes.Make<FinalizedExpressionRenderer>(expr, finalize);
    }
    return nodes.Make<ExpressionRenderer>(expr);
}
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDERER_H
