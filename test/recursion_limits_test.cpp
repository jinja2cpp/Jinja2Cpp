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
#include <utility>

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

// A parenthesised assignment target nests like an expression (0118 P5b-3b)
TEST_F(RecursionLimitsTest, DeepTargetIsParseError)
{
    for (const auto& source : { "{% for " + Repeat("(", 5000) + "a" + Repeat(")", 5000) + " in [1] %}{% endfor %}",
                                "{% set " + Repeat("(", 5000) + "a" + Repeat(")", 5000) + " = 1 %}" })
    {
        Template tpl(&m_env);
        auto result = tpl.Load(source);
        ASSERT_FALSE(result.has_value()) << source.substr(0, 20);
        EXPECT_EQ(ErrorCode::RecursionLimitExceeded, result.error().GetCode()) << source.substr(0, 20);
    }
    EXPECT_EQ("123", Render("{% for " + Repeat("(", 30) + "a" + Repeat(")", 30) + " in [1, 2] %}{{ a }}{% endfor %}{% set " + Repeat("(", 30) + "b" + Repeat(")", 30) + " = 3 %}{{ b }}"));
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

// range() reports its length without storing items, so a list built from a huge one asked
// for terabytes up front and ASan aborted (fuzz finding, docs/tasks/0097). Sequences longer
// than 2^26 now fail the render at once. Python raises MemoryError for most of these; it
// keeps a sliced range lazy and rejects range + list and range * n with TypeError.
TEST_F(RecursionLimitsTest, HugeSequencesFailFast)
{
    const std::string templates[] = {
        "{{ range(9223372036854775807, -9223372036854775807, -922375807)|list }}",
        "{{ range(2**26 + 1)|list }}",
        "{{ range(2**40)|map('abs')|list }}",
        "{{ range(2**40)|map(attribute='x')|list }}",
        "{{ range(2**40)|select('odd')|list }}",
        "{{ range(2**40)[1:] }}",
        "{{ range(2**40) + [1] }}",
        "{{ range(2**40) * 2 }}",
        "{{ 'ab' * 2**40 }}",
        "{{ 'x'|center(2**40) }}",
        "{{ 'x\ny'|indent(2**40) }}",
        "{{ ('a\n' * 1000)|indent(2**30) }}",
        "{{ range(2**40)|reverse|first }}",
        "{{ ['x']|tojson(2**40) }}",
    };
    for (const auto& source : templates)
    {
        Template tpl(&m_env);
        ASSERT_TRUE(tpl.Load(source).has_value()) << source;
        auto result = tpl.RenderAsString({});
        ASSERT_FALSE(result.has_value()) << source;
        EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode()) << source;
        EXPECT_NE(std::string::npos, result.error().ToString().find("too long")) << result.error().ToString();
    }
    // An integer literal beyond int64 is a float here (docs/tasks/0178), which range()
    // refuses before computing a length; Python raises OverflowError
    {
        Template tpl(&m_env);
        ASSERT_TRUE(tpl.Load("{{ range(775807, -92233720368547753372036854775803, -9223758807)|list }}").has_value());
        auto result = tpl.RenderAsString({});
        ASSERT_FALSE(result.has_value());
        EXPECT_NE(std::string::npos, result.error().ToString().find("cannot be interpreted as an integer")) << result.error().ToString();
    }
    // Below the limit nothing changes
    Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load("{{ range(3)|list }}{{ 'x'|center(5) }}{{ 'ab' * 2 }}{{ range(2**40)|random is number }}").has_value());
    EXPECT_EQ("[0, 1, 2]  x  ababTrue", tpl.RenderAsString({}).value());
    TemplateW wide(&m_env);
    ASSERT_TRUE(wide.Load(L"{{ 'x'|center(2**40) }}").has_value());
    EXPECT_FALSE(wide.RenderAsString({}).has_value());
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
    ASSERT_EQ(0, pthread_attr_setstacksize(&attr, std::size_t{ 1024 } * 1024));
    pthread_t thread{};
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

