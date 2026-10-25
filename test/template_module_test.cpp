// Template::MakeModule against Python Jinja2 3.1.6's Template.module and make_module (docs/tasks/0284).
// The expected names, bodies and call results are Python's, printed by
// `sorted(k for k in vars(m) if not k.startswith('__'))`, `str(m)` and `m.name(...)`.
#include "test_tools.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__)
#define JINJA2CPP_TEST_SANITIZED 1
#endif
#ifdef __SANITIZE_THREAD__
#define JINJA2CPP_TEST_TSAN 1
#endif
#ifdef __has_feature
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) || __has_feature(undefined_behavior_sanitizer)
#define JINJA2CPP_TEST_SANITIZED 1
#endif
#if __has_feature(thread_sanitizer)
#define JINJA2CPP_TEST_TSAN 1
#endif
#endif

namespace
{
jinja2::Value Call(const jinja2::Value& fn, jinja2::ValuesList pos = {}, jinja2::ValuesMap kw = {})
{
    jinja2::UserCallableParams params;
    params.extraPosArgs = std::move(pos);
    params.extraKwArgs = std::move(kw);
    return fn.As<jinja2::UserCallable>().callable(params);
}

std::string Names(const jinja2::ValuesMap& exports)
{
    std::string names;
    for (const auto& [name, value] : exports)
    {
        names += names.empty() ? name : "," + name;
    }
    return names;
}
} // namespace

class TemplateModuleTest : public TemplateEnvFixture
{
protected:
    void SetUp() override
    {
        TemplateEnvFixture::SetUp();
        AddFile("a", "{% macro am() %}AM{% endmacro %}{% set ac = 1 %}");
        AddFile("base", "{% macro bm() %}BM{% endmacro %}{% set bv = 2 %}base-body");
    }

    jinja2::TemplateModule Make(const std::string& source, const jinja2::ValuesMap& params = {})
    {
        auto tpl = Load(source);
        auto module = tpl.MakeModule(params);
        EXPECT_TRUE(!!module) << (module ? std::string() : module.error().ToString());
        return module ? std::move(*module) : jinja2::TemplateModule{};
    }
};

struct ModuleCase
{
    const char* source;
    const char* names;
    const char* body;
};

// clang-format off
const ModuleCase moduleCases[] = {
    // Top-level sets and macros; not `_` names, nor those set in a loop or `with`
    { "{% set x = 1 %}{% set _y = 2 %}{% macro m(a, b=2) %}[{{a}}{{b}}]{% endmacro %}{% for i in [1] %}{% set f = 1 %}{% endfor %}{% with %}{% set w=1 %}{% endwith %} body {{ x }}",
      "m,x", " body 1" },
    // Names an import binds are not exported
    { R"({% import "a" as a %}{% from "a" import am, ac %}{% set k = 1 %})", "k", "" },
    // ... unless set again after it (to another object: task 0299)
    { R"({% from "a" import am %}{% set am = 2 %})", "am", "" },
    { R"({% set am = 1 %}{% from "a" import am %})", "", "" },
    // A set in a top-level `if`, tuple sets, a set block
    { "{% if true %}{% set i = 1 %}{% else %}{% set e = 2 %}{% endif %}{% set a, b = 1, 2 %}{% set blk %}x<y{% endset %}", "a,b,blk,i", "" },
    // The names of the template it extends, whose output is the body
    { R"({% extends "base" %}{% macro cm() %}CM{% endmacro %}{% set cv = 3 %})", "bm,bv,cm,cv", "base-body" },
    { R"({% set ns = namespace(a=1) %}{% set l = [1,{"k":2}] %}{% set u = nothing %})", "l,ns,u", "" },
};
// clang-format on

TEST_F(TemplateModuleTest, ExportsAndBody)
{
    for (const auto& testCase : moduleCases)
    {
        SCOPED_TRACE(testCase.source);
        auto module = Make(testCase.source);
        EXPECT_EQ(testCase.names, Names(module.exports));
        EXPECT_EQ(testCase.body, module.body);
    }
}

