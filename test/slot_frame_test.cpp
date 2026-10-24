#include "../src/internal_value.h"
#include "../src/lookup_result.h"
#include "../src/node_arena.h"
#include "../src/loop_attr.h"
#include "../src/render_context.h"
#include "../src/render_workspace.h"
#include "../src/slot_frame.h"
#include "../src/span.h"
#include "../src/value_visitors.h"

#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

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

    [[nodiscard]] FrameView For(Span<Slot> slots) const { return { slots, m_tree[m_list], m_tree.View() }; }

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
    context.BindSlot(slots[1], names.For(slots).names[1].hash, InternalValue(int64_t{ 5 }));
    const auto found = context.FindValue(std::string("x"));
    ASSERT_TRUE(found);
    EXPECT_TRUE(found.IsSame(LookupResult(slots[1])));
    EXPECT_EQ(5, IntOf(context.FindValue(HashedName{ "x", HashedName::Hash("x") })));

    context.UnbindSlots(slots, names.For(slots).names);
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
    context.BindSlot(slots[0], names.For(slots).names[0].hash, InternalValue(int64_t{ 5 }));
    scope["x"] = InternalValue(int64_t{ 7 });
    EXPECT_EQ(7, IntOf(context.FindValue(std::string("x"))));
    context.UnbindSlots(slots, names.For(slots).names);
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
    context.BindSlot(slots[0], names.For(slots).names[0].hash, InternalValue(int64_t{ 5 }));
    EXPECT_EQ(5, IntOf(context.FindValue(std::string("x"))));

    auto upper = context.EnterScope();
    upper["x"] = InternalValue(int64_t{ 9 });
    EXPECT_EQ(9, IntOf(context.FindValue(std::string("x"))));
    context.ExitScope();

    context.UnbindSlots(slots, names.For(slots).names);
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
    context.BindSlot(slots[0], names.For(slots).names[0].hash, InternalValue(int64_t{ 5 }));

    RenderContext below(context, 1);
    EXPECT_EQ(1, IntOf(below.FindValue(std::string("x"))));
    RenderContext all(context, context.GetScopesCount());
    EXPECT_EQ(5, IntOf(all.FindValue(std::string("x"))));
    const auto clone = context.Clone(true);
    EXPECT_EQ(5, IntOf(clone.FindValue(std::string("x"))));

    context.UnbindSlots(slots, names.For(slots).names);
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
    context.BindSlot(slots[0], names.For(slots).names[0].hash, InternalValue(int64_t{ 5 }));
    auto target = context.FindForWrite("x");
    ASSERT_TRUE(target);
    *target = InternalValue(int64_t{ 6 });
    EXPECT_TRUE(slots[0].IsBound());
    EXPECT_EQ(6, ConvertToInt(slots[0]));
    context.UnbindSlots(slots, names.For(slots).names);
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
    const auto cacheSlot = LookupCache::NewSlot(HashedName::Hash("x"));
    const HashedName x{ "x", HashedName::Hash("x") };
    EXPECT_EQ(1, IntOf(context.FindValueCached(&key, cacheSlot, x)));
    context.BindSlot(slots[0], names.For(slots).names[0].hash, InternalValue(int64_t{ 5 }));
    EXPECT_EQ(5, IntOf(context.FindValueCached(&key, cacheSlot, x)));
    context.BindSlot(slots[0], names.For(slots).names[0].hash, InternalValue(int64_t{ 6 }));
    EXPECT_EQ(6, IntOf(context.FindValueCached(&key, cacheSlot, x)));
    context.UnbindSlots(slots, names.For(slots).names);
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

