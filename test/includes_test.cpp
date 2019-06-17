#include <iostream>
#include <string>

#include "test_tools.h"

// #include <jinja2cpp/generic_list_iterator.h>

// Test cases are taken from the pandor/Jinja2 tests

class IncludeTest : public TemplateEnvFixture
{
protected:
    void SetUp() override
    {
        TemplateEnvFixture::SetUp();
        
        AddFile("header", "[{{ foo }}|{{ bar }}]");
        AddFile("o_printer", "({{ o }})");

        m_env.AddGlobal("bar", 23);
    }
};

TEST_F(IncludeTest, TestContextInclude)
{
    jinja2::ValuesMap params{{"foo", 42}};
    
    auto result = Render(R"({% include "header" %})", params);
    EXPECT_EQ("[42|23]", result);
    result = Render(R"({% include "header" with context %})", params);
    EXPECT_EQ("[42|23]", result);
    result = Render(R"({% include "header" without context %})", params);
    EXPECT_EQ("[|23]", result);
    result = Render(R"({% include "header" ignore missing with context %})", params);
    EXPECT_EQ("[42|23]", result);
    result = Render(R"({% include "header" ignore missing without context %})", params);
    EXPECT_EQ("[|23]", result);
}

TEST_F(IncludeTest, TestChoiceIncludes)
{
    jinja2::ValuesMap params{{"foo", 42}};
    
    auto result = Render(R"({% include ["missing", "header"] %})", params);
    EXPECT_EQ("[42|23]", result);
    
    result = Render(R"({% include ["missing", "missing2"] ignore missing %})", params);
    EXPECT_EQ("", result);

    auto testInclude = [&, this](std::string tpl, jinja2::ValuesMap params)
    {
        params["foo"] = 42;
        return Render(std::move(tpl), params);
    };

    EXPECT_EQ("[42|23]", testInclude(R"({% include ["missing", "header"] %})", {}));
    EXPECT_EQ("[42|23]", testInclude(R"({% include x %})", {{"x", jinja2::ValuesList{jinja2::Value("missing"), jinja2::Value("header")}}}));
    EXPECT_EQ("[42|23]", testInclude(R"({% include [x, "header"] %})", {{"x", jinja2::Value("header")}}));
    EXPECT_EQ("[42|23]", testInclude(R"({% include x %})", {{"x", jinja2::Value("header")}}));
    EXPECT_EQ("[42|23]", testInclude(R"({% include [x] %})", {{"x", jinja2::Value("header")}}));
    EXPECT_EQ("[42|23]", testInclude(R"({% include "head" ~ x %})", {{"x", jinja2::Value("er")}}));
}

TEST_F(IncludeTest, TestMissingIncludesError1)
{
    jinja2::ValuesMap params{};

    jinja2::Template tpl(&m_env);
    auto loadResult = tpl.Load(R"({% include "missing" %})");
    EXPECT_FALSE(!loadResult);

    auto renderResult = tpl.RenderAsString(params);
    EXPECT_TRUE(!renderResult);
    auto error = renderResult.error();
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, error.GetCode());
    auto& extraParams = error.GetExtraParams();
    ASSERT_EQ(1ull, extraParams.size());
    auto filesList = nonstd::get_if<jinja2::GenericList>(&extraParams[0].data());
    EXPECT_NE(nullptr, filesList);
    EXPECT_EQ(1ull, filesList->GetSize().value());
    // EXPECT_EQ("missing", (*filesList->begin()).asString());
}

TEST_F(IncludeTest, TestMissingIncludesError2)
{
    jinja2::ValuesMap params{};

    jinja2::Template tpl(&m_env);
    auto loadResult = tpl.Load(R"({% include ["missing", "missing2"] %})");
    EXPECT_FALSE(!loadResult);

    auto renderResult = tpl.RenderAsString(params);
    EXPECT_TRUE(!renderResult);
    auto error = renderResult.error();
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, error.GetCode());
    auto& extraParams = error.GetExtraParams();
    ASSERT_EQ(1ull, extraParams.size());
    auto filesList = nonstd::get_if<jinja2::GenericList>(&extraParams[0].data());
    EXPECT_NE(nullptr, filesList);
    EXPECT_EQ(2ull, filesList->GetSize().value());
//    EXPECT_EQ("missing", filesList->GetValueByIndex(0).asString());
//    EXPECT_EQ("missing2", filesList->GetValueByIndex(1).asString());
}

TEST_F(IncludeTest, TestContextIncludeWithOverrides)
{
    AddFile("item", "{{ item }}");
    EXPECT_EQ("123", Render(R"({% for item in [1, 2, 3] %}{% include 'item' %}{% endfor %})"));
}

TEST_F(IncludeTest, TestUnoptimizedScopes)
{
    auto result = Render(
R"({% macro outer(o) %}
{% macro inner() %}
{% include "o_printer" %}
{% endmacro %}
{{ inner() }}
{%- endmacro %}
{{ outer("FOO") }})");

    EXPECT_EQ("(FOO)", result);
}