TEST_F(TemplateModuleTest, Values)
{
    auto module = Make(R"({% set ns = namespace(a=1) %}{% set l = [1,{"k":2}] %}{% set u = nothing %}{% set blk %}x<y{% endset %})");
    EXPECT_EQ(jinja2::Value(int64_t{ 1 }), module.exports["ns"].AsMap()["a"]);
    const auto& list = module.exports["l"].AsList();
    ASSERT_EQ(2u, list.size());
    EXPECT_EQ(jinja2::Value(int64_t{ 1 }), list[0]);
    EXPECT_EQ(jinja2::Value(int64_t{ 2 }), list[1].AsMap().at("k"));
    EXPECT_TRUE(module.exports["u"].IsNone());
    EXPECT_EQ("x<y", module.exports["blk"].AsString());
    EXPECT_FALSE(module.exports["blk"].IsMarkup());
}

TEST_F(TemplateModuleTest, MacroCalls)
{
    auto module = Make("{% macro m(a, b=2) %}[{{a}}|{{b}}|{{kwargs}}|{{varargs}}]{% endmacro %}");
    const auto& m = module.exports["m"];
    EXPECT_EQ("[1|2|{}|()]", Call(m, { 1 }).AsString());
    EXPECT_EQ("[1|3|{}|()]", Call(m, { 1, 3 }).AsString());
    EXPECT_EQ("[1|4|{}|()]", Call(m, {}, { { "a", 1 }, { "b", 4 } }).AsString());
    EXPECT_EQ("[1|2|{}|(3,)]", Call(m, { 1, 2, 3 }).AsString());
    EXPECT_EQ("[|2|{}|()]", Call(m).AsString());
}

TEST_F(TemplateModuleTest, MakeModuleParams)
{
    auto tpl = Load("{% macro m() %}{{ x }}{% endmacro %}{{ x }}");
    auto withVars = tpl.MakeModule({ { "x", "X" } }).value();
    EXPECT_EQ("X", withVars.body);
    EXPECT_EQ("X", Call(withVars.exports["m"]).AsString());
    EXPECT_EQ("", Call(tpl.MakeModule().value().exports["m"]).AsString());
}

TEST_F(TemplateModuleTest, Autoescape)
{
    jinja2::Settings settings;
    settings.autoescape = true;
    m_env.SetSettings(settings);
    auto module = Make(R"({% macro m(x) %}<b>{{x}}</b>{% endmacro %}{% set s = "<i>" %}{% set blk %}<p>{% endset %})");
    auto result = Call(module.exports["m"], { "<&>" });
    EXPECT_TRUE(result.IsMarkup());
    EXPECT_EQ("<b>&lt;&amp;&gt;</b>", result.AsString());
    EXPECT_FALSE(module.exports["s"].IsMarkup());
    EXPECT_EQ("<i>", module.exports["s"].AsString());
    EXPECT_TRUE(module.exports["blk"].IsMarkup());
    EXPECT_EQ("<p>", module.exports["blk"].AsString());
}

// The calls share the module: a namespace one changes stays changed. The exported value is the
// copy made with the module (Python's is the live object, whose n is 2 here)
TEST_F(TemplateModuleTest, CallsShareTheModule)
{
    auto module = Make("{% set ns = namespace(n=0) %}{% macro inc() %}{% set ns.n = ns.n + 1 %}{{ ns.n }}{% endmacro %}");
    EXPECT_EQ("1", Call(module.exports["inc"]).AsString());
    EXPECT_EQ("2", Call(module.exports["inc"]).AsString());
    EXPECT_EQ(jinja2::Value(int64_t{ 0 }), module.exports["ns"].AsMap()["n"]);
}

TEST_F(TemplateModuleTest, Recursion)
{
    auto module = Make("{% macro r(n) %}{% if n %}{{ n }}{{ r(n-1) }}{% endif %}{% endmacro %}");
    EXPECT_EQ("321", Call(module.exports["r"], { 3 }).AsString());
}

// An argument the macro keeps is the module's copy, not a view of the host's string
TEST_F(TemplateModuleTest, KeptArgumentIsOwned)
{
    auto module = Make("{% set ns = namespace(v='', l=[]) %}{% macro keep(x) %}{% set ns.v = x %}{{ ns.l.append(x) }}{% endmacro %}{% macro get() %}{{ ns.v }}{{ ns.l|join(',') }}{% endmacro %}");
    {
        std::string arg(64, 'a');
        Call(module.exports["keep"], { arg });
        // A Value holding a list: `{ ValuesList{...} }` would copy the list itself under Clang
        Call(module.exports["keep"], jinja2::ValuesList{ jinja2::Value(jinja2::ValuesList{ std::string(40, 'b') }) });
    }
    EXPECT_EQ("['" + std::string(40, 'b') + "']" + std::string(64, 'a') + ",['" + std::string(40, 'b') + "']", Call(module.exports["get"]).AsString());
}

