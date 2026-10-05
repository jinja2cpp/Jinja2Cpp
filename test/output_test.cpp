// The output writer: values written without ValueRenderer, the buffer between the
// render and its string, and long fragments that skip it (docs/tasks/0113)

#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <cstdint>
#include <string>
#include <string_view>

using namespace jinja2;

namespace
{
std::wstring Widen(const std::string& str)
{
    return { str.begin(), str.end() };
}

// Renders `tpl` narrow and wide, twice each (the second render reserves from the first), and
// checks the output
void ExpectRender(const std::string& tpl, const std::string& expected, const ValuesMap& params = {})
{
    Template narrow;
    ASSERT_TRUE(narrow.Load(tpl));
    TemplateW wide;
    ASSERT_TRUE(wide.Load(Widen(tpl)));
    for (int pass = 0; pass != 2; ++pass)
    {
        auto narrowResult = narrow.RenderAsString(params);
        ASSERT_TRUE(narrowResult) << narrowResult.error();
        EXPECT_EQ(expected, narrowResult.value());
        auto wideResult = wide.RenderAsString(params);
        ASSERT_TRUE(wideResult);
        EXPECT_EQ(Widen(expected), wideResult.value());
    }
}
} // namespace

TEST(OutputTest, Integers)
{
    ExpectRender("{{ 0 }} {{ 7 }} {{ 10 }} {{ 99 }} {{ 100 }} {{ 12345 }} {{ -1 }} {{ -10 }} {{ -101 }}", "0 7 10 99 100 12345 -1 -10 -101");
    ExpectRender("{{ 9223372036854775807 }} {{ -9223372036854775807 - 1 }}", "9223372036854775807 -9223372036854775808");
    ValuesMap params{ { "small", int64_t{ 5 } }, { "big", INT64_MIN }, { "list", ValuesList{ int64_t{ 42 }, int64_t{ -3 } } } };
    ExpectRender("{{ small }} {{ big }} {{ list[0] }}{{ list[1] }}", "5 -9223372036854775808 42-3", params);
}

TEST(OutputTest, OtherValues)
{
    // Booleans have their own path; the rest go through ValueRenderer after the buffer
    ExpectRender("a{{ true }}b{{ false }}c{{ none }}d{{ 1.5 }}e{{ [1, 'x'] }}f{{ {'k': 2} }}g{{ undefined_name }}h",
                 "aTruebFalsecNoned1.5e[1, 'x']f{'k': 2}gh");
    ValuesMap params{ { "flag", true }, { "real", 2.25 }, { "nothing", Value() } };
    ExpectRender("{{ flag }}|{{ real }}|{{ nothing }}|", "True|2.25|None|", params);
}

TEST(OutputTest, Strings)
{
    ValuesMap params{ { "str", std::string("text") }, { "view", std::string_view("view") }, { "empty", std::string() } };
    ExpectRender("<{{ str }}><{{ view }}><{{ empty }}><{{ 'literal' }}><{{ str ~ '!' }}><{{ str | upper }}>",
                 "<text><view><><literal><text!><TEXT>",
                 params);
}

TEST(OutputTest, OutputLongerThanTheBuffer)
{
    std::string expected;
    for (int idx = 0; idx != 2000; ++idx)
    {
        expected += "<td>" + std::to_string(idx) + "</td>";
    }
    ExpectRender("{% for i in range(2000) %}<td>{{ i }}</td>{% endfor %}", expected);
}

TEST(OutputTest, LongFragments)
{
    // Text fragments around the buffer and the long-fragment thresholds, between values
    // that sit in the buffer when the long text arrives
    std::string tpl;
    std::string expected;
    for (size_t length : { 1, 15, 16, 17, 127, 128, 129, 511, 512, 513, 2000 })
    {
        const std::string text(length, static_cast<char>('a' + length % 26));
        tpl += "{{ " + std::to_string(length) + " }}" + text;
        expected += std::to_string(length) + text;
    }
    ExpectRender(tpl, expected);
    ExpectRender(tpl + "{% set captured %}" + tpl + "{% endset %}[{{ captured }}]", expected + "[" + expected + "]");
}

TEST(OutputTest, CapturedOutput)
{
    // Blocks rendered to a string of their own: set blocks, macros, call blocks, filter blocks
    const std::string tpl = R"({% set s %}{% for i in range(300) %}{{ i }},{% endfor %}{% endset %}{{ s | length }}
{% macro m(n) %}{% for i in range(n) %}{{ i }}{% endfor %}{% endmacro %}{{ m(200) | length }}
{% macro wrap() %}[{{ caller() }}]{% endmacro %}{% call wrap() %}{{ m(3) }}{% endcall %}
{% filter upper %}{% for i in range(100) %}ab{{ i }}{% endfor %}{% endfilter %})";
    std::string filtered;
    for (int idx = 0; idx != 100; ++idx)
    {
        filtered += "AB" + std::to_string(idx);
    }
    ExpectRender(tpl, "1090\n490\n[012]\n" + filtered);
}
