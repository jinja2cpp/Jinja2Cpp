#ifndef JINJA2CPP_SRC_RENDERER_H
#define JINJA2CPP_SRC_RENDERER_H

#include "expression_evaluator.h"
#include "internal_value.h"
#include "lexertk.h"
#include "out_stream.h"
#include "recursion_guard.h"
#include "render_context.h"

#include <boost/container/small_vector.hpp>

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace jinja2
{
class IRendererBase
{
public:
    IRendererBase() = default;
    IRendererBase(const IRendererBase&) = delete;
    IRendererBase(IRendererBase&&) = delete;
    IRendererBase& operator=(const IRendererBase&) = delete;
    IRendererBase& operator=(IRendererBase&&) = delete;
    virtual ~IRendererBase() = default;
    virtual void Render(OutStream& os, RenderContext& values) = 0;
};

using RendererPtr = std::shared_ptr<IRendererBase>;

class ComposedRenderer : public IRendererBase
{
public:
    void AddRenderer(RendererPtr r)
    {
        m_renderers.push_back(std::move(r));
    }
    // After the parse: gives back the spare capacity the list grew with
    void ShrinkToFit() { m_renderers.shrink_to_fit(); }
    void Render(OutStream& os, RenderContext& values) override
    {
        // Every statement body: nested blocks recurse through here
        CheckStack();
        for (auto& r : m_renderers)
        {
            r->Render(os, values);
            if (values.HasLoopControl())
            {
                return;
            }
        }
    }

private:
    // A statement body holds a few nodes, kept in place; the template root grows on the heap
    boost::container::small_vector<RendererPtr, 4> m_renderers;
};

class RawTextRenderer : public IRendererBase
{
public:
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
    explicit ExpressionRenderer(ExpressionEvaluatorPtr<> expr)
        : m_expression(std::move(expr))
    {
    }

    void Render(OutStream& os, RenderContext& values) override { m_expression->Render(os, values); }

protected:
    ExpressionEvaluatorPtr<> m_expression;
};

// `{{ ... }}` when Settings::finalize is set: only such templates pay for the callable
class FinalizedExpressionRenderer : public ExpressionRenderer
{
public:
    // finalize: Settings::finalize as a callable
    FinalizedExpressionRenderer(ExpressionEvaluatorPtr<> expr, InternalValue finalize)
        : ExpressionRenderer(std::move(expr))
        , m_finalize(std::move(finalize))
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        CallParams params;
        params.posParams.push_back(m_expression->Evaluate(values));
        os.WriteValue(GetIf<Callable>(&m_finalize)->GetExpressionCallable()(params, values));
    }

private:
    InternalValue m_finalize;
};

// finalize: Settings::finalize as a callable, or undefined
inline RendererPtr MakeExpressionRenderer(ExpressionEvaluatorPtr<> expr, const InternalValue& finalize)
{
    if (GetIf<Callable>(&finalize))
    {
        return std::make_shared<FinalizedExpressionRenderer>(std::move(expr), finalize);
    }
    return std::make_shared<ExpressionRenderer>(std::move(expr));
}
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDERER_H
