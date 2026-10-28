// Writes to the host's data: every name of it in the render sees them, the host's data does not
// change, and nothing of one render's copies reaches another one (docs/tasks/0049, P2)
#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace jinja2;

namespace
{
std::string Render(const std::string& source, const ValuesMap& params, TemplateEnv* env = nullptr)
{
    Template tpl(env);
    auto loaded = tpl.Load(source);
    EXPECT_TRUE(!!loaded) << (loaded ? std::string() : loaded.error().ToString());
    auto result = tpl.RenderAsString(params);
    EXPECT_TRUE(!!result) << (result ? std::string() : result.error().ToString());
    return result ? result.value() : std::string();
}

ValuesMap Messages()
{
    return { { "l", ValuesList{ 3, 1, 2 } },
             { "messages", ValuesList{ ValuesMap{ { "r", "u" } }, ValuesMap{ { "r", "a" } } } },
             { "nd", ValuesMap{ { "k", ValuesList{ 1 } } } } };
}
} // namespace

using BorrowedCellsTest = BasicTemplateRenderer;

MULTISTR_TEST(BorrowedCellsTest, AliasSeesAppend, R"({% set y = l %}{% set _ = y.append(9) %}{{ l }}|{{ y is sameas l }})", R"([3, 1, 2, 9]|True)")
{
    params = { { "l", ValuesList{ 3, 1, 2 } } };
}

MULTISTR_TEST(BorrowedCellsTest, LoopVariableUpdatesItsItem, R"({% for m in messages %}{% set _ = m.update(x=1) %}{% endfor %}{{ messages[0].x }}{{ messages[1].x }}{{ messages[1].r }})", R"(11a)")
{
    params = Messages();
}

MULTISTR_TEST(BorrowedCellsTest, BoundMethodOfHostList, R"({% set f = l.append %}{% set _ = f(9) %}{{ l }})", R"([3, 1, 2, 9])")
{
    params = { { "l", ValuesList{ 3, 1, 2 } } };
}

MULTISTR_TEST(BorrowedCellsTest, NestedItemReadTwice, R"({% set a = nd.k %}{% set _ = nd['k'].append(2) %}{{ a }}{{ nd.k|length }})", R"([1, 2]2)")
{
    params = Messages();
}

// A loop over the host's list that appends to it: the enumerator finds the cell at the next step
// and reads its size at each one from then on (CellStep::Recheck)
MULTISTR_TEST(BorrowedCellsTest, LoopSeesItsOwnAppend, R"({% for x in l %}{% set _ = l.append(x) if loop.index < 3 %}{{ x }},{% endfor %})", R"(3,1,2,3,1,)")
{
    params = { { "l", ValuesList{ 3, 1, 2 } } };
}

TEST(BorrowedCellsSingle, HostDataIsNotWritten)
{
    const auto params = Messages();
    EXPECT_EQ("[3, 1, 2, 9]|1", Render("{% set y = l %}{% set _ = y.append(9) %}{% set _ = messages[0].update(x=1) %}{{ l }}|{{ messages[0].x }}", params));
    // The host's data, and the next render's view of it, are unchanged (decision D2)
    EXPECT_EQ(3u, params.at("l").As<ValuesList>().size());
    EXPECT_EQ(1u, params.at("messages").As<ValuesList>()[0].As<ValuesMap>().size());
    EXPECT_EQ("[3, 1, 2]|{'r': 'u'}", Render("{{ l }}|{{ messages[0] }}", params));
}

TEST(BorrowedCellsSingle, GlobalsAreTheirOwnInEachRender)
{
    // The globals' adapters outlive the render on the thread (0211): their cells must not
    TemplateEnv env;
    env.AddGlobal("g", ValuesList{ 1, 2 });
    env.AddGlobal("gm", ValuesMap{ { "k", ValuesList{ 1 } } });
    const std::string source = "{% set _ = g.append(3) %}{% set _ = gm.k.append(2) %}{{ g }}{{ gm }}";
    EXPECT_EQ("[1, 2, 3]{'k': [1, 2]}", Render(source, {}, &env));
    EXPECT_EQ("[1, 2, 3]{'k': [1, 2]}", Render(source, {}, &env));
    EXPECT_EQ("[1, 2]{'k': [1]}", Render("{{ g }}{{ gm }}", {}, &env));
}

TEST(BorrowedCellsSingle, NestedRenderHasItsOwnCells)
{
    // A user callable renders another template over the same host list: each render changes
    // its own view of it, and the outer one keeps its change after the inner one ends
    ValuesMap params{ { "l", ValuesList{ 3 } } };
    Template inner;
    ASSERT_TRUE(!!inner.Load("{% set _ = l.append(2) %}{{ l }}"));
    params["inner"] = MakeCallable([&inner, &params]() { return inner.RenderAsString(params).value(); });
    EXPECT_EQ("[3, 2]|[3, 1]|[3, 1]", Render("{% set y = l %}{% set _ = l.append(1) %}{{ inner() }}|{{ l }}|{{ y }}", params));
}