TEST(SlotFrameTest, LentMapsComeBackEmptyWithTheirTables)
{
    RenderWorkspace workspace;
    auto& outer = workspace.LendMaps();
    outer.params["a"] = InternalValue(int64_t{ 1 });
    outer.topScope["b"] = InternalValue(int64_t{ 2 });
    // A render inside the first one is lent other maps
    auto& inner = workspace.LendMaps();
    EXPECT_NE(&outer, &inner);
    EXPECT_TRUE(inner.params.empty());
    workspace.ReturnMaps(inner);
    const auto mask = outer.params.mask();
    workspace.ReturnMaps(outer);

    // Emptied, with the table kept for the next render
    auto& again = workspace.LendMaps();
    EXPECT_EQ(&outer, &again);
    EXPECT_TRUE(again.params.empty());
    EXPECT_TRUE(again.topScope.empty());
    EXPECT_EQ(mask, again.params.mask());

    // A table too big to keep on the thread is let go
    for (int idx = 0; idx != 2000; ++idx)
    {
        again.topScope["n" + std::to_string(idx)] = InternalValue(int64_t{ idx });
    }
    EXPECT_GT(again.topScope.mask(), RenderWorkspace::MaxMapMask);
    workspace.ReturnMaps(again);
    auto& last = workspace.LendMaps();
    EXPECT_EQ(0u, last.topScope.mask());
    EXPECT_NE(0u, last.params.mask());
    workspace.ReturnMaps(last);

    // The renders nested past MaxKeptMaps keep no tables
    std::vector<RenderWorkspace::RenderMaps*> nested;
    for (size_t idx = 0; idx != RenderWorkspace::MaxKeptMaps + 1; ++idx)
    {
        nested.push_back(&workspace.LendMaps());
        nested.back()->params["a"] = InternalValue(int64_t{ 1 });
    }
    for (auto it = nested.rbegin(); it != nested.rend(); ++it)
    {
        workspace.ReturnMaps(**it);
    }
    EXPECT_EQ(0u, nested.back()->params.mask());
    EXPECT_NE(0u, nested.front()->params.mask());
}

// Where a closure's call runs (docs/tasks/0165): on the caller when it is the defining
// context, on the defining context, locked, while it is alive, on the caller with every
// scope hidden once it has ended
TEST(SlotFrameTest, ClosureSiteFindsItsHomeWhileItIsAlive)
{
    const InternalValueMap ext;
    const InternalValueMap globals;
    RenderContext caller(ext, globals, nullptr);
    caller.EnterScope();
    std::optional<ClosureSite> site;
    {
        RenderContext home(caller, caller.GetScopesCount());
        home.EnterScope();
        site.emplace(home);
        {
            const auto atHome = site->Resolve(home);
            EXPECT_EQ(&home, atHome.context);
            EXPECT_FALSE(atHome.lock);
            EXPECT_EQ(home.GetScopesCount(), atHome.link);
            const auto fromCaller = site->Resolve(caller);
            EXPECT_EQ(&home, fromCaller.context);
            EXPECT_TRUE(fromCaller.lock);
            EXPECT_EQ(home.GetScopesCount(), fromCaller.link);
        }
        // The scope it was defined in has ended: its names are hidden
        home.ExitScope();
        home.EnterScope();
        const auto ended = site->Resolve(home);
        EXPECT_EQ(&home, ended.context);
        EXPECT_EQ(0u, ended.link);
    }
    const auto dead = site->Resolve(caller);
    EXPECT_EQ(&caller, dead.context);
    EXPECT_FALSE(dead.lock);
    EXPECT_EQ(0u, dead.link);
}

