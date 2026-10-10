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
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace jinja2
{
class Statement : public IRendererBase
{
public:
    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;
    Statement& operator=(Statement&&) = delete;

protected:
    Statement() = default;
    // Only the arena moves a node, when it seals the tree into one buffer
    Statement(Statement&&) = default;
    ~Statement() = default;
};

template<typename CharT>
class TemplateImpl;

// A macro argument as the parser reads it
struct MacroParamInfo
{
    std::string paramName;
    NodeRef<Expression> defaultValue;
    // The default names an argument of the macro, so it is evaluated per call in the
    // macro scope; other defaults are evaluated where the macro is defined
    bool defaultRefersToArgs = false;
};

using MacroParamsInfo = std::vector<MacroParamInfo>;

// A macro argument the tree keeps
struct MacroParam
{
    ArenaText name;
    std::size_t hash = 0;
    NodeRef<Expression> defaultValue;
    bool defaultRefersToArgs = false;
};

// A name `from ... import` takes, and the name it gets: `name as alias`
struct ImportName
{
    ArenaText name;
    std::size_t nameHash = 0;
    ArenaText alias;
    std::size_t aliasHash = 0;
};

// A name a statement assigns and the expression of its value: `with a = 1`
struct NamedExpr
{
    ArenaText name;
    std::size_t hash = 0;
    NodeRef<Expression> value;
};

// One node of the target of `for` and `set`: a name, a tuple of targets (`a, (b, c)`) or,
// for `set`, a namespace attribute (`ns.attr`)
struct TargetNode
{
    // The name, or the namespace of `ns.attr`; empty for a tuple
    ArenaText name;
    // Set on a namespace attribute target: the attribute of the namespace `name`
    ArenaText attr;
    std::size_t hash = 0;
    // A tuple's items
    std::uint32_t count = 0;
    // The nodes of this target, itself and its items' included
    std::uint32_t size = 1;
    // A plain name's place among the distinct names of the whole target, from 1 (a loop's
    // slots start with `loop`)
    SlotIndex slot;
    bool isTuple = false;

    [[nodiscard]] bool IsPlainName() const { return !isTuple && attr.empty(); }
};

// A target, flattened in pre-order: the root first, then each item with its own items
// right after it
using AssignTarget = ArenaSpan<TargetNode>;

namespace detail
{
inline void VisitTargetRefs(RefChecker& refs, AssignTarget target)
{
    refs(target, [](RefChecker& checker, const TargetNode& node) {
        checker(node.name);
        checker(node.attr);
    });
}
inline void VisitSlotNameRefs(RefChecker& refs, ArenaSpan<SlotName> names)
{
    refs(names, [](RefChecker& checker, const SlotName& name) { checker(name.name); });
}
} // namespace detail

class ForStatement final : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::ForStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_value);
        refs(m_ifExpr);
        refs(m_mainBody);
        refs(m_elseBody);
        detail::VisitTargetRefs(refs, m_target);
        detail::VisitSlotNameRefs(refs, m_slotNames);
    }

    ForStatement(AssignTarget target, NodeRef<Expression> expr, NodeRef<Expression> ifExpr, bool isRecursive)
        : m_target(target)
        , m_value(expr)
        , m_ifExpr(ifExpr)
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
    // Gives the names the loop binds slots of its unit's frame from `first`: `loop`, the
    // target names, then the target names again for the filter (docs/design/0117-name-slots-plan.md)
    void BindSlots(SlotIndex first, UnitId unit);
    [[nodiscard]] bool HasSlots() const { return !m_firstSlot.IsDynamic(); }
    // As FilterInFrame, from wherever the loop's body peeks at its next item (`loop.last`
    // read in the body or a macro it calls): the loop's frame and tree are found again
    bool FetchFiltered(ListAccessorEnumeratorPtr& items, InternalValue& item, RenderContext& values, FrameHandle handle, const ArenaView& nodes) const;