TEST_F(TemplateModuleTest, ExportOutlivesTemplate)
{
    jinja2::Value m;
    {
        auto tpl = Load("{% set greeting = 'Hello' %}{% macro m(name) %}{{ greeting }}, {{ name }}!{% endmacro %}");
        m = tpl.MakeModule().value().exports["m"];
    }
    EXPECT_EQ("Hello, World!", Call(m, { "World" }).AsString());
}

// An exported value owns its text and items: the template's source, the params and the
// module are gone when it is read (ASan)
TEST_F(TemplateModuleTest, ExportedValuesAreOwned)
{
    jinja2::ValuesMap exports;
    {
        jinja2::Template tpl(&m_env);
        ASSERT_TRUE(!!tpl.Load("{% set lit = 'a literal of the source' %}{% set p = param %}{% set m = map %}{% set l = list %}"));
        jinja2::ValuesMap params{ { "param", std::string(48, 'p') },
                                  { "map", jinja2::ValuesMap{ { "k", std::string(48, 'v') } } },
                                  { "list", jinja2::ValuesList{ std::string(48, 'i') } } };
        exports = tpl.MakeModule(params).value().exports;
        ASSERT_TRUE(!!tpl.Load("{{ 'another source' }}"));
    }
    EXPECT_EQ("a literal of the source", exports["lit"].AsString());
    EXPECT_EQ(std::string(48, 'p'), exports["p"].AsString());
    EXPECT_EQ(std::string(48, 'v'), exports["m"].AsMap().at("k").AsString());
    EXPECT_EQ(std::string(48, 'i'), exports["l"].AsList().at(0).AsString());
}

// A module keeps the tree it was made from: Load replaces the template, not the module's tree
// (docs/tasks/0317, which reverses the second half of 0284 D1a)
TEST_F(TemplateModuleTest, LoadKeepsModule)
{
    jinja2::Template tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load("{% macro m() %}old{% endmacro %}"));
    auto module = tpl.MakeModule().value();
    EXPECT_EQ("old", Call(module.exports["m"]).AsString());
    ASSERT_TRUE(!!tpl.Load("{% macro m() %}new{% endmacro %}"));
    EXPECT_EQ("old", Call(module.exports["m"]).AsString());
    EXPECT_EQ("new", Call(tpl.MakeModule().value().exports["m"]).AsString());
    // A failed Load keeps the tree, and the modules made from it
    auto current = tpl.MakeModule().value();
    EXPECT_FALSE(!!tpl.Load("{% macro %}"));
    EXPECT_EQ("new", Call(current.exports["m"]).AsString());
    // The module outlives the template object
    tpl = jinja2::Template();
    EXPECT_EQ("old", Call(module.exports["m"]).AsString());
}

TEST_F(TemplateModuleTest, Errors)
{
    jinja2::Template notLoaded;
    auto result = notLoaded.MakeModule();
    ASSERT_FALSE(!!result);
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotParsed, result.error().GetCode());

    auto tpl = Load("{% set x = 1 %}{{ 1 // 0 }}");
    result = tpl.MakeModule();
    ASSERT_FALSE(!!result);
    EXPECT_EQ(jinja2::ErrorCode::TemplateRuntimeError, result.error().GetCode());

    auto module = Make("{% macro m(x) %}\n{{ x // 0 }}{% endmacro %}");
    try
    {
        Call(module.exports["m"], { 1 });
        FAIL() << "expected an error";
    }
    catch (const jinja2::BadResultAccess<>& failure)
    {
        EXPECT_EQ(jinja2::ErrorCode::TemplateRuntimeError, failure.error().GetCode());
        EXPECT_EQ(2u, failure.error().GetErrorLocation().line);
    }
    // The arguments' containers must be the documented ones
    jinja2::UserCallableParams params;
    params.extraPosArgs = 1;
    EXPECT_THROW(module.exports["m"].As<jinja2::UserCallable>().callable(params), jinja2::BadResultAccess<>);
}