TEST(BorrowedCellsSingle, NestedRenderOfAGlobalWrittenThroughAnAlias)
{
    // The thread keeps the globals' adapters for the next render; a write through an alias does
    // not mark the global scope written, yet a render nested in this one must not see its cell
    TemplateEnv env;
    env.AddGlobal("g", ValuesList{ 0 });
    Template inner(&env);
    ASSERT_TRUE(!!inner.Load("{% set _ = g.append(2) %}{{ g }}"));
    ValuesMap params;
    params["inner"] = MakeCallable([&inner]() { return inner.RenderAsString({}).value(); });
    EXPECT_EQ("[0, 2]|[0, 1]", Render("{% set y = g %}{% set _ = y.append(1) %}{{ inner() }}|{{ g }}", params, &env));
    EXPECT_EQ("[0]", Render("{{ g }}", {}, &env));
}

TEST(BorrowedCellsSingle, HostCallableSeesTheWrite)
{
    ValuesMap params{ { "l", ValuesList{ 3 } } };
    params["size"] = MakeCallable([](const GenericList& list) { return static_cast<int64_t>(list.GetSize().value_or(0)); }, ArgInfo{ "list" });
    EXPECT_EQ("2|2", Render("{% set y = l %}{% set _ = y.append(1) %}{{ size(l) }}|{{ size(y) }}", params));
}

// A container handed to the host before the render writes it is a view of the host's data,
// during the render too; one handed out after the write is the render's copy, and shows the
// render's later writes. Python shows them in both (`[[1, 2], 3]`), a deliberate divergence:
// what the host keeps is never mutable state a render shares (docs/tasks/0331)
TEST(BorrowedCellsSingle, KeptValueBeforeAndAfterTheWrite)
{
    Value kept;
    ValuesMap params{ { "l", ValuesList{ Value(ValuesList{ 1 }) } } };
    params["keep"] = UserCallable(
        [&kept](const UserCallableParams& p) {
            kept = p["v"];
            return Value();
        },
        { ArgInfo{ "v" } });
    params["show"] = UserCallable([&kept](const UserCallableParams&) { return kept; }, {});
    EXPECT_EQ("None[[1]]", Render("{{ keep(l) }}{% set _ = l[0].append(2) %}{% set _ = l.append(3) %}{{ show() }}", params));
    EXPECT_EQ("None[[1, 2], 3, 4]", Render("{% set _ = l[0].append(2) %}{% set _ = l.append(3) %}{{ keep(l) }}{% set _ = l.append(4) %}{{ show() }}", params));
}

// After the render, a kept host container the render wrote keeps the render's final state: the
// render's copy freezes, the host's data is never written (docs/tasks/0331)
TEST(BorrowedCellsSingle, KeptValueAfterTheRenderKeepsTheWrites)
{
    Value nested;
    Value direct;
    // The inner list as a Value: a ValuesList braced in a ValuesList is a copy of it for clang
    ValuesMap params{ { "l", ValuesList{ Value(ValuesList{ 1 }) } } };
    params["keep"] = UserCallable(
        [&nested, &direct](const UserCallableParams& p) {
            nested = p["nested"];
            direct = p["direct"];
            return Value();
        },
        { ArgInfo{ "nested" }, ArgInfo{ "direct" } });
    EXPECT_EQ("None", Render("{% set _ = l[0].append(2) %}{{ keep({'saved': l}, l[0]) }}", params));
    EXPECT_EQ("[1, 2]|[1, 2]", Render("{{ n.saved[0] }}|{{ d }}", ValuesMap{ { "n", nested }, { "d", direct } }));
    // The host's data is not written
    EXPECT_EQ("[[1]]", Render("{{ l }}", params));
}

namespace
{
// A host callable that keeps what the template hands it (docs/tasks/0331)
struct Keeper
{
    std::vector<Value> kept;

    void AddTo(ValuesMap& params)
    {
        params["keep"] = UserCallable(
            [this](const UserCallableParams& p) {
                kept.push_back(p["v"]);
                return Value();
            },
            { ArgInfo{ "v" } });
        params["touch"] = UserCallable([this](const UserCallableParams&) { return kept.back(); }, {});
    }
    // What the host keeps, printed by another render with params of its own
    [[nodiscard]] std::string Show(size_t idx = 0) const { return Render("{{ k }}", ValuesMap{ { "k", kept.at(idx) } }); }
};

// Renders `source` with params made for it and freed before the host reads what it kept
std::string RenderFreed(Keeper& keeper, const std::string& source, ValuesMap params)
{
    keeper.AddTo(params);
    auto result = Render(source, params);
    params.clear();
    return result;
}
} // namespace

