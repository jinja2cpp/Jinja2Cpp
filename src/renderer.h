#ifndef JINJA2CPP_SRC_RENDERER_H
#define JINJA2CPP_SRC_RENDERER_H

#include "out_stream.h"
#include "lexertk.h"
#include "expression_evaluator.h"
#include "render_context.h"
#include "ast_visitor.h"

#include <jinja2cpp/value.h>
#include <jinja2cpp/utils/i_comparable.h>

#include <string>
#include <vector>
#include <memory>

namespace jinja2
{
class IRendererBase : public virtual IComparable
{
public:
    virtual ~IRendererBase() = default;
    virtual void Render(OutStream& os, RenderContext& values) = 0;
};

class VisitableRendererBase : public IRendererBase
    , public VisitableStatement
{
};

using RendererPtr = std::shared_ptr<IRendererBase>;

inline bool operator==(const RendererPtr& lhs, const RendererPtr& rhs)
{
    if (lhs && rhs && !lhs->IsEqual(*rhs))
        return false;
    if ((lhs && !rhs) || (!lhs && rhs))
        return false;
    return true;
}

inline bool operator!=(const RendererPtr& lhs, const RendererPtr& rhs)
{
    return !(lhs == rhs);
}

class ComposedRenderer : public VisitableRendererBase
{
public:
    VISITABLE_STATEMENT();

    void AddRenderer(RendererPtr r)
    {
        m_renderers.push_back(std::move(r));
    }
    void Render(OutStream& os, RenderContext& values) override
    {
        for (auto& r : m_renderers)
        {
            r->Render(os, values);
            if (values.HasLoopControl())
                return;
        }
    }

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ComposedRenderer*>(&other);
        if (!val)
            return false;
        return m_renderers == val->m_renderers;
    }

private:
    std::vector<RendererPtr> m_renderers;
};

class RawTextRenderer : public VisitableRendererBase
{
public:
    VISITABLE_STATEMENT();

    RawTextRenderer(const void* ptr, size_t len, std::shared_ptr<const void> holder = {})
        : m_ptr(ptr)
        , m_length(len)
        , m_holder(std::move(holder))
    {
    }

    void Render(OutStream& os, RenderContext&) override
    {
        os.WriteBuffer(m_ptr, m_length);
    }

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const RawTextRenderer*>(&other);
        if (!val)
            return false;
        if (m_ptr != val->m_ptr)
            return false;
        return m_length == val->m_length;
    }
private:
    const void* m_ptr{};
    size_t m_length{};
    std::shared_ptr<const void> m_holder; // owns the text when it is not a part of the template source
};

class ExpressionRenderer : public VisitableRendererBase
{
public:
    VISITABLE_STATEMENT();

    // finalize: Settings::finalize as a callable, or undefined
    explicit ExpressionRenderer(ExpressionEvaluatorPtr<> expr, InternalValue finalize = InternalValue())
        : m_expression(std::move(expr))
        , m_finalize(std::move(finalize))
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        auto* finalize = GetIf<Callable>(&m_finalize);
        if (!finalize)
        {
            m_expression->Render(os, values);
            return;
        }
        CallParams params;
        params.posParams.push_back(m_expression->Evaluate(values));
        os.WriteValue(finalize->GetExpressionCallable()(params, values));
    }

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ExpressionRenderer*>(&other);
        if (!val)
            return false;
        return m_expression == val->m_expression;
    }
private:
    ExpressionEvaluatorPtr<> m_expression;
    InternalValue m_finalize;
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDERER_H
