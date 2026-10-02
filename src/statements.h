#ifndef JINJA2CPP_SRC_STATEMENTS_H
#define JINJA2CPP_SRC_STATEMENTS_H

#include "renderer.h"
#include "expression_evaluator.h"

#include <string>
#include <utility>
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
    // The default names an argument of the macro, so it is evaluated per call in the
    // macro scope; other defaults are evaluated where the macro is defined
    bool defaultRefersToArgs = false;
};
inline bool operator==(const MacroParam& lhs, const MacroParam& rhs)
{
    if (lhs.paramName != rhs.paramName)
        return false;
    if (lhs.defaultValue != rhs.defaultValue)
        return false;
    if (lhs.defaultRefersToArgs != rhs.defaultRefersToArgs)
        return false;
    return true;
}

using MacroParams = std::vector<MacroParam>;

// The target of `for` and `set`: a name, a tuple of targets (`a, (b, c)`) or, for `set`, a
// namespace attribute (`ns.attr`)
struct AssignTarget
{
    std::string name;
    // Set on a namespace attribute target: the attribute of the namespace `name`
    std::string attr;
    bool isTuple = false;
    std::vector<AssignTarget> items;
};
inline bool operator==(const AssignTarget& lhs, const AssignTarget& rhs)
{
    return lhs.name == rhs.name && lhs.attr == rhs.attr && lhs.isTuple == rhs.isTuple && lhs.items == rhs.items;
}
inline bool operator!=(const AssignTarget& lhs, const AssignTarget& rhs)
{
    return !(lhs == rhs);
}

class ForStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    ForStatement(AssignTarget target, ExpressionEvaluatorPtr<> expr, ExpressionEvaluatorPtr<> ifExpr, bool isRecursive)
        : m_target(std::move(target))
        , m_value(std::move(expr))
        , m_ifExpr(std::move(ifExpr))
        , m_isRecursive(isRecursive)
    {
    }

    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = std::move(renderer);
    }

    void SetElseBody(RendererPtr renderer)
    {
        m_elseBody = std::move(renderer);
    }

    void Render(OutStream& os, RenderContext& values) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ForStatement*>(&other);
        if (!val)
            return false;
        if (m_target != val->m_target)
            return false;
        if (m_value != val->m_value)
            return false;
        if (m_ifExpr != val->m_ifExpr)
            return false;
        if (m_isRecursive != val->m_isRecursive)
            return false;
        if (m_mainBody != val->m_mainBody)
            return false;
        if (m_elseBody != val->m_elseBody)
            return false;
        return true;
    }

private:
    void RenderLoop(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level);
    ListAdapter CreateFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const;

    AssignTarget m_target;
    ExpressionEvaluatorPtr<> m_value;
    ExpressionEvaluatorPtr<> m_ifExpr;
    bool m_isRecursive{};
    RendererPtr m_mainBody;
    RendererPtr m_elseBody;
};

class ElseBranchStatement;

class IfStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    explicit IfStatement(ExpressionEvaluatorPtr<> expr)
        : m_expr(std::move(expr))
    {
    }

    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = std::move(renderer);
    }

    void AddElseBranch(const StatementPtr<ElseBranchStatement>& branch)
    {
        m_elseBranches.push_back(branch);
    }

    void Render(OutStream& os, RenderContext& values) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const IfStatement*>(&other);
        if (!val)
            return false;
        if (m_expr != val->m_expr)
            return false;
        if (m_mainBody != val->m_mainBody)
            return false;
        if (m_elseBranches != val->m_elseBranches)
            return false;
        return true;
    }
private:
    ExpressionEvaluatorPtr<> m_expr;
    RendererPtr m_mainBody;
    std::vector<StatementPtr<ElseBranchStatement>> m_elseBranches;
};


class ElseBranchStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    explicit ElseBranchStatement(ExpressionEvaluatorPtr<> expr)
        : m_expr(std::move(expr))
    {
    }

    bool ShouldRender(RenderContext& values) const;
    // A plain `else`, as opposed to an `elif`
    [[nodiscard]] bool IsElse() const { return !m_expr; }
    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = std::move(renderer);
    }
    void Render(OutStream& os, RenderContext& values) override;
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ElseBranchStatement*>(&other);
        if (!val)
            return false;
        if (m_expr != val->m_expr)
            return false;
        if (m_mainBody != val->m_mainBody)
            return false;
        return true;
    }

private:
    ExpressionEvaluatorPtr<> m_expr;
    RendererPtr m_mainBody;
};

class SetStatement : public Statement
{
public:
    explicit SetStatement(AssignTarget target)
        : m_target(std::move(target))
    {
    }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const SetStatement*>(&other);
        if (!val)
            return false;
        if (m_target != val->m_target)
            return false;
        return true;
    }