// The acceptance rows of docs/tasks/0331
TEST(BorrowedCellsKept, WrittenInsideATemplateList)
{
    // K1: the list the template made holds h's adapter, which reads the frozen cell
    Keeper keeper;
    EXPECT_EQ("", RenderFreed(keeper, "{% set _ = h.append(3) %}{% set _ = keep([h]) %}{% set _ = h.append(4) %}", { { "h", ValuesList{ 1, 2 } } }));
    EXPECT_EQ("[[1, 2, 3, 4]]", keeper.Show());
}

TEST(BorrowedCellsKept, NestedWriteThenKept)
{
    // K2: a subscript write copies the spine; l's copy holds l[0]'s
    Keeper keeper;
    EXPECT_EQ("", RenderFreed(keeper, "{% set _ = l[0].append(2) %}{% set _ = keep(l) %}", { { "l", ValuesList{ Value(ValuesList{ 1 }) } } }));
    EXPECT_EQ("[[1, 2]]", keeper.Show());
}

TEST(BorrowedCellsKept, WrittenBeforeAndAfter)
{
    // K3: handed out after a write, the host has the render's copy and its final state
    Keeper keeper;
    EXPECT_EQ("[1, 2, 3, 4]", RenderFreed(keeper, "{% set _ = h.append(3) %}{% set _ = keep(h) %}{% set _ = h.append(4) %}{{ touch() }}", { { "h", ValuesList{ 1, 2 } } }));
    EXPECT_EQ("[1, 2, 3, 4]", keeper.Show());
}

TEST(BorrowedCellsKept, HandedOutBeforeTheWrite)
{
    // K4: a view of the host's data, during the render and after it (Python: [1, 2, 4] in both;
    // decision D1). The host keeps the params alive while it holds the view, as on master
    Keeper keeper;
    ValuesMap params{ { "l", ValuesList{ 1, 2 } } };
    keeper.AddTo(params);
    EXPECT_EQ("[1, 2]|[1, 2, 4]", Render("{% set _ = keep(l) %}{% set _ = l.append(4) %}{{ touch() }}|{{ l }}", params));
    EXPECT_EQ("[1, 2]", keeper.Show());
}

TEST(BorrowedCellsKept, NotKeptLeavesNoCell)
{
    // K5: the cell goes at the render's end (LSan in the sanitizer job)
    Keeper keeper;
    EXPECT_EQ("[1, 2, 3]", RenderFreed(keeper, "{% set _ = h.append(3) %}{{ h }}", { { "h", ValuesList{ 1, 2 } } }));
    EXPECT_EQ("[1, 2, 3]|[1, [...]]|{'a': 1, 'd': {...}}",
              RenderFreed(keeper, "{% set _ = h.append(3) %}{{ h }}|{% set _ = l.append(l) %}{{ l }}|{% set _ = d.update(d=d) %}{{ d }}",
                          { { "h", ValuesList{ 1, 2 } }, { "l", ValuesList{ 1 } }, { "d", ValuesMap{ { "a", 1 } } } }));
}

TEST(BorrowedCellsKept, NestedRenderReturnsItsKeptValue)
{
    // K6: a callable renders an inner template with params of its own and returns what it kept
    Template inner;
    ASSERT_TRUE(!!inner.Load("{% set _ = l.append(2) %}{% set _ = keep(l) %}"));
    ValuesMap params;
    params["f"] = UserCallable(
        [&inner](const UserCallableParams&) {
            Keeper keeper;
            ValuesMap local{ { "l", ValuesList{ 1 } } };
            keeper.AddTo(local);
            EXPECT_TRUE(!!inner.RenderAsString(local));
            local.clear();
            return keeper.kept.at(0);
        },
        {});
    EXPECT_EQ("[1, 2]", Render("{{ f() }}", params));
    // Written by the outer render, which copies it into a cell of its own
    EXPECT_EQ("[1, 2, 9]", Render("{% set x = f() %}{% set _ = x.append(9) %}{{ x }}", params));
}

TEST(BorrowedCellsKept, ModuleWritesAndKeeps)
{
    // K7: the module's cells freeze when the module goes
    Keeper keeper;
    {
        ValuesMap params{ { "l", ValuesList{ 0 } } };
        keeper.AddTo(params);
        Template tpl;
        ASSERT_TRUE(!!tpl.Load("{% macro add(x) %}{% set _ = l.append(x) %}{% set _ = keep(l) %}{% endmacro %}"));
        auto module = tpl.MakeModule(params).value();
        UserCallableParams callParams;
        callParams.extraPosArgs = ValuesList{ 1 };
        module.exports["add"].As<UserCallable>().callable(callParams);
    }
    EXPECT_EQ("[0, 1]", keeper.Show());
}

namespace
{
// A module whose macro writes the host's `l` with its argument and hands `l` to keep()
struct KeepingModule
{
    Keeper keeper;
    Template tpl;
    TemplateModule module;

