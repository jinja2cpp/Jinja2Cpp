#include "../src/expression_evaluator.h"
#include "../src/internal_value.h"
#include "../src/node_arena.h"
#include "../src/renderer.h"
#include "../src/statements.h"
#include "../src/template_slots.h"

#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace jinja2;

namespace jinja2::detail
{
// Handles from raw offsets and the bytes of a sealed tree, to corrupt a tree on purpose
struct ArenaTestAccess
{
    template<typename T>
    static NodeRef<T> Ref(std::uint32_t offset)
    {
        return NodeRef<T>(offset);
    }
    template<typename T>
    static std::uint32_t Offset(NodeRef<T> ref)
    {
        return ref.m_offset;
    }
    template<typename T>
    static std::uint32_t Offset(ArenaSpan<T> list)
    {
        return list.m_offset;
    }
    template<typename T>
    static ArenaSpan<T> Span(std::uint32_t offset, std::uint32_t size)
    {
        return ArenaSpan<T>(offset, size);
    }
    static std::byte* Bytes(const SealedArena& tree) { return tree.m_buffer.get(); }
};
} // namespace jinja2::detail

namespace
{
using Access = jinja2::detail::ArenaTestAccess;
} // namespace

// The handles of a parse tree (docs/design/0118-parse-tree-arena-plan.md, phase P3)

// The node classes' vtables are not exported from a shared library, so the tests that make
// nodes run against the static one only
#ifndef JINJA2CPP_LINK_AS_SHARED
TEST(NodeArenaTest, MakeRecordsTheKind)
{
    NodeArena nodes;
    auto constant = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 5 }));
    NodeRef<Expression> expr = constant;
    EXPECT_EQ(NodeKind::ConstantExpr, nodes[expr].GetKind());
    EXPECT_EQ(5, *GetIf<int64_t>(nodes[expr].GetConstant(nodes)));
}

TEST(NodeArenaTest, DowncastChecksTheKind)
{
    NodeArena nodes;
    NodeRef<Expression> name = nodes.Make<ValueRefExpression>("x");
    NodeRef<Expression> self = nodes.Make<SelfRefExpression>();
    NodeRef<Expression> constant = nodes.Make<ConstantExpression>(InternalValue());

    // `self` is a variable too, but a variable is not `self`
    EXPECT_TRUE(nodes.Is<ValueRefExpression>(name));
    EXPECT_TRUE(nodes.Is<ValueRefExpression>(self));
    EXPECT_TRUE(nodes.Is<SelfRefExpression>(self));
    EXPECT_FALSE(nodes.Is<SelfRefExpression>(name));
    EXPECT_FALSE(nodes.Is<ValueRefExpression>(constant));

    EXPECT_TRUE(nodes.As<ConstantExpression>(constant));
    EXPECT_FALSE(nodes.As<ConstantExpression>(name));
    EXPECT_FALSE(nodes.As<ConstantExpression>(NodeRef<Expression>()));
    EXPECT_EQ("x", nodes.Get<ValueRefExpression>(name).GetName());
}

TEST(NodeArenaTest, DowncastToAClassWithSubclasses)
{
    NodeArena nodes;
    NodeRef<IRendererBase> macro = nodes.Make<MacroStatement>("m", MacroParams());
    NodeRef<IRendererBase> call = nodes.Make<MacroCallStatement>("m", CallParamsInfo(), MacroParams());
    NodeRef<IRendererBase> rawSet = nodes.Make<SetRawBlockStatement>(AssignTarget{ "x", {}, false, {}, {} });

    // A call block's caller is a macro; a macro is not a call block
    EXPECT_TRUE(nodes.Is<MacroStatement>(macro));
    EXPECT_TRUE(nodes.Is<MacroStatement>(call));
    EXPECT_TRUE(nodes.Is<MacroCallStatement>(call));
    EXPECT_FALSE(nodes.Is<MacroCallStatement>(macro));

    EXPECT_TRUE(nodes.Is<SetBlockStatement>(rawSet));
    EXPECT_FALSE(nodes.Is<SetFilteredBlockStatement>(rawSet));
    EXPECT_FALSE(nodes.Is<SetBlockStatement>(macro));
}