protected:
    void AssignBody(InternalValue, RenderContext&);

private:
    const AssignTarget m_target;
};

class SetLineStatement final : public SetStatement
{
public:
    VISITABLE_STATEMENT();

    SetLineStatement(AssignTarget target, ExpressionEvaluatorPtr<> expr)
        : SetStatement(std::move(target)), m_expr(std::move(expr))
    {
    }

    void Render(OutStream& os, RenderContext& values) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const SetLineStatement*>(&other);
        if (!val)
            return false;
        if (m_expr != val->m_expr)
            return false;
        return true;
    }
private:
    const ExpressionEvaluatorPtr<> m_expr;
};

class SetBlockStatement : public SetStatement
{
public:
    using SetStatement::SetStatement;

    void SetBody(RendererPtr renderer)
    {
        m_body = std::move(renderer);
    }

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const SetBlockStatement*>(&other);
        if (!val)
            return false;
        if (!SetStatement::IsEqual(*val))
            return false;
        if (m_body != val->m_body)
            return false;
        return true;
    }
protected:
    InternalValue RenderBody(RenderContext&);

private:
    RendererPtr m_body;
};

class SetRawBlockStatement final : public SetBlockStatement
{
public:
    VISITABLE_STATEMENT();

    using SetBlockStatement::SetBlockStatement;

    void Render(OutStream&, RenderContext&) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const SetRawBlockStatement*>(&other);
        if (!val)
            return false;
        if (!SetBlockStatement::IsEqual(*val))
            return false;
        return true;
    }
};

class SetFilteredBlockStatement final : public SetBlockStatement
{
public:
    VISITABLE_STATEMENT();

    explicit SetFilteredBlockStatement(AssignTarget target, ExpressionEvaluatorPtr<ExpressionFilter> expr)
        : SetBlockStatement(std::move(target)), m_expr(std::move(expr))
    {
    }

    void Render(OutStream&, RenderContext&) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const SetFilteredBlockStatement*>(&other);
        if (!val)
            return false;
        if (!SetBlockStatement::IsEqual(*val))
            return false;
        if (m_expr != val->m_expr)
            return false;
        return true;
    }

private:
    const ExpressionEvaluatorPtr<ExpressionFilter> m_expr;
};

class BlockStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    BlockStatement(std::string name, bool isScoped, bool isRequired)
        : m_name(std::move(name))
        , m_isScoped(isScoped)
        , m_isRequired(isRequired)
    {
    }

    [[nodiscard]] auto& GetName() const { return m_name; }
    [[nodiscard]] bool IsRequired() const { return m_isRequired; }

    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = std::move(renderer);
    }
    // Renders the block that overrides this one most (the top of the block stack), as Jinja2 does
    void Render(OutStream& os, RenderContext& values) override;
    // Renders this definition's own body; `super()` refers to the block at depth + 1
    void RenderBody(OutStream& os, RenderContext& values, size_t depth) const;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const BlockStatement*>(&other);
        if (!val)
            return false;
        if (m_name != val->m_name)
            return false;
        if (m_isScoped != val->m_isScoped)
            return false;
        if (m_isRequired != val->m_isRequired)
            return false;
        if (m_mainBody != val->m_mainBody)
            return false;
        return true;
    }

private:
    std::string m_name;
    bool m_isScoped{};
    bool m_isRequired{};
    RendererPtr m_mainBody;
};

class ExtendsStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    explicit ExtendsStatement(ExpressionEvaluatorPtr<> templateExpr)
        : m_templateExpr(std::move(templateExpr))
    {
    }

    // Like Jinja2, only loads the parent and remembers it: the parent is rendered when the
    // child template ends, and the child's own output after this point is dropped
    void Render(OutStream& os, RenderContext& values) override;
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ExtendsStatement*>(&other);
        if (!val)
            return false;
        if (m_templateExpr != val->m_templateExpr)
            return false;
        return true;
    }

private:
    ExpressionEvaluatorPtr<> m_templateExpr;
};

// Blocks of one template rendering, by name, the most derived first (Jinja2's
// context.blocks). A parent template appends its blocks when it is extended
struct BlocksStack
{
    std::unordered_map<std::string, std::vector<const BlockStatement*>> blocks;
    // Parent templates, kept alive while their blocks are on the stack
    std::vector<RendererPtr> parents;
};

// One template's code running, in an inheritance chain: Jinja2's root render function
struct TemplateFrame
{
    BlocksStack* blocks = nullptr;
    // Set by `extends`; from then on this template's top-level output is dropped
    RendererPtr parent;
    // Scopes visible to the template's top level, and so to unscoped blocks
    size_t baseDepth = 0;
};

