#ifndef JINJA2CPP_SRC_STATEMENTS_H
#define JINJA2CPP_SRC_STATEMENTS_H

#include "expression_evaluator.h"
#include "internal_value.h"
#include "node_arena.h"
#include "out_stream.h"
#include "render_context.h"
#include "template_slots.h"
#include "renderer.h"
#include "slot_frame.h"

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

template<typename CharT>
class TemplateImpl;

struct MacroParam
{
    std::string paramName;
    NodeRef<Expression> defaultValue;
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
    // A loop target name's slot in its unit's frame (0117 P1); Dynamic otherwise
    SlotIndex slot;
};

class ForStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::ForStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_value);
        refs(m_ifExpr);
        refs(m_mainBody);
        refs(m_elseBody);
        refs(m_slotNames);
    }

    ForStatement(AssignTarget target, NodeRef<Expression> expr, NodeRef<Expression> ifExpr, bool isRecursive)
        : m_target(std::move(target))
        , m_value(std::move(expr))
        , m_ifExpr(std::move(ifExpr))
        , m_isRecursive(isRecursive)
    {
    }

    void SetMainBody(NodeRef<IRendererBase> renderer)
    {
        m_mainBody = renderer;
    }

    void SetElseBody(NodeRef<IRendererBase> renderer)
    {
        m_elseBody = renderer;
    }

    void Render(OutStream& os, RenderContext& values) override;

    // The loop(...) callable of a recursive loop at depth0 `level`
    static Callable MakeLoopRecursion(ForStatement* statement, const ArenaView& nodes, int level);

    [[nodiscard]] bool IsRecursive() const { return m_isRecursive; }
    [[nodiscard]] bool HasFilter() const { return static_cast<bool>(m_ifExpr); }
    // Makes the list of the names the loop binds: `loop`, then its target names in order,
    // each once. Called once, before BindSlots
    ArenaSpan<SlotName> MakeBinderNames(NodeArena& nodes);
    [[nodiscard]] ArenaSpan<SlotName> GetBinderNames() const { return m_slotNames; }
    // The binder names point into the targets: they follow the node when the arena moves it
    void OnRelocated(const ArenaView& nodes) const;
    // Gives the names the loop binds slots of its unit's frame from `first`: `loop`, the
    // target names, then the target names again for the filter (docs/design/0117-name-slots-plan.md)
    void BindSlots(SlotIndex first, UnitId unit);
    [[nodiscard]] bool HasSlots() const { return !m_firstSlot.IsDynamic(); }

private:
    void RenderLoop(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level);
    // A loop whose names live in a scope of their own: recursive loops, and loops in a
    // recursive one
    void RenderLoopInScopes(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level);
    // A loop whose names live in slots of its unit's frame (0117 P1)
    void RenderLoopInSlots(const InternalValue& loopVal, OutStream& os, RenderContext& values);
    ListAdapter CreateFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const;
    ListAdapter CreateSlottedFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const;

    AssignTarget m_target;
    NodeRef<Expression> m_value;
    NodeRef<Expression> m_ifExpr;
    bool m_isRecursive{};
    NodeRef<IRendererBase> m_mainBody;
    NodeRef<IRendererBase> m_elseBody;
    // Unique for the process, unlike the address: a reused loop frame keeps the names this
    // loop put in its scope (docs/tasks/0133)
    uint64_t m_loopId = NewLoopId();
    // The slots of the names the loop binds, from m_firstSlot, and their names; Dynamic for
    // a loop that keeps its names in scopes
    SlotIndex m_firstSlot;
    UnitId m_unit;
    ArenaSpan<SlotName> m_slotNames;

    static uint64_t NewLoopId();
};

class ElseBranchStatement;

class IfStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::IfStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expr);
        refs(m_mainBody);
        refs(m_elseBranches);
    }

    explicit IfStatement(NodeRef<Expression> expr)
        : m_expr(std::move(expr))
    {
    }

    void SetMainBody(NodeRef<IRendererBase> renderer)
    {
        m_mainBody = renderer;
    }

    void SetElseBranches(ArenaSpan<NodeRef<ElseBranchStatement>> branches)
    {
        m_elseBranches = branches;
    }

    void Render(OutStream& os, RenderContext& values) override;