private:
    void RenderLoop(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level);
    // A loop whose names live in a scope of their own: recursive loops, and loops in a
    // recursive one
    void RenderLoopInScopes(const InternalValue& loopVal, OutStream& os, RenderContext& values, int level);
    // A loop whose names live in slots of its unit's frame (0117 P1)
    void RenderLoopInSlots(const InternalValue& loopVal, OutStream& os, RenderContext& values);
    ListAdapter CreateFilteredAdapter(const ListAdapter& loopItems, RenderContext& values) const;
    // Moves `items` to the next item the filter keeps and stores it in `item`; false at the
    // end. Runs in the loop's own frame and tree, installed
    bool FilterInFrame(IListAccessorEnumerator& items, boost::span<Slot> frameSlots, InternalValue& item, RenderContext& values) const;

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

class IfStatement final : public Statement
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

class ElseBranchStatement final : public Statement
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
    void VisitRefs(detail::RefChecker& refs) const
    {
        detail::VisitTargetRefs(refs, m_target);
    }

    explicit SetStatement(AssignTarget target)
        : m_target(target)
    {
    }
    SetStatement(const SetStatement&) = delete;
    // For the arena, which moves the nodes when it seals the tree
    SetStatement(SetStatement&&) = default;
    SetStatement& operator=(const SetStatement&) = delete;
    SetStatement& operator=(SetStatement&&) = delete;

protected:
    ~SetStatement() = default;
    [[nodiscard]] AssignTarget GetTarget() const { return m_target; }

private:
    const AssignTarget m_target;
};

class SetLineStatement final : public SetStatement
{
public:
    static constexpr NodeKind Kind = NodeKind::SetLineStmt;
    // The arena calls each class's own: this one adds the value to its base's handles
    void VisitRefs(detail::RefChecker& refs) const // NOLINT(bugprone-derived-method-shadowing-base-method)
    {
        SetStatement::VisitRefs(refs);
        refs(m_expr);
    }

    SetLineStatement(AssignTarget target, NodeRef<Expression> expr)
        : SetStatement(target), m_expr(expr)
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
    // The arena calls each class's own: this one adds the body to its base's handles
    void VisitRefs(detail::RefChecker& refs) const // NOLINT(bugprone-derived-method-shadowing-base-method)
    {
        SetStatement::VisitRefs(refs);
        refs(m_body);
    }

    using SetStatement::SetStatement;
    SetBlockStatement(const SetBlockStatement&) = delete;
    // For the arena, which moves the nodes when it seals the tree
    SetBlockStatement(SetBlockStatement&&) = default;
    SetBlockStatement& operator=(const SetBlockStatement&) = delete;
    SetBlockStatement& operator=(SetBlockStatement&&) = delete;

    void SetBody(NodeRef<IRendererBase> renderer)
    {
        m_body = renderer;
    }
protected:
    ~SetBlockStatement() = default;
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
        : SetBlockStatement(target), m_expr(expr)
    {
    }

    void Render(OutStream&, RenderContext&) override;

private:
    const NodeRef<ExpressionFilter> m_expr;
};

class BlockStatement final : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::BlockStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_name);
        refs(m_mainBody);
    }

    BlockStatement(ArenaText name, bool isScoped, bool isRequired)
        : m_name(name)
        , m_isScoped(isScoped)
        , m_isRequired(isRequired)
    {
    }

    // `nodes` is the block's tree
    template<typename Nodes>
    [[nodiscard]] std::string_view GetName(const Nodes& nodes) const
    {
        return nodes.Text(m_name);
    }
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
    ArenaText m_name;
    bool m_isScoped{};
    bool m_isRequired{};
    NodeRef<IRendererBase> m_mainBody;
    UnitLayout m_unitLayout;
};