    explicit KeepingModule(const std::string& macro)
    {
        ValuesMap params{ { "l", ValuesList{ 0 } } };
        keeper.AddTo(params);
        EXPECT_TRUE(!!tpl.Load(macro));
        auto made = tpl.MakeModule(params);
        EXPECT_TRUE(!!made);
        if (made)
        {
            module = std::move(*made);
        }
    }
    void Call(int64_t x)
    {
        UserCallableParams callParams;
        callParams.extraPosArgs = ValuesList{ x };
        module.exports["add"].As<UserCallable>().callable(callParams);
    }
};
} // namespace

TEST(BorrowedCellsKept, ModuleCopiesOnALaterWrite)
{
    // D8: a value a module call handed out keeps that call's final state; a later call's write
    // copies the items first (Python: every kept value is the same list, [0, 1, 2, 3])
    KeepingModule m("{% macro add(x) %}{% set _ = l.append(x) %}{% set _ = keep(l) %}{% set _ = l.append(x * 10) %}{% endmacro %}");
    m.Call(1);
    m.Call(2);
    EXPECT_EQ("[0, 1, 10]", m.keeper.Show(0));
    EXPECT_EQ("[0, 1, 10, 2, 20]", m.keeper.Show(1));
    m.Call(3);
    EXPECT_EQ("[0, 1, 10]", m.keeper.Show(0));
    EXPECT_EQ("[0, 1, 10, 2, 20]", m.keeper.Show(1));
    EXPECT_EQ("[0, 1, 10, 2, 20, 3, 30]", m.keeper.Show(2));
}

TEST(BorrowedCellsKept, ModuleDictCopiesOnALaterWrite)
{
    // The same for a dict of the host's data
    Keeper keeper;
    ValuesMap params{ { "d", ValuesMap{ { "a", 0 } } } };
    keeper.AddTo(params);
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro add(x) %}{% set _ = d.update(b=x) %}{% set _ = keep(d) %}{% endmacro %}"));
    auto module = tpl.MakeModule(params).value();
    for (int64_t x = 1; x != 3; ++x)
    {
        UserCallableParams callParams;
        callParams.extraPosArgs = ValuesList{ x };
        module.exports["add"].As<UserCallable>().callable(callParams);
    }
    EXPECT_EQ("{'a': 0, 'b': 1}", keeper.Show(0));
    EXPECT_EQ("{'a': 0, 'b': 2}", keeper.Show(1));
}

TEST(BorrowedCellsKept, ModuleStoredArgumentCopiesOnALaterWrite)
{
    // D8 for an argument the module keeps in its namespace: its cell is held, so it stays, and a
    // later call's write copies the items the kept value shares (verifier round 12 of #673)
    Keeper keeper;
    ValuesMap params;
    keeper.AddTo(params);
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% set ns = namespace(l=none) %}{% macro store(xs) %}{% set _ = xs.append(2) %}{% set ns.l = xs %}{% set _ = keep(xs) %}{% endmacro %}"
                           "{% macro app(v) %}{% set _ = ns.l.append(v) %}{{ ns.l }}{% endmacro %}"));
    auto module = tpl.MakeModule(params).value();
    auto call = [&module](const std::string& name, Value arg) {
        UserCallableParams callParams;
        callParams.extraPosArgs = ValuesList{ std::move(arg) };
        return module.exports[name].As<UserCallable>().callable(callParams).AsString();
    };
    call("store", ValuesList{ 1 });
    EXPECT_EQ("[1, 2, 3]", call("app", 3));
    EXPECT_EQ("[1, 2, 3, 4]", call("app", 4));
    EXPECT_EQ("[1, 2]", keeper.Show(0));
}

TEST(BorrowedCellsKept, ModuleLoopWhileACallbackCopiesTwice)
{
    // A loop over the host's list in one module call, whose callback runs two calls that each
    // copy the list on write (D8): the loop reads the module's list as it is now, not storage the
    // second copy let go of (Codex review)
    Keeper keeper;
    const std::vector<int64_t> data{ 0 };
    auto made = std::make_shared<GenericList>(MakeGenericList(data.begin(), data.end()));
    ValuesMap params{ { "l", GenericList([made] { return made->GetAccessor(); }) } };
    keeper.AddTo(params);
    TemplateModule* self = nullptr;
    bool once = true;
    const auto add = [&self](int64_t x) {
        UserCallableParams callParams;
        callParams.extraPosArgs = ValuesList{ x };
        self->exports["add"].As<UserCallable>().callable(callParams);
    };
    params["again"] = UserCallable(
        [&add, &once](const UserCallableParams&) {
            if (std::exchange(once, false))
            {
                add(2);
                add(3);
            }
            return Value();
        },
        {});
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro add(x) %}{% set _ = l.append(x) %}{% set _ = keep(l) %}{% endmacro %}"
                           "{% macro walk() %}{% for i in l %}{{ i }}{% set _ = again() %};{% endfor %}{% endmacro %}"));
    auto module = tpl.MakeModule(params);
    ASSERT_TRUE(!!module);
    self = &*module;
    add(1);
    EXPECT_EQ("0;1;2;3;", module->exports["walk"].As<UserCallable>().callable(UserCallableParams{}).AsString());
    EXPECT_EQ("[0, 1]", keeper.Show(0));
    EXPECT_EQ("[0, 1, 2]", keeper.Show(1));
}

