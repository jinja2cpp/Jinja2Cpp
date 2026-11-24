// Template::MakeModule against Python Jinja2 3.1.6's Template.module and make_module (docs/tasks/0284).
// The expected names, bodies and call results are Python's, printed by
// `sorted(k for k in vars(m) if not k.startswith('__'))`, `str(m)` and `m.name(...)`.
#include "test_tools.h"

#include <jinja2cpp/exception.h>
#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/host_object.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#if defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>
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
        AddFile("empty", "");
        AddFile("sets_ac", "{% block b %}{% set am = 1 %}{% endblock %}{% if true %}{% set ac = 2 %}{% endif %}{% macro am() %}{% endmacro %}");
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
    // ... unless set again after it, to the same object too (task 0299)
    { R"({% from "a" import am %}{% set am = 2 %})", "am", "" },
    { R"({% from "a" import am %}{% import "a" as a %}{% set am = am %}{% macro a() %}{% endmacro %})", "a,am", "" },
    { R"({% set am = 1 %}{% from "a" import am %})", "", "" },
    // ... a failed `from` import too, which binds its alias undefined
    { R"({% from "empty" import nope %}{% from "empty" import nope as n2 %}{% set k = 1 %})", "k", "" },
    // ... and a top-level set or macro of the template it extends exports the name again
    { R"({% from "a" import am, ac %}{% extends "sets_ac" %})", "ac,am", "" },
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

namespace
{
struct ObjectsCase
{
    const char* source;
    // What the export `v` is: u undefined, t tuple, p dictsort pairs, k keyed map
    char kind;
};

// clang-format off
const ObjectsCase objectsCases[] = {
    { "{% set v = nothing %}", 'u' },
    { "{% set v = missing %}", 'u' },
    { "{% set v = (1, 2) %}", 't' },
    { "{% set v = {'b': 1, 'a': 2}|dictsort %}", 'p' },
    { "{% set v = {1: 'x', 'k': 2} %}", 'k' },
};
// clang-format on

// The items of a list export that is a GenericList
jinja2::ValuesList ItemsOf(const jinja2::Value& list)
{
    if (const auto* values = list.GetIf<jinja2::ValuesList>())
    {
        return *values;
    }
    jinja2::ValuesList items;
    const auto* generic = list.GetIf<jinja2::GenericList>();
    if (generic)
    {
        for (const auto& item : *generic)
        {
            items.push_back(item);
        }
    }
    return items;
}

jinja2::ModuleOptions ObjectsOptions()
{
    jinja2::ModuleOptions options;
    options.exports = jinja2::ModuleExports::Objects;
    return options;
}
} // namespace

TEST_F(TemplateModuleTest, ObjectsExports)
{
    for (const auto& testCase : objectsCases)
    {
        SCOPED_TRACE(testCase.source);
        auto tpl = Load(testCase.source);
        auto module = tpl.MakeModule({}, {}, ObjectsOptions());
        ASSERT_TRUE(!!module) << module.error().ToString();
        const auto& v = module->exports.at("v");
        switch (testCase.kind)
        {
        case 'u':
            EXPECT_TRUE(jinja2::UndefinedOf(v).has_value());
            break;
        case 't':
        {
            const auto* list = v.GetIf<jinja2::GenericList>();
            ASSERT_NE(nullptr, list);
            EXPECT_TRUE(list->IsTuple());
            EXPECT_EQ((jinja2::ValuesList{ int64_t{ 1 }, int64_t{ 2 } }), ItemsOf(v));
            break;
        }
        case 'p':
        {
            // A list of tuples, which is a list
            EXPECT_NE(nullptr, v.GetIf<jinja2::ValuesList>());
            const auto items = ItemsOf(v);
            ASSERT_EQ(2u, items.size());
            const auto* pair = items[0].GetIf<jinja2::GenericList>();
            ASSERT_NE(nullptr, pair);
            EXPECT_TRUE(pair->IsTuple());
            EXPECT_EQ((jinja2::ValuesList{ "a", int64_t{ 2 } }), ItemsOf(items[0]));
            EXPECT_EQ((jinja2::ValuesList{ "b", int64_t{ 1 } }), ItemsOf(items[1]));
            break;
        }
        case 'k':
        {
            const auto* map = v.GetIf<jinja2::GenericMap>();
            ASSERT_NE(nullptr, map);
            const auto items = map->GetItems();
            ASSERT_EQ(2u, items.size());
            EXPECT_EQ(jinja2::Value(int64_t{ 1 }), items[0].first);
            EXPECT_EQ(jinja2::Value("x"), items[0].second);
            EXPECT_EQ(jinja2::Value("k"), items[1].first);
            // The lookups use the str() spelling
            EXPECT_EQ(jinja2::Value("x"), map->GetValueByName("1"));
            EXPECT_EQ(jinja2::Value(int64_t{ 2 }), map->GetValueByName("k"));
            EXPECT_EQ((std::vector<std::string>{ "1", "k" }), map->GetKeys());
            break;
        }
        default:
            FAIL();
        }
        // The default mode is unchanged
        auto copies = tpl.MakeModule({}, {}, jinja2::ModuleOptions{});
        ASSERT_TRUE(!!copies);
        EXPECT_FALSE(jinja2::UndefinedOf(copies->exports.at("v")).has_value());
    }
}

// What leaves in Objects mode owns its strings: a key from the template's source and a name the undefined carries stay valid
// after the template is loaded with another source and the module's params are gone (ASan checks the views)
TEST_F(TemplateModuleTest, ObjectsExportsOwnTheirStrings)
{
    auto tpl = Load("{% set ds = {'a_key_in_the_source': 'a value in the source'}|dictsort %}{% set u = missing_name_in_source %}{% set t = ('a tuple text', param) %}{% set d = {1: 'k text', 'm': 'v'} %}");
    auto result = [&] {
        jinja2::ValuesMap params;
        params["param"] = std::string("a param text of the host");
        return tpl.MakeModule(params, {}, ObjectsOptions());
    }();
    ASSERT_TRUE(!!result);
    ASSERT_TRUE(!!tpl.Load("{{ 'another source that replaces the old one, with a text that is much longer than the old one' }}"));
    auto& exports = result->exports;
    EXPECT_EQ((jinja2::ValuesList{ "a_key_in_the_source", "a value in the source" }), ItemsOf(ItemsOf(exports.at("ds"))[0]));
    const auto undefined = jinja2::UndefinedOf(exports.at("u"));
    ASSERT_TRUE(undefined.has_value());
    EXPECT_EQ("missing_name_in_source", undefined.value().name);
    EXPECT_EQ((jinja2::ValuesList{ "a tuple text", "a param text of the host" }), ItemsOf(exports.at("t")));
    const auto items = exports.at("d").GetIf<jinja2::GenericMap>()->GetItems();
    EXPECT_EQ(jinja2::Value("k text"), items[0].second);
    EXPECT_EQ(jinja2::Value("v"), items[1].second);
}

TEST_F(TemplateModuleTest, ObjectsExportsWide)
{
    jinja2::TemplateW tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load(L"{% set u = nothing %}{% set t = (1, 'w') %}{% set d = {1: 'x'} %}"));
    auto module = tpl.MakeModule({}, {}, ObjectsOptions());
    ASSERT_TRUE(!!module);
    EXPECT_TRUE(jinja2::UndefinedOf(module->exports.at("u")).has_value());
    EXPECT_TRUE(module->exports.at("t").GetIf<jinja2::GenericList>()->IsTuple());
    EXPECT_EQ(jinja2::Value(int64_t{ 1 }), module->exports.at("d").GetIf<jinja2::GenericMap>()->GetItems()[0].first);
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

namespace
{
struct ModuleStop
    : std::runtime_error
    , jinja2::StopIteration
{
    ModuleStop()
        : std::runtime_error("stop")
    {
    }
};
} // namespace

// Only a call the template makes stops (docs/tasks/0479): a host's call of a module's macro fails with the exception kept
TEST_F(TemplateModuleTest, MacroStoppedByAHostException)
{
    m_env.AddFilter("stop", jinja2::MakeCallable([](const jinja2::Value&) -> jinja2::Value { throw ModuleStop(); }, jinja2::ArgInfo{ "v" }));
    auto module = Make("{% macro m() %}a{{ 1|stop }}{% endmacro %}{% macro o() %}[{{ m() }}]{% endmacro %}");
    try
    {
        Call(module.exports["m"]);
        ADD_FAILURE() << "the call did not fail";
    }
    catch (const jinja2::BadResultAccess<char>& failure)
    {
        EXPECT_EQ(jinja2::ErrorCode::UserCallableException, failure.error().GetCode());
        EXPECT_THROW(std::rethrow_exception(failure.error().GetException()), ModuleStop);
    }
    // The template's own call of it gives undefined
    EXPECT_EQ("[]", Call(module.exports["o"]).AsString());
}

