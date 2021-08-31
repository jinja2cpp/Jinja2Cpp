#ifndef JINJA2CPP_SRC_RENDERER_H
#define JINJA2CPP_SRC_RENDERER_H

#include "jinja2cpp/value.h"
#include "out_stream.h"
#include "lexertk.h"
#include "expression_evaluator.h"
#include "render_context.h"
#include "ast_visitor.h"

#include <iostream>
#include <string>
#include <vector>
#include <memory>

namespace jinja2
{
class RendererBase
{
public:
    virtual ~RendererBase() = default;
    virtual void Render(OutStream& os, RenderContext& values) = 0;
};

class VisitableRendererBase : public RendererBase,  public VisitableStatement
{  
};

using RendererPtr = std::shared_ptr<RendererBase>;

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
            r->Render(os, values);
    }

private:
    std::vector<RendererPtr> m_renderers;
};

class RawTextRenderer : public VisitableRendererBase
{
public:
    VISITABLE_STATEMENT();
    
    RawTextRenderer(const void* ptr, size_t len)
        : m_ptr(ptr)
        , m_length(len)
    {
    }

    void Render(OutStream& os, RenderContext&) override
    {
        os.WriteBuffer(m_ptr, m_length);
    }
private:
    const void* m_ptr;
    size_t m_length;
};

class ExpressionRenderer : public VisitableRendererBase
{
public:
    VISITABLE_STATEMENT();

    explicit ExpressionRenderer(ExpressionEvaluatorPtr<> expr)
        : m_expression(std::move(expr))
    {
    }

    void Render(OutStream& os, RenderContext& values) override
    {
        m_expression->Render(os, values);
    }
private:
    ExpressionEvaluatorPtr<> m_expression;
};
} // namespace jinja2

#endif // JINJA2CPP_SRC_RENDERER_H
