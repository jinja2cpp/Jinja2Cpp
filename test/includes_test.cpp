#include "test_tools.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// Test cases are taken from the pandor/Jinja2 tests

class IncludeTest : public TemplateEnvFixture
{
protected:
    void SetUp() override
    {
        TemplateEnvFixture::SetUp();
        
        AddFile("header", "[{{ foo }}|{{ bar }}]");
        AddFile("header1", "{% set inner_foo = 10 %}[{{ foo }}|{{ bar }}]{{inner_foo}}");
        AddFile("o_printer", "({{ o }})");
        AddFile("missing_inner_header", "{% include 'missing' %}");

        m_env.AddGlobal("bar", 23);
    }
};

TEST_F(IncludeTest, TestContextInclude)
{
    jinja2::ValuesMap params{{"foo", 42}};
    
    auto result = Render(R"({% include "header1" with context %})", params);
    EXPECT_EQ("[42|23]10", result);
    result = Render(R"({% include "header" %})", params);
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

    auto testInclude = [&, this](const std::string& tpl, jinja2::ValuesMap vars) {
        vars["foo"] = 42;
        return Render(tpl, vars);
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
    const auto& error = renderResult.error();
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, error.GetCode());
    const auto& extraParams = error.GetExtraParams();
    ASSERT_EQ(1ULL, extraParams.size());
    const auto* filesList = std::get_if<jinja2::GenericList>(&extraParams[0].GetData());
    EXPECT_NE(nullptr, filesList);
    EXPECT_EQ(1ULL, filesList->GetSize().value());
    EXPECT_EQ("missing", (*filesList->begin()).AsString());
}

TEST_F(IncludeTest, TestMissingInnerIncludesError)
{
  jinja2::ValuesMap params{};

  jinja2::Template tpl(&m_env);
  auto loadResult = tpl.Load(R"({% include "missing_inner_header" %})");
  EXPECT_FALSE(!loadResult);

  auto renderResult = tpl.RenderAsString(params);
  EXPECT_TRUE(!renderResult);
  const auto& error = renderResult.error();
  EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, error.GetCode());
  const auto& extraParams = error.GetExtraParams();
  ASSERT_EQ(1ULL, extraParams.size());
  const auto* filesList = std::get_if<jinja2::GenericList>(&extraParams[0].GetData());
  EXPECT_NE(nullptr, filesList);
  EXPECT_EQ(1ULL, filesList->GetSize().value());
  EXPECT_EQ("missing", (*filesList->begin()).AsString());
}

// `ignore missing` skips the included template only when it is not found: a missing template
// included by it fails the render, as in Python's Jinja2 (docs/tasks/0254)
TEST_F(IncludeTest, IgnoreMissingCoversTheLookupOnly)
{
    jinja2::Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load(R"({% include "missing_inner_header" ignore missing %})").has_value());
    auto renderResult = tpl.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, renderResult.error().GetCode());
    EXPECT_EQ("missing_inner_header", renderResult.error().GetErrorLocation().fileName);

    ASSERT_TRUE(tpl.Load(R"({% include ["missing", "missing_inner_header"] ignore missing %})").has_value());
    renderResult = tpl.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, renderResult.error().GetCode());
}

TEST_F(IncludeTest, TestMissingIncludesError2)
{
    jinja2::ValuesMap params{};

    jinja2::Template tpl(&m_env);
    auto loadResult = tpl.Load(R"({% include ["missing", "missing2"] %})");
    EXPECT_FALSE(!loadResult);

    auto renderResult = tpl.RenderAsString(params);
    EXPECT_TRUE(!renderResult);
    const auto& error = renderResult.error();
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, error.GetCode());
    const auto& extraParams = error.GetExtraParams();
    // A list of names is marked as one (0458): Python's select_template raises TemplatesNotFound
    ASSERT_EQ(2ULL, extraParams.size());
    const auto* filesList = std::get_if<jinja2::GenericList>(&extraParams[0].GetData());
    EXPECT_NE(nullptr, filesList);
    EXPECT_EQ(2ULL, filesList->GetSize().value());
    EXPECT_EQ("list", extraParams[1].AsString());
    auto params_iter = filesList->begin();
    EXPECT_EQ("missing", (*params_iter++).AsString());
    EXPECT_EQ("missing2", (*params_iter++).AsString());
}