TEST_F(TemplateModuleTest, MacroAttributes)
{
    auto module = Make("{% macro foo(a, b) %}{% endmacro %}{% macro bar() %}{{ varargs }}{{ kwargs }}{% endmacro %}{% macro baz() %}{{ caller() }}{% endmacro %}{% set alias = foo %}{% set n = 1 %}");
    const auto attributes = [&module](const char* name) { return jinja2::MacroAttributes(module.exports[name].As<jinja2::UserCallable>()); };
    const auto foo = attributes("foo");
    EXPECT_EQ("foo", foo.at("name").AsString());
    EXPECT_EQ((jinja2::ValuesList{ "a", "b" }), foo.at("arguments").AsList());
    EXPECT_FALSE(foo.at("catch_kwargs").As<bool>());
    EXPECT_FALSE(foo.at("catch_varargs").As<bool>());
    EXPECT_FALSE(foo.at("caller").As<bool>());
    const auto bar = attributes("bar");
    EXPECT_TRUE(bar.at("arguments").AsList().empty());
    EXPECT_TRUE(bar.at("catch_kwargs").As<bool>());
    EXPECT_TRUE(bar.at("catch_varargs").As<bool>());
    EXPECT_TRUE(attributes("baz").at("caller").As<bool>());
    // An alias is the same macro
    EXPECT_EQ("foo", attributes("alias").at("name").AsString());
    // Any other callable has none
    EXPECT_TRUE(jinja2::MacroAttributes(jinja2::UserCallable{}).empty());
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

// A closure a macro stores in the module's values keeps the macro's names past the host's call,
// read live by the calls that follow (docs/tasks/0373)
TEST_F(TemplateModuleTest, ClosureKeptPastAHostCall)
{
    auto module = Make("{% set ns = namespace(f=none) %}{% macro keep(n) %}{% macro i() %}I{{ n }}{% endmacro %}{% set ns.f = i %}{% endmacro %}"
                       "{% macro later() %}{{ ns.f() }}{% endmacro %}{{ keep('top') }}");
    EXPECT_EQ("Itop", Call(module.exports["later"]).AsString());
    Call(module.exports["keep"], { std::string("host") });
    EXPECT_EQ("Ihost", Call(module.exports["later"]).AsString());
    EXPECT_EQ("Ihost", Call(module.exports["later"]).AsString());
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
// (docs/tasks/0321, which reverses the second half of 0284 D1a)
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

// A macro or `caller` of another render given to an export runs in that render while the call
// lasts; called later, it fails with TemplateExpired (docs/tasks/0300)
TEST_F(TemplateModuleTest, TemplateCallableArguments)
{
    auto module = Make(
        "{% set ns = namespace(c=none) %}"
        "{% macro wrap() %}<{{ caller() }}>{% endmacro %}"
        "{% macro apply(fn) %}{{ fn(1, k=2) }}{% endmacro %}"
        "{% macro count(fn) %}{{ fn(3) | list | length }}{% endmacro %}"
        "{% macro keep() %}{% set ns.c = caller %}{{ caller() }}{% endmacro %}"
        "{% macro later() %}{{ ns.c() }}{% endmacro %}");
    const jinja2::ValuesMap params{ { "mod", module.exports }, { "l", jinja2::ValuesList{ 1 } } };
    EXPECT_EQ("<X2>", Render("{% set y = 2 %}{% call mod.wrap() %}X{{ y }}{% endcall %}", params));
    EXPECT_EQ("[1|2]", Render("{% macro f(a, k=0) %}[{{ a }}|{{ k }}]{% endmacro %}{{ mod.apply(f) }}", params));
    // A builtin callable goes the same way
    EXPECT_EQ("3", Render("{{ mod.count(range) }}", params));
    // The caller's writes are its render's (its collector is current): the host's list is
    // copied once, and the render sees the copy
    EXPECT_EQ("<>[1, 3]", Render("{% call mod.wrap() %}{% set _ = l.append(3) %}{% endcall %}{{ l }}", params));
    EXPECT_EQ("K", Render("{% call mod.keep() %}K{% endcall %}", params));
    try
    {
        Call(module.exports["later"]);
        FAIL() << "expected TemplateExpired";
    }
    catch (const jinja2::BadResultAccess<>& failure)
    {
        EXPECT_EQ(jinja2::ErrorCode::TemplateExpired, failure.error().GetCode());
        ASSERT_EQ(1u, failure.error().GetExtraParams().size());
        EXPECT_EQ(jinja2::Value(std::string("lent")), failure.error().GetExtraParams()[0]);
        EXPECT_NE(std::string::npos, failure.error().ToString().find("A template callable was called after the host's call it was passed to ended"));
    }
}

// A lent callable runs under the limits of the render that lent it, not of the module's call
// that calls it back (Codex review of 0300), and what it uses of the budgets counts for that render
TEST_F(TemplateModuleTest, LentCallableKeepsRenderLimits)
{
    auto module = Make("{% macro wrap() %}<{{ caller() }}>{% endmacro %}{% macro rep() %}{% for i in range(20) %}{{ caller() }}{% endfor %}{% endmacro %}");
    const jinja2::ValuesMap params{ { "mod", module.exports } };
    jinja2::Settings settings;
    settings.limits.maxSequenceSize = 16;
    settings.limits.maxRenderSteps = 1000;
    jinja2::TemplateEnv env;
    env.SetSettings(settings);
    const auto render = [&env, &params](const char* source) {
        jinja2::Template tpl(&env);
        EXPECT_TRUE(!!tpl.Load(source));
        return tpl.RenderAsString(params);
    };
    EXPECT_EQ("<10>", render("{% call mod.wrap() %}{{ ('x' * 10)|length }}{% endcall %}").value());
    const auto expectLimit = [](const jinja2::Result<std::string>& result) {
        ASSERT_FALSE(!!result);
        EXPECT_EQ(jinja2::ErrorCode::ResourceLimitExceeded, result.error().GetCode());
    };
    expectLimit(render("{% call mod.wrap() %}{{ ('x' * 1000)|length }}{% endcall %}"));
    // The steps of every call back count
    expectLimit(render("{% call mod.rep() %}{% for j in range(100) %}{% endfor %}{% endcall %}"));
    // ... and stay counted once the module's call returns
    EXPECT_TRUE(!!render("{% call mod.wrap() %}{% for j in range(400) %}{% endfor %}{% endcall %}"));
    expectLimit(render("{% call mod.wrap() %}{% for j in range(600) %}{% endfor %}{% endcall %}{% for j in range(600) %}{% endfor %}"));
}

// A host that catches what a lent callable throws and goes on finds the render's scopes as they
// were (verifier round 1 of 0300): the failed body's scope and names are gone
TEST_F(TemplateModuleTest, LentCallableFailureCaught)
{
    jinja2::UserCallable catcher(
        [](const jinja2::UserCallableParams& params) {
            try
            {
                Call(params.args.at("f"));
            }
            catch (...) // NOLINT(bugprone-empty-catch): the test checks what is left after it
            {
            }
            return jinja2::Value(std::string("caught"));
        },
        { jinja2::ArgInfo("f") });
    const jinja2::ValuesMap params{ { "catcher", catcher } };
    EXPECT_EQ("caught7False", Render("{% set y = 7 %}{% macro bad() %}{% set z = 3 %}{{ 1 // 0 }}{% endmacro %}{{ catcher(bad) }}{{ y }}{{ z is defined }}", params));
    EXPECT_EQ("caught5", Render("{% macro bad() %}{{ 1 // 0 }}{% endmacro %}{% macro outer() %}{% set q = 5 %}{{ catcher(bad) }}{{ q }}{% endmacro %}{{ outer() }}", params));
    EXPECT_EQ("[caught]", Render("{% macro w() %}[{{ catcher(caller) }}]{% endmacro %}{% call w() %}{% set z = 1 %}{{ 1 // 0 }}{% endcall %}", params));
}

// A macro nested in an exported value, or returned by a call, is a call in the module too
// (docs/tasks/0300); a host's callable goes back as it came
TEST_F(TemplateModuleTest, NestedCallables)
{
    jinja2::UserCallable host([](const jinja2::UserCallableParams&) { return jinja2::Value(std::string("H")); }, {});
    auto module = Make("{% macro m(a) %}M{{ a }}{% endmacro %}{% set d = {'m': m, 'l': [m], 'h': h} %}{% macro pass(x) %}{{ x(1) }}{% endmacro %}", { { "h", host } });
    const auto& dict = module.exports["d"].AsMap();
    EXPECT_EQ("M1", Call(dict.at("m"), { 1 }).AsString());
    EXPECT_EQ("M2", Call(dict.at("l").AsList()[0], { 2 }).AsString());
    ASSERT_TRUE(dict.at("h").IsCallable());
    EXPECT_EQ("H", Call(dict.at("h")).AsString());
    // A host's callable a template passes to another host's callable is the same callable
    jinja2::UserCallable check([&host](const jinja2::UserCallableParams& params) { return jinja2::Value(params.args.at("f").As<jinja2::UserCallable>().IsEqual(host)); }, { jinja2::ArgInfo("f") });
    EXPECT_EQ("True", Render("{{ check(h) }}", { { "check", check }, { "h", host } }));
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

// A cycle a call makes below a value of the module that is in none is freed with the module (LSan)
TEST_F(TemplateModuleTest, CycleMadeInACall)
{
    auto module = Make("{% set l = [{'a': []}] %}{% macro c() %}{% set _ = l[0].a.append(l[0]) %}{{ l[0].a|length }}{% endmacro %}");
    EXPECT_EQ("1", Call(module.exports["c"]).AsString());
    EXPECT_EQ("2", Call(module.exports["c"]).AsString());
}

// A call that stores into a container the module holds does not walk all of it: calls stay
// linear in their count, not quadratic (docs/tasks/0450). Timed: n calls against 8n, where a
// walk of the whole container per call takes 64 times as long
TEST_F(TemplateModuleTest, StoresIntoModuleContainersStayLinear)
{
#ifdef JINJA2CPP_CELL_TRACE
    // 20000 calls log some 270000 cell events, more than TLC checks as one trace (tla/check.py
    // --trace); the build times nothing, and the other module tests trace the same steps
    GTEST_SKIP() << "a timing test: its trace is too long for TLC";
#endif
    const char* const sources[] = {
        "{% set cache = [] %}{% macro m(a, k) %}{% set _ = cache.append([1]) %}{% endmacro %}",
        "{% set cache = [] %}{% macro m(a, k) %}{% set _ = cache.append({'n': a.name}) %}{% endmacro %}",
        "{% set cache = [] %}{% macro m(a, k) %}{% set _ = cache.append(a.items) %}{% endmacro %}",
        // A string that views the text of the list it was read from, which it keeps alive
        "{% set cache = [] %}{% macro m(a, k) %}{% set _ = cache.append(a.items[0]) %}{% endmacro %}",
        "{% set cache = {} %}{% macro m(a, k) %}{% set _ = cache.update({k: [a.n]}) %}{% endmacro %}",
        "{% set ns = namespace(log=[]) %}{% macro m(a, k) %}{% set _ = ns.log.append([a.n]) %}{% endmacro %}",
    };
    jinja2::ValuesMap arg;
    arg["name"] = std::string("a name long enough to live on the heap");
    arg["n"] = int64_t{ 1 };
    jinja2::ValuesList items;
    items.emplace_back(std::string("an item long enough to live on the heap"));
    arg["items"] = std::move(items);
    for (const auto* source : sources)
    {
        auto time = [this, source, &arg](int calls) {
            auto module = Make(source);
            const auto& fn = module.exports["m"];
            const auto start = std::chrono::steady_clock::now();
            for (int call = 0; call != calls; ++call)
            {
                jinja2::ValuesList pos;
                pos.emplace_back(arg);
                pos.emplace_back(int64_t{ call });
                Call(fn, std::move(pos));
            }
            return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
        };
        // Each the best of three, and the bound far from both ratios (linear is 8 times as long,
        // quadratic 64), so that a shared runner's stalls neither fail a linear walk nor pass a
        // quadratic one: 4n against n at a bound of 10 failed once on macOS
        const auto small = std::min({ time(500), time(500), time(500) });
        const auto large = std::min({ time(4000), time(4000), time(4000) });
        EXPECT_LT(large, small * 24) << source << ": 500 calls " << small << " s, 4000 calls " << large << " s";
    }
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

// A macro of a module a call imported with context, stored by that call, keeps the module while a value of the module
// reaches it, as Jinja2 does (docs/tasks/0312): AMAM, AM, AM in 3.1.6. So do a macro defined in a macro of that module,
// the module itself, and a macro of a module it imported with context (BAM, IN1, AM, BAM in 3.1.6)
TEST_F(TemplateModuleTest, MacroStoredAcrossCallsKeepsItsModule)
{
    AddFile("b", "{% import 'a' as a2 with context %}{% macro bm() %}B{{ a2.am() }}{% endmacro %}");
    AddFile("inner", "{% set ac = 1 %}{% macro im() %}{% macro inn() %}IN{{ ac }}{% endmacro %}{% set ns.i = inn %}{% endmacro %}");
    auto module = Make(
        "{% set ns = namespace() %}"
        "{% macro keep() %}{% import 'a' as a with context %}{% set ns.f = a.am %}{% set ns.g = a.am.__call__ %}{{ ns.f() }}{{ ns.g() }}{% endmacro %}"
        "{% macro keepMore() %}{% import 'b' as b with context %}{% import 'inner' as n with context %}{% import 'a' as a with context %}"
        "{% set ns.b = b.bm %}{{ n.im() }}{% set ns.l = [{'m': a}] %}{% endmacro %}"
        "{% macro use(name) %}{{ ns[name]() }}{% endmacro %}{% macro useModule() %}{{ ns.l[0].m.am() }}{% endmacro %}");
    EXPECT_EQ("AMAM", Call(module.exports["keep"]).AsString());
    EXPECT_EQ("AM", Call(module.exports["use"], { std::string("f") }).AsString());
    EXPECT_EQ("AM", Call(module.exports["use"], { std::string("g") }).AsString());
    EXPECT_EQ("", Call(module.exports["keepMore"]).AsString());
    EXPECT_EQ("BAM", Call(module.exports["use"], { std::string("b") }).AsString());
    EXPECT_EQ("IN1", Call(module.exports["use"], { std::string("i") }).AsString());
    EXPECT_EQ("AM", Call(module.exports["useModule"]).AsString());
    // Stored again, it runs again; the first `a` the store replaced ends
    EXPECT_EQ("AMAM", Call(module.exports["keep"]).AsString());
    EXPECT_EQ("AM", Call(module.exports["use"], { std::string("f") }).AsString());
    EXPECT_EQ("BAM", Call(module.exports["use"], { std::string("b") }).AsString());
}

// A module whose top level failed partway, inside a loop and a `with`, after storing its macro, stays for that macro with
// the scopes the failure left: G1 on each later call, as in Jinja2 3.1.6 (docs/tasks/0312, security review)
TEST_F(TemplateModuleTest, MacroStoredByAFailedImportKeepsItsModule)
{
    AddFile("half", "{% macro g() %}G{{ v }}{% endmacro %}{% set v = 1 %}{% set ns.f = g %}{% for i in [1] %}{% with w = 2 %}{{ 1 // 0 }}{% endwith %}{% endfor %}");
    auto module = Make("{% set ns = namespace(f=none) %}{% macro keep() %}{% import 'half' as h with context %}{% endmacro %}{% macro use() %}{{ ns.f() }}{% endmacro %}");
    for (int i = 0; i != 3; ++i)
    {
        EXPECT_THROW(Call(module.exports["keep"]), jinja2::BadResultAccess<>);
        EXPECT_EQ("G1", Call(module.exports["use"]).AsString());
        EXPECT_EQ("G1", Call(module.exports["use"]).AsString());
    }
}

// With EveryUse and no cache a module drops the trees a source change replaced when a call ends (docs/tasks/0482).
// What a call stored from one still runs it, as Jinja2 keeps the old Template alive: the next call prints the old
// output, the one after it the new (3.1.6; the rows are its make_module() results)
class TemplateModuleChangingSourceTest : public TemplateModuleTest
{
protected:
    void SetUp() override
    {
        TemplateModuleTest::SetUp();
        m_env.GetSettings().templateLookup = jinja2::TemplateLookup::EveryUse;
        m_env.GetSettings().cacheSize = 0;
    }

    // Calls `m` once for each text of `lib`, which the file system serves from the call on
    std::vector<std::string> CallWithLibs(const std::string& source, const std::vector<std::string>& libs)
    {
        AddFile("lib", libs.front());
        auto module = Make(source);
        std::vector<std::string> results;
        for (const auto& lib : libs)
        {
            AddFile("lib", lib);
            results.push_back(Call(module.exports["m"]).AsString());
        }
        return results;
    }
};

TEST_F(TemplateModuleChangingSourceTest, StoredMacroOfIncludeRunsItsOldTree)
{
    const auto results = CallWithLibs("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                      { "{% macro x() %}OLD{% endmacro %}{% set ns.f = x %}",
                                        "{% macro x() %}NEW{% endmacro %}{% set ns.f = x %}",
                                        "{% macro x() %}NEW2{% endmacro %}{% set ns.f = x %}" });
    EXPECT_EQ((std::vector<std::string>{ "[]", "[OLD]", "[NEW]" }), results);
}

TEST_F(TemplateModuleChangingSourceTest, StoredSelfRunsItsOldBlocks)
{
    const auto results = CallWithLibs("{% set ns = namespace(s=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s.b() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                      { "{% block b %}OLD{% endblock %}{% set ns.s = self %}",
                                        "{% block b %}NEW{% endblock %}{% set ns.s = self %}",
                                        "{% block b %}NEW2{% endblock %}{% set ns.s = self %}" });
    EXPECT_EQ((std::vector<std::string>{ "[]OLD", "[OLD]NEW", "[NEW]NEW2" }), results);
}

// A `self` an include's top level puts in a list or dict, and that a later call reads through a namespace, keeps the
// include's frame as Jinja2 keeps its context (docs/tasks/0542; the rows are 3.1.6 make_module() results)
TEST_F(TemplateModuleChangingSourceTest, SelfInAContainerOutlivesTheInclude)
{
    const std::string selfMain = "{% set ns = namespace(s=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s.b() }}{% endif %}]{% include 'lib' %}{% endmacro %}";
    const std::string head = "{% set t='T1' %}{% block b %}B1{{ t }}{% endblock %}";
    const std::vector<std::string> selfTail = { "{% block b %}B2{% endblock %}", "{% block b %}B3{% endblock %}" };
    const std::vector<std::string> selfWanted = { "[]B1T1", "[B1T1]B2", "[B1T1]B3" };
    for (const char* store : { "{% set l = [self] %}{% set ns.s = l[0] %}", "{% set d = {'s': self} %}{% set ns.s = d.s %}",
                               "{% set d = {'k': [self]} %}{% set ns.s = d.k[0] %}" })
    {
        EXPECT_EQ(selfWanted, CallWithLibs(selfMain, { head + store, selfTail[0], selfTail[1] })) << store;
    }

    // An alias of the list and an append share the one list
    EXPECT_EQ((std::vector<std::string>{ "[]B1T1", "[B1T11]B2", "[B1T12]B3", "[B1T13]B4" }),
              CallWithLibs("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                           { head + "{% set l = [self] %}{% set k = l %}{% macro f() %}{{ l[0].b() }}{{ k|length }}{% set _ = l.append(1) %}{% endmacro %}{% set ns.f = f %}",
                             "{% block b %}B2{% endblock %}", "{% block b %}B3{% endblock %}", "{% block b %}B4{% endblock %}" }));

    // `l[0]` stored by a call is read after the closure that stored it is gone
    EXPECT_EQ((std::vector<std::string>{ "[|]B1T1", "[F|B1T1]B2", "[|B1T1]B3", "[|B1T1]B4" }),
              CallWithLibs("{% set ns = namespace(f=none, s=none, n=0) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}|{% if ns.s %}{{ ns.s.b() }}{% endif %}]"
                           "{% include 'lib' %}{% set ns.n = ns.n + 1 %}{% endmacro %}",
                           { head + "{% set l = [self] %}{% macro f() %}{% set ns.s = l[0] %}F{% endmacro %}{% set ns.f = f %}",
                             "{% block b %}B2{% endblock %}{% set ns.f = none %}", "{% block b %}B3{% endblock %}", "{% block b %}B4{% endblock %}" }));
}

// A list too large for the check at the call's end to walk (more than its 256 objects or 4,096 items) is kept, and
// reads as above
TEST_F(TemplateModuleChangingSourceTest, SelfInALargeListOutlivesTheInclude)
{
    const auto results = CallWithLibs("{% set ns = namespace(s=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s.b() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                      { "{% set t='T1' %}{% block b %}B1{{ t }}{% endblock %}{% set l = [self] %}{% for i in range(300) %}{% set _ = l.append([i]) %}"
                                        "{% endfor %}{% set ns.s = l[0] %}{{ l|length }}",
                                        "{% block b %}B2{% endblock %}", "{% block b %}B3{% endblock %}" });
    EXPECT_EQ((std::vector<std::string>{ "[]B1T1301", "[B1T1]B2", "[B1T1]B3" }), results);
    // More items than it visits (4,096)
    const auto longList = CallWithLibs("{% set ns = namespace(s=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s.b() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                       { "{% set t='T1' %}{% block b %}B1{{ t }}{% endblock %}{% set l = [self] + range(5000)|list %}{% set ns.s = l[0] %}{{ l|length }}",
                                         "{% block b %}B2{% endblock %}", "{% block b %}B3{% endblock %}" });
    EXPECT_EQ((std::vector<std::string>{ "[]B1T15001", "[B1T1]B2", "[B1T1]B3" }), longList);
}

