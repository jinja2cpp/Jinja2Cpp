#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"

#include "test_tools.h"

using namespace jinja2;

using SetTest = BasicTemplateRenderer;

MULTISTR_TEST(SetTest, SimpleSetTest,
R"(
{% set val = intValue %}
localVal: {{val}}
paramsVal: {{intValue}}
)",
//------------
R"(
localVal: 3
paramsVal: 3
)"
)
{
    params = {
        {"intValue", 3},
        {"doubleValue", 12.123f},
        {"stringValue", "rain"},
        {"boolFalseValue", false},
        {"boolTrueValue", true},
    };
}

MULTISTR_TEST(SetTest, Tuple1AssignmentTest,
R"(
{% set firstName, lastName = emploee %}
firtsName: {{firstName}}
lastName: {{lastName}}
)",
//--------------
R"(
firtsName: John
lastName: Dow
)")
{
    params = {
        {"emploee", ValuesMap{
             {"firstName", "John"},
             {"lastName", "Dow"}
         }},
    };
}

MULTISTR_TEST(SetTest, Tuple2AssignmentTest,
R"(
{% set tuple = ("Hello", "World") %}
hello: {{tuple[0]}}
world: {{tuple[1]}}
)",
//------------
R"(
hello: Hello
world: World
)")
{
}

MULTISTR_TEST(SetTest, Tuple3AssignmentTest,
R"(
{% set tuple = ["Hello", "World"] %}
hello: {{tuple[0]}}
world: {{tuple[1]}}
)",
R"(
hello: Hello
world: World
)"
)
{
}


MULTISTR_TEST(SetTest, Tuple4AssignmentTest,
R"(
{% set dict = {'hello' = "Hello", 'world' = "World"} %}
hello: {{dict.hello}}
world: {{dict.world}}
)",
//--------
R"(
hello: Hello
world: World
)"
)
{
}

using WithTest = BasicTemplateRenderer;

MULTISTR_TEST(WithTest, SimpleTest,
R"(
{% with inner = 42 %}
{{ inner }}
{%- endwith %}
)",
//----------
"\n42"
)
{
}

MULTISTR_TEST(WithTest, MultiVarsTest,
R"(
{% with inner1 = 42, inner2 = 'Hello World' %}
{{ inner1 }}
{{ inner2 }}
{%- endwith %}
)",
//----------
"\n42\nHello World"
)
{
}

MULTISTR_TEST(WithTest, ScopeTest1,
R"(
{{ outer }}
{% with inner = 42, outer = 'Hello World' %}
{{ inner }}
{{ outer }}
{%- endwith %}
{{ outer }}
)",
//---------------
"\nWorld Hello\n42\nHello WorldWorld Hello\n"
)
{
    params = {{"outer", "World Hello"}};
}

MULTISTR_TEST(WithTest, ScopeTest2,
R"(
{{ outer }}
{% with outer = 'Hello World', inner = outer %}
{{ inner }}
{{ outer }}
{%- endwith %}
{{ outer }}
)",
//--------------
"\nWorld Hello\nWorld Hello\nHello WorldWorld Hello\n"
)
{
    params = {{"outer", "World Hello"}};
}

MULTISTR_TEST(WithTest, ScopeTest3,
R"(
{{ outer }}
{% with outer = 'Hello World' %}
{% set inner = outer %}
{{ inner }}
{{ outer }}
{%- endwith %}
{{ outer }}
)",
//--------------
"\nWorld Hello\nHello World\nHello WorldWorld Hello\n"
)
{
    params = {{"outer", "World Hello"}};
}

MULTISTR_TEST(WithTest, ScopeTest4,
R"(
{% with inner1 = 42 %}
{% set inner2 = outer %}
{{ inner1 }}
{{ inner2 }}
{%- endwith %}
>> {{ inner1 }} <<
>> {{ inner2 }} <<
)",
//---------------
"\n42\nWorld Hello>>  <<\n>>  <<\n"
)
{
    params = {{"outer", "World Hello"}};
}

TEST(FilterStatement, General)
{
    const std::string source = R"(
{% filter upper %}
    This text becomes uppercase
{% endfilter %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("\n    THIS TEXT BECOMES UPPERCASE\n", result.c_str());
}

TEST(FilterStatement, ChainAndParams)
{
    const std::string source = R"(
{% filter list | sort(reverse=true) | unique | join("+") %}
11222333445556677890
{% endfilter %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("\n9+8+7+6+5+4+3+2+1+0+\n", result.c_str());
}
