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
#include <stdexcept>
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

// A list or a dict among the names fails as Jinja2's template cache hashes it, `ignore missing` or not, once the
// names before it are tried (docs/tasks/0494)
TEST_F(IncludeTest, UnhashableNameIsTypeError)
{
    for (const std::string source : { "{% include [[]] %}", "{% include [[]] ignore missing %}", "{% include [{}] %}", "{% include ['missing', []] %}", "{% include [1, []] %}", "{% include [([],)] %}", "{% include [{}.keys()] %}" })
    {
        jinja2::Template tpl(&m_env);
        ASSERT_TRUE(tpl.Load(source).has_value()) << source;
        auto result = tpl.RenderAsString({});
        ASSERT_FALSE(result.has_value()) << source;
        EXPECT_EQ(jinja2::ErrorCode::TemplateRuntimeError, result.error().GetCode()) << source;
        ASSERT_EQ(2ULL, result.error().GetExtraParams().size()) << source;
        EXPECT_EQ("TypeError", result.error().GetExtraParams()[1].AsString()) << source;
    }
    jinja2::Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{% include [{}] %}").has_value());
    EXPECT_EQ("unhashable type: 'dict'", tpl.RenderAsString({}).error().GetExtraParams()[0].AsString());
    // A name found before it renders; a tuple of strings is hashable
    ASSERT_TRUE(tpl.Load("{% include ['header', []] %}|{% include [('a',), 'header'] %}").has_value());
    EXPECT_EQ("[|23]|[|23]", tpl.RenderAsString({}).value());
}

// An undefined name among the names keeps its message for TemplatesNotFound (ExtraParams[2], docs/tasks/0498)
TEST_F(IncludeTest, UndefinedNamesKeepTheirMessage)
{
    jinja2::Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{% include ['missing', nope, d.x] %}").has_value());
    auto result = tpl.RenderAsString({ { "d", jinja2::ValuesMap{} } });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, result.error().GetCode());
    const auto& extraParams = result.error().GetExtraParams();
    ASSERT_EQ(3ULL, extraParams.size());
    EXPECT_EQ("list", extraParams[1].AsString());
    const jinja2::ValuesList* messages = nullptr;
    ASSERT_TRUE(extraParams[2].AsList(messages));
    ASSERT_EQ(3ULL, messages->size());
    EXPECT_TRUE((*messages)[0].IsNone());
    EXPECT_EQ("'nope' is undefined", (*messages)[1].AsString());
    EXPECT_EQ("'dict object' has no attribute 'x'", (*messages)[2].AsString());
    // No undefined name, no third entry
    ASSERT_TRUE(tpl.Load("{% include ['missing', none] %}").has_value());
    EXPECT_EQ(2ULL, tpl.RenderAsString({}).error().GetExtraParams().size());
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