private:
    NodeRef<Expression> m_expr;
    NodeRef<IRendererBase> m_mainBody;
    ArenaSpan<NodeRef<ElseBranchStatement>> m_elseBranches;
};

class ElseBranchStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::ElseBranchStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expr);
        refs(m_mainBody);
    }

    explicit ElseBranchStatement(NodeRef<Expression> expr)
        : m_expr(std::move(expr))
    {
    }

    bool ShouldRender(RenderContext& values) const;
    // A plain `else`, as opposed to an `elif`
    [[nodiscard]] bool IsElse() const { return !m_expr; }
    void SetMainBody(NodeRef<IRendererBase> renderer)
    {
        m_mainBody = renderer;
    }
    void Render(OutStream& os, RenderContext& values) override;

private:
    NodeRef<Expression> m_expr;
    NodeRef<IRendererBase> m_mainBody;
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
    static constexpr NodeKind Kind = NodeKind::SetLineStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expr);
    }

    SetLineStatement(AssignTarget target, NodeRef<Expression> expr)
        : SetStatement(std::move(target)), m_expr(std::move(expr))
    {
    }

    void Render(OutStream& os, RenderContext& values) override;
private:
    const NodeRef<Expression> m_expr;
};

class SetBlockStatement : public SetStatement
{
public:
    static bool MatchesKind(NodeKind kind) { return kind == NodeKind::SetRawBlockStmt || kind == NodeKind::SetFilteredBlockStmt; }
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_body);
    }

    using SetStatement::SetStatement;

    void SetBody(NodeRef<IRendererBase> renderer)
    {
        m_body = renderer;
    }
protected:
    InternalValue RenderBody(RenderContext&);

private:
    NodeRef<IRendererBase> m_body;
};

class SetRawBlockStatement final : public SetBlockStatement
{
public:
    static constexpr NodeKind Kind = NodeKind::SetRawBlockStmt;
    static bool MatchesKind(NodeKind kind) { return kind == Kind; }

    using SetBlockStatement::SetBlockStatement;

    void Render(OutStream&, RenderContext&) override;
};

class SetFilteredBlockStatement final : public SetBlockStatement
{
public:
    static constexpr NodeKind Kind = NodeKind::SetFilteredBlockStmt;
    static bool MatchesKind(NodeKind kind) { return kind == Kind; }
    // The arena calls each class's own: this one adds the filter to its base's handles
    void VisitRefs(detail::RefChecker& refs) const // NOLINT(bugprone-derived-method-shadowing-base-method)
    {
        SetBlockStatement::VisitRefs(refs);
        refs(m_expr);
    }

    explicit SetFilteredBlockStatement(AssignTarget target, NodeRef<ExpressionFilter> expr)
        : SetBlockStatement(std::move(target)), m_expr(std::move(expr))
    {
    }

    void Render(OutStream&, RenderContext&) override;

private:
    const NodeRef<ExpressionFilter> m_expr;
};

class BlockStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::BlockStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_mainBody);
    }

    BlockStatement(std::string name, bool isScoped, bool isRequired)
        : m_name(std::move(name))
        , m_isScoped(isScoped)
        , m_isRequired(isRequired)
    {
    }

    [[nodiscard]] auto& GetName() const { return m_name; }
    [[nodiscard]] bool IsRequired() const { return m_isRequired; }

    void SetMainBody(NodeRef<IRendererBase> renderer)
    {
        m_mainBody = renderer;
    }
    // Renders the block that overrides this one most (the top of the block stack), as Jinja2 does
    void Render(OutStream& os, RenderContext& values) override;
    // Renders this definition's own body; `super()` refers to the block at depth + 1
    void RenderBody(OutStream& os, RenderContext& values, size_t depth) const;
    // The slots a call of the body takes (0117 P1)
    void SetUnitLayout(UnitLayout layout) { m_unitLayout = layout; }

