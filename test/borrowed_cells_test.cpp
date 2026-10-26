// Writes to the host's data: every name of it in the render sees them, the host's data does not
// change, and nothing of one render's copies reaches another one (docs/tasks/0049, P2)
#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstdint>
#include <string>
#include <thread>
#include <vector>

#if !defined(_MSC_VER) && defined(__SANITIZE_ADDRESS__)
#define JINJA2CPP_TEST_LSAN 1
#endif
#if !defined(_MSC_VER) && defined(__has_feature)
#if __has_feature(address_sanitizer)
#define JINJA2CPP_TEST_LSAN 1
#endif
#endif
#ifdef JINJA2CPP_TEST_LSAN
#include <sanitizer/lsan_interface.h>
#endif

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

TEST(BorrowedCellsSingle, HostCallableSeesTheWrite)
{
    ValuesMap params{ { "l", ValuesList{ 3 } } };
    params["size"] = MakeCallable([](const GenericList& list) { return static_cast<int64_t>(list.GetSize().value_or(0)); }, ArgInfo{ "list" });
    EXPECT_EQ("2|2", Render("{% set y = l %}{% set _ = y.append(1) %}{{ size(l) }}|{{ size(y) }}", params));
}

// A value the host keeps past the render shows the render's writes, also to a nested container:
// the host's list it was given is the render's copy, whose items are the render's copies of the
// containers it wrote to. A host list inside a list or dict of the template the host keeps reads
// the host's data again after the render (left for later in docs/tasks/0049)
TEST(BorrowedCellsSingle, KeptValueKeepsTheWrites)
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
    EXPECT_EQ("None", Render("{% set _ = l[0].append(2) %}{{ keep(l, l[0]) }}", params));
    EXPECT_EQ("[1, 2]|[1, 2]", Render("{{ n[0] }}|{{ d }}", ValuesMap{ { "n", nested }, { "d", direct } }));
    // The host's data is not written
    EXPECT_EQ("[[1]]", Render("{{ l }}", params));
}

// A host list that holds itself, given to the host: the render's copy, which holds itself
TEST(BorrowedCellsSingle, ExportOfACycle)
{
    ValuesMap params{ { "l", ValuesList{ 3 } } };
    params["size"] = MakeCallable([](const GenericList& list) { return static_cast<int64_t>(list.GetSize().value_or(0)); }, ArgInfo{ "list" });
    EXPECT_EQ("2|2", Render("{% set _ = l.append(l) %}{{ size(l) }}|{{ size(l[1]) }}", params));
}

// The host's container is the render's while the render runs, as Python's object is: a write
// after the call reaches it, and it keeps what it showed when the render ended
TEST(BorrowedCellsSingle, KeptValueSeesLaterWrites)
{
    Value kept;
    ValuesMap params{ { "l", ValuesList{ Value(ValuesList{ 1 }), Value(ValuesList{ 2 }) } } };
    params["keep"] = UserCallable(
        [&kept](const UserCallableParams& p) {
            kept = p["value"];
            return Value();
        },
        { ArgInfo{ "value" } });
    params["show"] = UserCallable([&kept](const UserCallableParams& /*p*/) { return kept; }, {});
    EXPECT_EQ("None[[1, 9, 7], [2]]", Render("{% set _ = l[0].append(9) %}{{ keep(l) }}{% set _ = l[0].append(7) %}{{ show() }}", params));
    EXPECT_EQ("[[1, 9, 7], [2]]", Render("{{ k }}", ValuesMap{ { "k", kept } }));
    // Kept before the first write
    EXPECT_EQ("None", Render("{{ keep(l) }}{% set _ = l[1].append(3) %}", params));
    EXPECT_EQ("[[1], [2, 3]]", Render("{{ k }}", ValuesMap{ { "k", kept } }));
}

// A dict with keys that are not text reaches the host as a copy with text keys (D13, the
// public map has text keys only), so what comes back is that copy, as on master
TEST(BorrowedCellsSingle, KeptDictWithNumberKeys)
{
    Value kept;
    ValuesMap params{ { "l", ValuesList{ 1 } } };
    params["keep"] = UserCallable(
        [&kept](const UserCallableParams& p) {
            kept = p["value"];
            return kept;
        },
        { ArgInfo{ "value" } });
    EXPECT_EQ("{'1': 'a', 'x': 'b'}", Render("{% set _ = l.append(1) %}{{ keep({1: 'a', 'x': 'b'}) }}", params));
    EXPECT_EQ("{'1': 'a', 'x': 'b'}", Render("{{ k }}", ValuesMap{ { "k", kept } }));
}