TEST_F(TemplateModuleChangingSourceTest, WideSelfInAListOutlivesTheInclude)
{
    const std::string head = "{% set t='T1' %}{% block b %}B1{{ t }}{% endblock %}";
    const std::vector<std::string> libs = { head + "{% set l = [self] %}{% set ns.s = l[0] %}", "{% block b %}B2{% endblock %}", "{% block b %}B3{% endblock %}" };
    AddFile("lib", libs.front());
    jinja2::TemplateW tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load(L"{% set ns = namespace(s=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s.b() }}{% endif %}]{% include 'lib' %}{% endmacro %}"));
    auto module = tpl.MakeModule().value();
    std::vector<std::wstring> results;
    for (const auto& lib : libs)
    {
        AddFile("lib", lib);
        results.push_back(Call(module.exports["m"]).AsWString());
    }
    EXPECT_EQ((std::vector<std::wstring>{ L"[]B1T1", L"[B1T1]B2", L"[B1T1]B3" }), results);
}

// Trees that take a slot go too once no `self`, `super` or module holds a handle of one (docs/tasks/0503): the
// module's own blocks keep the slots, a stored `super` or `self` keeps them while it lives (and, until 0529, for
// the module's life), and a parent that changes under a child that extends it is the new one each call (3.1.6
// make_module() results)
TEST_F(TemplateModuleChangingSourceTest, ModuleWithBlocksIncludesChangingBlocks)
{
    const auto results = CallWithLibs("{% block t %}T{% endblock %}{% macro m() %}{{ self.t() }}{% include 'lib' %}{% endmacro %}",
                                      { "{% block b %}L1{% endblock %}", "{% block b %}L2{% endblock %}", "{% block b %}L3{% endblock %}" });
    EXPECT_EQ((std::vector<std::string>{ "TL1", "TL2", "TL3" }), results);
}

// The module's handles: a block inline in its macro, and one a plain import's module holds, keep the slots
TEST_F(TemplateModuleChangingSourceTest, BlockInAModuleMacroKeepsTheSlots)
{
    const auto results = CallWithLibs("{% macro m() %}{% block t %}T{% endblock %}{% include 'lib' %}{% endmacro %}", { "L1", "L2", "L3" });
    EXPECT_EQ((std::vector<std::string>{ "TL1", "TL2", "TL3" }), results);
}

TEST_F(TemplateModuleChangingSourceTest, BlockOfASharedModuleKeepsTheSlots)
{
    AddFile("lib", "{% macro x() %}{% block b %}B1{% endblock %}{% endmacro %}");
    AddFile("child", "C0");
    auto module = Make("{% macro m() %}{% import 'lib' as l %}{{ l.x() }}{% include 'child' %}{% endmacro %}");
    std::vector<std::string> results;
    for (const char* child : { "C0", "C1", "C2", "C3" })
    {
        AddFile("child", child);
        results.push_back(Call(module.exports["m"]).AsString());
    }
    EXPECT_EQ((std::vector<std::string>{ "B1C0", "B1C1", "B1C2", "B1C3" }), results);
}

TEST_F(TemplateModuleChangingSourceTest, StoredSuperRunsItsOldParent)
{
    AddFile("base", "<{% block b %}B{% endblock %}>");
    const auto results = CallWithLibs("{% set ns = namespace(s=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                      { "{% extends 'base' %}{% block b %}{% set ns.s = super %}OLD{% endblock %}",
                                        "{% extends 'base' %}{% block b %}{% set ns.s = super %}NEW{% endblock %}",
                                        "{% extends 'base' %}{% block b %}{% set ns.s = none %}NEW2{% endblock %}", "{% extends 'base' %}{% block b %}NEW3{% endblock %}" });
    EXPECT_EQ((std::vector<std::string>{ "[]<OLD>", "[B]<NEW>", "[B]<NEW2>", "[]<NEW3>" }), results);
}

