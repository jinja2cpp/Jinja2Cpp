#ifndef STATEMENTS_H
#define STATEMENTS_H

#include "renderer.h"
#include "expression_evaluator.h"

#include <string>
#include <vector>

namespace jinja2
{
struct Statement : public RendererBase
{
};

template<typename T = Statement>
using StatementPtr = std::shared_ptr<T>;

class ForStatement : public Statement
{
public:
    ForStatement(std::vector<std::string> vars, ExpressionEvaluatorPtr<> expr, ExpressionEvaluatorPtr<> ifExpr)
        : m_vars(std::move(vars))
        , m_value(expr)
        , m_ifExpr(ifExpr)
    {
    }

    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = renderer;
    }

    void SetElseBody(RendererPtr renderer)
    {
        m_elseBody = renderer;
    }

    void Render(OutStream& os, RenderContext& values) override;

private:
    std::vector<std::string> m_vars;
    ExpressionEvaluatorPtr<> m_value;
    ExpressionEvaluatorPtr<> m_ifExpr;
    RendererPtr m_mainBody;
    RendererPtr m_elseBody;
};

class ElseBranchStatement;

class IfStatement : public Statement
{
public:
    IfStatement(ExpressionEvaluatorPtr<> expr)
        : m_expr(expr)
    {
    }

    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = renderer;
    }

    void AddElseBranch(StatementPtr<ElseBranchStatement> branch)
    {
        m_elseBranches.push_back(branch);
    }

    void Render(OutStream& os, RenderContext& values) override;

private:
    ExpressionEvaluatorPtr<> m_expr;
    RendererPtr m_mainBody;
    std::vector<StatementPtr<ElseBranchStatement>> m_elseBranches;
};


class ElseBranchStatement : public Statement
{
public:
    ElseBranchStatement(ExpressionEvaluatorPtr<> expr)
        : m_expr(expr)
    {
    }

    bool ShouldRender(RenderContext& values) const;
    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = renderer;
    }
    void Render(OutStream& os, RenderContext& values) override;

private:
    ExpressionEvaluatorPtr<> m_expr;
    RendererPtr m_mainBody;
};

class SetStatement : public Statement
{
public:
    SetStatement(std::vector<std::string> fields)
        : m_fields(std::move(fields))
    {
    }

    void SetAssignmentExpr(ExpressionEvaluatorPtr<> expr)
    {
        m_expr = expr;
    }
    void Render(OutStream& os, RenderContext& values) override;

private:
    std::vector<std::string> m_fields;
    ExpressionEvaluatorPtr<> m_expr;
};
} // jinja2

#endif // STATEMENTS_H