private:
    std::string m_name;
    bool m_isScoped{};
    bool m_isRequired{};
    NodeRef<IRendererBase> m_mainBody;
    UnitLayout m_unitLayout;
};

class ExtendsStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::ExtendsStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_templateExpr);
    }

    explicit ExtendsStatement(NodeRef<Expression> templateExpr)
        : m_templateExpr(std::move(templateExpr))
    {
    }

    // Like Jinja2, only loads the parent and remembers it: the parent is rendered when the
    // child template ends, and the child's own output after this point is dropped
    void Render(OutStream& os, RenderContext& values) override;

private:
    NodeRef<Expression> m_templateExpr;
};

class TemplateRenderer;

// Blocks of one template rendering, by name, the most derived first (Jinja2's
// context.blocks). A parent template appends its blocks when it is extended
struct BlocksStack
{
    // A block, in the template that defines it. The render keeps that template alive
    using Entry = TemplateNode<BlockStatement>;

    std::unordered_map<std::string, std::vector<Entry>> blocks;
};

// One template's code running, in an inheritance chain: Jinja2's root render function
struct TemplateFrame
{
    const BlocksStack* blocks = nullptr;
    // Set by `extends`; from then on this template's top-level output is dropped
    TemplateNode<TemplateRenderer> parent;
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
    static constexpr NodeKind Kind = NodeKind::TemplateRoot;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_body);
        refs.All(m_blocks);
    }

    using BlocksCollection = std::unordered_map<std::string, NodeRef<BlockStatement>>;

    TemplateRenderer() = default;

    void SetBody(NodeRef<ComposedRenderer> body) { m_body = body; }
    // False if a block of this name is defined already
    bool AddBlock(const std::string& name, NodeRef<BlockStatement> block)
    {
        return m_blocks.emplace(name, block).second;
    }
    void SetHasExtends() { m_hasExtends = true; }
    // The slots a render of the body takes (0117 P1)
    void SetUnitLayout(UnitLayout layout) { m_unitLayout = layout; }

    // Renders the template on its own (for Template::Render, include and import)
    void Render(OutStream& os, RenderContext& values) override;
    // Renders the template as the parent of the one rendering now: its blocks go below
    // the child's on the child's `stack`
    void RenderAsParent(OutStream& os, RenderContext& values, BlocksStack& stack);
    // Adds this template's blocks below the ones already on `stack`; `nodes` is its tree
    void PushBlocks(IRendererCallback& callback, const ArenaView& nodes, BlocksStack& stack) const;

private:
    // `stack` is null for a template that defines no blocks and extends nothing
    void RenderBody(OutStream& os, RenderContext& values, BlocksStack* stack);

    NodeRef<ComposedRenderer> m_body;
    BlocksCollection m_blocks;
    bool m_hasExtends = false;
    UnitLayout m_unitLayout;
};

class IncludeStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::IncludeStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expr);
    }

    IncludeStatement(bool ignoreMissing, bool withContext)
        : m_ignoreMissing(ignoreMissing)
        , m_withContext(withContext)
    {}

    void SetIncludeNamesExpr(NodeRef<Expression> expr)
    {
        m_expr = std::move(expr);
    }

    void Render(OutStream& os, RenderContext& values) override;
private:
    bool m_ignoreMissing{};
    bool m_withContext{};
    NodeRef<Expression> m_expr;
};

class ImportStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::ImportStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_nameExpr);
    }

    explicit ImportStatement(bool withContext)
        : m_withContext(withContext)
    {}

    void SetImportNameExpr(NodeRef<Expression> expr)
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
    NodeRef<Expression> m_nameExpr;
    std::optional<std::string> m_namespace;
    std::unordered_map<std::string, std::string> m_namesToImport;
};

class MacroStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::MacroStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs.All(m_params, [](detail::RefChecker& r, const MacroParam& param) { r(param.defaultValue); });
        refs(m_mainBody);
    }
    // A call block's caller is a macro too
    static bool MatchesKind(NodeKind kind) { return kind == NodeKind::MacroStmt || kind == NodeKind::MacroCallStmt; }

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

    void SetMainBody(NodeRef<IRendererBase> renderer)
    {
        m_mainBody = renderer;
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
    // The slots a call of the body takes (0117 P1)
    void SetUnitLayout(UnitLayout layout) { m_unitLayout = layout; }

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
    NodeRef<IRendererBase> m_mainBody;
    unsigned m_specialNames = 0;
    unsigned m_assignedNames = 0;
    std::shared_ptr<const InternalValueMap> m_attributes;
    UnitLayout m_unitLayout;
};

class MacroCallStatement : public MacroStatement
{
public:
    static constexpr NodeKind Kind = NodeKind::MacroCallStmt;
    static bool MatchesKind(NodeKind kind) { return kind == Kind; }
    // The arena calls each class's own: this one adds the call's arguments to its base's
    void VisitRefs(detail::RefChecker& refs) const // NOLINT(bugprone-derived-method-shadowing-base-method)
    {
        MacroStatement::VisitRefs(refs);
        VisitCallParams(refs, m_callParams);
    }

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
    static constexpr NodeKind Kind = NodeKind::DoStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expr);
    }

    explicit DoStatement(NodeRef<Expression> expr)
        : m_expr(std::move(expr)) {}

    void Render(OutStream& os, RenderContext& values) override;
private:
    NodeRef<Expression> m_expr;
};

// `{% trans %}` (Jinja2's i18n extension). Output renders the gettext call that Jinja2 makes of
// the block; its arguments refer to the variables of the block by VariableSlot(index), which
// Render sets in a scope of their own, so each variable is evaluated once, before the call.
class TransStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::TransStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs.All(m_variables);
        refs(m_output);
    }

    TransStatement(std::vector<std::pair<std::string, NodeRef<Expression>>> variables, NodeRef<IRendererBase> output)
        : m_variables(std::move(variables))
        , m_output(output)
    {
    }

    static std::string VariableSlot(size_t index) { return "$trans" + std::to_string(index); }

    void Render(OutStream& os, RenderContext& values) override;
private:
    std::vector<std::pair<std::string, NodeRef<Expression>>> m_variables;
    NodeRef<IRendererBase> m_output;
};

// `break` or `continue` (Jinja2's loopcontrols extension)
class LoopControlStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::LoopControlStmt;
    // NOLINTNEXTLINE(readability-convert-member-functions-to-static)
    void VisitRefs(detail::RefChecker& /*refs*/) const {}

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
    static constexpr NodeKind Kind = NodeKind::WithStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs.All(m_scopeVars);
        refs(m_mainBody);
    }

    void SetScopeVars(std::vector<std::pair<std::string, NodeRef<Expression>>> vars)
    {
        m_scopeVars = std::move(vars);
    }
    void SetMainBody(NodeRef<IRendererBase> renderer)
    {
        m_mainBody = renderer;
    }

    void Render(OutStream& os, RenderContext& values) override;
private:
    std::vector<std::pair<std::string, NodeRef<Expression>>> m_scopeVars;
    NodeRef<IRendererBase> m_mainBody;
};

class FilterStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::FilterStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expr);
        refs(m_body);
    }

    explicit FilterStatement(NodeRef<ExpressionFilter> expr)
        : m_expr(expr) {}

    void SetBody(NodeRef<IRendererBase> renderer)
    {
        m_body = renderer;
    }

    void Render(OutStream&, RenderContext&) override;
private:
    NodeRef<ExpressionFilter> m_expr;
    NodeRef<IRendererBase> m_body;
};

// {% autoescape expr %}: turns output escaping on or off for its body, in a new scope
class AutoescapeStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::AutoescapeStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_expr);
        refs(m_body);
    }

    explicit AutoescapeStatement(NodeRef<Expression> expr)
        : m_expr(std::move(expr))
    {
    }

    void SetBody(NodeRef<IRendererBase> renderer) { m_body = renderer; }

    void Render(OutStream&, RenderContext&) override;

private:
    NodeRef<Expression> m_expr;
    NodeRef<IRendererBase> m_body;
};

} // namespace jinja2

#endif // JINJA2CPP_SRC_STATEMENTS_H
