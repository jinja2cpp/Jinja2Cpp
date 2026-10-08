#include "../src/internal_value.h"
#include "../src/lookup_result.h"
#include "../src/node_arena.h"
#include "../src/loop_attr.h"
#include "../src/render_context.h"
#include "../src/render_workspace.h"
#include "../src/slot_frame.h"
#include "../src/value_visitors.h"

#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>

using namespace jinja2;

// Slots and frame views as lookups by name see them (docs/design/0117-name-slots-plan.md, P1)

TEST(SlotFrameTest, SlotStartsUnboundAndKeepsTheValueSize)
{
    Slot slot;
    EXPECT_FALSE(slot.IsBound());
    slot.Bind(InternalValue(int64_t{ 3 }));
    EXPECT_TRUE(slot.IsBound());
    EXPECT_EQ(3, ConvertToInt(slot));
    slot.Unbind();
    EXPECT_FALSE(slot.IsBound());
    EXPECT_TRUE(slot.IsUndefined());
    static_assert(sizeof(Slot) == sizeof(InternalValue));
}

TEST(SlotFrameTest, LoopAttributeNamesRoundTrip)
{
    for (const auto* name :
         { "index", "index0", "revindex", "revindex0", "first", "last", "length", "depth", "depth0", "previtem", "nextitem", "cycle", "changed" })
    {
        EXPECT_NE(LoopAttr::None, FindLoopAttr(name)) << name;
    }
    EXPECT_EQ(LoopAttr::None, FindLoopAttr("indexx"));
    EXPECT_EQ(LoopAttr::None, FindLoopAttr(""));
}

// The arena and the workspace are not exported from a shared library
#ifndef JINJA2CPP_LINK_AS_SHARED
namespace
{
// The names of some slots, kept as a loop or a macro keeps them: texts of a sealed tree
class SlotNames
{
public:
    template<typename... Names>
    explicit SlotNames(Names... names)
    {
        NodeArena nodes;
        const std::array<std::string_view, sizeof...(Names)> views{ names... };
        const std::array<ArenaText, sizeof...(Names)> texts{ nodes.MakeText(names)... };
        std::array<SlotName, sizeof...(Names)> list{};
        for (std::size_t idx = 0; idx != list.size(); ++idx)
        {
            list[idx] = { texts[idx], HashedName::Hash(views[idx]) };
        }
        m_list = nodes.MakeSpan(list);
        // The texts are roots too, for the FULL checks to let them be read
        m_tree = std::apply([&nodes, this](auto... text) { return nodes.Seal(m_list, text...); }, texts);
    }

    [[nodiscard]] FrameView For(boost::span<Slot> slots) const { return { slots, m_tree[m_list], m_tree.View() }; }

private:
    SealedArena m_tree;
    ArenaSpan<SlotName> m_list;
};

int64_t IntOf(const LookupResult& result)
{
    return ConvertToInt(*result);
}
} // namespace

TEST(SlotFrameTest, BoundViewSlotIsFoundUnboundOneIsPassedOver)
{
    InternalValueMap ext = { { "x", InternalValue(int64_t{ 1 }) } };
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    std::array<Slot, 2> slots;
    const SlotNames names("loop", "x");
    context.EnterScope();
    context.PushFrameView(names.For(slots));

    EXPECT_EQ(1, IntOf(context.FindValue(std::string("x"))));
    context.BindSlot(slots[1], InternalValue(int64_t{ 5 }));
    const auto found = context.FindValue(std::string("x"));
    ASSERT_TRUE(found);
    EXPECT_TRUE(found.IsSame(LookupResult(slots[1])));
    EXPECT_EQ(5, IntOf(context.FindValue(HashedName{ "x", HashedName::Hash("x") })));

    context.UnbindSlots(slots);
    EXPECT_EQ(1, IntOf(context.FindValue(std::string("x"))));
    context.PopFrameView();
    context.ExitScope();
}