// The tags call the environment's join-path function with the name of the template whose source holds them, and
// load what it returns, as Python's Environment.join_path does (docs/tasks/0410)
TEST_F(IncludeTest, JoinPathResolvesTagNames)
{
    std::vector<std::string> calls;
    m_env.SetJoinPath([&calls](const std::string& name, const std::string& parent) {
        calls.push_back(name + "@" + parent);
        if (name.compare(0, 2, "./") != 0)
        {
            return name;
        }
        return parent.substr(0, parent.rfind('/') + 1) + name.substr(2);
    });
    AddFile("a/x", "AX");
    AddFile("b/x", "BX");
    AddFile("a/y", "AY");
    AddFile("c/y", "CY");
    AddFile("c/z", "CZ");
    AddFile("a/mac", "{% macro f() %}{% include './x' %}{% endmacro %}{% macro g() %}[{{ caller() }}]{% endmacro %}{% set v = 'V' %}");
    AddFile("b/base", "{% include './x' %}{% block c %}{% endblock %}");
    AddFile("a/p", "{% include './x' %}");
    AddFile("b/p", "{% include './x' %}");

    auto renderNamed = [this](const std::string& name) {
        auto tpl = m_env.LoadTemplate(name);
        EXPECT_TRUE(tpl.has_value());
        if (!tpl)
        {
            return std::string();
        }
        auto result = tpl.value().RenderAsString({});
        EXPECT_TRUE(result.has_value());
        return result ? result.value() : std::string();
    };
    auto expectCalls = [&calls](std::vector<std::string> expected) {
        EXPECT_EQ(expected, calls);
        calls.clear();
    };

    AddFile("a/main", "{% include './x' %}");
    EXPECT_EQ("AX", renderNamed("a/main"));
    expectCalls({ "./x@a/main" });

    AddFile("a/imp", "{% import './mac' as m %}{{ m.v }}{% from './mac' import v as w %}{{ w }}");
    EXPECT_EQ("VV", renderNamed("a/imp"));
    expectCalls({ "./mac@a/imp", "./mac@a/imp" });

    AddFile("a/child", "{% extends './base0' %}{% block c %}{% include './y' %}{% endblock %}");
    AddFile("a/base0", "<{% block c %}{% endblock %}>");
    EXPECT_EQ("<AY>", renderNamed("a/child"));
    expectCalls({ "./base0@a/child", "./y@a/child" });

    AddFile("a/list", "{% include ['./none', './x'] %}");
    EXPECT_EQ("AX", renderNamed("a/list"));
    expectCalls({ "./none@a/list", "./x@a/list" });

    // The template of the macro, not of the caller
    AddFile("c/main", "{% import 'a/mac' as m %}{{ m.f() }}");
    EXPECT_EQ("AX", renderNamed("c/main"));
    expectCalls({ "a/mac@c/main", "./x@a/mac" });

    // The calling template for the body of a call block
    AddFile("c/call", "{% import 'a/mac' as m %}{% call m.g() %}{% include './y' %}{% endcall %}");
    EXPECT_EQ("[CY]", renderNamed("c/call"));
    expectCalls({ "a/mac@c/call", "./y@c/call" });

    // The base's own tag is joined with the base's name, a block of the child with the child's
    AddFile("c/child", "{% extends 'b/base' %}{% block c %}{% include './z' %}{% endblock %}");
    EXPECT_EQ("BXCZ", renderNamed("c/child"));
    expectCalls({ "b/base@c/child", "./x@b/base", "./z@c/child" });

    AddFile("c/loop", "{% for i in range(2) %}{% include './z' %}{% endfor %}");
    EXPECT_EQ("CZCZ", renderNamed("c/loop"));
    expectCalls({ "./z@c/loop", "./z@c/loop" });

    // The joined name is the cache key: two templates with the same tag name load different files
    AddFile("c/keys", "{% include 'a/p' %}{% include 'b/p' %}");
    EXPECT_EQ("AXBX", renderNamed("c/keys"));
    expectCalls({ "a/p@c/keys", "./x@a/p", "b/p@c/keys", "./x@b/p" });

    // A single name that is not found is reported joined
    AddFile("a/miss", "{% include './nope' %}");
    auto miss = m_env.LoadTemplate("a/miss");
    ASSERT_TRUE(miss.has_value());
    auto missResult = miss.value().RenderAsString({});
    ASSERT_FALSE(missResult.has_value());
    EXPECT_EQ(jinja2::ErrorCode::TemplateNotFound, missResult.error().GetCode());
    const auto* tried = std::get_if<jinja2::GenericList>(&missResult.error().GetExtraParams().at(0).GetData());
    ASSERT_NE(nullptr, tried);
    EXPECT_EQ("a/nope", (*tried->begin()).AsString());
    calls.clear();

    // A template without a name is not joined
    EXPECT_EQ("AX", Render("{% include 'a/x' %}"));
    expectCalls({});

    // An exception of the function fails the render
    m_env.SetJoinPath([](const std::string&, const std::string&) -> std::string { throw std::runtime_error("no join"); });
    // A template cached before keeps the function it was loaded with
    AddFile("a/main2", "{% include './x' %}");
    auto thrown = m_env.LoadTemplate("a/main2");
    ASSERT_TRUE(thrown.has_value());
    EXPECT_FALSE(thrown.value().RenderAsString({}).has_value());

    // Without a function the names load as written
    m_env.SetJoinPath({});
    AddFile("./x", "PLAIN");
    AddFile("a/main3", "{% include './x' %}");
    EXPECT_EQ("PLAIN", renderNamed("a/main3"));
}

TEST_F(IncludeTest, JoinPathWideTemplate)
{
    std::vector<std::string> calls;
    m_env.SetJoinPath([&calls](const std::string& name, const std::string& parent) {
        calls.push_back(name + "@" + parent);
        return "a/" + name;
    });
    AddFile("a/x", "AX");
    AddFile("w/main", "{% include 'x' %}");
    auto tpl = m_env.LoadTemplateW("w/main");
    ASSERT_TRUE(tpl.has_value());
    auto result = tpl.value().RenderAsString({});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(L"AX", result.value());
    EXPECT_EQ((std::vector<std::string>{ "x@w/main" }), calls);
}