class ExtendsStatement final : public Statement
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

    // The names are the blocks' own, in the trees the render keeps alive: a stack must not
    // outlive the render that made it, and what may (`self`) copies them
    std::unordered_map<std::string_view, std::vector<Entry>> blocks;
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
class TemplateRenderer final : public IRendererBase
{
public:
    static constexpr NodeKind Kind = NodeKind::TemplateRoot;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_body);
        refs(m_blocks);
    }

    TemplateRenderer() = default;

    void SetBody(NodeRef<ComposedRenderer> body) { m_body = body; }
    // The blocks the template defines, each name once
    void SetBlocks(ArenaSpan<NodeRef<BlockStatement>> blocks) { m_blocks = blocks; }
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
    ArenaSpan<NodeRef<BlockStatement>> m_blocks;
    bool m_hasExtends = false;
    UnitLayout m_unitLayout;
};

class IncludeStatement final : public Statement
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

class ImportStatement final : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::ImportStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_nameExpr);
        refs(m_namespace);
        refs(m_namesToImport, [](detail::RefChecker& checker, const ImportName& name) {
            checker(name.name);
            checker(name.alias);
        });
    }

    explicit ImportStatement(bool withContext)
        : m_withContext(withContext)
    {}

    void SetImportNameExpr(NodeRef<Expression> expr)
    {
        m_nameExpr = std::move(expr);
    }

    // `import 'm' as namespace`
    void SetNamespace(ArenaText name, std::size_t hash)
    {
        m_namespace = name;
        m_namespaceHash = hash;
    }

    // `from 'm' import ...`: each name once, in the order they are written
    void SetNamesToImport(ArenaSpan<ImportName> names) { m_namesToImport = names; }

    void Render(OutStream& os, RenderContext& values) override;
private:
    void ImportNames(RenderContext& values, const ArenaView& nodes, const InternalValueMap& importedScope, const std::string& scopeName) const;

    bool m_withContext{};
    NodeRef<Expression> m_nameExpr;
    // Empty for `from ... import`
    ArenaText m_namespace;
    std::size_t m_namespaceHash = 0;
    ArenaSpan<ImportName> m_namesToImport;
};

class MacroStatement : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::MacroStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_name);
        refs(m_params, [](detail::RefChecker& r, const MacroParam& param) {
            r(param.name);
            r(param.defaultValue);
        });
        refs(m_mainBody);
        detail::VisitSlotNameRefs(refs, m_slotNames);
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

    // The name and the arguments go to the tree; a call block's caller has no name
    MacroStatement(NodeArena& nodes, std::string_view name, const MacroParamsInfo& params);
    MacroStatement(const MacroStatement&) = delete;
    // For the arena, which moves the nodes when it seals the tree
    MacroStatement(MacroStatement&&) = default;
    MacroStatement& operator=(const MacroStatement&) = delete;
    MacroStatement& operator=(MacroStatement&&) = delete;
    // Virtual while it owns its attributes: MacroCallStatement derives from it
    virtual ~MacroStatement() = default;

    void SetMainBody(NodeRef<IRendererBase> renderer)
    {
        m_mainBody = renderer;
        // A declared argument named like a special name is an ordinary argument
        m_caughtNames = m_specialNames & ~m_declaredNames;
        CompleteAttributes();
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
    [[nodiscard]] bool HasInvalidCallerParam() const { return (m_specialNames & UsesCaller) != 0 && m_callerWithoutDefault; }

    void Render(OutStream& os, RenderContext& values) override;
    // The slots a call of the body takes (0117 P1)
    void SetUnitLayout(UnitLayout layout) { m_unitLayout = layout; }
    // Makes the list of the names a call binds: the arguments in order, then the special
    // names the body catches (caller, kwargs, varargs). Called once the body is parsed
    ArenaSpan<SlotName> MakeBinderNames(NodeArena& nodes) const;
    // Binds `names`, from MakeBinderNames, to the first slots of the frame of each call
    // (docs/design/0117-name-slots-plan.md, phase P2)
    ArenaSpan<SlotName> BindSlots(ArenaSpan<SlotName> names)
    {
        m_slotNames = names;
        return names;
    }

protected:
    Callable MakeCallable(RenderContext& values) const;
    void InvokeMacroRenderer(const std::vector<InternalValue>& definedDefaults, const CallParams& callParams, OutStream& stream, RenderContext& context) const;
    // Adds the attributes that depend on the body to the ones the constructor made
    void CompleteAttributes();
    // The macro's name in messages: none for the caller of a call block
    template<typename Nodes>
    [[nodiscard]] std::string GetDisplayName(const Nodes& nodes) const
    {
        return m_name.empty() ? "None" : "'" + std::string(nodes.Text(m_name)) + "'";
    }

    ArenaText m_name;
    std::size_t m_nameHash = 0;
    ArenaSpan<MacroParam> m_params;
    NodeRef<IRendererBase> m_mainBody;
    unsigned m_specialNames = 0;
    unsigned m_assignedNames = 0;
    // The special names declared as arguments
    unsigned m_declaredNames = 0;
    // The special names bound when the macro is called, once the body is parsed
    unsigned m_caughtNames = 0;
    // A `caller` argument without a default
    bool m_callerWithoutDefault = false;
    // `macro.name`, `macro.arguments` and the others, complete once the body is parsed
    std::shared_ptr<InternalValueMap> m_attributes;
    UnitLayout m_unitLayout;
    // The names bound in the first slots of the frame; empty for a macro whose arguments
    // live in its scope. They share the texts of m_params
    ArenaSpan<SlotName> m_slotNames;
};