// A list given back by a callable is the render's own list, as Python's object is: a list that
// appends its own export holds itself, a cycle the collector frees (LSan)
TEST(BorrowedCellsSingle, ExportInACycleIsFreed)
{
    ValuesMap params{ { "l", ValuesList{ Value(ValuesList{ 1 }) } }, { "d", ValuesMap{ { "a", 1 } } } };
    params["id"] = UserCallable([](const UserCallableParams& p) { return p["value"]; }, { ArgInfo{ "value" } });
    EXPECT_EQ("2|2", Render("{% set _ = l[0].append(2) %}{% set _ = l.append(id(l)) %}{{ l | length }}|{% set _ = d.update(s=id(d)) %}{{ d | length }}", params));
}

// The slots of one container share its copy at the render's end, as Python's names share the object
TEST(BorrowedCellsSingle, KeptSlotsOfOneContainer)
{
    ValuesList kept;
    ValuesMap params{ { "l", ValuesList{ Value(ValuesList{ 1 }) } } };
    params["keep"] = UserCallable(
        [&kept](const UserCallableParams& p) {
            kept.push_back(p["value"]);
            return Value();
        },
        { ArgInfo{ "value" } });
    EXPECT_EQ("", Render("{% set _ = l[0].append(2) %}{% for i in range(3) %}{% set _ = keep(l) %}{% endfor %}", params));
    ASSERT_EQ(3U, kept.size());
    EXPECT_EQ("[[1, 2]]|[[1, 2]]|[[1, 2]]", Render("{{ k[0] }}|{{ k[1] }}|{{ k[2] }}", ValuesMap{ { "k", kept } }));
}

// A host list the render wrote to and appended to itself, kept by the host: after the render its
// item is the render's copy of the nested list, and its last item is the list itself, as
// Python's object is. Nothing collects a cycle the host holds, so the host breaks it (LSan)
TEST(BorrowedCellsSingle, KeptSelfAppendIsTheSameObject)
{
    Value kept;
    ValuesMap params{ { "l", ValuesList{ Value(ValuesList{ 1 }) } } };
    params["keep"] = UserCallable(
        [&kept](const UserCallableParams& p) {
            kept = p["value"];
            return Value();
        },
        { ArgInfo{ "value" } });
    EXPECT_EQ("None", Render("{% set _ = l[0].append(2) %}{% set _ = l.append(l) %}{{ keep(l) }}", params));
    EXPECT_EQ("[1, 2]|2", Render("{{ k[0] }}|{{ k|length }}", ValuesMap{ { "k", kept } }));
    ValuesMap again;
    again["give"] = UserCallable([&kept](const UserCallableParams& /*p*/) { return kept; }, {});
    EXPECT_EQ("[1, 2]|True|1", Render("{% set m = give() %}{{ m[0] }}|{{ m[1] is sameas m }}{% set _ = m.pop() %}|{{ m|length }}", again));
    kept = Value();
    EXPECT_EQ("[[1]]", Render("{{ l }}", params));
}

// A host list given to the host before the render writes to it is the render's copy too
TEST(BorrowedCellsSingle, KeptBeforeTheFirstWrite)
{
    Value kept;
    ValuesMap params{ { "l", ValuesList{ 1 } } };
    params["keep"] = UserCallable(
        [&kept](const UserCallableParams& p) {
            kept = p["value"];
            return Value();
        },
        { ArgInfo{ "value" } });
    EXPECT_EQ("None", Render("{{ keep(l) }}{% set _ = l.append(7) %}", params));
    EXPECT_EQ("[1, 7]", Render("{{ k }}", ValuesMap{ { "k", kept } }));
    EXPECT_EQ("[1]", Render("{{ l }}", params));
}

// A host list that holds, through a host list the callable makes, its own export twice: the render
// ends. The cycle through the host's list leaks, as on master (docs/tasks/0251)
TEST(BorrowedCellsSingle, ExportThroughAHostWrapperEnds)
{
#ifdef JINJA2CPP_TEST_LSAN
    const __lsan::ScopedDisabler leaks;
#endif
    ValuesMap params{ { "l", ValuesList{ Value(ValuesList{ 1 }) } } };
    params["idl"] = UserCallable([](const UserCallableParams& p) { return Value(ValuesList{ p["value"] }); }, { ArgInfo{ "value" } });
    EXPECT_EQ("3", Render("{% set _ = l[0].append(2) %}{% set _ = l.append(idl(l)) %}{% set _ = l.append(idl(l)) %}{{ l|length }}", params));
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