// The root of a parsed template
class TemplateRenderer : public IRendererBase
{
public:
    using BlocksCollection = std::unordered_map<std::string, StatementPtr<BlockStatement>>;

    explicit TemplateRenderer(std::shared_ptr<ComposedRenderer> body)
        : m_body(std::move(body))
    {
    }

    // False if a block of this name is defined already
    bool AddBlock(const StatementPtr<BlockStatement>& block)
    {
        return m_blocks.emplace(block->GetName(), block).second;
    }
    void SetHasExtends() { m_hasExtends = true; }

    // Renders the template on its own (for Template::Render, include and import)
    void Render(OutStream& os, RenderContext& values) override;
    // Renders the template as the parent of the one rendering now: its blocks go below
    // the child's on the same stack
    void RenderAsParent(OutStream& os, RenderContext& values);

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const TemplateRenderer*>(&other);
        if (!val)
            return false;
        if (m_hasExtends != val->m_hasExtends)
            return false;
        if (m_blocks != val->m_blocks)
            return false;
        return m_body == val->m_body;
    }

private:
    void PushBlocks(BlocksStack& stack) const;
    void RenderBody(OutStream& os, RenderContext& values, BlocksStack& stack);

    std::shared_ptr<ComposedRenderer> m_body;
    BlocksCollection m_blocks;
    bool m_hasExtends = false;
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
        m_expr = std::move(expr);
    }

    void Render(OutStream& os, RenderContext& values) override;
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const IncludeStatement*>(&other);
        if (!val)
            return false;
        if (m_ignoreMissing != val->m_ignoreMissing)
            return false;
        if (m_withContext != val->m_withContext)
            return false;
        if (m_expr != val->m_expr)
            return false;
        return true;
    }
private:
    bool m_ignoreMissing{};
    bool m_withContext{};
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
        m_nameExpr = std::move(expr);
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

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const ImportStatement*>(&other);
        if (!val)
            return false;
        if (m_namespace != val->m_namespace)
            return false;
        if (m_withContext != val->m_withContext)
            return false;
        if (m_namesToImport != val->m_namesToImport)
            return false;
        if (m_nameExpr != val->m_nameExpr)
            return false;
        return true;
    }
private:
    void ImportNames(RenderContext& values, InternalValueMap& importedScope, const std::string& scopeName) const;

    bool m_withContext{};
    ExpressionEvaluatorPtr<> m_nameExpr;
    std::optional<std::string> m_namespace;
    std::unordered_map<std::string, std::string> m_namesToImport;
};

class MacroStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    // Special names a macro body refers to. Like Jinja2, a macro accepts a caller, extra
    // positional or extra keyword arguments only when its body uses the matching name
    enum SpecialName : unsigned
    {
        UsesCaller = 1,
        UsesVarargs = 2,
        UsesKwargs = 4
    };

    MacroStatement(std::string name, MacroParams params)
        : m_name(std::move(name))
        , m_params(std::move(params))
    {
    }

    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = std::move(renderer);
        m_attributes = MakeAttributes();
    }

    // The body reads these names (counts only before they are assigned)
    void AddSpecialNames(unsigned names)
    {
        m_specialNames |= names & ~m_assignedNames;
    }

    // The body assigns these names
    void DiscardSpecialNames(unsigned names)
    {
        m_assignedNames |= names & ~m_specialNames;
    }

    // Jinja2: a declared `caller` argument of a macro that uses caller needs a default
    [[nodiscard]] bool HasInvalidCallerParam() const
    {
        if ((m_specialNames & UsesCaller) == 0)
            return false;
        for (const auto& p : m_params)
        {
            if (p.paramName == "caller")
                return !p.defaultValue;
        }
        return false;
    }

    void Render(OutStream& os, RenderContext& values) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const MacroStatement*>(&other);
        if (!val)
            return false;
        if (m_name != val->m_name)
            return false;
        if (m_params != val->m_params)
            return false;
        if (m_specialNames != val->m_specialNames)
            return false;
        if (m_mainBody != val->m_mainBody)
            return false;
        return true;
    }

protected:
    Callable MakeCallable(RenderContext& values) const;
    void InvokeMacroRenderer(const std::vector<InternalValue>& definedDefaults, const CallParams& callParams, OutStream& stream, RenderContext& context) const;
    // The special names bound when the macro is called: a declared argument named like
    // one of them is an ordinary argument
    [[nodiscard]] unsigned GetCaughtNames() const;
    // Value of `macro.name`: none for the caller of a call block
    [[nodiscard]] virtual InternalValue GetMacroName() const;
    [[nodiscard]] std::string GetDisplayName() const;
    [[nodiscard]] std::shared_ptr<const InternalValueMap> MakeAttributes() const;

    std::string m_name;
    MacroParams m_params;
    RendererPtr m_mainBody;
    unsigned m_specialNames = 0;
    unsigned m_assignedNames = 0;
    std::shared_ptr<const InternalValueMap> m_attributes;
};