namespace
{
// A module whose `l` is the host's list and whose host callable `again` calls the module's `add`
// twice, the first time only: two nested calls that each copy `l` on write (D8), between which the
// host lets go of the oldest value it kept
struct ReenteredModule
{
    Keeper keeper;
    Template tpl;
    TemplateModule module;
    bool once = true;

    explicit ReenteredModule(const std::string& macros)
    {
        ValuesMap params{ { "l", ValuesList{ 0 } } };
        keeper.AddTo(params);
        params["again"] = UserCallable(
            [this](const UserCallableParams&) {
                if (std::exchange(once, false))
                {
                    Add(2);
                    // The host lets go of what the call before this one handed it
                    keeper.kept.erase(keeper.kept.begin());
                    Add(3);
                }
                return Value();
            },
            {});
        EXPECT_TRUE(!!tpl.Load("{% macro add(x) %}{% set _ = l.append(x) %}{% set _ = keep(l) %}{% endmacro %}" + macros));
        auto made = tpl.MakeModule(params);
        EXPECT_TRUE(!!made);
        if (made)
        {
            module = std::move(*made);
        }
    }
    void Add(int64_t x)
    {
        UserCallableParams callParams;
        callParams.extraPosArgs = ValuesList{ x };
        module.exports["add"].As<UserCallable>().callable(callParams);
    }
    std::string Call(const std::string& name) { return module.exports[name].As<UserCallable>().callable(UserCallableParams{}).AsString(); }
};
} // namespace

TEST(BorrowedCellsKept, ModuleCallAfterANestedCallCopies)
{
    // The outer call writes after a nested call of the same module exported `l`: the nested
    // call's snapshot stays as it was when that call returned (Codex review)
    ReenteredModule m("{% macro outer() %}{% set _ = again() %}{% set _ = l.append(9) %}{% endmacro %}");
    m.Call("outer");
    ASSERT_EQ(1u, m.keeper.kept.size());
    EXPECT_EQ("[0, 2, 3]", m.keeper.Show(0));
}

TEST(BorrowedCellsKept, ModuleNestedKeepOfTheOuterCallsExport)
{
    // The outer call writes and hands out `l`, a nested call of the same module hands out `l`
    // without writing it, and the outer call writes again after the nested call returned. The
    // cell keeps the outer call's export mark, so that write goes in place and changes what the
    // nested call handed out after it ended. Found by the TLA+ model (tla/, finding F1). This
    // row pins the current behaviour. Task 0337 makes the nested call's value "[0, 1]"; the
    // outer call's value then misses its own later write too, which is the 0335 limit
    Keeper keeper;
    ValuesMap params{ { "l", ValuesList{ 0 } } };
    keeper.AddTo(params);
    TemplateModule module;
    params["again"] = UserCallable(
        [&module](const UserCallableParams&) {
            module.exports["show"].As<UserCallable>().callable(UserCallableParams{});
            return Value();
        },
        {});
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro show() %}{% set _ = keep(l) %}{% endmacro %}"
                           "{% macro outer() %}{% set _ = l.append(1) %}{% set _ = keep(l) %}"
                           "{% set _ = again() %}{% set _ = l.append(2) %}{% endmacro %}"));
    auto made = tpl.MakeModule(params);
    ASSERT_TRUE(!!made);
    module = std::move(*made);
    module.exports["outer"].As<UserCallable>().callable(UserCallableParams{});
    ASSERT_EQ(2u, keeper.kept.size());
    EXPECT_EQ("[0, 1, 2]", keeper.Show(0));
    EXPECT_EQ("[0, 1, 2]", keeper.Show(1));
}

TEST(BorrowedCellsKept, ModuleSortWhoseKeyCopiesTwice)
{
    // The key function of sort() runs two calls that each copy `l` on write (D8) and hand it to
    // the host: sort() finds the list's items again after its key calls, so it never writes what
    // the host was handed (Codex review). The key function changed the list, which Python
    // rejects too
    ReenteredModule m("{% macro srt() %}{% set _ = l.sort(key=k) %}{% endmacro %}"
                      "{% macro k(x) %}{% set _ = again() %}{{ x }}{% endmacro %}");
    m.Add(1);
    EXPECT_THROW(m.Call("srt"), BadResultAccess<>);
    EXPECT_EQ("[2]", m.keeper.Show(0));
    EXPECT_EQ("[2, 3]", m.keeper.Show(1));
}