TEST(SlotFrameTest, MapOfTheSameScopeHidesTheView)
{
    const InternalValueMap ext;
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    std::array<Slot, 1> slots;
    const SlotNames names("x");
    auto scope = context.EnterScope();
    context.PushFrameView(names.For(slots));
    context.BindSlot(slots[0], InternalValue(int64_t{ 5 }));
    scope["x"] = InternalValue(int64_t{ 7 });
    EXPECT_EQ(7, IntOf(context.FindValue(std::string("x"))));
    context.UnbindSlots(slots);
    context.PopFrameView();
    context.ExitScope();
}

TEST(SlotFrameTest, ViewHidesLowerScopesAndIsHiddenByUpperOnes)
{
    const InternalValueMap ext;
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    context.GetCurrentScope()["x"] = InternalValue(int64_t{ 1 });
    std::array<Slot, 1> slots;
    const SlotNames names("x");
    context.EnterScope();
    context.PushFrameView(names.For(slots));
    context.BindSlot(slots[0], InternalValue(int64_t{ 5 }));
    EXPECT_EQ(5, IntOf(context.FindValue(std::string("x"))));

    auto upper = context.EnterScope();
    upper["x"] = InternalValue(int64_t{ 9 });
    EXPECT_EQ(9, IntOf(context.FindValue(std::string("x"))));
    context.ExitScope();

    context.UnbindSlots(slots);
    context.PopFrameView();
    context.ExitScope();
    EXPECT_EQ(1, IntOf(context.FindValue(std::string("x"))));
}

TEST(SlotFrameTest, ChildSeesOnlyTheViewsOfTheScopesItSees)
{
    const InternalValueMap ext;
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    context.GetCurrentScope()["x"] = InternalValue(int64_t{ 1 });
    std::array<Slot, 1> slots;
    const SlotNames names("x");
    context.EnterScope();
    context.PushFrameView(names.For(slots));
    context.BindSlot(slots[0], InternalValue(int64_t{ 5 }));

    RenderContext below(context, 1);
    EXPECT_EQ(1, IntOf(below.FindValue(std::string("x"))));
    RenderContext all(context, context.GetScopesCount());
    EXPECT_EQ(5, IntOf(all.FindValue(std::string("x"))));
    const auto clone = context.Clone(true);
    EXPECT_EQ(5, IntOf(clone.FindValue(std::string("x"))));

    context.UnbindSlots(slots);
    context.PopFrameView();
    context.ExitScope();
}

TEST(SlotFrameTest, WriteThroughTheLookupReachesTheSlot)
{
    const InternalValueMap ext;
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    std::array<Slot, 1> slots;
    const SlotNames names("x");
    context.EnterScope();
    context.PushFrameView(names.For(slots));
    EXPECT_FALSE(context.FindForWrite("x"));
    context.BindSlot(slots[0], InternalValue(int64_t{ 5 }));
    auto target = context.FindForWrite("x");
    ASSERT_TRUE(target);
    *target = InternalValue(int64_t{ 6 });
    EXPECT_TRUE(slots[0].IsBound());
    EXPECT_EQ(6, ConvertToInt(slots[0]));
    context.UnbindSlots(slots);
    context.PopFrameView();
    context.ExitScope();
}

// Binding a slot may hide what a cached lookup found; unbinding may show it again
TEST(SlotFrameTest, BindingAndUnbindingStartLookupEpochs)
{
    InternalValueMap ext = { { "x", InternalValue(int64_t{ 1 }) } };
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    context.SetLookupCache(&LookupCache::ForThisThread());
    std::array<Slot, 1> slots;
    const SlotNames names("x");
    context.EnterScope();
    context.PushFrameView(names.For(slots));

    int key = 0;
    const auto cacheSlot = LookupCache::NewSlot();
    const HashedName x{ "x", HashedName::Hash("x") };
    EXPECT_EQ(1, IntOf(context.FindValueCached(&key, cacheSlot, x)));
    context.BindSlot(slots[0], InternalValue(int64_t{ 5 }));
    EXPECT_EQ(5, IntOf(context.FindValueCached(&key, cacheSlot, x)));
    context.BindSlot(slots[0], InternalValue(int64_t{ 6 }));
    EXPECT_EQ(6, IntOf(context.FindValueCached(&key, cacheSlot, x)));
    context.UnbindSlots(slots);
    EXPECT_EQ(1, IntOf(context.FindValueCached(&key, cacheSlot, x)));
    context.PopFrameView();
    context.ExitScope();
}