class MacroCallStatement : public MacroStatement
{
public:
    VISITABLE_STATEMENT();

    MacroCallStatement(std::string macroName, CallParamsInfo callParams, MacroParams callbackParams)
        : MacroStatement("$call$", std::move(callbackParams))
        , m_macroName(std::move(macroName))
        , m_callParams(std::move(callParams))
    {
    }

    void Render(OutStream& os, RenderContext& values) override;

    bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const MacroCallStatement*>(&other);
        if (!val)
            return false;
        if (m_macroName != val->m_macroName)
            return false;
        if (m_callParams != val->m_callParams)
            return false;
        return true;
    }
protected:
    InternalValue GetMacroName() const override;

    std::string m_macroName;
    CallParamsInfo m_callParams;
};

class DoStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    explicit DoStatement(ExpressionEvaluatorPtr<> expr)
        : m_expr(std::move(expr)) {}

    void Render(OutStream& os, RenderContext& values) override;
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const DoStatement*>(&other);
        if (!val)
            return false;
        if (m_expr != val->m_expr)
            return false;
        return true;
    }
private:
    ExpressionEvaluatorPtr<> m_expr;
};

// `{% trans %}` (Jinja2's i18n extension). Output renders the gettext call that Jinja2 makes of
// the block; its arguments refer to the variables of the block by VariableSlot(index), which
// Render sets in a scope of their own, so each variable is evaluated once, before the call.
class TransStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    TransStatement(std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> variables, RendererPtr output)
        : m_variables(std::move(variables))
        , m_output(std::move(output))
    {
    }

    static std::string VariableSlot(size_t index) { return "$trans" + std::to_string(index); }

    void Render(OutStream& os, RenderContext& values) override;
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const TransStatement*>(&other);
        if (!val)
            return false;
        if (m_variables != val->m_variables)
            return false;
        return m_output == val->m_output;
    }
private:
    std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> m_variables;
    RendererPtr m_output;
};

// `break` or `continue` (Jinja2's loopcontrols extension)
class LoopControlStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    explicit LoopControlStatement(LoopControl control)
        : m_control(control)
    {
    }

    void Render(OutStream&, RenderContext& values) override { values.SetLoopControl(m_control); }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const LoopControlStatement*>(&other);
        return val != nullptr && m_control == val->m_control;
    }

private:
    LoopControl m_control;
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
        m_mainBody = std::move(renderer);
    }

    void Render(OutStream& os, RenderContext& values) override;
    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const WithStatement*>(&other);
        if (!val)
            return false;
        if (m_scopeVars != val->m_scopeVars)
            return false;
        if (m_mainBody != val->m_mainBody)
            return false;
        return true;
    }
private:
    std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> m_scopeVars;
    RendererPtr m_mainBody;
};

class FilterStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    explicit FilterStatement(ExpressionEvaluatorPtr<ExpressionFilter> expr)
        : m_expr(std::move(expr)) {}

    void SetBody(RendererPtr renderer)
    {
        m_body = std::move(renderer);
    }

    void Render(OutStream&, RenderContext&) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const FilterStatement*>(&other);
        if (!val)
            return false;
        if (m_expr != val->m_expr)
            return false;
        if (m_body != val->m_body)
            return false;
        return true;
    }
private:
    ExpressionEvaluatorPtr<ExpressionFilter> m_expr;
    RendererPtr m_body;
};

// {% autoescape expr %}: turns output escaping on or off for its body, in a new scope
class AutoescapeStatement : public Statement
{
public:
    VISITABLE_STATEMENT();

    explicit AutoescapeStatement(ExpressionEvaluatorPtr<FullExpressionEvaluator> expr)
        : m_expr(std::move(expr))
    {
    }

    void SetBody(RendererPtr renderer) { m_body = std::move(renderer); }

    void Render(OutStream&, RenderContext&) override;

    [[nodiscard]] bool IsEqual(const IComparable& other) const override
    {
        const auto* val = dynamic_cast<const AutoescapeStatement*>(&other);
        if (!val)
            return false;
        if (m_expr != val->m_expr)
            return false;
        if (m_body != val->m_body)
            return false;
        return true;
    }

private:
    ExpressionEvaluatorPtr<FullExpressionEvaluator> m_expr;
    RendererPtr m_body;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_STATEMENTS_H