TEST(NodeArenaTest, SpanKeepsACopyOfTheItems)
{
    NodeArena nodes;
    std::vector<NodeRef<Expression>> items{ nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 })),
                                            nodes.Make<ConstantExpression>(InternalValue(int64_t{ 2 })) };
    auto span = nodes.MakeSpan(items);
    items.clear();

    ASSERT_EQ(2u, span.size());
    const auto view = nodes[span];
    EXPECT_EQ(1, *GetIf<int64_t>(nodes[view[0]].GetConstant(nodes)));
    EXPECT_EQ(2, *GetIf<int64_t>(nodes[view[1]].GetConstant(nodes)));
    EXPECT_TRUE(nodes.MakeSpan(items).empty());
}

TEST(NodeArenaTest, SealingKeepsTheHandles)
{
    NodeArena nodes;
    std::vector<NodeRef<Expression>> items;
    // Enough nodes and lists to fill several blocks
    for (int64_t n = 0; n != 2000; ++n)
    {
        items.emplace_back(nodes.Make<ConstantExpression>(InternalValue(std::string(40, static_cast<char>('a' + (n % 26))) + std::to_string(n))));
    }
    const auto span = nodes.MakeSpan(items);

    // The test reads the nodes through the list: no node holds it
    SealedArena sealed = nodes.Seal(span);
    SealedArena moved(std::move(sealed));
    SealedArena assigned;
    assigned = std::move(moved);

    const auto view = assigned.View();
    const auto list = view[span];
    ASSERT_EQ(2000u, list.size());
    EXPECT_EQ(std::string(40, 'x') + "1999", AsString(*view[list[1999]].GetConstant(nodes)));
    EXPECT_EQ(std::string(40, 'a') + "0", AsString(*view[items[0]].GetConstant(nodes)));
}

// The sealed tree has no slack: it is as large as the objects in it
TEST(NodeArenaTest, SealedTreeIsExact)
{
    NodeArena nodes;
    auto first = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    auto second = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 2 }));
    const NodeRef<Expression> exprs[] = { first, second };
    const auto span = nodes.MakeSpan(boost::span<const NodeRef<Expression>>(exprs));
    const SealedArena sealed = nodes.Seal(span);

    const auto view = sealed.View();
    EXPECT_EQ(2, *GetIf<int64_t>(view[view[span][1]].GetConstant(nodes)));
    EXPECT_EQ(NodeKind::ConstantExpr, view[first].GetKind());
}

// A template whose nodes fill several blocks: Seal moves every node out of them
TEST(NodeArenaTest, TemplateOfSeveralBlocksRendersAfterSealing)
{
    std::string source;
    std::string expected;
    for (int n = 0; n != 200; ++n)
    {
        source += "{% for k in ['k" + std::to_string(n) + "'] %}{{ k ~ x }}{% endfor %},";
        expected += "k" + std::to_string(n) + "!,";
    }
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    EXPECT_EQ(expected, tpl.RenderAsString({ { "x", "!" } }).value());
}

// A loop's names point into its targets, wherever the node is: the `set` makes `k` a
// lookup by name, which finds the loop's `k` by those names
TEST(NodeArenaTest, LoopNamesFollowTheLoop)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{% for k, v in d|dictsort %}{% set k = k ~ v %}{{ k }};{% endfor %}"));
    EXPECT_EQ("a1;b2;", tpl.RenderAsString({ { "d", jinja2::ValuesMap{ { "a", 1 }, { "b", 2 } } } }).value());
}