// An export passed to another render runs there as a user callable; its error is the render's
TEST_F(TemplateModuleTest, ExportInAnotherRender)
{
    auto module = Make("{% macro m(x, y=1) %}<{{ x }}|{{ y }}>{% endmacro %}{% macro bad() %}{{ 1 // 0 }}{% endmacro %}");
    EXPECT_EQ("<1|2>", Render("{{ m(1, y=2) }}", { { "m", module.exports["m"] } }));
    auto tpl = Load("{{ bad() }}");
    auto result = tpl.RenderAsString({ { "bad", module.exports["bad"] } });
    ASSERT_FALSE(!!result);
    EXPECT_EQ(jinja2::ErrorCode::TemplateRuntimeError, result.error().GetCode());
}

// A container that holds itself is cut where it repeats; the module frees it (LSan)
TEST_F(TemplateModuleTest, SelfReferencingContainers)
{
    auto module = Make("{% set l = [1] %}{{ l.append(l) }}{% set d = {} %}{{ d.update({'self': d}) }}");
    const auto& list = module.exports["l"].AsList();
    ASSERT_EQ(2u, list.size());
    EXPECT_EQ(jinja2::Value(int64_t{ 1 }), list[0]);
    EXPECT_TRUE(list[1].IsNone());
    EXPECT_TRUE(module.exports["d"].AsMap().at("self").IsNone());
}

// A dict with keys that are not strings is copied as its public view spells them, as str()
// does, and a cycle through it is cut too (Codex review of #634)
TEST_F(TemplateModuleTest, NonStringKeys)
{
    auto module = Make("{% set e = {1: 'x', none: 'y'} %}{% set d = {1: 0} %}{{ d.update({2: d}) }}");
    EXPECT_EQ("1,None", Names(module.exports["e"].AsMap()));
    EXPECT_EQ("x", module.exports["e"].AsMap().at("1").AsString());
    const auto& dict = module.exports["d"].AsMap();
    EXPECT_EQ("1,2", Names(dict));
    EXPECT_EQ(jinja2::Value(int64_t{ 0 }), dict.at("1"));
    EXPECT_TRUE(dict.at("2").IsNone());
}

// maxOutputSize bounds the module's body and each macro call's output; copying the exports
// after the body never reads the body's registration after it is gone (Codex review of #634)
TEST_F(TemplateModuleTest, OutputLimit)
{
    jinja2::Settings settings;
    settings.limits.maxOutputSize = 4;
    m_env.SetSettings(settings);
    auto module = Make("{% set x = [1, {'a': 2}] %}{% macro m(n) %}{{ '1' * n }}{% endmacro %}abc");
    EXPECT_EQ("abc", module.body);
    EXPECT_EQ("1111", Call(module.exports["m"], { 4 }).AsString());
    try
    {
        Call(module.exports["m"], { 5 });
        FAIL() << "expected ResourceLimitExceeded";
    }
    catch (const jinja2::BadResultAccess<>& failure)
    {
        EXPECT_EQ(jinja2::ErrorCode::ResourceLimitExceeded, failure.error().GetCode()) << failure.error().ToString();
    }
    auto over = Load("{% set x = 1 %}12345").MakeModule();
    ASSERT_FALSE(!!over);
    EXPECT_EQ(jinja2::ErrorCode::ResourceLimitExceeded, over.error().GetCode());
}

// A value nested deeper than the stack allows fails as a render of it does
TEST_F(TemplateModuleTest, DeepNesting)
{
#ifdef JINJA2CPP_TEST_TSAN
    // The access history of the whole run, which already has 0093's deep values, makes LLVM
    // 18's ThreadSanitizer reset its traces and corrupt memory on the CI runners (0093)
    const std::string levels = "2000";
#elif defined(JINJA2CPP_TEST_SANITIZED)
    const std::string levels = "20000";
#else
    const std::string levels = "100000";
#endif
    auto tpl = Load("{% set ns = namespace(l=[]) %}{% for i in range(" + levels + ") %}{% set ns.l = [ns.l] %}{% endfor %}{% set l = ns.l %}");
    // Whether the copy fits depends on the thread's stack and the copier's frame size, which
    // differ between platforms: it either succeeds or stops cleanly
    auto module = tpl.MakeModule();
    if (!module)
    {
        EXPECT_EQ(jinja2::ErrorCode::RecursionLimitExceeded, module.error().GetCode()) << module.error().ToString();
    }
}

