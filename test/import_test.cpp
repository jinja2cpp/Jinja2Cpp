#include <iostream>
#include <string>

#include "test_tools.h"

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