// A render links to the trees of the templates it runs by slot (phase P4b)
TEST(NodeArenaTest, TemplateSlotsOfOneRender)
{
    NodeArena first;
    first.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    NodeArena second;
    second.Make<ConstantExpression>(InternalValue(int64_t{ 2 }));
    const SealedArena firstTree = first.Seal();
    const SealedArena secondTree = second.Seal();

    TemplateSlots templates;
    const auto a = templates.Add(firstTree.View());
    const auto b = templates.Add(secondTree.View());
    EXPECT_NE(a.Slot(), b.Slot());
    EXPECT_TRUE(templates[a] == firstTree.View());
    EXPECT_TRUE(templates[b] == secondTree.View());
    // A template takes one slot however often the render asks for it
    const auto again = templates.Add(firstTree.View());
    EXPECT_EQ(a.Slot(), again.Slot());
    EXPECT_EQ(a.Generation(), again.Generation());
}

// Past a few templates the table finds a tree by hash, still one slot each
TEST(NodeArenaTest, TemplateSlotsOfManyTemplates)
{
    std::vector<SealedArena> trees;
    trees.reserve(20);
    for (int64_t n = 0; n != 20; ++n)
    {
        NodeArena nodes;
        nodes.Make<ConstantExpression>(InternalValue(n));
        trees.push_back(nodes.Seal());
    }

    TemplateSlots templates;
    std::vector<TemplateHandle> handles;
    handles.reserve(trees.size());
    for (const auto& tree : trees)
    {
        handles.push_back(templates.Add(tree.View()));
    }
    for (std::size_t n = 0; n != trees.size(); ++n)
    {
        EXPECT_EQ(n, handles[n].Slot());
        EXPECT_EQ(n, templates.Add(trees[n].View()).Slot());
        EXPECT_TRUE(templates[handles[n]] == trees[n].View());
    }
}

// A handle of another render's table, or one no table made, resolves nowhere: its template
// may be gone
TEST(NodeArenaTest, TemplateSlotsRejectAnotherRendersHandle)
{
    NodeArena nodes;
    nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    const SealedArena tree = nodes.Seal();

    TemplateSlots render1;
    const auto handle = render1.Add(tree.View());
    TemplateSlots render2;
    EXPECT_THROW((void)render2[handle], std::logic_error);
    const TemplateHandle pastTheEnd(handle.Slot() + 1, handle.Generation());
    EXPECT_THROW((void)render1[pastTheEnd], std::logic_error);
    // The generation tells the renders apart unless the checks are OFF
    render2.Add(tree.View());
    if constexpr (NodeRefChecks >= 1)
    {
        EXPECT_THROW((void)render2[handle], std::logic_error);
        EXPECT_THROW((void)render1[TemplateHandle()], std::logic_error);
    }
}

namespace
{
using IExpressionFilter = ExpressionFilter::IExpressionFilter;

// Counts the filters alive, wherever the arena moved them
struct FilterCount
{
    int alive = 0;
};

class AliveMark
{
public:
    explicit AliveMark(FilterCount& count)
        : m_count(&count)
    {
        ++m_count->alive;
    }
    AliveMark(const AliveMark& other)
        : m_count(other.m_count)
    {
        ++m_count->alive;
    }
    AliveMark& operator=(const AliveMark&) = delete;
    ~AliveMark() { --m_count->alive; }

private:
    FilterCount* m_count;
};

class CountingFilter : public IExpressionFilter
{
public:
    CountingFilter(FilterCount& count, std::string text)
        : m_mark(count)
        , m_text(std::move(text))
    {
    }