TEST_F(TemplateModuleTest, ConcurrentCalls)
{
    auto module = Make("{% set ns = namespace(n=0) %}{% macro inc() %}{% set ns.n = ns.n + 1 %}{{ ns.n }}{% endmacro %}{% macro get() %}{{ ns.n }}{% endmacro %}");
    static constexpr int perThread = 200;
    std::vector<std::thread> threads;
    std::atomic<bool> failed{ false };
    for (int t = 0; t != 4; ++t)
    {
        threads.emplace_back([&module, &failed]() {
            for (int i = 0; i != perThread; ++i)
            {
                if (Call(module.exports["inc"]).AsString().empty())
                {
                    failed = true;
                }
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_FALSE(failed);
    EXPECT_EQ(std::to_string(4 * perThread), Call(module.exports["get"]).AsString());
}

// What a call imports with context ends with the call (docs/tasks/0301). A module imported
// without context is shared by the calls, as Jinja2 keeps a template's default module: its
// namespace counts on, 1 then 2 in Jinja2 3.1.6
TEST_F(TemplateModuleTest, CallsShareModulesImportedWithoutContext)
{
    AddFile("cnt", "{% set ns = namespace(n=0) %}{% macro inc() %}{% set ns.n = ns.n + 1 %}{{ ns.n }}{% endmacro %}");
    auto module = Make("{% macro m() %}{% import 'cnt' as c %}{{ c.inc() }}{% import 'cnt' as c with context %}{{ c.inc() }}{% endmacro %}");
    EXPECT_EQ("11", Call(module.exports["m"]).AsString());
    EXPECT_EQ("21", Call(module.exports["m"]).AsString());
}

// What a shared module's top level imports with context lives as long as that module does:
// 5CM5 on every call, as in Jinja2 3.1.6 (the 0301 verifier)
TEST_F(TemplateModuleTest, SharedModuleKeepsItsImports)
{
    AddFile("c", "{% macro cm() %}CM{{ cv }}{% endmacro %}{% set cv = 5 %}");
    AddFile("b", "{% import 'c' as c with context %}{% macro bm() %}{{ c.cm() }}{% endmacro %}{% set cx = c.cv %}");
    // A module that fails is not shared: what it imported ends with the call
    AddFile("bad", "{% import 'c' as c with context %}{{ 1 // 0 }}");
    auto module = Make("{% macro m() %}{% import 'b' as b %}{{ b.cx }}{{ b.bm() }}{% endmacro %}{% macro f() %}{% import 'bad' as x %}{% endmacro %}");
    for (int i = 0; i != 3; ++i)
    {
        EXPECT_EQ("5CM5", Call(module.exports["m"]).AsString());
        EXPECT_THROW(Call(module.exports["f"]), jinja2::BadResultAccess<>);
    }
}

// A module shared on keeps what it imported when a module that imported it fails: D7 after
// `bad` failed, as in Jinja2 3.1.6 (Codex review of #667)
TEST_F(TemplateModuleTest, FailedImporterLeavesSharedModuleWhole)
{
    AddFile("dep", "{% set dv = 7 %}{% macro m() %}D{{ dv }}{% endmacro %}");
    AddFile("good", "{% import 'dep' as d with context %}{% macro gm() %}{{ d.m() }}{% endmacro %}");
    AddFile("bad", "{% import 'good' as g %}{{ 1 // 0 }}");
    auto module = Make("{% macro f() %}{% import 'bad' as b %}{% endmacro %}{% macro m() %}{% import 'good' as g %}{{ g.gm() }}{% endmacro %}");
    for (int i = 0; i != 2; ++i)
    {
        EXPECT_THROW(Call(module.exports["f"]), jinja2::BadResultAccess<>);
        EXPECT_EQ("D7", Call(module.exports["m"]).AsString());
    }
}

// A macro of a module a call imported with context, stored by that call, runs within it; a
// later call of it fails, naming the export called, as the module ended with the call. A
// documented divergence: Jinja2 keeps the module while the macro reaches it and prints AM
TEST_F(TemplateModuleTest, MacroStoredAcrossCallsExpires)
{
    auto module = Make(
        "{% set ns = namespace() %}"
        "{% macro keep() %}{% import 'a' as a with context %}{% set ns.f = a.am %}{% set ns.g = a.am.__call__ %}{{ ns.f() }}{{ ns.g() }}{% endmacro %}"
        "{% macro use(name) %}{{ ns[name]() }}{% endmacro %}");
    EXPECT_EQ("AMAM", Call(module.exports["keep"]).AsString());
    for (const char* name : { "f", "g" })
    {
        try
        {
            Call(module.exports["use"], { std::string(name) });
            FAIL() << "expected TemplateExpired for ns." << name;
        }
        catch (const jinja2::BadResultAccess<>& failure)
        {
            EXPECT_EQ(jinja2::ErrorCode::TemplateExpired, failure.error().GetCode());
            ASSERT_EQ(2u, failure.error().GetExtraParams().size());
            EXPECT_EQ(jinja2::Value(std::string("use")), failure.error().GetExtraParams()[0]);
            EXPECT_EQ("noname.j2tpl:1:1: error: Macro 'use' called a macro of a module that ended with an earlier call", failure.error().ToString().substr(0, failure.error().ToString().find('\n')));
        }
    }
    // Stored again, it runs again
    EXPECT_EQ("AMAM", Call(module.exports["keep"]).AsString());
}

// A host callable that calls the module again: the inner call ends only what it imported
TEST_F(TemplateModuleTest, NestedCallEndsItsOwnModules)
{
    jinja2::TemplateModule module;
    const jinja2::ValuesMap params{ { "again", jinja2::MakeCallable([&module]() { return Call(module.exports["inner"]); }) } };
    module = Make("{% macro inner() %}{% import 'a' as a with context %}<{{ a.am() }}>{% endmacro %}"
                  "{% macro outer() %}{% import 'a' as a with context %}{{ again() }}{{ a.am() }}{{ again() }}{{ a.ac }}{% endmacro %}",
                  params);
    EXPECT_EQ("<AM>AM<AM>1", Call(module.exports["outer"]).AsString());
    EXPECT_EQ("<AM>AM<AM>1", Call(module.exports["outer"]).AsString());
}

// A call that fails ends what it imported and the scopes its body pushed on the module's
// context; the next one runs as the first
TEST_F(TemplateModuleTest, FailedCallEndsItsModules)
{
    auto module = Make("{% set x = 'top' %}{% set ns = namespace() %}"
                       "{% macro m(fail, x='arg') %}{% import 'a' as a with context %}{% set ns.f = a.am %}{{ a.am() }}{% for i in [1] %}{% set x = 'loop' %}{% if fail %}{{ 1 // 0 }}{% endif %}{% endfor %}{{ x }}{% endmacro %}"
                       "{% macro use() %}{{ ns.f() }}{% endmacro %}{% macro top() %}{{ x }}{% endmacro %}");
    EXPECT_THROW(Call(module.exports["m"], { true }), jinja2::BadResultAccess<>);
    EXPECT_EQ("top", Call(module.exports["top"]).AsString());
    EXPECT_THROW(Call(module.exports["use"]), jinja2::BadResultAccess<>);
    EXPECT_EQ("AMarg", Call(module.exports["m"], { false }).AsString());
    EXPECT_EQ("top", Call(module.exports["top"]).AsString());
}

TEST_F(TemplateModuleTest, Wide)
{
    jinja2::TemplateW tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load(L"{% set x = 'y' %}{% macro m(a) %}[{{ a }}]{% endmacro %}body"));
    auto module = tpl.MakeModule().value();
    EXPECT_EQ(L"body", module.body);
    EXPECT_EQ("m,x", Names(module.exports));
    EXPECT_EQ(L"[w]", Call(module.exports["m"], { std::wstring(L"w") }).AsWString());

    // A macro stored across calls expires there too (docs/tasks/0301)
    ASSERT_TRUE(!!tpl.Load(L"{% set ns = namespace() %}{% macro keep() %}{% import 'a' as a with context %}{% set ns.f = a.am %}{{ ns.f() }}{% endmacro %}"
                           L"{% macro use() %}{{ ns.f() }}{% endmacro %}"));
    auto stored = tpl.MakeModule().value();
    EXPECT_EQ(L"AM", Call(stored.exports["keep"]).AsWString());
    try
    {
        Call(stored.exports["use"]);
        FAIL() << "expected TemplateExpired";
    }
    catch (const jinja2::BadResultAccess<wchar_t>& failure)
    {
        EXPECT_EQ(jinja2::ErrorCode::TemplateExpired, failure.error().GetCode());
        EXPECT_EQ(2u, failure.error().GetExtraParams().size());
    }
}