// A loop nests a value as deep as it iterates (docs/tasks/0093). Destroying such a value
// recursed once per level and overflowed the stack; walks over it (printing, tojson, pprint,
// ==) still recurse, but stop with RecursionLimitExceeded like Python's RecursionError.
namespace
{
#ifdef __SANITIZE_THREAD__
#define JINJA2CPP_TEST_TSAN 1
#elif defined(__has_feature)
#if __has_feature(thread_sanitizer)
#define JINJA2CPP_TEST_TSAN 1
#endif
#endif
#ifdef JINJA2CPP_TEST_TSAN
// Deep enough to overflow the walks' thread under TSan too. Deeper, the access history of
// the whole run makes LLVM 18's TSan reset its traces, which fails its own CHECK
// (trace_part_recycle) and corrupts memory on the CI runners
constexpr char DeepLevels[] = "20000";
#else
constexpr char DeepLevels[] = "100000";
#endif

std::string DeepLists()
{
    return "{% set ns = namespace(v=[], w=[]) %}{% for i in range(" + std::string(DeepLevels) + ") %}{% set ns.v = [ns.v] %}{% set ns.w = [ns.w] %}{% endfor %}";
}

std::string DeepDicts()
{
    return "{% set ns = namespace(v={}, w={}) %}{% for i in range(" + std::string(DeepLevels) + ") %}{% set ns.v = {'a': ns.v} %}{% set ns.w = {'a': ns.w} %}{% endfor %}";
}

void RenderDeepValues(TemplateEnv* env)
{
    const std::string lists = DeepLists();
    const std::string dicts = DeepDicts();
    // Python compares an item with itself as equal without looking into it
    const std::pair<std::string, std::string> renders[] = {
        { lists + "{{ ns.v|length }} {{ ns.v == ns.v + [] }} {{ ns.v != ns.v + [] }} {{ ns.v < ns.v + [] }} {{ ns.v <= ns.v + [] }}", "1 True False False True" },
        { dicts + "{{ ns.v|length }} {{ ns.v == {'a': ns.v.a} }}", "1 True" },
        // tojson used to stop at 1000 levels; Python prints 5000
        { "{% set ns = namespace(v=[]) %}{% for i in range(1500) %}{% set ns.v = [ns.v] %}{% endfor %}{{ ns.v|tojson|length }}", "3002" },
    };
    for (const auto& [source, expected] : renders)
    {
        Template tpl(env);
        ASSERT_TRUE(tpl.Load(source).has_value()) << source.substr(source.size() - 30);
        auto result = tpl.RenderAsString({});
        EXPECT_EQ(expected, result ? result.value() : result.error().ToString()) << source.substr(source.size() - 30);
    }
}

void WalkDeepValues(TemplateEnv* env)
{
    const std::string lists = DeepLists();
    const std::string dicts = DeepDicts();
    const std::string walks[] = {
        lists + "{{ ns.v }}",
        lists + "{{ ns.v|tojson }}",
        lists + "{{ ns.v|pprint }}",
        lists + "{{ ns.v < ns.w }}",
        dicts + "{{ ns.v }}",
        dicts + "{{ ns.v == ns.w }}",
    };
    for (const auto& source : walks)
    {
        Template tpl(env);
        ASSERT_TRUE(tpl.Load(source).has_value()) << source.substr(source.size() - 30);
        auto result = tpl.RenderAsString({});
        EXPECT_FALSE(result.has_value()) << source.substr(source.size() - 30);
        if (!result)
        {
            EXPECT_EQ(ErrorCode::RecursionLimitExceeded, result.error().GetCode()) << source.substr(source.size() - 30) << ": " << result.error().ToString();
        }
    }
}

#if defined(__linux__) || defined(__APPLE__)
struct DeepValuesJob
{
    void (*check)(TemplateEnv*);
    TemplateEnv* env;
};

void* RunDeepValuesJob(void* job)
{
    auto* deep = static_cast<DeepValuesJob*>(job);
    deep->check(deep->env);
    return nullptr;
}

// On a thread of a known stack: a main thread may have a stack deep enough for the walks
void RunOnThread(std::size_t stackSize, void (*check)(TemplateEnv*), TemplateEnv* env)
{
    pthread_attr_t attr;
    ASSERT_EQ(0, pthread_attr_init(&attr));
    ASSERT_EQ(0, pthread_attr_setstacksize(&attr, stackSize));
    DeepValuesJob job{ check, env };
    pthread_t thread{};
    ASSERT_EQ(0, pthread_create(&thread, &attr, RunDeepValuesJob, &job));
    pthread_join(thread, nullptr);
    pthread_attr_destroy(&attr);
}
#endif
} // namespace

TEST_F(RecursionLimitsTest, DeepValues)
{
#if defined(__linux__) || defined(__APPLE__)
    // The renders need room for 1500 levels of tojson; the walks a stack their depth surely overflows
    RunOnThread(std::size_t{ 4 } * 1024 * 1024, RenderDeepValues, &m_env);
    RunOnThread(std::size_t{ 1024 } * 1024, WalkDeepValues, &m_env);
#else
    RenderDeepValues(&m_env);
    WalkDeepValues(&m_env);
#endif
}
