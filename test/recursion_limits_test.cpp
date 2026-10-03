// Nesting and recursion limits (docs/tasks/0003): templates Python Jinja2 stops with
// RecursionError or SyntaxError report ErrorCode::RecursionLimitExceeded instead of
// overflowing the stack, and nesting Jinja2 accepts still renders.
#include "test_tools.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <string>

using namespace jinja2;

namespace
{

std::string Repeat(const std::string& part, std::size_t count)
{
    std::string result;
    result.reserve(part.size() * count);
    for (std::size_t i = 0; i < count; ++i)
    {
        result += part;
    }
    return result;
}

// '{{ ' + open * depth + '1' + close * depth + ' }}'
std::string Nested(const std::string& open, const std::string& close, std::size_t depth)
{
    return "{{ " + Repeat(open, depth) + "1" + Repeat(close, depth) + " }}";
}

// "item, item, ..., item" with count items
std::string Items(const std::string& item, std::size_t count)
{
    std::string result = item;
    for (std::size_t i = 1; i < count; ++i)
    {
        result += ", " + item;
    }
    return result;
}

} // namespace

using RecursionLimitsTest = TemplateEnvFixture;

TEST_F(RecursionLimitsTest, DeepExpressionIsParseError)
{
    const std::string templates[] = {
        Nested("(", ")", 5000),    Nested("[", "]", 5000),          Nested("not ", "", 5000), Nested("-", "", 5000),
        Nested("+", "", 5000),     Nested("{'a': ", "}", 5000),     Nested("x(", ")", 5000),  Nested("x[", "]", 5000),
        Nested("x|f(", ")", 5000), Nested("1 if 1 else ", "", 5000),
    };
    for (const auto& source : templates)
    {
        Template tpl(&m_env);
        auto result = tpl.Load(source);
        ASSERT_FALSE(result.has_value()) << source.substr(0, 20);
        EXPECT_EQ(ErrorCode::RecursionLimitExceeded, result.error().GetCode()) << source.substr(0, 20);
    }

    TemplateW tpl(&m_env);
    auto result = tpl.Load(L"{{ " + std::wstring(5000, L'(') + L"1" + std::wstring(5000, L')') + L" }}");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::RecursionLimitExceeded, result.error().GetCode());
}

TEST_F(RecursionLimitsTest, NestingJinja2AcceptsRenders)
{
    // Python Jinja2 fails at about 80 nested brackets and 300 unary operators
    EXPECT_EQ("1", Render(Nested("(", ")", 60)));
    EXPECT_EQ("1", Render(Nested("[", "][0]", 40)));
    EXPECT_EQ("1", Render(Nested("-", "", 200)));
    EXPECT_EQ("True", Render(Nested("not ", "", 200)));
}

TEST_F(RecursionLimitsTest, ErrorMessage)
{
    Template tpl(&m_env);
    auto result = tpl.Load(Nested("(", ")", 5000));
    ASSERT_FALSE(result.has_value());
    EXPECT_NE(std::string::npos, ErrorToString(result.error()).find("error: Maximum recursion depth exceeded\n")) << ErrorToString(result.error());
}

TEST_F(RecursionLimitsTest, InfiniteRecursionIsRenderError)
{
    AddFile("include_self", "{% include 'include_self' %}");
    AddFile("extends_self", "{% extends 'extends_self' %}");
    AddFile("import_self", "{% import 'import_self' as m %}");
    const std::string templates[] = {
        "{% macro m() %}{{ m() }}{% endmacro %}{{ m() }}",
        "{% include 'include_self' %}",
        "{% extends 'extends_self' %}",
        "{% import 'import_self' as m %}",
        "{% block b %}{{ self.b() }}{% endblock %}",
        "{% set l = [[1]] %}{% for i in l recursive %}{{ loop(l) }}{% endfor %}",
    };
    for (const auto& source : templates)
    {
        Template tpl(&m_env);
        ASSERT_TRUE(tpl.Load(source).has_value()) << source;
        auto result = tpl.RenderAsString({});
        ASSERT_FALSE(result.has_value()) << source;
        EXPECT_EQ(ErrorCode::RecursionLimitExceeded, result.error().GetCode()) << source;
        EXPECT_EQ("noname.j2tpl:1:1: error: Maximum recursion depth exceeded\n", ErrorToString(result.error())) << source;
    }

    TemplateW tpl(&m_env);
    ASSERT_TRUE(tpl.Load(L"{% macro m() %}{{ m() }}{% endmacro %}{{ m() }}").has_value());
    auto result = tpl.RenderAsString({});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::RecursionLimitExceeded, result.error().GetCode());
}

