#ifndef JINJA2CPP_SRC_STATEMENTS_H
#define JINJA2CPP_SRC_STATEMENTS_H

#include "expression_evaluator.h"
#include "internal_value.h"
#include "out_stream.h"
#include "render_context.h"
#include "renderer.h"

#include <boost/container/small_vector.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace jinja2
{
class Statement : public IRendererBase
{
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

class ForStatement : public Statement
{
public:
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

    // The loop(...) callable of a recursive loop at depth0 `level`
    static Callable MakeLoopRecursion(ForStatement* statement, int level);

private:
    void RenderLoop(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level);
    ListAdapter CreateFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const;

    AssignTarget m_target;
    ExpressionEvaluatorPtr<> m_value;
    ExpressionEvaluatorPtr<> m_ifExpr;
    bool m_isRecursive{};
    RendererPtr m_mainBody;
    RendererPtr m_elseBody;
    // Unique for the process, unlike the address: a reused loop frame keeps the names this
    // loop put in its scope (docs/tasks/0133)
    uint64_t m_loopId = NewLoopId();

    static uint64_t NewLoopId();
};

class ElseBranchStatement;

class IfStatement : public Statement
{
public:
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
private:
    ExpressionEvaluatorPtr<> m_expr;
    RendererPtr m_mainBody;
    boost::container::small_vector<StatementPtr<ElseBranchStatement>, 1> m_elseBranches;
};

class ElseBranchStatement : public Statement
{
public:
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
protected:
    [[nodiscard]] const AssignTarget& GetTarget() const { return m_target; }

private:
    const AssignTarget m_target;
};

class SetLineStatement final : public SetStatement
{
public:
    SetLineStatement(AssignTarget target, ExpressionEvaluatorPtr<> expr)
        : SetStatement(std::move(target)), m_expr(std::move(expr))
    {
    }

    void Render(OutStream& os, RenderContext& values) override;
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
protected:
    InternalValue RenderBody(RenderContext&);

private:
    RendererPtr m_body;
};

class SetRawBlockStatement final : public SetBlockStatement
{
public:
    using SetBlockStatement::SetBlockStatement;

    void Render(OutStream&, RenderContext&) override;
};

class SetFilteredBlockStatement final : public SetBlockStatement
{
public:
    explicit SetFilteredBlockStatement(AssignTarget target, ExpressionEvaluatorPtr<ExpressionFilter> expr)
        : SetBlockStatement(std::move(target)), m_expr(std::move(expr))
    {
    }

    void Render(OutStream&, RenderContext&) override;

private:
    const ExpressionEvaluatorPtr<ExpressionFilter> m_expr;
};

class BlockStatement : public Statement
{
public:
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

private:
    std::string m_name;
    bool m_isScoped{};
    bool m_isRequired{};
    RendererPtr m_mainBody;
};

class ExtendsStatement : public Statement
{
public:
    explicit ExtendsStatement(ExpressionEvaluatorPtr<> templateExpr)
        : m_templateExpr(std::move(templateExpr))
    {
    }

    // Like Jinja2, only loads the parent and remembers it: the parent is rendered when the
    // child template ends, and the child's own output after this point is dropped
    void Render(OutStream& os, RenderContext& values) override;

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
    // The template's `self`, made when a name first asks for it (docs/tasks/0139)
    std::optional<InternalValue> self;
    // The frame this one was entered from (an importer, for a macro it imported); only
    // compared, so that `self` finds the template it came from
    TemplateFrame* outer = nullptr;
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
    // Adds this template's blocks below the ones already on `stack`
    void PushBlocks(BlocksStack& stack) const;

private:
    void RenderBody(OutStream& os, RenderContext& values, BlocksStack& stack);

    std::shared_ptr<ComposedRenderer> m_body;
    BlocksCollection m_blocks;
    bool m_hasExtends = false;
};

class IncludeStatement : public Statement
{
public:
    IncludeStatement(bool ignoreMissing, bool withContext)
        : m_ignoreMissing(ignoreMissing)
        , m_withContext(withContext)
    {}

    void SetIncludeNamesExpr(ExpressionEvaluatorPtr<> expr)
    {
        m_expr = std::move(expr);
    }

    void Render(OutStream& os, RenderContext& values) override;
private:
    bool m_ignoreMissing{};
    bool m_withContext{};
    ExpressionEvaluatorPtr<> m_expr;
};

class ImportStatement : public Statement
{
public:
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
private:
    void ImportNames(RenderContext& values, const InternalValueMap& importedScope, const std::string& scopeName) const;

    bool m_withContext{};
    ExpressionEvaluatorPtr<> m_nameExpr;
    std::optional<std::string> m_namespace;
    std::unordered_map<std::string, std::string> m_namesToImport;
};

class MacroStatement : public Statement
{
public:
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
        {
            return false;
        }
        for (const auto& p : m_params)
        {
            if (p.paramName == "caller")
            {
                return !p.defaultValue;
            }
        }
        return false;
    }

    void Render(OutStream& os, RenderContext& values) override;

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
    MacroCallStatement(std::string macroName, CallParamsInfo callParams, MacroParams callbackParams)
        : MacroStatement("$call$", std::move(callbackParams))
        , m_macroName(std::move(macroName))
        , m_callParams(std::move(callParams))
    {
    }

    void Render(OutStream& os, RenderContext& values) override;
protected:
    InternalValue GetMacroName() const override;

    std::string m_macroName;
    CallParamsInfo m_callParams;
};

class DoStatement : public Statement
{
public:
    explicit DoStatement(ExpressionEvaluatorPtr<> expr)
        : m_expr(std::move(expr)) {}

    void Render(OutStream& os, RenderContext& values) override;
private:
    ExpressionEvaluatorPtr<> m_expr;
};

// `{% trans %}` (Jinja2's i18n extension). Output renders the gettext call that Jinja2 makes of
// the block; its arguments refer to the variables of the block by VariableSlot(index), which
// Render sets in a scope of their own, so each variable is evaluated once, before the call.
class TransStatement : public Statement
{
public:
    TransStatement(std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> variables, RendererPtr output)
        : m_variables(std::move(variables))
        , m_output(std::move(output))
    {
    }

    static std::string VariableSlot(size_t index) { return "$trans" + std::to_string(index); }

    void Render(OutStream& os, RenderContext& values) override;
private:
    std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> m_variables;
    RendererPtr m_output;
};

// `break` or `continue` (Jinja2's loopcontrols extension)
class LoopControlStatement : public Statement
{
public:
    explicit LoopControlStatement(LoopControl control)
        : m_control(control)
    {
    }

    void Render(OutStream&, RenderContext& values) override { values.SetLoopControl(m_control); }

private:
    LoopControl m_control;
};

class WithStatement : public Statement
{
public:
    void SetScopeVars(std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> vars)
    {
        m_scopeVars = std::move(vars);
    }
    void SetMainBody(RendererPtr renderer)
    {
        m_mainBody = std::move(renderer);
    }

    void Render(OutStream& os, RenderContext& values) override;
private:
    std::vector<std::pair<std::string, ExpressionEvaluatorPtr<>>> m_scopeVars;
    RendererPtr m_mainBody;
};

class FilterStatement : public Statement
{
public:
    explicit FilterStatement(ExpressionEvaluatorPtr<ExpressionFilter> expr)
        : m_expr(std::move(expr)) {}

    void SetBody(RendererPtr renderer)
    {
        m_body = std::move(renderer);
    }

    void Render(OutStream&, RenderContext&) override;
private:
    ExpressionEvaluatorPtr<ExpressionFilter> m_expr;
    RendererPtr m_body;
};

// {% autoescape expr %}: turns output escaping on or off for its body, in a new scope
class AutoescapeStatement : public Statement
{
public:
    explicit AutoescapeStatement(ExpressionEvaluatorPtr<Expression> expr)
        : m_expr(std::move(expr))
    {
    }

    void SetBody(RendererPtr renderer) { m_body = std::move(renderer); }

    void Render(OutStream&, RenderContext&) override;

private:
    ExpressionEvaluatorPtr<Expression> m_expr;
    RendererPtr m_body;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_STATEMENTS_H