TEST_F(TemplateModuleChangingSourceTest, ChildOfAChangingParent)
{
    AddFile("child", "{% extends 'lib' %}{% block b %}C{{ super() }}{% endblock %}");
    const auto results = CallWithLibs("{% macro m() %}{% include 'child' %}{% endmacro %}",
                                      { "<{% block b %}L1{% endblock %}>", "<{% block b %}L2{% endblock %}>", "[{% block b %}L3{% endblock %}]" });
    EXPECT_EQ((std::vector<std::string>{ "<CL1>", "<CL2>", "[CL3]" }), results);
}

// A module imported without context goes with its replaced tree once nothing else holds it (docs/tasks/0528): what a
// call stored of it, a macro, the module object or a macro its macro made, still runs with the module's names
TEST_F(TemplateModuleChangingSourceTest, StoredMacroOfASharedModuleSeesItsNames)
{
    const std::vector<std::string> libs = { "{% set v = 'V1' %}{% macro x() %}OLD{{ v }}{% endmacro %}", "{% set v = 'V2' %}{% macro x() %}NEW{{ v }}{% endmacro %}",
                                            "{% set v = 'V3' %}{% macro x() %}NEW2{{ v }}{% endmacro %}" };
    EXPECT_EQ((std::vector<std::string>{ "[]", "[OLDV1]", "[NEWV2]" }),
              CallWithLibs("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% import 'lib' as l %}{% set ns.f = l.x %}{% endmacro %}", libs));
    EXPECT_EQ((std::vector<std::string>{ "[]", "[OLDV1]", "[NEWV2]" }),
              CallWithLibs("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% from 'lib' import x %}{% set ns.f = x %}{% endmacro %}", libs));
    EXPECT_EQ((std::vector<std::string>{ "[]", "[OLDV1V1]", "[NEWV2V2]" }),
              CallWithLibs("{% set ns = namespace(l=none) %}{% macro m() %}[{% if ns.l %}{{ ns.l.x() }}{{ ns.l.v }}{% endif %}]{% import 'lib' as l %}{% set ns.l = l %}{% endmacro %}", libs));
    // Stored by the first call only: its module stays while the macro does
    auto four = libs;
    four.emplace_back("{% set v = 'V4' %}{% macro x() %}NEW3{{ v }}{% endmacro %}");
    EXPECT_EQ((std::vector<std::string>{ "[]", "[OLDV1]", "[OLDV1]", "[OLDV1]" }),
              CallWithLibs("{% set ns = namespace(f=none, n=0) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% import 'lib' as l %}"
                           "{% if ns.n == 0 %}{% set ns.f = l.x %}{% endif %}{% set ns.n = ns.n + 1 %}{% endmacro %}",
                           four));
    // A macro the module's macro made and stored
    EXPECT_EQ((std::vector<std::string>{ "[]", "[Y1V1]", "[Y2V2]" }),
              CallWithLibs("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% import 'lib' as l %}{% set _ = l.mk(ns) %}{% endmacro %}",
                           { "{% set v = 'V1' %}{% macro mk(ns) %}{% macro y() %}Y1{{ v }}{% endmacro %}{% set ns.f = y %}{% endmacro %}",
                             "{% set v = 'V2' %}{% macro mk(ns) %}{% macro y() %}Y2{{ v }}{% endmacro %}{% set ns.f = y %}{% endmacro %}",
                             "{% set v = 'V3' %}{% macro mk(ns) %}{% macro y() %}Y3{{ v }}{% endmacro %}{% set ns.f = y %}{% endmacro %}" }));
}

// The same for a module whose top level imports a helper, which goes with it (docs/tasks/0541): what a call stored of
// either still runs with its names, while the helper changes too or stays shared
TEST_F(TemplateModuleChangingSourceTest, StoredMacroOfAnImportingModuleSeesItsHelper)
{
    struct Row
    {
        const char* importer;
        const char* store;
    };
    for (const auto& [importer, store] : { Row{ "{% import 'helper' as h %}", "l.x" }, Row{ "{% from 'helper' import y %}", "l.x" },
                                           Row{ "{% import 'helper' as h with context %}", "l.x" }, Row{ "{% import 'helper' as h %}{% set hy = h.y %}", "l.hy" },
                                           Row{ "{% import 'helper' as h %}", "l" } })
    {
        const std::string uses = std::string(importer).find("from") != std::string::npos ? "{{ y() }}" : "{{ h.y() }}";
        const std::string call = std::string(store) == "l" ? "ns.f.x()" : "ns.f()";
        std::string source = "{% set ns = namespace(f=none, n=0) %}{% macro m() %}[{% if ns.f %}{{ ";
        source += call;
        source += " }}{% endif %}]{% import 'lib' as l %}{% if ns.n == 0 %}{% set ns.f = ";
        source += store;
        source += " %}{% endif %}{% set ns.n = ns.n + 1 %}{{ l.x() }}{% endmacro %}";
        auto module = Make(source);
        for (const bool helperChanges : { true, false })
        {
            std::vector<std::string> results;
            for (int i = 1; i != 5; ++i)
            {
                const auto n = std::to_string(i);
                std::string helper = "{% set hv = 'H";
                helper += helperChanges ? n : "0";
                helper += "' %}{% macro y() %}{{ hv }}{% endmacro %}";
                AddFile("helper", helper);
                std::string lib = importer;
                lib += "{% set v = 'V";
                lib += n;
                lib += "' %}{% macro x() %}{{ v }}";
                lib += uses;
                lib += "{% endmacro %}";
                AddFile("lib", lib);
                try
                {
                    results.push_back(Call(module.exports["m"]).AsString());
                }
                catch (const jinja2::BadResultAccess<>& failure)
                {
                    results.push_back("error: " + failure.error().ToString());
                }
            }
            const bool helperOnly = std::string(store) == "l.hy";
            const std::string first = helperOnly ? "H1" : "V1H1";
            const auto h = [helperChanges](int i) { return "H" + std::to_string(helperChanges ? i : 0); };
            if (!helperChanges)
            {
                // Stored by the first loop already
                const std::string kept = helperOnly ? "H1" : "V1H1";
                EXPECT_EQ((std::vector<std::string>{ "[" + kept + "]V1" + h(1), "[" + kept + "]V2" + h(2), "[" + kept + "]V3" + h(3), "[" + kept + "]V4" + h(4) }), results)
                    << importer << store;
            }
            else
            {
                EXPECT_EQ((std::vector<std::string>{ "[]V1" + h(1), "[" + first + "]V2" + h(2), "[" + first + "]V3" + h(3), "[" + first + "]V4" + h(4) }), results)
                    << importer << store;
            }
        }
    }
}

// A `super` of a block with no parent block, stored and called then and by later calls, fails naming the block,
// as Jinja2's undefined does, while the module frees the tree it was taken in (docs/tasks/0503)
TEST_F(TemplateModuleChangingSourceTest, StoredSuperWithNoParentNamesItsBlock)
{
    // Stored by the second call: the tree of the first stays for the module's life
    AddFile("lib", "{% block b %}OLD{% endblock %}");
    auto module = Make("{% set ns = namespace(s=none) %}{% macro m() %}{% include 'lib' %}[{% if ns.s is not none %}{{ ns.s() }}{% endif %}]{% endmacro %}");
    EXPECT_EQ("OLD[]", Call(module.exports["m"]).AsString());
    for (const char* lib : { "{% block b %}{% set ns.s = super %}S{% endblock %}", "{% block b %}NEW{% endblock %}", "{% block b %}NEW2{% endblock %}",
                             "{% block b %}NEW3{% endblock %}" })
    {
        AddFile("lib", lib);
        try
        {
            Call(module.exports["m"]);
            FAIL() << "expected the stored super() to fail";
        }
        catch (const jinja2::BadResultAccess<>& failure)
        {
            EXPECT_NE(std::string::npos, failure.error().ToString().find("there is no parent block called 'b'.")) << failure.error().ToString();
        }
    }
}

TEST_F(TemplateModuleChangingSourceTest, StoredSelfLetGoOfStopsRendering)
{
    const auto results = CallWithLibs("{% set ns = namespace(s=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s.b() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                      { "{% block b %}OLD{% endblock %}{% set ns.s = self %}", "{% block b %}NEW{% endblock %}{% set ns.s = none %}",
                                        "{% block b %}NEW2{% endblock %}", "{% block b %}NEW3{% endblock %}" });
    EXPECT_EQ((std::vector<std::string>{ "[]OLD", "[OLD]NEW", "[]NEW2", "[]NEW3" }), results);
}

// A `self` the included template's top level set, which a stored macro reads or the call stores, renders the old
// blocks: the copy of the include's names holds it without its capture, which it keeps again when read (0529)
TEST_F(TemplateModuleChangingSourceTest, SelfSetAtTheTopLevelOutlivesTheInclude)
{
    EXPECT_EQ((std::vector<std::string>{ "[]B1", "[B1T1True]B2", "[B2T2]B3" }),
              CallWithLibs("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                           { "{% block b %}B1{% endblock %}{% set q = self %}{% set t = 'T1' %}{% macro f() %}{{ q.b() }}{{ t }}{{ q is defined }}{% endmacro %}{% set ns.f = f %}",
                             "{% block b %}B2{% endblock %}{% set q = self %}{% set t = 'T2' %}{% macro f() %}{{ q.b() }}{{ t }}{% endmacro %}{% set ns.f = f %}",
                             "{% block b %}B3{% endblock %}" }));
    EXPECT_EQ((std::vector<std::string>{ "[]B1T1", "[B1T1]B2", "[B1T1]B3" }),
              CallWithLibs("{% set ns = namespace(s=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s.b() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                           { "{% set t='T1' %}{% block b %}B1{{ t }}{% endblock %}{% set q = self %}{% set ns.s = q %}", "{% block b %}B2{% endblock %}",
                             "{% block b %}B3{% endblock %}" }));
}

// Two names holding one `self` hold one object once the include has ended, in a render and in a module's call
TEST_F(TemplateModuleChangingSourceTest, SelfSetTwiceStaysOneObject)
{
    AddFile("lib", "{% block b %}B0{% endblock %}{% set q = self %}{% set r = self %}{% macro f() %}{{ q is sameas r }}|{{ q == r }}{% endmacro %}{% set ns.f = f %}");
    jinja2::Template tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load("{% set ns = namespace(f=none) %}{% include 'lib' %}|{{ ns.f() }}"));
    EXPECT_EQ("B0|True|True", tpl.RenderAsString({}).value());
    jinja2::TemplateW wide(&m_env);
    ASSERT_TRUE(!!wide.Load(L"{% set ns = namespace(f=none) %}{% include 'lib' %}|{{ ns.f() }}"));
    EXPECT_EQ(L"B0|True|True", wide.RenderAsString({}).value());
    auto module = Make("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% include 'lib' %}{% endmacro %}");
    EXPECT_EQ("[]B0", Call(module.exports["m"]).AsString());
    EXPECT_EQ("[True|True]B0", Call(module.exports["m"]).AsString());
    // And the same object as a copy stored outside the include (Codex review of #946)
    AddFile("lib", "{% block b %}B{% endblock %}{% set q = self %}{% set ns.s = q %}{% macro f() %}{{ q is sameas ns.s }}|{{ q == ns.s }}{% endmacro %}{% set ns.f = f %}");
    jinja2::Template stored(&m_env);
    ASSERT_TRUE(!!stored.Load("{% set ns = namespace(s=none, f=none) %}{% include 'lib' %}|{{ ns.f() }}"));
    EXPECT_EQ("B|True|True", stored.RenderAsString({}).value());
}