TEST(BorrowedCellsKept, ModuleSortWhoseKeyHandsOutTheList)
{
    // The key function makes the host call the module again, and that call hands the list to
    // the host while sort() holds its items: the host keeps the emptied list as that call left it,
    // and the sorted items go to a copy (D8)
    Keeper keeper;
    ValuesMap params{ { "l", ValuesList{ 3, 1, 2 } } };
    keeper.AddTo(params);
    const TemplateModule* self = nullptr;
    params["reexp"] = UserCallable(
        [&self](const UserCallableParams&) {
            self->exports.at("exp").As<UserCallable>().callable(UserCallableParams{});
            return Value();
        },
        {});
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro srt() %}{% set _ = l.sort(key=k) %}{{ l }}{% endmacro %}"
                           "{% macro k(v) %}{% set _ = reexp() %}{{ v }}{% endmacro %}"
                           "{% macro exp() %}{% set _ = keep(l) %}{% endmacro %}"));
    auto module = tpl.MakeModule(params);
    ASSERT_TRUE(!!module);
    self = &*module;
    EXPECT_EQ("[1, 2, 3]", module->exports["srt"].As<UserCallable>().callable(UserCallableParams{}).AsString());
    ASSERT_EQ(3u, keeper.kept.size());
    EXPECT_EQ("[]", keeper.Show(0));
    EXPECT_EQ("[]", keeper.Show(2));
}

TEST(BorrowedCellsKept, ModuleCellInsideAKeptList)
{
    // A limit (docs/tasks/0332): a list the module call made holds `l`'s adapter, which reads the
    // module's live cell, so a later call's write shows through it until the module goes. The
    // target is the first call's state, [[0, 1]], as for `keep(l)` (ModuleCopiesOnALaterWrite)
    KeepingModule m("{% macro add(x) %}{% set _ = l.append(x) %}{% set _ = keep([l]) %}{% endmacro %}");
    m.Call(1);
    m.Call(2);
    EXPECT_EQ("[[0, 1, 2]]", m.keeper.Show(0));
}

TEST(BorrowedCellsKept, ModuleCalledFromThreadsWhileTheHostReads)
{
    // D8, TSan: two threads call the module, which writes `l` in every call, while a third reads
    // what the first call handed out
    KeepingModule m("{% macro add(x) %}{% set _ = l.append(x) %}{% set _ = keep(l) %}{% endmacro %}");
    m.Call(1);
    const Value first = m.keeper.kept.at(0);
    std::vector<std::thread> threads;
    threads.reserve(3);
    for (int t = 0; t != 2; ++t)
    {
        threads.emplace_back([&m]() {
            for (int i = 0; i != 50; ++i)
            {
                UserCallableParams callParams;
                callParams.extraPosArgs = ValuesList{ int64_t{ 2 } };
                m.module.exports.at("add").As<UserCallable>().callable(callParams);
            }
        });
    }
    std::string seen;
    threads.emplace_back([&first, &seen]() {
        for (int i = 0; i != 50; ++i)
        {
            seen = Render("{{ k }}", ValuesMap{ { "k", first } });
        }
    });
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_EQ("[0, 1]", seen);
}

TEST(BorrowedCellsKept, GivenBackAndWritten)
{
    // K8: a kept value given to a later render is that render's borrowed data: it writes a copy
    Keeper keeper;
    EXPECT_EQ("", RenderFreed(keeper, "{% set _ = h.append(3) %}{% set _ = keep(h) %}", { { "h", ValuesList{ 1, 2 } } }));
    EXPECT_EQ("[1, 2, 3, 9]", Render("{% set _ = k.append(9) %}{{ k }}", ValuesMap{ { "k", keeper.kept.at(0) } }));
    EXPECT_EQ("[1, 2, 3]", keeper.Show());
}

TEST(BorrowedCellsKept, GlobalInsideAKeptList)
{
    // K9: a global's adapter detaches at the render's end: the next render and the kept value read
    // the global's data (decision D4; Python keeps [[1, 2, 1]])
    TemplateEnv env;
    env.AddGlobal("g", ValuesList{ 1, 2 });
    Keeper keeper;
    ValuesMap params;
    keeper.AddTo(params);
    EXPECT_EQ("[1, 2, 1]", Render("{% set _ = g.append(1) %}{% set _ = keep([g]) %}{{ g }}", params, &env));
    EXPECT_EQ("[1, 2]", Render("{{ g }}", {}, &env));
    EXPECT_EQ("[[1, 2]]", keeper.Show());
}