TEST(SlotFrameTest, FrameGivenBackCannotBeResolved)
{
    RenderWorkspace workspace;
    const auto frame = workspace.Take({ UnitId{ 0 }, 3 });
    EXPECT_EQ(3u, frame.slots.size());
    EXPECT_EQ(UnitId{ 0 }, frame.unit);
    EXPECT_EQ(frame.slots.data(), workspace.Resolve(frame.handle).slots.data());
    workspace.Release(frame);
    EXPECT_EQ(0u, workspace.FrameCount());
    EXPECT_THROW((void)workspace.Resolve(frame.handle), std::logic_error);

    // A frame taken again at the same depth is another call's
    const auto next = workspace.Take({ UnitId{ 0 }, 3 });
    EXPECT_THROW((void)workspace.Resolve(frame.handle), std::logic_error);
    workspace.Release(next);
}

TEST(SlotFrameTest, FramesKeepTheirSlotsWhenTheWorkspaceGrows)
{
    RenderWorkspace workspace;
    const auto first = workspace.Take({ UnitId{ 0 }, 10 });
    first.slots[0].Bind(InternalValue(int64_t{ 1 }));
    const auto big = workspace.Take({ UnitId{ 1 }, 1000 });
    EXPECT_EQ(1000u, big.slots.size());
    EXPECT_EQ(first.slots.data(), workspace.Resolve(first.handle).slots.data());
    EXPECT_EQ(1, ConvertToInt(first.slots[0]));
    EXPECT_EQ(2u, workspace.FrameCount());
    workspace.Release(big);
    first.slots[0].Unbind();
    workspace.Release(first);

    // The space given back is taken again
    const auto again = workspace.Take({ UnitId{ 0 }, 10 });
    EXPECT_EQ(first.slots.data(), again.slots.data());
    workspace.Release(again);
}
#endif

// Loops whose names live in slots (0117 P1), through the public API

TEST(SlotRenderTest, ErrorInALoopInAMacroThenACleanRender)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% macro m(xs) %}{% for x in xs %}{{ 10 // x }}{% endfor %}{% endmacro %}"
                         "{% for y in ys %}[{{ m(y) }}]{% endfor %}"));
    // Built item by item: a braced list of one list is that list itself for some compilers
    ValuesList failing;
    failing.emplace_back(ValuesList{ 1, 0 });
    EXPECT_FALSE(tpl.RenderAsString({ { "ys", failing } }));
    // The frames and slots the failed render took are all given back
    EXPECT_EQ("[51][2]", tpl.RenderAsString({ { "ys", ValuesList{ ValuesList{ 2, 10 }, ValuesList{ 5 } } } }).value());
}

TEST(SlotRenderTest, CallableRendersALoopingTemplateInsideALoop)
{
    Template inner;
    ASSERT_TRUE(inner.Load("{% for i in range(n) %}{{ i }}{% endfor %}"));
    Template outer;
    ASSERT_TRUE(outer.Load("{% for n in [1, 3] %}{{ n }}:{{ render(n) }};{% endfor %}"));
    ValuesMap params{ { "render", MakeCallable([&inner](int64_t n) { return inner.RenderAsString({ { "n", n } }).value(); }, ArgInfo{ "n" }) } };
    EXPECT_EQ("1:0;3:012;", outer.RenderAsString(params).value());
}

TEST(SlotRenderTest, MacroRecursionTakesAFramePerCall)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% macro m(n) %}{% for i in range(n) %}{{ n }}{{ m(n - 1) }}{% endfor %}{% endmacro %}{{ m(3) }}"));
    EXPECT_EQ("321213212132121", tpl.RenderAsString({}).value());
}
