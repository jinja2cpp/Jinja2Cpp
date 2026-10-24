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
#ifdef __has_feature
#if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) || __has_feature(undefined_behavior_sanitizer)
#define JINJA2CPP_TEST_SANITIZED 1
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

TEST_F(TemplateModuleTest, LoadExpiresModule)
{
    jinja2::Template tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load("{% macro m() %}old{% endmacro %}"));
    auto module = tpl.MakeModule().value();
    EXPECT_EQ("old", Call(module.exports["m"]).AsString());
    ASSERT_TRUE(!!tpl.Load("{% macro m() %}new{% endmacro %}"));
    try
    {
        Call(module.exports["m"]);
        FAIL() << "expected TemplateExpired";
    }
    catch (const jinja2::BadResultAccess<>& failure)
    {
        EXPECT_EQ(jinja2::ErrorCode::TemplateExpired, failure.error().GetCode());
        EXPECT_EQ(jinja2::Value(std::string("m")), failure.error().GetExtraParams()[0]);
    }
    EXPECT_EQ("new", Call(tpl.MakeModule().value().exports["m"]).AsString());
    // A failed Load keeps the tree, and the modules made from it
    auto current = tpl.MakeModule().value();
    EXPECT_FALSE(!!tpl.Load("{% macro %}"));
    EXPECT_EQ("new", Call(current.exports["m"]).AsString());
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

// A value nested deeper than the stack allows fails as a render of it does
TEST_F(TemplateModuleTest, DeepNesting)
{
#ifdef JINJA2CPP_TEST_SANITIZED
    // LLVM 18's ThreadSanitizer corrupts its trace on a history this long (docs/tasks/0093)
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

TEST_F(TemplateModuleTest, Wide)
{
    jinja2::TemplateW tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load(L"{% set x = 'y' %}{% macro m(a) %}[{{ a }}]{% endmacro %}body"));
    auto module = tpl.MakeModule().value();
    EXPECT_EQ(L"body", module.body);
    EXPECT_EQ("m,x", Names(module.exports));
    EXPECT_EQ(L"[w]", Call(module.exports["m"], { std::wstring(L"w") }).AsWString());
}