// 0458: the names of an include as Python's get_or_select_template takes them
TEST_F(IncludeTest, NamesAsSelectTemplateTakesThem)
{
    auto render = [this](const std::string& source) {
        jinja2::Template tpl(&m_env);
        EXPECT_TRUE(tpl.Load(source).has_value()) << source;
        return tpl.RenderAsString({});
    };
    auto expectNotFound = [](const auto& result, size_t names, bool selected, const std::string& source) {
        ASSERT_FALSE(result.has_value()) << source;
        EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, result.error().GetCode()) << source;
        const auto& extraParams = result.error().GetExtraParams();
        ASSERT_EQ(selected ? 2ULL : 1ULL, extraParams.size()) << source;
        const auto* filesList = std::get_if<jinja2::GenericList>(&extraParams[0].GetData());
        ASSERT_NE(nullptr, filesList) << source;
        EXPECT_EQ(names, filesList->GetSize().value()) << source;
        if (selected)
        {
            EXPECT_EQ("list", extraParams[1].AsString()) << source;
        }
    };
    // A list of one name, an empty list and none select (TemplatesNotFound); a string does not
    expectNotFound(render("{% include ['missing'] %}"), 1, true, "list of one");
    expectNotFound(render("{% include ('missing',) %}"), 1, true, "tuple of one");
    expectNotFound(render("{% include [] %}"), 0, true, "empty list");
    expectNotFound(render("{% include none %}"), 0, true, "none");
    expectNotFound(render("{% include 'missing' %}"), 1, false, "string");
    EXPECT_EQ("", render("{% include none ignore missing %}").value());
    EXPECT_EQ("", render("{% include [] ignore missing %}").value());

    // What is neither a name nor iterable is Python's TypeError, `ignore missing` or not
    for (const std::string source : { "{% include 5 %}", "{% include 5 ignore missing %}", "{% include 1.5 %}", "{% include true %}" })
    {
        auto result = render(source);
        ASSERT_FALSE(result.has_value()) << source;
        EXPECT_EQ(jinja2::ErrorCode::TemplateRuntimeError, result.error().GetCode()) << source;
        ASSERT_EQ(2ULL, result.error().GetExtraParams().size()) << source;
        EXPECT_EQ("TypeError", result.error().GetExtraParams()[1].AsString()) << source;
    }
    auto number = render("{% include 5 %}");
    ASSERT_FALSE(number.has_value());
    EXPECT_EQ("'int' object is not iterable", number.error().GetExtraParams()[0].AsString());

    // A name the caller's values hold, as a string, a list item or a map item, is a name
    jinja2::Template byValue(&m_env);
    ASSERT_TRUE(byValue.Load("{% include n %}|{% include l %}|{% include d.k %}|{% for i in l %}{% include i %}{% endfor %}").has_value());
    const jinja2::ValuesMap values{ { "foo", 1 },
                                    { "bar", 2 },
                                    { "n", "header" },
                                    { "l", jinja2::ValuesList{ "header" } },
                                    { "d", jinja2::ValuesMap{ { "k", std::string("header") } } } };
    const auto byValueResult = byValue.RenderAsString(values);
    ASSERT_TRUE(byValueResult.has_value()) << byValueResult.error().ToString();
    EXPECT_EQ("[1|2]|[1|2]|[1|2]|[1|2]", byValueResult.value());

    // An undefined name fails as it is used, whatever its policy (UndefinedError)
    for (const std::string source : { "{% include nope %}", "{% include nope ignore missing %}" })
    {
        auto result = render(source);
        ASSERT_FALSE(result.has_value()) << source;
        EXPECT_EQ(jinja2::ErrorCode::UndefinedError, result.error().GetCode()) << source;
    }
}

// RealFileSystem names follow Python's FileSystemLoader: a ".." piece is not found
TEST_F(IncludeTest, RealFileSystemNameRules)
{
    // Rooted at the copied test data: "../test_data/..." names an existing file through ".."
    m_env.AddFilesystemHandler(std::string(), std::make_shared<jinja2::RealFileSystem>("test_data"));

    EXPECT_EQ("Hello World!", Render(R"({% include "simple_template1.j2tpl" %})"));
    EXPECT_EQ("Hello World!", Render(R"({% include "/simple_template1.j2tpl" %})"));
    EXPECT_EQ("", Render(R"({% include "../test_data/simple_template1.j2tpl" ignore missing %})"));
    EXPECT_EQ("", Render(R"({% include ["../test_data/simple_template1.j2tpl", "./../test_data/simple_template1.j2tpl"] ignore missing %})"));

    jinja2::Template tpl(&m_env);
    ASSERT_TRUE(!!tpl.Load(R"({% include "../test_data/simple_template1.j2tpl" %})"));
    auto renderResult = tpl.RenderAsString({});
    ASSERT_FALSE(renderResult);
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, renderResult.error().GetCode());
}

TEST_F(IncludeTest, TestContextIncludeWithOverrides)
{
    AddFile("item", "{{ item }}");
    EXPECT_EQ("123", Render(R"({% for item in [1, 2, 3] %}{% include 'item' %}{% endfor %})"));
}

// The included template reads the loop's name through the loop's own tree (0118 P5b-3b)
TEST_F(IncludeTest, LoopNameReadByAnInclude)
{
    AddFile("long_item", "{{ item_with_a_long_name }};");
    EXPECT_EQ("1;2;", Render(R"({% for item_with_a_long_name in [1, 2] %}{% include 'long_item' %}{% endfor %})"));
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

    EXPECT_EQ("\n\n\n\n(FOO)\n", result);
}

// A list of names a host passes (a GenericList, a reflected vector, a generated list) is tried
// name by name, as a template list is (docs/tasks/0432)
TEST_F(IncludeTest, TestChoiceIncludesFromHostList)
{
    const std::vector<std::string> names{ "missing", "header" };
    jinja2::ValuesMap params{ { "foo", 42 }, { "x", jinja2::MakeGenericList(names.begin(), names.end()) } };
    EXPECT_EQ("[42|23]", Render(R"({% include x %})", params));

    params["x"] = jinja2::Reflect(names);
    EXPECT_EQ("[42|23]", Render(R"({% include x %})", params));

    params["x"] = jinja2::Value(jinja2::MakeGenericList(jinja2::ListGenerator([n = 0]() mutable -> std::optional<jinja2::Value> {
        static const char* const generated[] = { "missing", "header" };
        return n < 2 ? std::optional<jinja2::Value>(jinja2::Value(generated[n++])) : std::nullopt;
    })));
    EXPECT_EQ("[42|23]", Render(R"({% include x %})", params));

    // Nothing found: the names tried are the error's list
    jinja2::Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{% include x %}").has_value());
    const std::vector<std::string> none{ "missing", "missing2" };
    auto result = tpl.RenderAsString({ { "x", jinja2::MakeGenericList(none.begin(), none.end()) } });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, result.error().GetCode());
    const auto* tried = std::get_if<jinja2::GenericList>(&result.error().GetExtraParams().at(0).GetData());
    ASSERT_NE(nullptr, tried);
    EXPECT_EQ(2ULL, tried->GetSize().value());
}