TEST_F(TemplateModuleChangingSourceTest, StoredRecursiveLoopRunsItsOldBody)
{
    const std::string body = "{{ i }}{{ loop(i) if i is iterable and i is not string }}";
    const auto results = CallWithLibs("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f([[2],[3]]) }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                      { "{% for i in [[1]] recursive %}{% if loop.depth == 1 %}{% set ns.f = loop %}{% endif %}" + body + "{% endfor %}",
                                        "{% for i in [[1]] recursive %}{% if loop.depth == 1 %}{% set ns.f = loop %}{% endif %}A" + body + "{% endfor %}",
                                        "{% for i in [[1]] recursive %}{% if loop.depth == 1 %}{% set ns.f = loop %}{% endif %}B" + body + "{% endfor %}" });
    EXPECT_EQ((std::vector<std::string>{ "[][1]1", "[[2]2[3]3]A[1]A1", "[A[2]A2A[3]A3]B[1]B1" }), results);
}

// A stored recursive loop whose body stores it on its first item runs past its include: the body's names resolve
// where the loop was defined, not to whatever the later call sees (docs/tasks/0502)
TEST_F(TemplateModuleChangingSourceTest, StoredRecursiveLoopSetsANamespaceAttribute)
{
    const std::string body = "{{ i }}{{ loop(i) if i is iterable and i is not string }}";
    const auto results = CallWithLibs("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f([[2],[3]]) }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                      { "{% for i in [[1]] recursive %}{% if loop.first %}{% set ns.f = loop %}{% endif %}" + body + "{% endfor %}",
                                        "{% for i in [[1]] recursive %}{% if loop.first %}{% set ns.f = loop %}{% endif %}A" + body + "{% endfor %}",
                                        "{% for i in [[1]] recursive %}{% if loop.first %}{% set ns.f = loop %}{% endif %}B" + body + "{% endfor %}" });
    EXPECT_EQ((std::vector<std::string>{ "[][1]1", "[[2]2[3]3]A[1]A1", "[A[2]A2A[3]A3]B[1]B1" }), results);
}

// A stored `super` of a block deep in an `extends` chain renders the parent block it was taken from, in that block's
// chain, when a later call runs it (docs/tasks/0501)
TEST_F(TemplateModuleChangingSourceTest, StoredSuperRendersItsParentBlock)
{
    AddFile("mid", "{% extends 'base' %}{% block b %}M{{ super() }}{% endblock %}");
    AddFile("base", "{% block b %}B{% endblock %}");
    const std::string source = "{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% include 'lib' %}{% endmacro %}";
    const std::string lib = "{% extends 'mid' %}{% block b %}{% set ns.f = super %}CH{% endblock %}";
    EXPECT_EQ((std::vector<std::string>{ "[]CH", "[MB]CH", "[MB]CH" }), CallWithLibs(source, { lib, lib, lib }));
    const std::string once = "{% extends 'mid' %}{% block b %}{% if not ns.f %}{% set ns.f = super %}{% endif %}CH{{ super() }}{% endblock %}";
    EXPECT_EQ((std::vector<std::string>{ "[]CHMB", "[MB]CHMB", "[MB]CHMB" }), CallWithLibs(source, { once, once, once }));
    // The parent blocks see the names of the include that took it, which has ended
    AddFile("mid", "{% extends 'base' %}{% block b %}M{{ t }}{{ super() }}{% endblock %}");
    AddFile("base", "{% block b %}B{{ t }}{% endblock %}");
    const std::string named = "{% set t = 'T' %}{% extends 'mid' %}{% block b %}{% set ns.f = super %}CH{% endblock %}";
    EXPECT_EQ((std::vector<std::string>{ "[]CH", "[MTBT]CH", "[MTBT]CH" }), CallWithLibs(source, { named, named, named }));
}

TEST_F(TemplateModuleChangingSourceTest, WideStoredSuperRendersItsParentBlock)
{
    AddFile("mid", "{% extends 'base' %}{% block b %}M{{ super() }}{% endblock %}");
    AddFile("base", "{% block b %}B{% endblock %}");
    AddFile("lib", "{% extends 'mid' %}{% block b %}{% set ns.f = super %}CH{% endblock %}");
    jinja2::TemplateW tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load(L"{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% include 'lib' %}{% endmacro %}"));
    auto module = tpl.MakeModule().value();
    std::vector<std::wstring> results;
    for (int i = 0; i != 3; ++i)
    {
        results.push_back(Call(module.exports["m"]).AsWString());
    }
    EXPECT_EQ((std::vector<std::wstring>{ L"[]CH", L"[MB]CH", L"[MB]CH" }), results);
}

// A stored macro of a tree a later call let go of makes a macro that keeps that tree too (Codex review of #884)
TEST_F(TemplateModuleChangingSourceTest, MacroMadeByAStoredMacroOfAReleasedTree)
{
    const auto results = CallWithLibs("{% set ns = namespace(f=none, g=none) %}{% macro m() %}[{% include 'lib' %}]{% endmacro %}",
                                      { "",
                                        "{% macro f() %}{% macro g() %}G{{ 'long enough to leave small string storage' }}{% endmacro %}{% set ns.g = g %}{% endmacro %}{% set ns.f = f %}",
                                        "3",
                                        "{% set _ = ns.f() %}{% set ns.f = none %}{{ ns.g() }}",
                                        "5",
                                        "{{ ns.g() }}" });
    const std::string g = "Glong enough to leave small string storage";
    EXPECT_EQ((std::vector<std::string>{ "[]", "[]", "[3]", "[" + g + "]", "[5]", "[" + g + "]" }), results);
}

// A value a call stored from the template, text and containers of literals, outlives the tree it came from
TEST_F(TemplateModuleChangingSourceTest, StoredLiteralsOutliveTheirTree)
{
    const auto results = CallWithLibs("{% set ns = namespace(s=none, l=none) %}{% macro m() %}[{% if ns.s %}{{ ns.s }}{{ ns.l|join(',') }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                      { "{% set ns.s = 'one literal long enough to leave small string storage' %}{% set ns.l = ['a1', 'b1'] %}",
                                        "{% set ns.s = 'two literal long enough to leave small string storage' %}{% set ns.l = ['a2', 'b2'] %}",
                                        "{% set ns.s = 'three literal long enough to leave small string storage' %}{% set ns.l = ['a3', 'b3'] %}" });
    EXPECT_EQ((std::vector<std::string>{ "[]", "[one literal long enough to leave small string storagea1,b1]", "[two literal long enough to leave small string storagea2,b2]" }), results);
}

// A call block's caller, stored, views the tree of the template it was written in
TEST_F(TemplateModuleChangingSourceTest, StoredCallerRunsItsOldTree)
{
    const auto results = CallWithLibs("{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% include 'lib' %}{% endmacro %}",
                                      { "{% macro w() %}{% set ns.f = caller %}{% endmacro %}{% call w() %}OLD{% endcall %}",
                                        "{% macro w() %}{% set ns.f = caller %}{% endmacro %}{% call w() %}NEW{% endcall %}",
                                        "{% macro w() %}{% set ns.f = caller %}{% endmacro %}{% call w() %}NEW2{% endcall %}" });
    EXPECT_EQ((std::vector<std::string>{ "[]", "[OLD]", "[NEW]" }), results);
}

// A stored macro of a module imported with context keeps that module and its old tree (docs/tasks/0312): OLD, OLD,
// NEW, NEW in Jinja2 3.1.6
TEST_F(TemplateModuleChangingSourceTest, MacroStoredAcrossCallsKeepsItsOldModule)
{
    AddFile("lib", "{% macro x() %}OLD{% endmacro %}");
    auto module = Make("{% set ns = namespace() %}"
                       "{% macro keep() %}{% import 'lib' as l with context %}{% set ns.f = l.x %}{{ ns.f() }}{% endmacro %}"
                       "{% macro use() %}{{ ns.f() }}{% endmacro %}");
    EXPECT_EQ("OLD", Call(module.exports["keep"]).AsString());
    AddFile("lib", "{% macro x() %}NEW{% endmacro %}");
    EXPECT_EQ("OLD", Call(module.exports["use"]).AsString());
    EXPECT_EQ("NEW", Call(module.exports["keep"]).AsString());
    EXPECT_EQ("NEW", Call(module.exports["use"]).AsString());
}

// Imports and includes of the changing template in one call each see its current text
TEST_F(TemplateModuleChangingSourceTest, ImportsSeeEachChange)
{
    const auto results = CallWithLibs("{% macro m() %}{% import 'lib' as l with context %}{% from 'lib' import x %}{{ l.x() }}{{ x() }}{% include 'lib' %}{% endmacro %}",
                                      { "{% macro x() %}1{% endmacro %}", "{% macro x() %}2{% endmacro %}", "{% macro x() %}3{% endmacro %}" });
    EXPECT_EQ((std::vector<std::string>{ "11", "22", "33" }), results);
}

TEST_F(TemplateModuleChangingSourceTest, WideStoredMacroRunsItsOldTree)
{
    AddFile("lib", "{% macro x() %}OLD{% endmacro %}{% set ns.f = x %}");
    jinja2::TemplateW tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load(L"{% set ns = namespace(f=none) %}{% macro m() %}[{% if ns.f %}{{ ns.f() }}{% endif %}]{% include 'lib' %}{% endmacro %}"));
    auto module = tpl.MakeModule().value();
    std::vector<std::wstring> results;
    for (const char* text : { "OLD", "NEW", "NEW2" })
    {
        AddFile("lib", std::string("{% macro x() %}") + text + "{% endmacro %}{% set ns.f = x %}");
        results.push_back(Call(module.exports["m"]).AsWString());
    }
    EXPECT_EQ((std::vector<std::wstring>{ L"[]", L"[OLD]", L"[NEW]" }), results);
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
    // What the failed call stored keeps its module, as in Jinja2 3.1.6 (docs/tasks/0312)
    EXPECT_EQ("AM", Call(module.exports["use"]).AsString());
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

    // A macro stored across calls keeps its module there too (docs/tasks/0312)
    ASSERT_TRUE(!!tpl.Load(L"{% set ns = namespace() %}{% macro keep() %}{% import 'a' as a with context %}{% set ns.f = a.am %}{{ ns.f() }}{% endmacro %}"
                           L"{% macro use() %}{{ ns.f() }}{% endmacro %}"));
    auto stored = tpl.MakeModule().value();
    EXPECT_EQ(L"AM", Call(stored.exports["keep"]).AsWString());
    EXPECT_EQ(L"AM", Call(stored.exports["use"]).AsWString());
}

namespace
{
// A host value nested `depth` deep: a list or a dict holding the next level, `leaf` at the bottom
jinja2::Value DeepValue(std::size_t depth, bool asMap, jinja2::Value leaf = jinja2::Value())
{
    jinja2::Value value = std::move(leaf);
    for (std::size_t level = 0; level < depth; ++level)
    {
        if (asMap)
        {
            jinja2::ValuesMap map;
            map.emplace("a", std::move(value));
            value = jinja2::Value(std::move(map));
        }
        else
        {
            jinja2::ValuesList list;
            list.push_back(std::move(value));
            value = jinja2::Value(std::move(list));
        }
    }
    return value;
}

// A list of values of DeepValue, destroyed with jinja2::DestroyDeep: Value's destructor recurses
// once per level, as its copy does (docs/tasks/0449)
struct DeepValues
{
    std::vector<jinja2::Value> values;
    ~DeepValues()
    {
        for (auto& value : values)
        {
            jinja2::DestroyDeep(std::move(value));
        }
    }
};

constexpr std::size_t DeepArgument = 200000;

#if defined(__linux__) || defined(__APPLE__)
void* RunJob(void* job)
{
    (*static_cast<const std::function<void()>*>(job))();
    return nullptr;
}
#endif

// On a thread of a known stack, which the copy of DeepArgument levels surely overflows: a main
// thread's may be deep enough for it (8 MB on macOS, with a Release build's small frames)
void RunOnSmallStack(const std::function<void()>& job)
{
#if defined(__linux__) || defined(__APPLE__)
    pthread_attr_t attr;
    ASSERT_EQ(0, pthread_attr_init(&attr));
    ASSERT_EQ(0, pthread_attr_setstacksize(&attr, std::size_t{ 2 } * 1024 * 1024));
    pthread_t thread{};
    ASSERT_EQ(0, pthread_create(&thread, &attr, RunJob, const_cast<std::function<void()>*>(&job)));
    pthread_join(thread, nullptr);
    pthread_attr_destroy(&attr);
#else
    job();
#endif
}
} // namespace