TEST_F(RecursionLimitsTest, RecursionJinja2AcceptsRenders)
{
    // Python Jinja2 renders 200 nested macro calls and fails at about 250
    EXPECT_EQ("ok", Render("{% macro m(n) %}{% if n > 0 %}{{ m(n - 1) }}{% else %}ok{% endif %}{% endmacro %}{{ m(200) }}"));
    // Loops and blocks inside the recursion do not count against it
    EXPECT_EQ("ok",
              Render("{% macro m(n) %}{% for i in [1] %}{% for j in [1] %}{% if n > 0 %}{{ m(n - 1) }}{% else %}ok{% endif %}{% endfor %}{% endfor %}{% endmacro %}{{ m(200) }}"));
    AddFile("countdown", "{% if n > 0 %}{% for i in [1] %}{% with n = n - 1 %}{% include 'countdown' %}{% endwith %}{% endfor %}{% else %}ok{% endif %}");
    EXPECT_EQ("ok", Render("{% include 'countdown' %}", { { "n", 200 } }));

    ValuesList tree;
    for (int i = 0; i < 200; ++i)
    {
        tree = ValuesList{ ValuesMap{ { "children", tree } } };
    }
    EXPECT_EQ("200", Render("{% for n in tree recursive %}{% if not n.children %}{{ loop.depth }}{% endif %}{{ loop(n.children) }}{% endfor %}", { { "tree", tree } }));
}

TEST_F(RecursionLimitsTest, LongChains)
{
    // The parser reads chains in a loop but the evaluator recurses once per operator.
    // Python Jinja2 fails at 200-300 chained operators.
    EXPECT_EQ("200", Render("{{ " + Repeat("1 + ", 199) + "1 }}"));
    EXPECT_EQ("3", Render("{{ x" + Repeat("|abs", 200) + " }}", { { "x", -3 } }));
    EXPECT_EQ("3", Render("{{ ((x" + Repeat("|abs", 100) + ")" + Repeat("|abs", 100) + ")" + Repeat("|abs", 50) + " }}", { { "x", -3 } }));

    const std::string templates[] = {
        "{{ " + Repeat("1 + ", 20000) + "1 }}",
        "{{ " + Repeat("1 * ", 20000) + "1 }}",
        "{{ " + Repeat("1 and ", 20000) + "1 }}",
        "{{ " + Repeat("'a' ~ ", 20000) + "'a' }}",
        "{{ x" + Repeat("|abs", 20000) + " }}",
        "{{ x" + Repeat(".a", 20000) + " }}",
        "{{ x" + Repeat("[0]", 20000) + " }}",
        "{{ x" + Repeat("()", 20000) + " }}",
        "{% filter upper" + Repeat("|lower", 20000) + " %}{% endfilter %}",
        // A chain inside brackets continues outside them: the depth is their sum
        "{{ " + Repeat("(", 60) + "x" + Repeat("|abs)", 60) + Repeat("|abs", 200) + " }}",
    };
    for (const auto& source : templates)
    {
        Template tpl(&m_env);
        auto result = tpl.Load(source);
        ASSERT_FALSE(result.has_value()) << source.substr(0, 20);
        EXPECT_EQ(ErrorCode::RecursionLimitExceeded, result.error().GetCode()) << source.substr(0, 20);
    }
}

TEST_F(RecursionLimitsTest, SiblingsDoNotAddUp)
{
    // Long literals and argument lists are flat, however many operators their items use
    const ValuesMap params{ { "x", -3 }, { "d", ValuesMap{ { "a", 1 } } }, { "s", "abc" } };
    EXPECT_EQ("1100", Render("{{ [" + Items("d.a", 1100) + "]|length }}", params));
    EXPECT_EQ("1100", Render("{{ (" + Items("x|abs", 1100) + ")|length }}", params));
    EXPECT_EQ("1100", Render("{% macro m() %}{{ varargs|length }}{% endmacro %}{{ m(" + Items("x|abs", 1100) + ") }}", params));
    EXPECT_EQ("600", Render("{% set rows = [" + Items("{'id': x|abs, 'name': s|title, 'val': d.a}", 600) + "] %}{{ rows|length }}", params));
    EXPECT_EQ("ok", Render("{% for i in [" + Items("x|abs", 1100) + "] %}{% endfor %}ok", params));
}

TEST_F(RecursionLimitsTest, NestedBlocks)
{
    // Python Jinja2 fails at about 100 nested blocks
    EXPECT_EQ("x", Render(Repeat("{% if 1 %}", 90) + "x" + Repeat("{% endif %}", 90)));

    const std::string templates[] = {
        Repeat("{% if 1 %}", 300) + "x" + Repeat("{% endif %}", 300),
        Repeat("{% for i in [1] %}", 300) + "x" + Repeat("{% endfor %}", 300),
        Repeat("{% filter upper %}", 300) + "x" + Repeat("{% endfilter %}", 300),
        Repeat("{% with %}", 300) + "x" + Repeat("{% endwith %}", 300),
    };
    for (const auto& source : templates)
    {
        Template tpl(&m_env);
        auto result = tpl.Load(source);
        ASSERT_FALSE(result.has_value()) << source.substr(0, 20);
        EXPECT_EQ(ErrorCode::RecursionLimitExceeded, result.error().GetCode()) << source.substr(0, 20);
    }
}
