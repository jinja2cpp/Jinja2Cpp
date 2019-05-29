#ifndef STATEMENTS_H
#define STATEMENTS_H

#include "renderer.h"
#include "expression_evaluator.h"

#include <string>
#include <vector>

namespace jinja2
{
class Statement : public VisitableRendererBase
{
public:
    VISITABLE_STATEMENT();
};

template<typename T = Statement>
using StatementPtr = std::shared_ptr<T>;

template<typename CharT>
class TemplateImpl;

struct MacroParam
{
    std::string paramName;
    ExpressionEvaluatorPtr<> defaultValue;
};

using MacroParams = std::vector<MacroParam>;

class ForStatement : public Statement
{
public:
    VISITABLE_STATEMENT();
    
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
    VISITABLE_STATEMENT();

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
    VISITABLE_STATEMENT();

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
    VISITABLE_STATEMENT();

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
    VISITABLE_STATEMENT();

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
    VISITABLE_STATEMENT();

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
    VISITABLE_STATEMENT();

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

class IncludeStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    IncludeStatement(bool ignoreMissing, bool withContext)
        : m_ignoreMissing(ignoreMissing)
        , m_withContext(withContext)
    {}

    void SetIncludeNamesExpr(ExpressionEvaluatorPtr<> expr)
    {
        m_expr = expr;
    }

    void Render(OutStream& os, RenderContext& values) override;
private:
    bool m_ignoreMissing;
    bool m_withContext;
    ExpressionEvaluatorPtr<> m_expr;
};

class ImportStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    explicit ImportStatement(bool withContext)
        : m_withContext(withContext)
    {}

    void SetImportNameExpr(ExpressionEvaluatorPtr<> expr)
    {
        m_nameExpr = expr;
    }

    void SetNamespace(std::string name)
    {
        m_namespace = std::move(name);
    }

    void AddNameToImport(std::string name, std::string alias)
    {
        m_namesToImport[std::move(name)] = std::move(alias);
    }

    void Render(OutStream& os, RenderContext& values) override;

private:
    void ImportNames(RenderContext& values, InternalValueMap& importedScope, const std::string& scopeName) const;

private:
    bool m_withContext;
    RendererPtr m_renderer;
    ExpressionEvaluatorPtr<> m_nameExpr;
    nonstd::optional<std::string> m_namespace;
    std::unordered_map<std::string, std::string> m_namesToImport;
};

class MacroStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    MacroStatement(std::string name, MacroParams params)
        : m_name(std::move(name))
        , m_params(std::move(params))
    {
    }

    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = renderer;
    }

    void Render(OutStream &os, RenderContext &values) override;

protected:
    void InvokeMacroRenderer(const CallParams& callParams, OutStream& stream, RenderContext& context);
    void SetupCallArgs(const std::vector<ArgumentInfo>& argsInfo, const CallParams& callParams, RenderContext& context, InternalValueMap& callArgs, InternalValueMap& kwArgs, InternalValueList& varArgs);
    virtual void SetupMacroScope(InternalValueMap& scope);

protected:
    std::string m_name;
    MacroParams m_params;
    std::vector<ArgumentInfo> m_preparedParams;
    RendererPtr m_mainBody;
    void PrepareMacroParams(RenderContext& values);
};

class MacroCallStatement : public MacroStatement
{
public:
    VISITABLE_STATEMENT();

    MacroCallStatement(std::string macroName, CallParams callParams, MacroParams callbackParams)
        : MacroStatement("$call$", std::move(callbackParams))
        , m_macroName(std::move(macroName))
        , m_callParams(std::move(callParams))
    {
    }

    void Render(OutStream &os, RenderContext &values) override;

protected:
    void SetupMacroScope(InternalValueMap& scope) override;

protected:
    std::string m_macroName;
    CallParams m_callParams;
};

class DoStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    DoStatement(ExpressionEvaluatorPtr<> expr) : m_expr(expr) {}

    void Render(OutStream &os, RenderContext &values) override;

private:
    ExpressionEvaluatorPtr<> m_expr;
};

class WithStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    void SetScopeVars(std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> vars)
    {
        m_scopeVars = std::move(vars);
    }
    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = renderer;
    }

    void Render(OutStream &os, RenderContext &values) override;

private:
    std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> m_scopeVars;
    RendererPtr m_mainBody;
};
} // jinja2


#endif // STATEMENTS_H