// The host's value is copied for the module, a copy that recursed once per level and
// overflowed the stack; it stops with RecursionLimitExceeded as a render does (docs/tasks/0418)
TEST_F(TemplateModuleTest, DeepArgumentFails)
{
    RunOnSmallStack([this] {
        auto module = Make("{% macro m(p) %}{{ p|length }}{% endmacro %}");
        for (const bool asMap : { false, true })
        {
            // Moved, never copied, and destroyed by DestroyDeep: Value's own copy and destructor recurse
            DeepValues kept;
            const auto callError = [&module, &kept, asMap](bool asKeyword) {
                jinja2::UserCallableParams params;
                if (asKeyword)
                {
                    params.extraKwArgs = jinja2::ValuesMap{};
                    params.extraKwArgs.GetIf<jinja2::ValuesMap>()->emplace("p", DeepValue(DeepArgument, asMap));
                }
                else
                {
                    params.extraPosArgs = jinja2::ValuesList{};
                    params.extraPosArgs.GetIf<jinja2::ValuesList>()->push_back(DeepValue(DeepArgument, asMap));
                }
                auto code = jinja2::ErrorCode::Unspecified;
                try
                {
                    module.exports["m"].As<jinja2::UserCallable>().callable(params);
                }
                catch (const jinja2::BadResultAccess<>& failure)
                {
                    code = failure.error().GetCode();
                }
                kept.values.push_back(std::move(asKeyword ? params.extraKwArgs : params.extraPosArgs));
                return code;
            };
            EXPECT_EQ(jinja2::ErrorCode::RecursionLimitExceeded, callError(false));
            EXPECT_EQ(jinja2::ErrorCode::RecursionLimitExceeded, callError(true));

            auto tpl = Load("{{ p|length }}");
            jinja2::ValuesMap params;
            params.emplace("p", DeepValue(DeepArgument, asMap));
            auto failed = tpl.MakeModule(params);
            kept.values.emplace_back(std::move(params));
            ASSERT_FALSE(failed);
            EXPECT_EQ(jinja2::ErrorCode::RecursionLimitExceeded, failed.error().GetCode()) << failed.error().ToString();

            // A global of the environment the module copies too. The environment is never
            // destroyed, which would recurse; it stays reachable, so it does not leak
            static auto* const env = new jinja2::TemplateEnv();
            env->AddGlobal(asMap ? "pm" : "pl", DeepValue(DeepArgument, asMap));
            jinja2::Template withGlobal(env);
            ASSERT_TRUE(!!withGlobal.Load("{{ 1 }}"));
            auto globalFailed = withGlobal.MakeModule();
            ASSERT_FALSE(globalFailed);
            EXPECT_EQ(jinja2::ErrorCode::RecursionLimitExceeded, globalFailed.error().GetCode()) << globalFailed.error().ToString();

            // Copied within the stack, the copy must also be destroyed within it: Value's
            // destructor takes several times the stack its copy takes. Deep enough that a copy
            // checked against the stack alone overflows in its destruction; the module either
            // fails or ends without a crash, also when the deep item follows a sibling it copied
            for (const std::size_t depth : { std::size_t{ 3000 }, std::size_t{ 20000 } })
            {
                jinja2::ValuesList pair;
                pair.push_back(DeepValue(depth, asMap));
                pair.push_back(DeepValue(DeepArgument, asMap));
                for (std::size_t items = 1; items != 3; ++items)
                {
                    jinja2::ValuesMap deepParams;
                    jinja2::ValuesList list;
                    std::move(pair.begin(), pair.begin() + static_cast<std::ptrdiff_t>(items), std::back_inserter(list));
                    deepParams.emplace("p", std::move(list));
                    {
                        auto deepModule = tpl.MakeModule(deepParams);
                        if (deepModule)
                        {
                            EXPECT_EQ(std::to_string(items), deepModule->body);
                        }
                        else
                        {
                            EXPECT_EQ(jinja2::ErrorCode::RecursionLimitExceeded, deepModule.error().GetCode()) << deepModule.error().ToString();
                        }
                    }
                    // Back for the next round
                    auto& back = deepParams["p"].As<jinja2::ValuesList>();
                    std::move(back.begin(), back.end(), pair.begin());
                    kept.values.emplace_back(std::move(deepParams));
                }
                kept.values.emplace_back(std::move(pair));
            }

            // Within the stack, it is the macro's
            const auto nested = DeepValue(50, asMap, 7);
            EXPECT_EQ("1", Call(module.exports["m"], { nested }).AsString());
            auto made = tpl.MakeModule({ { "p", nested } });
            ASSERT_TRUE(!!made);
            EXPECT_EQ("1", made->body);
        }
    });
}

// A view of the host's text is copied: the module's values outlive the call that passed it
TEST_F(TemplateModuleTest, ViewArgumentIsCopied)
{
    auto module = Make("{% set ns = namespace(s=none) %}{% macro put(a) %}{% set ns.s = a %}{% endmacro %}{% macro get() %}{{ ns.s }}{% endmacro %}");
    auto text = std::make_unique<std::string>("first text, long enough for the heap");
    Call(module.exports["put"], { std::string_view(*text) });
    text.reset();
    EXPECT_EQ("first text, long enough for the heap", Call(module.exports["get"]).AsString());

    // Built with emplace_back: a braced `{ list }` copies the list on Clang and MSVC
    jinja2::ValuesList inner;
    inner.emplace_back(std::string_view("x"));
    jinja2::ValuesList nested;
    nested.emplace_back(std::move(inner));
    auto wide = std::make_unique<std::wstring>(L"wide text, long enough for the heap");
    nested.emplace_back(std::wstring_view(*wide));
    jinja2::ValuesList args;
    args.emplace_back(std::move(nested));
    Call(module.exports["put"], std::move(args));
    wide.reset();
    EXPECT_EQ("[['x'], 'wide text, long enough for the heap']", Call(module.exports["get"]).AsString());
}

// 0407: a render hands its own dict through a host callable to a module macro that writes it.
// Nothing is a cycle, so nothing may leak (LSan). The host holds a read-only snapshot of the
// render's dict (docs/api-2.0.md 3.10, item 9), so the macro writes a copy of its own and the render's
// `d|list` stays ['a'], where Python, which shares the one dict, gives ['a', 'l']: deliberate
TEST_F(TemplateModuleTest, RenderDictWrittenThroughAHostCallIntoAModule)
{
    auto module = Make("{% macro n(d) %}{% set _ = d.update(l=[1]) %}{% endmacro %}");
    jinja2::ValuesMap params;
    params["cbn"] = jinja2::UserCallable([&module](const jinja2::UserCallableParams& p) { return Call(module.exports["n"], { p["d"] }); }, { jinja2::ArgInfo{ "d" } });
    jinja2::Template tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load("{% set d = {'a': [1]} %}{{ cbn(d) }}{{ d|list }}"));
    auto out = tpl.RenderAsString(params);
    ASSERT_TRUE(!!out) << out.error().ToString();
    EXPECT_EQ("['a']", out.value());
}

// 0407: a module list that holds itself, captured by an inner macro the module stores, goes with
// the module (LSan)
TEST_F(TemplateModuleTest, SelfHoldingListCapturedByAStoredClosure)
{
    {
        auto module = Make("{% set ns = namespace(f=none) %}{% macro outer() %}{% set cap = [] %}{% set _ = cap.append(cap) %}"
                           "{% macro inner() %}{{ cap|length }}{% endmacro %}{% set ns.f = inner %}{% endmacro %}"
                           "{% macro get() %}{{ ns.f() }}{% endmacro %}");
        Call(module.exports["outer"]);
        EXPECT_EQ("1", Call(module.exports["get"]).AsString());
    }
    {
        auto module = Make("{% set cap = [] %}{% set _ = cap.append(cap) %}{% set ns = namespace(f=none) %}"
                           "{% macro inner() %}{{ cap|length }}{% endmacro %}{% macro store() %}{% set ns.f = inner %}{% endmacro %}"
                           "{% macro get() %}{{ ns.f() }}{% endmacro %}");
        Call(module.exports["store"]);
        EXPECT_EQ("1", Call(module.exports["get"]).AsString());
    }
}

// 0460: a self-holding list a call stores into a value of the module goes when a later call
// replaces or removes it, through a namespace, a dict or a list of the module (LSan)
TEST_F(TemplateModuleTest, SelfHoldingListALaterCallDrops)
{
    // clang-format off
    const char* const sources[] = {
        "{% set ns = namespace(v=none) %}{% macro n() %}{% set c = [] %}{% set _ = c.append(c) %}{% set ns.v = c %}{{ ns.v|length }}{% endmacro %}",
        "{% set d = {} %}{% macro n() %}{% set c = [] %}{% set _ = c.append(c) %}{% set _ = d.update(v=c) %}{{ d.v|length }}{% endmacro %}",
        "{% set d = {} %}{% macro n() %}{% set c = {} %}{% set _ = c.update(c=c) %}{% set _ = d.pop('v', none) %}{% set _ = d.setdefault('v', c) %}{{ d.v|length }}{% endmacro %}",
        "{% set d = {} %}{% macro n() %}{% set c = [] %}{% set _ = c.append(c) %}{% set _ = d.clear() %}{% set _ = d.update(v=c) %}{{ d.v|length }}{% endmacro %}",
        "{% set d = {'v': 0} %}{% macro n() %}{% set c = [] %}{% set _ = c.append(c) %}{% set _ = d.popitem() %}{% set _ = d.update(v=c) %}{{ d.v|length }}{% endmacro %}",
        "{% set l = [] %}{% macro n() %}{% set c = [] %}{% set _ = c.append(c) %}{% set _ = l.pop() if l %}{% set _ = l.append(c) %}{{ l[0]|length }}{% endmacro %}",
        "{% set l = [] %}{% macro n() %}{% set c = [] %}{% set _ = c.append(c) %}{% set _ = l.clear() %}{% set _ = l.append(c) %}{{ l[0]|length }}{% endmacro %}",
        "{% set l = [] %}{% macro n() %}{% set c = [] %}{% set _ = c.append(c) %}{% set _ = l.remove(l[0]) if l %}{% set _ = l.append(c) %}{{ l[0]|length }}{% endmacro %}",
        // Below a value of the module that the call reaches through another one
        "{% set ns = namespace(d={}) %}{% macro n() %}{% set c = [] %}{% set _ = c.append(c) %}{% set _ = ns.d.update(v=c) %}{{ ns.d.v|length }}{% endmacro %}",
    };
    // clang-format on
    for (const auto* source : sources)
    {
        SCOPED_TRACE(source);
        auto module = Make(source);
        EXPECT_EQ("1", Call(module.exports["n"]).AsString());
        EXPECT_EQ("1", Call(module.exports["n"]).AsString());
        EXPECT_EQ("1", Call(module.exports["n"]).AsString());
    }
}