TEST(BorrowedCellsKept, CyclesThroughFrozenCellsAreFreed)
{
    // Cycles through the render's copies that nothing keeps: the collector frees them through the
    // frozen links (LSan in the sanitizer job)
    Keeper keeper;
    EXPECT_EQ("2", RenderFreed(keeper, "{% set _ = l.append(l) %}{{ l|length }}", { { "l", ValuesList{ 1 } } }));
    EXPECT_EQ("1", RenderFreed(keeper, "{% set _ = m[0].append(m) %}{{ m|length }}", { { "m", ValuesList{ Value(ValuesList{ 1 }) } } }));
    EXPECT_EQ("2", RenderFreed(keeper, "{% set _ = n.append([n]) %}{{ n|length }}", { { "n", ValuesList{ 2 } } }));
}

TEST(BorrowedCellsKept, KeptSelfCycleIsTheRendersCopy)
{
    // A kept container that holds itself is the render's copy, cycle included, after the params
    // are freed. As on master the cycle leaks once the host lets it go, as no collector runs then
    // (docs/tasks/0331, decision D7): a static keeps it reachable, so that LSan does not report
    // the leak this row documents
    static auto* keeper = new Keeper;
    EXPECT_EQ("[1, [...]]", RenderFreed(*keeper, "{% set _ = l.append(l) %}{% set _ = keep(l) %}{{ l }}", { { "l", ValuesList{ 1 } } }));
    const auto kept = keeper->kept.back().ToGenericList();
    EXPECT_EQ(2u, kept.GetSize().value_or(0));
    EXPECT_EQ(2u, kept.GetAccessor()->GetIndexer()->GetItemByIndex(1).ToGenericList().GetSize().value_or(0));
}

