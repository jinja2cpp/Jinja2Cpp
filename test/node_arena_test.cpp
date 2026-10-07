#include "../src/expression_evaluator.h"
#include "../src/internal_value.h"
#include "../src/node_arena.h"
#include "../src/renderer.h"
#include "../src/statements.h"

#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <cstdint>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

using namespace jinja2;

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
        items.emplace_back(nodes.Make<ConstantExpression>(InternalValue(std::string(40, static_cast<char>('a' + n % 26)) + std::to_string(n))));
    }
    const auto span = nodes.MakeSpan(items);

    SealedArena sealed = nodes.Seal();
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
    const SealedArena sealed = nodes.Seal();

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
        LoopControlStatement, WithStatement, FilterStatement, AutoescapeStatement>();
    EXPECT_EQ(0, mismatches);
}