    InternalValue Filter(const InternalValue& /*baseVal*/, RenderContext& /*context*/) override { return InternalValue(); }
    [[nodiscard]] std::string GetArgumentsError() const override { return m_text; }
    void VisitRefs(jinja2::detail::RefChecker& /*refs*/) const override {}

private:
    AliveMark m_mark;
    std::string m_text;
};

// A base before the interface, so that the interface does not start the object
struct LeadingBase
{
    LeadingBase() = default;
    LeadingBase(const LeadingBase&) = default;
    LeadingBase(LeadingBase&&) = default;
    LeadingBase& operator=(const LeadingBase&) = default;
    LeadingBase& operator=(LeadingBase&&) = default;
    virtual ~LeadingBase() = default;
    std::int64_t padding[3] = {};
};

class OffsetFilter
    : public LeadingBase
    , public CountingFilter
{
public:
    using CountingFilter::CountingFilter;
};

std::string LongText(int n)
{
    // Longer than a short string, so that a move that copied the bytes would show
    return "a filter's text, past the short string buffer, number " + std::to_string(n);
}
} // namespace

// Filters and tests are arena nodes that move themselves at Seal (phase P4c)
TEST(NodeArenaTest, FilterObjectsLiveInTheArena)
{
    constexpr int count = 300;
    FilterCount filters;
    for (const bool seal : { false, true })
    {
        {
            NodeArena nodes;
            std::vector<NodeRef<IExpressionFilter>> refs;
            refs.reserve(count);
            for (int n = 0; n != count; ++n)
            {
                refs.push_back(n % 2 ? nodes.MakeObject<IExpressionFilter, CountingFilter>(filters, LongText(n))
                                     : nodes.MakeObject<IExpressionFilter, OffsetFilter>(filters, LongText(n)));
                // Other nodes between them, so that they fill several blocks
                nodes.Make<ConstantExpression>(InternalValue(int64_t{ n }));
            }
            ASSERT_EQ(count, filters.alive);
            EXPECT_EQ(NodeKind::FilterObject, nodes[refs[0]].GetKind());
            EXPECT_EQ(LongText(0), nodes[refs[0]].GetArgumentsError());
            if (seal)
            {
                SealedArena sealed = nodes.Seal(nodes.MakeSpan(refs));
                SealedArena moved(std::move(sealed));
                const auto view = moved.View();
                EXPECT_EQ(count, filters.alive);
                for (int n = 0; n != count; ++n)
                {
                    EXPECT_EQ(LongText(n), view[refs[static_cast<std::size_t>(n)]].GetArgumentsError());
                }
                EXPECT_EQ(NodeKind::FilterObject, view[refs[1]].GetKind());
            }
        }
        EXPECT_EQ(0, filters.alive);
    }
}

// A filter's state prepared at Load moves with it: `format` parses a literal format once
TEST(NodeArenaTest, FilterKeepsItsStateAcrossSeal)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{{ '%s=%d' | format(x, 2) }};{{ 'v' | default(x) | upper }};{{ x is equalto 'k' }}"));
    EXPECT_EQ("k=2;V;True", tpl.RenderAsString({ { "x", "k" } }).value());
}

// A link into another template's tree is checked where it is followed, unless the checks
// are OFF: its own template's Seal cannot check it (phase P4c)
TEST(NodeArenaTest, ResolveChecksTemplateNodes)
{
    NodeArena nodes;
    const NodeRef<Expression> constant = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 7 }));
    const NodeRef<IRendererBase> text = nodes.Make<RawTextRenderer>("text", std::size_t{ 4 });
    const SealedArena tree = nodes.Seal(constant, text);
    TemplateSlots templates;
    const auto handle = templates.Add(tree.View());

    const auto found = templates.Resolve(TemplateNode<Expression>{ handle, constant });
    EXPECT_TRUE(found.nodes == tree.View());
    EXPECT_EQ(7, *GetIf<int64_t>(found.node.GetConstant(nodes)));
    EXPECT_EQ(NodeKind::RawText, templates.Resolve(TemplateNode<IRendererBase>{ handle, text }).node.GetKind());

    if constexpr (NodeRefChecks >= 1)
    {
        // A node of another family, the middle of a node, past the end, no node at all
        const auto wrongFamily = Access::Ref<IRendererBase>(Access::Offset(constant));
        EXPECT_THROW((void)templates.Resolve(TemplateNode<IRendererBase>{ handle, wrongFamily }), InvalidNodeRef);
        const auto inside = Access::Ref<Expression>(Access::Offset(constant) + 8);
        EXPECT_THROW((void)templates.Resolve(TemplateNode<Expression>{ handle, inside }), InvalidNodeRef);
        const auto pastTheEnd = Access::Ref<Expression>(1U << 20);
        EXPECT_THROW((void)templates.Resolve(TemplateNode<Expression>{ handle, pastTheEnd }), InvalidNodeRef);
        EXPECT_THROW((void)templates.Resolve(TemplateNode<Expression>{ handle, NodeRef<Expression>() }), InvalidNodeRef);
    }
}

