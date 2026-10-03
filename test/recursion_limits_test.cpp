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

#if defined(__linux__) || defined(__APPLE__)
#include <pthread.h>
#endif

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
        Nested("(", ")", 5000),
        Nested("[", "]", 5000),
        Nested("not ", "", 5000),
        Nested("-", "", 5000),
        Nested("+", "", 5000),
        Nested("{'a': ", "}", 5000),
        Nested("x(", ")", 5000),
        Nested("x[", "]", 5000),
        Nested("x|f(", ")", 5000),
        Nested("1 if 1 else ", "", 5000),
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
    // Python Jinja2 fails at 300-500 chained operators.
    EXPECT_EQ("400", Render("{{ " + Repeat("1 + ", 399) + "1 }}"));
    EXPECT_EQ("3", Render("{{ x" + Repeat("|abs", 350) + " }}", { { "x", -3 } }));
    EXPECT_EQ("3", Render("{{ ((x" + Repeat("|abs", 100) + ")" + Repeat("|abs", 100) + ")" + Repeat("|abs", 50) + " }}", { { "x", -3 } }));
    EXPECT_EQ("1", Render("{{ " + Repeat("-", 390) + "1 }}"));
    EXPECT_EQ("ok", Render("{{ " + Repeat("1 if 0 else ", 390) + "'ok' }}"));
    // Operands nest under their operator, not under each other
    EXPECT_EQ(Repeat("1", 300), Render("{{ x|string" + Repeat(" ~ x|string", 299) + " }}", { { "x", 1 } }));
    EXPECT_EQ("600", Render("{{ (1 + 1)" + Repeat(" + (1 + 1)", 299) + " }}"));

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
        "{{ " + Repeat("(", 60) + "x" + Repeat("|abs)", 60) + Repeat("|abs", 400) + " }}",
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
    std::string bindings = "a0=x|abs";
    for (int i = 1; i < 600; ++i)
    {
        bindings += ", a" + std::to_string(i) + "=x|abs";
    }
    EXPECT_EQ("3", Render("{% with " + bindings + " %}{{ a599 }}{% endwith %}", params));
    EXPECT_EQ("3", Render("{% macro m(" + bindings + ") %}{{ a599 }}{% endmacro %}{{ m() }}", params));
    EXPECT_EQ("u", Render("{{ d[" + Items("x|abs", 600) + "]|default('u') }}", params));
}

TEST_F(RecursionLimitsTest, NestedBlocks)
{
    // Python Jinja2 fails at about 100 nested blocks
    EXPECT_EQ("x", Render(Repeat("{% if 1 %}", 90) + "x" + Repeat("{% endif %}", 90)));
    // elif and else branches are not nested
    EXPECT_EQ("ok", Render("{% if 0 %}a" + Repeat("{% elif 0 %}b", 1000) + "{% else %}ok{% endif %}"));

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

TEST_F(RecursionLimitsTest, StackUseIsBounded)
{
    // Each limit alone allows these, but together they need more stack than a thread may
    // have: they render or stop with RecursionLimitExceeded, never crash
    const std::string templates[] = {
        "{% macro m(n) %}{% if n > 0 %}{{ m(n - 1)" + Repeat(" ~ ''", 200) + " }}{% else %}ok{% endif %}{% endmacro %}{{ m(250) }}",
        "{% macro m(n) %}" + Repeat("{% if 1 %}", 120) + "{% if n > 0 %}{{ m(n - 1) }}{% else %}ok{% endif %}" + Repeat("{% endif %}", 120) + "{% endmacro %}{{ m(250) }}",
    };
    for (const auto& source : templates)
    {
        Template tpl(&m_env);
        ASSERT_TRUE(tpl.Load(source).has_value()) << source.substr(0, 40);
        auto result = tpl.RenderAsString({});
        if (!result)
        {
            EXPECT_EQ(ErrorCode::RecursionLimitExceeded, result.error().GetCode()) << source.substr(0, 40);
        }
    }
}

#if defined(__linux__) || defined(__APPLE__)
namespace
{
struct SmallStackRender
{
    TemplateEnv* env;
    std::string source;
    nonstd::expected<std::string, ErrorInfo> result = std::string();
};

void* RenderOnSmallStack(void* arg)
{
    auto* job = static_cast<SmallStackRender*>(arg);
    Template tpl(job->env);
    auto loaded = tpl.Load(job->source);
    job->result = loaded ? tpl.RenderAsString({}) : nonstd::make_unexpected(loaded.error());
    return nullptr;
}
} // namespace

TEST_F(RecursionLimitsTest, SmallThreadStack)
{
    // 200 nested macro calls (Python renders them) do not fit a 1 MiB thread stack in every
    // build; the render stops before the stack runs out
    SmallStackRender job{ &m_env, "{% macro m(n) %}{% if n > 0 %}{{ m(n - 1)" + Repeat(" ~ ''", 20) + " }}{% else %}ok{% endif %}{% endmacro %}{{ m(200) }}" };
    pthread_attr_t attr;
    ASSERT_EQ(0, pthread_attr_init(&attr));
    ASSERT_EQ(0, pthread_attr_setstacksize(&attr, 1024 * 1024));
    pthread_t thread;
    ASSERT_EQ(0, pthread_create(&thread, &attr, RenderOnSmallStack, &job));
    pthread_join(thread, nullptr);
    pthread_attr_destroy(&attr);
    if (job.result)
    {
        EXPECT_EQ("ok", job.result.value());
    }
    else
    {
        EXPECT_EQ(ErrorCode::RecursionLimitExceeded, job.result.error().GetCode());
    }
}
#endif
