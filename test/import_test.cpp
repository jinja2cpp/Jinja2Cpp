#include "test_tools.h"

#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <string>

// Test cases are taken from the pandor/Jinja2 tests

class ImportTest : public TemplateEnvFixture
{
protected:
    void SetUp() override
    {
        TemplateEnvFixture::SetUp();

        AddFile("module", R"(
{% macro test(extra='') %}[{{ foo }}{{ extra }}|{{ 23 }}]{% endmacro %}
{% set sbar=56 %}
{% macro __inner() %}77{% endmacro %}
{% macro test_set() %}[{{ foo }}|{{ sbar }}]{% endmacro %}
{% macro test_inner() %}[{{ foo }}|{{ __inner() }}]{% endmacro %}
)");
        AddFile("header", "[{{ foo }}|{{ 23 }}]");
        AddFile("o_printer", "({{ o }})");
    }
};

TEST_F(ImportTest, TestContextImports)
{
    jinja2::ValuesMap params{{"foo", 42}};

    auto result = Render(R"({% import "module" as m %}{{ m.test() }}{{ m.test_set() }})", params);
    EXPECT_EQ("[|23][|56]", result);
    result = Render(R"({% import "module" as m %}{{ m.test(foo) }}{{ m.test_set() }})", params);
    EXPECT_EQ("[42|23][|56]", result);
    result = Render(R"({% import "module" as m without context %}{{ m.test() }}{{ m.test_set() }})", params);
    EXPECT_EQ("[|23][|56]", result);
    result = Render(R"({% import "module" as m without context %}{{ m.test(foo) }}{{ m.test_set() }})", params);
    EXPECT_EQ("[42|23][|56]", result);
    result = Render(R"({% import "module" as m with context %}{{ m.test() }}{{ m.test_set() }})", params);
    EXPECT_EQ("[42|23][42|56]", result);
    result = Render(R"({% import "module" as m with context %}{{ m.test(foo) }}{{ m.test_set() }})", params);
    EXPECT_EQ("[4242|23][42|56]", result);
    result = Render(R"({% import "module" as m without context %}{% set sbar=88 %}{{ m.test() }}{{ m.test_set() }})", params);
    EXPECT_EQ("[|23][|56]", result);
    result = Render(R"({% import "module" as m with context %}{% set sbar=88 %}{{ m.test() }}{{ m.test_set() }})", params);
    EXPECT_EQ("[42|23][42|56]", result);
    result = Render(R"({% import "module" as m without context %}{{ m.test() }}{{ m.test_inner() }})", params);
    EXPECT_EQ("[|23][|77]", result);
    result = Render(R"({% import "module" as m with context %}{{ m.test() }}{{ m.test_inner() }})", params);
    EXPECT_EQ("[42|23][42|77]", result);
    result = Render(R"({% from "module" import test %}{{ test() }})", params);
    EXPECT_EQ("[|23]", result);
    result = Render(R"({% from "module" import test without context %}{{ test() }})", params);
    EXPECT_EQ("[|23]", result);
    result = Render(R"({% from "module" import test with context %}{{ test() }})", params);
    EXPECT_EQ("[42|23]", result);
}

TEST_F(ImportTest, TestImportSyntax)
{
    Load(R"({% from "foo" import bar %})");
    Load(R"({% from "foo" import bar, baz %})");
    Load(R"({% from "foo" import bar, baz with context %})");
    Load(R"({% from "foo" import bar, baz, with context %})");
    Load(R"({% from "foo" import bar, with context %})");
    Load(R"({% from "foo" import bar, with, context %})");
    Load(R"({% from "foo" import bar, with with context %})");
}


// An imported macro's arguments in slots, and read by name from a template it includes,
// which runs under another tree than the names' (0118 P5b-3b)
TEST_F(ImportTest, MacroArgumentsReadInTheMacroTree)
{
    AddFile("long_args", "{% macro m(a_long_parameter_name) %}{% set x = 1 %}{{ a_long_parameter_name }}{{ x }}{% endmacro %}");
    EXPECT_EQ("p1 q1", Render(R"({% from 'long_args' import m %}{% import 'long_args' as lib %}{% for i in [1] %}{{ m('p') }} {{ lib.m(a_long_parameter_name='q') }}{% endfor %})"));
    AddFile("includes_header", "{% macro m(foo) %}{% include 'header' %}{% endmacro %}");
    EXPECT_EQ("[7|23]", Render(R"({% import 'includes_header' as lib %}{{ lib.m(7) }})"));
}

// The imported module owns its macros: a call must work even when the environment does
// not cache templates, so nothing else keeps the module alive
TEST_F(ImportTest, MacrosOutliveUncachedModule)
{
    m_env.GetSettings().cacheSize = 0;
    AddFile("lib", "{% macro m() %}hello{% endmacro %}");

    EXPECT_EQ("hello", Render(R"({% import "lib" as l %}{{ l.m() }})", {}));
    EXPECT_EQ("hello", Render(R"({% from "lib" import m %}{{ m() }})", {}));
    EXPECT_EQ("hellohello", Render(R"({% for i in [1, 2] %}{% import "lib" as l %}{{ l.m() }}{% endfor %})", {}));
}

// A module that keeps one of its own macros in its namespace: the macro holds the module's
// context weakly and the render owns the module, so no cycle keeps it alive (LSan checks the
// render frees it) and a second render makes a new one (docs/tasks/0167)
TEST_F(ImportTest, ModuleKeepingItsOwnExportIsFreed)
{
    AddFile("self_store", "{% set ns = namespace(f=none) %}{% macro m() %}M{% endmacro %}{% set ns.f = m %}{% macro call_it() %}{{ ns.f() }}{% endmacro %}");
    const std::string tpl = R"({% import 'self_store' as l %}{{ l.call_it() }}{{ l.ns.f() }}{% from 'self_store' import ns %}{{ ns.f() }})";
    EXPECT_EQ("MMM", Render(tpl));
    EXPECT_EQ("MMM", Render(tpl));
}

// A render started by a user callable owns the modules it imports, and the outer render's
// modules stay alive across it
TEST_F(ImportTest, NestedRenderKeepsItsOwnModules)
{
    AddFile("lib", "{% set v = 'V' %}{% macro m() %}{{ v }}{% endmacro %}");
    jinja2::ValuesMap params;
    params["inner"] = jinja2::MakeCallable([this]() { return Render("{% from 'lib' import m %}<{{ m() }}>"); });
    EXPECT_EQ("V<V>V<V>V", Render(R"({% from 'lib' import m %}{{ m() }}{{ inner() }}{{ m() }}{{ inner() }}{{ m() }})", params));
}

// An imported macro runs in its module: kept past the macro that imported it, or passed to
// another module's macro, it still reads the module's names
TEST_F(ImportTest, ImportedMacroRunsInItsModule)
{
    AddFile("mv", "{% set v = 'V' %}{% macro show() %}{{ v }}{% endmacro %}");
    AddFile("apply", "{% macro apply(f) %}<{{ f() }}>{% endmacro %}");
    EXPECT_EQ("<V>", Render(R"({% from 'apply' import apply %}{% from 'mv' import show %}{{ apply(show) }})"));
    EXPECT_EQ("V", Render(R"({% set ns = namespace(f=none) %}{% macro load() %}{% from 'mv' import show %}{% set ns.f = show %}{% endmacro %}{{ load() }}{{ ns.f() }})"));
}
