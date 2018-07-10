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

template<typename CharT>
class TemplateImpl;

class ForStatement : public Statement
{
public:
    ForStatement(std::vector<std::string> vars, ExpressionEvaluatorPtr<> expr, ExpressionEvaluatorPtr<> ifExpr, bool isRecursive)
        : m_vars(std::move(vars))
        , m_value(expr)
        , m_ifExpr(ifExpr)
        , m_isRecursive(isRecursive)
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
    void RenderLoop(const InternalValue& val, OutStream& os, RenderContext& values);

private:
    std::vector<std::string> m_vars;
    ExpressionEvaluatorPtr<> m_value;
    ExpressionEvaluatorPtr<> m_ifExpr;
    bool m_isRecursive;
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

class ParentBlockStatement : public Statement
{
public:
    ParentBlockStatement(std::string name, bool isScoped)
        : m_name(std::move(name))
        , m_isScoped(isScoped)
    {
    }

    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = renderer;
    }
    void Render(OutStream &os, RenderContext &values) override;

private:
    std::string m_name;
    bool m_isScoped;
    RendererPtr m_mainBody;
};

class BlockStatement : public Statement
{
public:
    BlockStatement(std::string name)
        : m_name(std::move(name))
    {
    }

    auto& GetName() const {return m_name;}

    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = renderer;
    }
    void Render(OutStream &os, RenderContext &values) override;

private:
    std::string m_name;
    RendererPtr m_mainBody;
};

class ExtendsStatement : public Statement
{
public:
    using BlocksCollection = std::unordered_map<std::string, StatementPtr<BlockStatement>>;

    ExtendsStatement(std::string name, bool isPath)
        : m_templateName(std::move(name))
        , m_isPath(isPath)
    {
    }

    void Render(OutStream &os, RenderContext &values) override;
    void AddBlock(StatementPtr<BlockStatement> block)
    {
        m_blocks[block->GetName()] = block;
    }
private:
    std::string m_templateName;
    bool m_isPath;
    BlocksCollection m_blocks;
    void DoRender(OutStream &os, RenderContext &values);
};
} // jinja2

#endif // STATEMENTS_H
