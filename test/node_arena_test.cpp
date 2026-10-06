#include "../src/expression_evaluator.h"
#include "../src/internal_value.h"
#include "../src/node_arena.h"
#include "../src/renderer.h"
#include "../src/statements.h"

#include "gtest/gtest.h"

#include <cstdint>
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
    EXPECT_EQ(5, *GetIf<int64_t>(nodes[expr].GetConstant(nodes.View())));
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
    NodeRef<IRendererBase> rawSet = nodes.Make<SetRawBlockStatement>(AssignTarget{ "x", {}, false, {} });

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
    EXPECT_EQ(1, *GetIf<int64_t>(nodes[view[0]].GetConstant(nodes.View())));
    EXPECT_EQ(2, *GetIf<int64_t>(nodes[view[1]].GetConstant(nodes.View())));
    EXPECT_TRUE(nodes.MakeSpan(items).empty());
}

TEST(NodeArenaTest, MovingTheArenaKeepsTheNodes)
{
    NodeArena nodes;
    std::vector<NodeRef<Expression>> items;
    // Enough nodes and lists to fill more blocks than the arena keeps inline
    for (int64_t n = 0; n != 2000; ++n)
    {
        items.emplace_back(nodes.Make<ConstantExpression>(InternalValue(n)));
    }
    const auto span = nodes.MakeSpan(items);

    NodeArena moved(std::move(nodes));
    NodeArena assigned;
    assigned.Make<ConstantExpression>(InternalValue());
    assigned = std::move(moved);

    const auto view = assigned[span];
    ASSERT_EQ(2000u, view.size());
    EXPECT_EQ(1999, *GetIf<int64_t>(assigned[view[1999]].GetConstant(assigned.View())));
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