// With FULL checks every access checks its handle
TEST(NodeArenaTest, FullChecksEveryAccess)
{
    if constexpr (NodeRefChecks < 2)
    {
        GTEST_SKIP() << "node reference checks below FULL";
    }
    NodeArena nodes;
    std::vector<NodeRef<Expression>> items{ nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 })),
                                            nodes.Make<ConstantExpression>(InternalValue(int64_t{ 2 })) };
    const auto span = nodes.MakeSpan(items);
    const NodeRef<IRendererBase> text = nodes.Make<RawTextRenderer>("text", std::size_t{ 4 });
    EXPECT_THROW((void)nodes[Access::Ref<Expression>(Access::Offset(text))], InvalidNodeRef);
    const SealedArena tree = nodes.Seal(span, text);
    const auto view = tree.View();

    EXPECT_THROW((void)view[Access::Ref<Expression>(Access::Offset(text))], InvalidNodeRef);
    EXPECT_THROW((void)view[Access::Ref<Expression>(1U << 20)], InvalidNodeRef);
    EXPECT_THROW((void)view[Access::Ref<Expression>(Access::Offset(items[0]) + 4)], InvalidNodeRef);
    EXPECT_THROW((void)view[Access::Span<NodeRef<Expression>>(Access::Offset(span), 1U << 20)], InvalidNodeRef);

    // A list item that points into nowhere: the access that follows it throws
    const std::uint32_t bad = 1U << 20;
    std::memcpy(Access::Bytes(tree) + Access::Offset(span), &bad, sizeof(bad));
    const auto list = view[span];
    EXPECT_THROW((void)view[list[0]], InvalidNodeRef);
    EXPECT_EQ(2, *GetIf<int64_t>(view[list[1]].GetConstant(nodes)));
}

namespace
{
// A filter that holds a handle, as `map` holds its arguments
class RefHoldingFilter : public IExpressionFilter
{
public:
    explicit RefHoldingFilter(NodeRef<Expression> ref)
        : m_ref(ref)
    {
    }

    InternalValue Filter(const InternalValue& /*baseVal*/, RenderContext& /*context*/) override { return InternalValue(); }
    void VisitRefs(jinja2::detail::RefChecker& refs) const override { refs(m_ref); }

private:
    NodeRef<Expression> m_ref;
};

// Seals a tree whose `bad` node holds a handle that is not a node of its type
template<typename Make>
void ExpectSealRejects(const Make& makeBad)
{
    NodeArena nodes;
    const auto good = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    const auto text = nodes.Make<RawTextRenderer>("text", std::size_t{ 4 });
    makeBad(nodes, good, text);
    if constexpr (NodeRefChecks >= 1)
    {
        EXPECT_THROW((void)nodes.Seal(good), InvalidNodeRef);
    }
    else
    {
        EXPECT_NO_THROW((void)nodes.Seal(good));
    }
}
} // namespace

// Seal checks every handle a node holds, unless the checks are OFF
TEST(NodeArenaTest, SealRejectsOutOfRangeRef)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> /*good*/, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, Access::Ref<Expression>(1U << 20));
    });
}

TEST(NodeArenaTest, SealRejectsRefIntoANode)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, Access::Ref<Expression>(Access::Offset(good) + 8));
    });
}