// 0480: a container that holds itself, which a module's call hands a host callable that keeps it,
// goes when the host lets go of it (LSan): after the module, before it, and before a later call
TEST_F(TemplateModuleTest, KeptSelfHoldingExportGoesWithTheHostsValue)
{
    // clang-format off
    const char* const sources[] = {
        "{% macro m() %}{% set xs = [1] %}{% set _ = xs.append(xs) %}{% set d = {'a': 1} %}{% set _ = d.update(d=d) %}"
        "{% set _ = keep(xs) %}{% set _ = keep(d) %}{% endmacro %}",
        "{% macro m() %}{% set ns = namespace(a=1) %}{% set ns.self = ns %}{% set _ = keep(ns) %}{% endmacro %}",
        "{% macro m() %}{% set xs = [1] %}{% set d = {'l': xs} %}{% set _ = xs.append(d) %}{% set _ = keep(d) %}{% endmacro %}",
        // Stored in the module's values too: a later call reads it
        "{% set ns = namespace(v=none) %}{% macro m() %}{% set xs = [1] %}{% set _ = xs.append(xs) %}{% set ns.v = xs %}"
        "{% set _ = keep(xs) %}{% endmacro %}",
        // The module's cell of the host's list `p`
        "{% macro m() %}{% set _ = p.append(p) %}{% set _ = keep(p) %}{% endmacro %}",
        "{% macro m() %}{% set xs = [p] %}{% set _ = p.append(xs) %}{% set _ = keep(xs) %}{% endmacro %}",
    };
    // clang-format on
    enum class Order
    {
        ModuleFirst,
        HostFirst,
        HostThenCall,
    };
    for (const auto* source : sources)
    {
        for (const auto order : { Order::ModuleFirst, Order::HostFirst, Order::HostThenCall })
        {
            SCOPED_TRACE(std::string(source) + " order " + std::to_string(static_cast<int>(order)));
            auto kept = std::make_shared<std::vector<jinja2::Value>>();
            jinja2::ValuesMap params;
            params["keep"] = jinja2::UserCallable(
                [kept](const jinja2::UserCallableParams& p) {
                    kept->push_back(p["x"]);
                    return jinja2::Value();
                },
                { jinja2::ArgInfo{ "x" } });
            params["p"] = jinja2::ValuesList{ 1 };
            auto module = std::make_unique<jinja2::TemplateModule>(Make(source, params));
            Call(module->exports["m"]);
            Call(module->exports["m"]);
            ASSERT_FALSE(kept->empty());
            for (const auto& value : *kept)
            {
                EXPECT_TRUE(value.GetIf<jinja2::GenericList>() || value.GetIf<jinja2::GenericMap>());
            }
            if (order == Order::ModuleFirst)
            {
                module.reset();
                kept->clear();
                continue;
            }
            kept->clear();
            if (order == Order::HostThenCall)
            {
                Call(module->exports["m"]);
                kept->clear();
            }
            module.reset();
        }
    }
}

// 0545: a dict view or set a module's call hands a host callable keeps its Python type for the host
// (IListItemAccessor::GetViewClass of CallListView)
TEST_F(TemplateModuleTest, AViewOfAModuleCallKeepsItsViewClass)
{
    auto seen = std::make_shared<std::string>();
    jinja2::ValuesMap params;
    params["keep"] = jinja2::UserCallable(
        [seen](const jinja2::UserCallableParams& p) {
            const jinja2::Value x = p["x"];
            const auto* list = x.GetIf<jinja2::GenericList>();
            const auto viewClassOf = [](const jinja2::GenericList* l) { return l && l->GetViewClass() ? l->GetViewClass() : "-"; };
            if (list && list->GetViewClass() == nullptr && list->GetSize().value_or(0) == 5)
            {
                // The views in a list the template made
                const auto* indexer = list->GetAccessor()->GetIndexer();
                for (int64_t i = 0; i < 5; ++i)
                {
                    const jinja2::Value item = indexer->GetItemByIndex(i);
                    *seen += viewClassOf(item.GetIf<jinja2::GenericList>()) + std::string(";");
                }
                return jinja2::Value();
            }
            *seen += viewClassOf(list) + std::string(";");
            return jinja2::Value();
        },
        { jinja2::ArgInfo{ "x" } });
    auto module = Make("{% macro m() %}{% set d = {'a': 1} %}{% set _ = keep(d.keys()) %}{% set _ = keep(d.values()) %}{% set _ = keep(d.items()) %}"
                       "{% set _ = keep(d.keys() - []) %}{% set _ = keep([1]) %}"
                       "{% set _ = keep([d.keys(), d.values(), d.items(), d.keys() - [], [1]]) %}{% endmacro %}",
                       params);
    Call(module.exports["m"]);
    EXPECT_EQ("dict_keys;dict_values;dict_items;set;-;dict_keys;dict_values;dict_items;set;-;", *seen);
}

// A namespace, a cycler and a joiner of a module in Objects mode are the module's own objects (docs/tasks/0411): what a macro
// set is what the host reads, and the host's next() is the cycler the macros read. Expected values are Python Jinja2 3.1.6's:
// m.ns.count 0 then 1 after m.inc(); m.cyc.next() 'a' 'b', m.cyc.current 'c', a macro's cyc.next() 'c' and then current 'a'
namespace
{
constexpr const char* liveSource = "{% set ns = namespace(count=0) %}{% set cyc = cycler('a', 'b', 'c') %}{% set j = joiner('|') %}"
                                   "{% macro inc() %}{% set ns.count = ns.count + 1 %}{{ ns.count }}{% endmacro %}"
                                   "{% macro cur() %}{{ cyc.current }}{% endmacro %}{% macro nxt() %}{{ cyc.next() }}{% endmacro %}"
                                   "{% macro show(x) %}{{ x.count }}{% endmacro %}{% macro first(l) %}{{ l[0].count }}{% endmacro %}{% macro set_on(x) %}{% set x.count = 40 %}{% endmacro %}";

const jinja2::GenericMap& MapOf(const jinja2::Value& value)
{
    const auto* map = value.GetIf<jinja2::GenericMap>();
    EXPECT_NE(nullptr, map);
    static const jinja2::GenericMap none;
    return map ? *map : none;
}

jinja2::Value Attr(const jinja2::Value& object, const std::string& name)
{
    return MapOf(object).GetValueByName(name);
}
} // namespace

TEST_F(TemplateModuleTest, ObjectExportsAreLive)
{
    auto tpl = Load(liveSource);
    auto made = tpl.MakeModule({}, {}, ObjectsOptions());
    ASSERT_TRUE(!!made) << made.error().ToString();
    auto& exports = made->exports;
    const auto& ns = exports.at("ns");
    const auto& cyc = exports.at("cyc");
    const auto& joiner = exports.at("j");
    EXPECT_STREQ("Namespace", MapOf(ns).GetObjectClass());
    EXPECT_STREQ("Cycler", MapOf(cyc).GetObjectClass());
    EXPECT_STREQ("Joiner", MapOf(joiner).GetObjectClass());
    EXPECT_EQ(jinja2::Value(int64_t{ 0 }), Attr(ns, "count"));
    EXPECT_EQ("1", Call(exports.at("inc")).AsString());
    EXPECT_EQ(jinja2::Value(int64_t{ 1 }), Attr(ns, "count"));
    EXPECT_EQ((std::vector<std::string>{ "count" }), MapOf(ns).GetKeys());
    EXPECT_EQ(1u, MapOf(ns).GetSize());
    EXPECT_TRUE(MapOf(ns).HasValue("count"));
    EXPECT_FALSE(MapOf(ns).HasValue("other"));
    EXPECT_FALSE(MapOf(ns).Find("other").has_value());
    ASSERT_EQ(1u, MapOf(ns).GetItems().size());
    EXPECT_EQ(jinja2::Value("count"), MapOf(ns).GetItems()[0].first);
    // The same object each time it is asked for
    EXPECT_EQ(MapOf(ns).GetIdentity(), MapOf(exports.at("ns")).GetIdentity());

    EXPECT_EQ(jinja2::Value(int64_t{ 0 }), Attr(cyc, "pos"));
    EXPECT_EQ(jinja2::Value("a"), Call(Attr(cyc, "next")));
    EXPECT_EQ(jinja2::Value("b"), Call(Attr(cyc, "next")));
    EXPECT_EQ(jinja2::Value("c"), Attr(cyc, "current"));
    // The macros read the cycler the host advanced, and the host the one they advanced
    EXPECT_EQ("c", Call(exports.at("cur")).AsString());
    EXPECT_EQ("c", Call(exports.at("nxt")).AsString());
    EXPECT_EQ(jinja2::Value("a"), Attr(cyc, "current"));
    EXPECT_EQ(jinja2::Value(int64_t{ 0 }), Attr(cyc, "pos"));
    EXPECT_TRUE(Call(Attr(cyc, "reset")).IsNone());
    EXPECT_EQ(jinja2::Value("a"), Call(Attr(cyc, "next")));
    EXPECT_EQ(3u, ItemsOf(Attr(cyc, "items")).size());

    EXPECT_EQ(jinja2::Value(""), Call(Attr(joiner, "__call__")));
    EXPECT_EQ(jinja2::Value("|"), Call(Attr(joiner, "__call__")));

    // The default mode is unchanged: a copy, which a macro does not change
    auto copies = tpl.MakeModule();
    ASSERT_TRUE(!!copies);
    EXPECT_EQ(nullptr, copies->exports.at("ns").GetIf<jinja2::GenericMap>());
    EXPECT_NE(nullptr, copies->exports.at("ns").GetIf<jinja2::ValuesMap>());
    Call(copies->exports.at("inc"));
    EXPECT_EQ(jinja2::Value(int64_t{ 0 }), copies->exports.at("ns").AsMap().at("count"));
}

// The view passed back to a macro of its module is the object: the macro reads and writes the module's own namespace
TEST_F(TemplateModuleTest, ObjectViewPassedBackIsTheObject)
{
    auto tpl = Load(liveSource);
    auto module = tpl.MakeModule({}, {}, ObjectsOptions());
    ASSERT_TRUE(!!module);
    auto& exports = module->exports;
    const auto& ns = exports.at("ns");
    Call(exports.at("inc"));
    EXPECT_EQ("1", Call(exports.at("show"), { ns }).AsString());
    Call(exports.at("set_on"), { ns });
    EXPECT_EQ(jinja2::Value(int64_t{ 40 }), Attr(ns, "count"));
    EXPECT_EQ("41", Call(exports.at("inc")).AsString());
    // In a list the macro may store it is a copy, not a reference that keeps the module alive through itself
    jinja2::ValuesList wrapped;
    wrapped.emplace_back(ns);
    jinja2::ValuesList pos;
    pos.emplace_back(std::move(wrapped));
    EXPECT_EQ("41", Call(exports.at("first"), std::move(pos)).AsString());
    jinja2::ValuesMap kw;
    kw["x"] = ns;
    EXPECT_EQ("41", Call(exports.at("show"), {}, kw).AsString());
}

// The object and its module outlive the template, the module struct and the source (ASan)
TEST_F(TemplateModuleTest, ObjectExportOutlivesTemplate)
{
    jinja2::Value ns;
    jinja2::Value cyc;
    jinja2::Value inc;
    {
        auto tpl = Load(liveSource);
        auto module = tpl.MakeModule({}, {}, ObjectsOptions());
        ASSERT_TRUE(!!module);
        ns = module->exports.at("ns");
        cyc = module->exports.at("cyc");
        inc = module->exports.at("inc");
        // The source is replaced and the template with its tree goes
        ASSERT_TRUE(!!tpl.Load("{{ 'another source that replaces the old one, with a text that is much longer than the old one' }}"));
    }
    EXPECT_EQ(jinja2::Value(int64_t{ 0 }), Attr(ns, "count"));
    Call(inc);
    EXPECT_EQ(jinja2::Value(int64_t{ 1 }), Attr(ns, "count"));
    EXPECT_EQ(jinja2::Value("a"), Call(Attr(cyc, "next")));
    EXPECT_EQ(jinja2::Value("b"), Attr(cyc, "current"));
}