TEST(BorrowedCellsKept, ConcurrentRendersOfAKeptView)
{
    // T1: a view kept from one render is given to four concurrent renders, each writing its own
    // `l`: the view is read-only, so no render links it into its cell (TSan)
    Keeper keeper;
    ValuesMap first{ { "l", ValuesList{ 1, 2 } } };
    keeper.AddTo(first);
    EXPECT_EQ("", Render("{% set _ = keep(l) %}", first));
    ValuesMap params{ { "l", ValuesList{ 1, 2 } }, { "t", keeper.kept.at(0) } };
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% set _ = l.append(3) %}{{ t|length }},{{ l|length }}"));
    std::vector<std::thread> threads;
    threads.reserve(4);
    std::vector<std::string> results(4);
    for (auto& result : results)
    {
        threads.emplace_back([&tpl, &params, &result]() {
            for (int i = 0; i != 50; ++i)
            {
                auto out = tpl.RenderAsString(params);
                result = out ? out.value() : out.error().ToString();
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    for (const auto& result : results)
    {
        EXPECT_EQ("2,3", result);
    }
}

TEST(BorrowedCellsKept, KeptGlobalReadWhileItsThreadWrites)
{
    // T2: a list holding a global's adapter is read on another thread while the global's thread
    // renders writes to that global (TSan)
    TemplateEnv env;
    env.AddGlobal("g", ValuesList{ 1, 2 });
    Keeper keeper;
    ValuesMap params;
    keeper.AddTo(params);
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{% set _ = keep([g]) if not done %}{% set _ = g.append(1) %}{{ g|length }}"));
    params["done"] = false;
    const auto first = tpl.RenderAsString(params);
    EXPECT_EQ("3", first ? first.value() : first.error().ToString());
    params["done"] = true;
    const Value kept = keeper.kept.at(0);
    std::thread reader([&kept]() {
        for (int i = 0; i != 200; ++i)
        {
            const auto list = kept.ToGenericList();
            const auto item = list.GetAccessor()->GetIndexer()->GetItemByIndex(0);
            EXPECT_EQ(2u, item.ToGenericList().GetSize().value_or(0));
        }
    });
    for (int i = 0; i != 200; ++i)
    {
        const auto out = tpl.RenderAsString(params);
        EXPECT_EQ("3", out ? out.value() : out.error().ToString());
    }
    reader.join();
}

TEST(BorrowedCellsKept, FrozenValueReadFromThreads)
{
    // T4: a kept frozen list read from four threads after its render (TSan)
    Keeper keeper;
    EXPECT_EQ("", RenderFreed(keeper, "{% set _ = l[0].append(2) %}{% set _ = keep([l]) %}", { { "l", ValuesList{ Value(ValuesList{ 1 }) } } }));
    const Value kept = keeper.kept.at(0);
    std::vector<std::thread> threads;
    threads.reserve(4);
    std::vector<std::string> results(4);
    for (auto& result : results)
    {
        threads.emplace_back([&kept, &result]() {
            for (int i = 0; i != 50; ++i)
            {
                result = Render("{{ k }}", ValuesMap{ { "k", kept } });
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    for (const auto& result : results)
    {
        EXPECT_EQ("[[[1, 2]]]", result);
    }
}

// An export of a cycle is a view of it
TEST(BorrowedCellsSingle, ExportOfACycle)
{
    ValuesMap params{ { "l", ValuesList{ 3 } } };
    params["size"] = MakeCallable([](const GenericList& list) { return static_cast<int64_t>(list.GetSize().value_or(0)); }, ArgInfo{ "list" });
    EXPECT_EQ("2|2", Render("{% set _ = l.append(l) %}{{ size(l) }}|{{ size(l[1]) }}", params));
}

TEST(BorrowedCellsSingle, SelfAppendIsFreed)
{
    // The cell holds the adapter that holds the cell: the render's end breaks the cycle (LSan)
    const ValuesMap params{ { "l", ValuesList{ 1 } }, { "d", ValuesMap{ { "a", 1 } } } };
    EXPECT_EQ("[1, [...]]|{'a': 1, 'd': {...}}", Render("{% set _ = l.append(l) %}{% set _ = d.update(d=d) %}{{ l }}|{{ d }}", params));
}

TEST(BorrowedCellsSingle, ModuleKeepsItsWritesAcrossCalls)
{
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro add(x) %}{% set _ = l.append(x) %}{{ l }}{% endmacro %}"));
    auto module = tpl.MakeModule({ { "l", ValuesList{ 0 } } }).value();
    auto call = [&module](int64_t x) {
        UserCallableParams callParams;
        callParams.extraPosArgs = ValuesList{ x };
        return module.exports["add"].As<UserCallable>().callable(callParams).AsString();
    };
    EXPECT_EQ("[0, 1]", call(1));
    EXPECT_EQ("[0, 1, 2]", call(2));
}

TEST(BorrowedCellsSingle, ModuleCallFreesTheCellsOfItsOwnData)
{
    // An argument or a user callable's result that the macro writes lives for the call: once
    // nothing holds it, the module drops its cell when the call ends, instead of keeping one
    // more for each call. The callable in the data holds the token
    auto token = std::make_shared<int>(0);
    UserCallable held([token](const UserCallableParams&) { return Value(); }, {});
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro consume(xs, d) %}{% set _ = xs.append(1) %}{% set _ = d.update(k=1) %}{% set r = make() %}"
                           "{% set _ = r.append(1) %}{{ xs|length }}{{ d|length }}{{ r|length }}{% endmacro %}"));
    ValuesMap params{ { "make", UserCallable([&held](const UserCallableParams&) { return Value(ValuesList{ held }); }, {}) } };
    auto module = tpl.MakeModule(params).value();
    for (int i = 0; i != 3; ++i)
    {
        UserCallableParams callParams;
        callParams.extraPosArgs = ValuesList{ ValuesList{ 0, held }, ValuesMap{ { "f", held } } };
        EXPECT_EQ("322", module.exports["consume"].As<UserCallable>().callable(callParams).AsString());
    }
    held = UserCallable();
    EXPECT_EQ(1, token.use_count());
}

TEST(BorrowedCellsSingle, ModuleCallFreesTheCellsOfItsNestedData)
{
    // The same when the data holds containers, written at both levels: the copy's items hold
    // the nested adapters, whose cells go once the outer cell has gone
    auto token = std::make_shared<int>(0);
    UserCallable held([token](const UserCallableParams&) { return Value(); }, {});
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro consume(xs) %}{% set _ = xs.append(1) %}{% set _ = xs[0].append(2) %}{% set _ = xs[1].update(k=1) %}"
                           "{{ xs|length }}{{ xs[0] }}{{ xs[1]|length }}{% endmacro %}"));
    auto module = tpl.MakeModule({}).value();
    for (int i = 0; i != 3; ++i)
    {
        UserCallableParams callParams;
        // Value(...): some compilers take a braced ValuesList holding one ValuesList for a copy of it
        callParams.extraPosArgs = ValuesList{ Value(ValuesList{ ValuesList{ 0, held }, ValuesMap{ { "f", held } } }) };
        EXPECT_EQ("3[0, , 2]2", module.exports["consume"].As<UserCallable>().callable(callParams).AsString());
    }
    held = UserCallable();
    EXPECT_EQ(1, token.use_count());
}

TEST(BorrowedCellsSingle, ThreadsRenderSharedHostData)
{
    // One host map, rendered by several threads that each write to it: each render has its own
    // cells (run under TSan in the sanitizer job)
    const ValuesMap params = Messages();
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% set y = l %}{% for m in messages %}{% set _ = m.update(n=loop.index) %}{% set _ = y.append(m.n) %}{% endfor %}{{ l }}{{ messages[1].n }}"));
    std::vector<std::thread> threads;
    std::vector<std::string> results(4);
    threads.reserve(results.size());
    for (auto& result : results)
    {
        threads.emplace_back([&tpl, &params, &result]() {
            for (int i = 0; i != 50; ++i)
            {
                auto out = tpl.RenderAsString(params);
                result = out ? out.value() : out.error().ToString();
                if (result != "[3, 1, 2, 1, 2]2")
                {
                    return;
                }
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    for (const auto& result : results)
    {
        EXPECT_EQ("[3, 1, 2, 1, 2]2", result);
    }
}