class MacroCallStatement final : public MacroStatement
{
public:
    static constexpr NodeKind Kind = NodeKind::MacroCallStmt;
    static bool MatchesKind(NodeKind kind) { return kind == Kind; }
    // The arena calls each class's own: this one adds the call's arguments to its base's
    void VisitRefs(detail::RefChecker& refs) const // NOLINT(bugprone-derived-method-shadowing-base-method)
    {
        MacroStatement::VisitRefs(refs);
        refs(m_macroName);
        m_callParams.VisitRefs(refs);
    }

    MacroCallStatement(NodeArena& nodes, std::string_view macroName, const CallParamsInfo& callParams, const MacroParamsInfo& callbackParams)
        : MacroStatement(nodes, {}, callbackParams)
        , m_macroName(nodes.MakeText(macroName))
        , m_macroNameHash(HashedName::Hash(macroName))
        , m_callParams(ArenaCallParams::Make(nodes, callParams))
    {
    }

    void Render(OutStream& os, RenderContext& values) override;

private:
    ArenaText m_macroName;
    std::size_t m_macroNameHash = 0;
    ArenaCallParams m_callParams;
};

class DoStatement final : public Statement
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
class TransStatement final : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::TransStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_variables);
        refs(m_output);
    }

    // The gettext call already names the variables: the statement keeps their values only
    TransStatement(ArenaSpan<NodeRef<Expression>> variables, NodeRef<IRendererBase> output)
        : m_variables(variables)
        , m_output(output)
    {
    }

    static std::string VariableSlot(size_t index) { return "$trans" + std::to_string(index); }

    void Render(OutStream& os, RenderContext& values) override;
private:
    ArenaSpan<NodeRef<Expression>> m_variables;
    NodeRef<IRendererBase> m_output;
};

// `break` or `continue` (Jinja2's loopcontrols extension)
class LoopControlStatement final : public Statement
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

class WithStatement final : public Statement
{
public:
    static constexpr NodeKind Kind = NodeKind::WithStmt;
    void VisitRefs(detail::RefChecker& refs) const
    {
        refs(m_scopeVars, [](detail::RefChecker& checker, const NamedExpr& var) {
            checker(var.name);
            checker(var.value);
        });
        refs(m_mainBody);
    }

    // Assigned in this order, so that the last of two equal names wins
    void SetScopeVars(ArenaSpan<NamedExpr> vars) { m_scopeVars = vars; }
    void SetMainBody(NodeRef<IRendererBase> renderer)
    {
        m_mainBody = renderer;
    }

    void Render(OutStream& os, RenderContext& values) override;
private:
    ArenaSpan<NamedExpr> m_scopeVars;
    NodeRef<IRendererBase> m_mainBody;
};

class FilterStatement final : public Statement
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
class AutoescapeStatement final : public Statement
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