TEST(NodeArenaTest, SealRejectsRefOfTheWrongFamily)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> /*good*/, NodeRef<RawTextRenderer> text) {
        nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, Access::Ref<Expression>(Access::Offset(text)));
    });
}

TEST(NodeArenaTest, SealRejectsSpanOutOfRange)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> good, NodeRef<RawTextRenderer> /*text*/) {
        nodes.Make<TupleCreator>(Access::Span<NodeRef<Expression>>(Access::Offset(good), 1U << 20));
    });
}

TEST(NodeArenaTest, SealRejectsCorruptRefInAFilterObject)
{
    ExpectSealRejects([](NodeArena& nodes, NodeRef<ConstantExpression> /*good*/, NodeRef<RawTextRenderer> text) {
        nodes.MakeObject<IExpressionFilter, RefHoldingFilter>(Access::Ref<Expression>(Access::Offset(text)));
    });
}

// A tree with good handles only seals, a filter's included
TEST(NodeArenaTest, SealAcceptsGoodRefs)
{
    NodeArena nodes;
    const NodeRef<Expression> good = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    const auto filter = nodes.MakeObject<IExpressionFilter, RefHoldingFilter>(good);
    const auto unary = nodes.Make<UnaryExpression>(UnaryExpression::LogicalNot, good);
    EXPECT_NO_THROW((void)nodes.Seal(filter, unary));
}

// With FULL checks a node Seal did not reach cannot be read: a handle that a node's
// VisitRefs forgets fails the first render that follows it
TEST(NodeArenaTest, FullRejectsAnUnvalidatedNode)
{
    if constexpr (NodeRefChecks < 2)
    {
        GTEST_SKIP() << "node reference checks below FULL";
    }
    NodeArena nodes;
    const NodeRef<Expression> reached = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 1 }));
    const NodeRef<Expression> forgotten = nodes.Make<ConstantExpression>(InternalValue(int64_t{ 2 }));
    const SealedArena tree = nodes.Seal(reached);
    EXPECT_EQ(1, *GetIf<int64_t>(tree[reached].GetConstant(nodes)));
    EXPECT_THROW((void)tree[forgotten], InvalidNodeRef);
}
#endif // JINJA2CPP_LINK_AS_SHARED

namespace
{
// A node of kind T::Kind is a U exactly when T derives from U: catches a subclass that
// inherits its base's Kind or MatchesKind instead of declaring its own
template<typename T, typename... Us>
int CountKindMismatches()
{
    return ((ArenaView::KindIs<Us>(T::Kind) != std::is_base_of_v<Us, T> ? 1 : 0) + ... + 0);
}

template<typename... Ts>
int CountAllKindMismatches()
{
    return (CountKindMismatches<Ts, Ts...>() + ... + 0);
}
} // namespace

TEST(NodeArenaTest, EveryNodeClassHasItsOwnKind)
{
    const int mismatches = CountAllKindMismatches<
        FullExpressionEvaluator, ValueRefExpression, SelfRefExpression, SubscriptExpression, LoopAttrExpression, FilteredExpression, ConstantExpression,
        TupleCreator, DictCreator, UnaryExpression, IsExpression, BinaryExpression, CompareExpression, SliceExpression, CallExpression,
        ExpressionFilter, IfExpression, ComposedRenderer, RawTextRenderer, ExpressionRenderer, FinalizedExpressionRenderer, TemplateRenderer,
        ForStatement, IfStatement, ElseBranchStatement, SetLineStatement, SetRawBlockStatement, SetFilteredBlockStatement, BlockStatement,
        ExtendsStatement, IncludeStatement, ImportStatement, MacroStatement, MacroCallStatement, DoStatement, TransStatement,
        LoopControlStatement, WithStatement, FilterStatement, AutoescapeStatement, ExpressionFilter::IExpressionFilter, IsExpression::ITester>();
    EXPECT_EQ(0, mismatches);
}