// A body left by an exception leaves its home as it found it: scopes, views, autoescape,
// loop control (docs/tasks/0165)
TEST(SlotFrameTest, HomeCallRestoresTheHomeOnAnException)
{
    const InternalValueMap ext;
    const InternalValueMap globals;
    RenderContext home(ext, globals, nullptr);
    home.SetLookupCache(&LookupCache::ForThisThread());
    home.GetCurrentScope()["x"] = InternalValue(int64_t{ 1 });
    const ClosureSite site(home);
    home.EnterScope()["x"] = InternalValue(int64_t{ 2 });
    RenderContext caller(home, home.GetScopesCount());
    const auto scopes = home.GetScopesCount();
    const auto views = home.ViewCount();
    const bool autoescape = home.IsAutoescape();
    try
    {
        const HomeCall call(site, caller);
        auto& body = call.Context();
        EXPECT_EQ(&home, &body);
        EXPECT_EQ(1u, call.Link());
        body.EnterScope();
        body.PushFrameView({}, call.Link());
        EXPECT_EQ(1, IntOf(body.FindValue(std::string("x"))));
        body.SetAutoescape(!autoescape);
        body.SetLoopControl(LoopControl::Break);
        throw std::runtime_error("body");
    }
    catch (const std::runtime_error& error)
    {
        EXPECT_STREQ("body", error.what());
    }
    EXPECT_EQ(scopes, home.GetScopesCount());
    EXPECT_EQ(views, home.ViewCount());
    EXPECT_EQ(autoescape, home.IsAutoescape());
    EXPECT_FALSE(home.HasLoopControl());
    EXPECT_EQ(2, IntOf(home.FindValue(std::string("x"))));
    EXPECT_EQ(2, IntOf(caller.FindValue(std::string("x"))));
    home.ExitScope();
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

// The params and top scope tables are reused from render to render (docs/tasks/0160)
TEST(SlotRenderTest, NamesOfARenderDoNotReachTheNext)
{
    Template setter;
    ASSERT_TRUE(setter.Load("{% set b = a + 1 %}{{ b }}"));
    Template reader;
    ASSERT_TRUE(reader.Load("{% if a is defined %}a{% endif %}{% if b is defined %}b{% endif %}."));
    for (int idx = 0; idx != 2; ++idx)
    {
        EXPECT_EQ("2", setter.RenderAsString({ { "a", 1 } }).value());
        EXPECT_EQ(".", reader.RenderAsString({}).value());
    }
}

TEST(SlotRenderTest, CallableRendersATemplateThatSetsNames)
{
    Template inner;
    ASSERT_TRUE(inner.Load("{% set x = n * 10 %}{% if outer is defined %}!{% endif %}{{ x }}"));
    Template outer;
    ASSERT_TRUE(outer.Load("{% set outer = 1 %}{% for n in [1, 2] %}{{ render(n) }}{% set y = n %};{% endfor %}{{ outer }}{% if x is defined %}x{% endif %}"));
    ValuesMap params{ { "render", MakeCallable([&inner](int64_t n) { return inner.RenderAsString({ { "n", n } }).value(); }, ArgInfo{ "n" }) } };
    for (int idx = 0; idx != 2; ++idx)
    {
        EXPECT_EQ("10;20;1", outer.RenderAsString(params).value());
    }
}

TEST(SlotRenderTest, FailedAndHugeRendersThenAPlainOne)
{
    std::string many;
    for (int idx = 0; idx != 1500; ++idx)
    {
        many += "{% set v" + std::to_string(idx) + " = " + std::to_string(idx) + " %}";
    }
    Template huge;
    ASSERT_TRUE(huge.Load(many + "{{ v1499 }}"));
    Template failing;
    ASSERT_TRUE(failing.Load("{% set a = 1 %}{{ 1 // zero }}"));
    Template plain;
    ASSERT_TRUE(plain.Load("{% if a is defined or v0 is defined %}leak{% endif %}{{ p }}"));

    EXPECT_EQ("1499", huge.RenderAsString({}).value());
    EXPECT_EQ("p", plain.RenderAsString({ { "p", "p" } }).value());
    EXPECT_FALSE(failing.RenderAsString({ { "zero", 0 } }));
    EXPECT_EQ("q", plain.RenderAsString({ { "p", "q" } }).value());
}

// A macro called inside `with` runs on the context that defined it; an error in its body
// leaves nothing behind there (docs/tasks/0165)
TEST(SlotRenderTest, ErrorInAMacroBodyOnItsHomeThenACleanRender)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% set g = 'G' %}{% macro m(n) %}{{ g }}{{ 10 // n }}{% endmacro %}"
                         "{% for g in [1] %}{% with a = 1 %}{{ m(d) }}{% endwith %}{% endfor %}{{ g }}"));
    EXPECT_FALSE(tpl.RenderAsString({ { "d", 0 } }));
    EXPECT_EQ("G5G", tpl.RenderAsString({ { "d", 2 } }).value());
}

TEST(SlotRenderTest, MacroRecursionTakesAFramePerCall)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% macro m(n) %}{% for i in range(n) %}{{ n }}{{ m(n - 1) }}{% endfor %}{% endmacro %}{{ m(3) }}"));
    EXPECT_EQ("321213212132121", tpl.RenderAsString({}).value());
}
