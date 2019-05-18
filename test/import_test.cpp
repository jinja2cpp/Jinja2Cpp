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

        AddFile("module", "{% macro test() %}[{{ foo }}|{{ bar }}]{% endmacro %}");
        AddFile("header", "[{{ foo }}|{{ 23 }}]");
        AddFile("o_printer", "({{ o }})");
    }
};

TEST_F(ImportTest, TestContextImports)
{
    jinja2::ValuesMap params{{"foo", 42}};

    auto result = Render(R"({% import "module" as m %}{{ m.test() }})", params);
    EXPECT_EQ("[|23]", result);
    result = Render(R"({% import "module" as m without context %}{{ m.test() }})", params);
    EXPECT_EQ("[|23]", result);
    result = Render(R"({% import "module" as m with context %}{{ m.test() }})", params);
    EXPECT_EQ("[42|23]", result);
    result = Render(R"({% from "module" import test %}{{ test() }})", params);
    EXPECT_EQ("[|23]", result);
    result = Render(R"({% from "module" import test without context %}{{ test() }})", params);
    EXPECT_EQ("[|23]", result);
    result = Render(R"({% from "module" import test with context %}{{ test() }})", params);
    EXPECT_EQ("[42|23]", result);
}