// A nested namespace is an object too, and one a call made stays the module's
TEST_F(TemplateModuleTest, NestedObjectExports)
{
    auto tpl = Load("{% set ns = namespace(inner=namespace(n=1), l=[namespace(k=2)]) %}{% macro bump() %}{% set i = ns.inner %}{% set i.n = i.n + 1 %}{% endmacro %}"
                    "{% set d = {'o': ns.inner} %}");
    auto module = tpl.MakeModule({}, {}, ObjectsOptions());
    ASSERT_TRUE(!!module);
    const auto inner = Attr(module->exports.at("ns"), "inner");
    EXPECT_STREQ("Namespace", MapOf(inner).GetObjectClass());
    EXPECT_EQ(jinja2::Value(int64_t{ 1 }), Attr(inner, "n"));
    Call(module->exports.at("bump"));
    EXPECT_EQ(jinja2::Value(int64_t{ 2 }), Attr(inner, "n"));
    EXPECT_EQ(MapOf(inner).GetIdentity(), MapOf(Attr(module->exports.at("ns"), "inner")).GetIdentity());
    const auto fromList = ItemsOf(Attr(module->exports.at("ns"), "l"));
    ASSERT_EQ(1u, fromList.size());
    EXPECT_EQ(jinja2::Value(int64_t{ 2 }), Attr(fromList[0], "k"));
    const auto fromDict = module->exports.at("d").AsMap().at("o");
    EXPECT_EQ(jinja2::Value(int64_t{ 2 }), Attr(fromDict, "n"));
}

// A namespace that holds itself, read through views and dropped: nothing recurses, and the module frees it (LSan)
TEST_F(TemplateModuleTest, ObjectExportHoldingItself)
{
    auto tpl = Load("{% set ns = namespace(n=1) %}{% set ns.me = ns %}{% macro mk() %}{% set o = namespace(n=2) %}{% set o.me = o %}{{ o.n }}{% endmacro %}");
    {
        auto module = tpl.MakeModule({}, {}, ObjectsOptions());
        ASSERT_TRUE(!!module);
        for (int i = 0; i != 3; ++i)
        {
            auto me = Attr(module->exports.at("ns"), "me");
            EXPECT_EQ(jinja2::Value(int64_t{ 1 }), Attr(me, "n"));
            EXPECT_EQ(jinja2::Value(int64_t{ 1 }), Attr(Attr(me, "me"), "n"));
            EXPECT_EQ(2u, MapOf(me).GetItems().size());
            EXPECT_EQ("2", Call(module->exports.at("mk")).AsString());
        }
    }
    // The module goes with its last view
    auto last = [&] {
        auto module = tpl.MakeModule({}, {}, ObjectsOptions());
        return Attr(module->exports.at("ns"), "me");
    }();
    EXPECT_EQ(jinja2::Value(int64_t{ 1 }), Attr(last, "n"));
}

// Readers on threads alongside the calls that write: the view takes the module's turn (TSan)
TEST_F(TemplateModuleTest, ConcurrentObjectReads)
{
    auto tpl = Load(liveSource);
    auto module = tpl.MakeModule({}, {}, ObjectsOptions());
    ASSERT_TRUE(!!module);
    auto& exports = module->exports;
    static constexpr int perThread = 100;
    std::atomic<bool> failed{ false };
    std::vector<std::thread> threads;
    for (int t = 0; t != 2; ++t)
    {
        threads.emplace_back([&exports]() {
            for (int i = 0; i != perThread; ++i)
            {
                Call(exports.at("inc"));
            }
        });
        threads.emplace_back([&exports, &failed]() {
            int64_t last = 0;
            for (int i = 0; i != perThread; ++i)
            {
                const auto count = Attr(exports.at("ns"), "count");
                const auto* number = count.GetIf<int64_t>();
                // The count only grows
                failed = failed || !number || *number < last;
                last = number ? *number : last;
                Call(Attr(exports.at("cyc"), "next"));
                Attr(exports.at("j"), "sep");
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_FALSE(failed);
    EXPECT_EQ(jinja2::Value(int64_t{ 2 } * perThread), Attr(exports.at("ns"), "count"));
}

// A host's callable in a macro reads the module's object, within the macro's call (re-entry on the thread)
TEST_F(TemplateModuleTest, ObjectReadFromHostCallableInMacro)
{
    auto ns = std::make_shared<jinja2::Value>();
    jinja2::ValuesMap params;
    params["peek"] = jinja2::UserCallable([ns](const jinja2::UserCallableParams&) { return Attr(*ns, "count"); }, {});
    auto tpl = Load("{% set ns = namespace(count=5) %}{% macro m() %}{% set ns.count = ns.count + 1 %}{{ peek() }}{% endmacro %}");
    auto module = tpl.MakeModule(params, {}, ObjectsOptions());
    ASSERT_TRUE(!!module);
    *ns = module->exports.at("ns");
    EXPECT_EQ("6", Call(module->exports.at("m")).AsString());
    // The host's callable is the module's own: it holds the view that holds the module
    *ns = jinja2::Value();
}

// A macro of module B reads a view of module A through a host callable: B's lock is held, A's is taken inside it
TEST_F(TemplateModuleTest, ViewOfAnotherModuleReadFromAMacro)
{
    auto tplA = Load("{% set ns = namespace(count=7) %}{% macro inc() %}{% set ns.count = ns.count + 1 %}{% endmacro %}");
    auto moduleA = tplA.MakeModule({}, {}, ObjectsOptions());
    ASSERT_TRUE(!!moduleA);
    const auto view = moduleA->exports.at("ns");
    jinja2::ValuesMap params;
    params["peek"] = jinja2::UserCallable([view](const jinja2::UserCallableParams&) { return Attr(view, "count"); }, {});
    auto tplB = Load("{% macro m() %}{{ peek() }}{% endmacro %}");
    auto moduleB = tplB.MakeModule(params, {}, ObjectsOptions());
    ASSERT_TRUE(!!moduleB);
    EXPECT_EQ("7", Call(moduleB->exports.at("m")).AsString());
    Call(moduleA->exports.at("inc"));
    EXPECT_EQ("8", Call(moduleB->exports.at("m")).AsString());
}

// A poll that reads a view of its own module, within the call of a macro that the poll interrupts
// A thread that waits for another thread's run in the module calls ModuleOptions::waitForTurn with the wait, where a
// host lets go of a lock the other thread's callables need (Python's GIL); a free module does not call it (docs/tasks/0411)
TEST_F(TemplateModuleTest, WaitForTurnIsCalledOnlyWhenTheModuleIsBusy)
{
    auto tpl = Load("{% set ns = namespace(count=0) %}{% macro slow(f) %}{{ f() }}{% endmacro %}");
    std::atomic<bool> inside{ false };
    std::atomic<bool> release{ false };
    std::atomic<int> waits{ 0 };
    auto options = ObjectsOptions();
    options.waitForTurn = [&](const std::function<void()>& wait) {
        ++waits;
        release = true;
        wait();
    };
    auto module = tpl.MakeModule({}, {}, options);
    ASSERT_TRUE(!!module);
    const auto& exports = module->exports;
    EXPECT_EQ(jinja2::Value(int64_t{ 0 }), Attr(exports.at("ns"), "count"));
    EXPECT_EQ(0, waits.load());
    const jinja2::Value pause = jinja2::UserCallable(
        [&](const jinja2::UserCallableParams&) -> jinja2::Value {
            inside = true;
            const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
            while (!release && std::chrono::steady_clock::now() < deadline)
            {
                std::this_thread::yield();
            }
            return std::string();
        },
        {});
    std::thread worker([&exports, &pause]() { Call(exports.at("slow"), { pause }); });
    while (!inside)
    {
        std::this_thread::yield();
    }
    EXPECT_EQ(jinja2::Value(int64_t{ 0 }), Attr(exports.at("ns"), "count"));
    worker.join();
    EXPECT_EQ(1, waits.load());
}

TEST_F(TemplateModuleTest, PollReadsAView)
{
    auto ns = std::make_shared<jinja2::Value>();
    int polls = 0;
    int64_t last = -1;
    jinja2::RenderOptions options;
    options.poll = [ns, &polls, &last] {
        const auto current = Attr(*ns, "count");
        if (const auto* count = current.GetIf<int64_t>())
        {
            ++polls;
            last = *count;
        }
    };
    auto tpl = Load("{% set ns = namespace(count=0) %}{% macro spin() %}{% for i in range(5000) %}{% set ns.count = ns.count + 1 %}{% endfor %}{% endmacro %}");
    auto module = tpl.MakeModule({}, options, ObjectsOptions());
    ASSERT_TRUE(!!module);
    *ns = module->exports.at("ns");
    Call(module->exports.at("spin"));
    EXPECT_GT(polls, 0);
    EXPECT_LE(last, 5000);
    EXPECT_EQ(jinja2::Value(int64_t{ 5000 }), Attr(*ns, "count"));
    // The poll holds the view that holds the module
    *ns = jinja2::Value();
}

// A read runs under the module's limits and fails as a call does, with an error; the module goes on
TEST_F(TemplateModuleTest, ObjectReadFailureIsAnError)
{
    jinja2::Settings settings;
    settings.limits.maxSequenceSize = 4;
    jinja2::TemplateEnv env;
    env.SetSettings(settings);
    jinja2::Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{% set ns = namespace(small={'a': 1}, big={'a': 1, 'b': 2, 'c': 3, 'd': 4, 'e': 5, 'f': 6}) %}"));
    auto module = tpl.MakeModule({}, {}, ObjectsOptions());
    ASSERT_TRUE(!!module);
    const auto& ns = module->exports.at("ns");
    EXPECT_THROW(Attr(ns, "big"), jinja2::BadResultAccess<char>);
    EXPECT_THROW(static_cast<void>(MapOf(ns).GetItems()), jinja2::BadResultAccess<char>);
    EXPECT_EQ(1u, Attr(ns, "small").AsMap().size());
}

TEST_F(TemplateModuleTest, ObjectExportsWide)
{
    jinja2::TemplateW tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load(L"{% set ns = namespace(count=0) %}{% set cyc = cycler('a', 'b') %}{% macro inc() %}{% set ns.count = ns.count + 1 %}{% endmacro %}"
                           L"{% macro show(x) %}{{ x.count }}{% endmacro %}"));
    auto module = tpl.MakeModule({}, {}, ObjectsOptions());
    ASSERT_TRUE(!!module);
    auto& exports = module->exports;
    Call(exports.at("inc"));
    EXPECT_EQ(jinja2::Value(int64_t{ 1 }), Attr(exports.at("ns"), "count"));
    EXPECT_EQ(jinja2::Value(std::wstring(L"a")), Call(Attr(exports.at("cyc"), "next")));
    EXPECT_EQ(jinja2::Value(std::wstring(L"b")), Call(Attr(exports.at("cyc"), "next")));
    EXPECT_EQ(jinja2::Value(std::wstring(L"a")), Call(Attr(exports.at("cyc"), "next")));
    EXPECT_EQ(L"1", Call(exports.at("show"), { exports.at("ns") }).AsWString());
}
