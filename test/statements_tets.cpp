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
paramsVal: 3)")
{
    params = {
        { "intValue", 3 },
        { "doubleValue", 12.123F },
        { "stringValue", "rain" },
        { "boolFalseValue", false },
        { "boolTrueValue", true },
    };
}

// `set` targets are kept in the parse tree (0118 P5b-3b)
// clang-format off
MULTISTR_TEST(SetTest, TargetsKeptInTheTree,
R"({% set a_long_namespace_name = namespace(some_long_attribute=1) %}{% set a_long_namespace_name.some_long_attribute = 2 %}{{ a_long_namespace_name.some_long_attribute }} {% set a_long_first_name, a_long_second_name = 1, 2 %}{{ a_long_first_name }}{{ a_long_second_name }} {% set a_long_block_target_name %}body{% endset %}{{ a_long_block_target_name }} {% set a_long_filtered_target_name | upper %}body{% endset %}{{ a_long_filtered_target_name }} {% set (a, (b, c)) = (1, (2, 3)) %}{{ a }}{{ b }}{{ c }})",
//-----------
R"(2 12 body BODY 123)")
{
}
// clang-format on

MULTISTR_TEST(SetTest, Tuple1AssignmentTest,
              R"(
{% set firstName, lastName = emploee %}
firtsName: {{firstName}}
lastName: {{lastName}}
)",
              //--------------
              R"(

firtsName: John
lastName: Dow)")
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
world: World)")
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
world: World)")
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
world: World)")
{
}

using WithTest = BasicTemplateRenderer;

// Name lookups are cached (docs/tasks/0100 idea 7): each line changes where a name resolves
// after it has been looked up
MULTISTR_TEST(SetTest, LookupsFollowScopeChanges,
              R"({% for i in [1,2] %}{{ x }}{% set x = 5 %}{{ x }}{% endfor %}
{% macro m(i) %}{{ i }}{% endmacro %}{% for i in [1,2] %}{{ i }}{{ m(i*5) }}{{ i }};{% endfor %}
{% for i in [1,2] %}{{ i }}{% for i in [7] %}{{ i }}{% endfor %}{{ i }};{% endfor %}
{% for i in [1,2] %}{% with i = i + 100 %}{{ i }}{% endwith %}{{ i }};{% endfor %}
{% set z = 1 %}{% for i in [1,2] %}{{ z }}{% set z = i * 10 %}{{ z }};{% endfor %}{{ z }}
{% set y = "g" %}{% for i in [1,2] %}{{ y }}{% if i == 1 %}{% set y = "l" %}{% endif %}{{ y }};{% endfor %})",
              //------------
              R"(3535
151;2102;
171;272;
1011;1022;
110;120;1
gl;gg;)")
{
    params = { { "x", 3 } };
}

MULTISTR_TEST(WithTest, SimpleTest,
              R"(
{% with inner = 42 %}
{{ inner }}
{%- endwith %}
)",
              //----------
              "\n\n42")
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
              "\n\n42\nHello World")
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
              "\nWorld Hello\n\n42\nHello World\nWorld Hello")
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
              "\nWorld Hello\n\nWorld Hello\nHello World\nWorld Hello")
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
              "\nWorld Hello\n\n\nHello World\nHello World\nWorld Hello")
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
              "\n\n\n42\nWorld Hello\n>>  <<\n>>  <<")
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
    EXPECT_STREQ("\n\n    THIS TEXT BECOMES UPPERCASE\n", result.c_str());
}

TEST(FilterStatement, ChainAndParams)
{
    const std::string source = R"(
{% filter trim | list | sort(reverse=true) | unique | join("+") %}
11222333445556677890
{% endfilter %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    const auto result = tpl.RenderAsString({}).value();
    EXPECT_STREQ("\n9+8+7+6+5+4+3+2+1+0", result.c_str());
}

TEST(SetBlockStatement, OneVar)
{
    const std::string source = R"(
{% set foo %}
11222333445556677890
{% endset %}
|{{foo}}|
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    const auto result = tpl.RenderAsString({}).value();
    EXPECT_STREQ("\n\n|\n11222333445556677890\n|", result.c_str());
}

TEST(SetBlockStatement, MoreVars)
{
    const std::string source = R"(
{% set foo1,foo2,foo3,foo4,foo5 %}abcde{% endset %}
|{{foo1}}|
|{{foo2}}|
|{{foo5}}|
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    const auto result = tpl.RenderAsString({}).value();
    EXPECT_STREQ("\n\n|a|\n|b|\n|e|", result.c_str());
}

// Jinja2: the body is unpacked like any other value, so its length must match the names
TEST(SetBlockStatement, MoreVarsWrongCount)
{
    const std::string source = R"({% set foo1,foo2 %}abc{% endset %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    EXPECT_FALSE(tpl.RenderAsString({}));
}

TEST(SetBlockStatement, OneVarFiltered)
{
    const std::string source = R"(
{% set foo | trim | list | sort(reverse=true) | unique | join("+") %}
11222333445556677890
{% endset %}
|{{foo}}|
)";

    Template tpl;
    const auto load = tpl.Load(source);
    ASSERT_TRUE(load) << load.error();

    const auto result = tpl.RenderAsString({}).value();
    EXPECT_STREQ("\n\n|9+8+7+6+5+4+3+2+1+0|", result.c_str());
}

TEST(SetBlockStatement, MoreVarsFiltered)
{
    const std::string source = R"(
{% set foo1,foo2,foo3,foo4,foo5 | trim | list | sort(reverse=true) | unique | list %}
11222333445
{% endset %}
|{{foo1}}|
|{{foo2}}|
|{{foo5}}|
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    const auto result = tpl.RenderAsString({}).value();
    EXPECT_STREQ("\n\n|5|\n|4|\n|1|", result.c_str());
}

using RawTest = BasicTemplateRenderer;

TEST(RawTest, General)
{
    const std::string source = R"(
{% raw %}
    This is a raw text {{ 2 + 2 }}
{% endraw %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("\n\n    This is a raw text {{ 2 + 2 }}\n", result.c_str());
}

TEST(RawTest, KeywordsInside)
{
    const std::string source = R"(
{% raw %}
    <ul>
    {% for item in seq %}
        <li>{{ item }}</li>
    {% endfor %}
    </ul>{% endraw %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("\n\n    <ul>\n    {% for item in seq %}\n        <li>{{ item }}</li>\n    {% endfor %}\n    </ul>", result.c_str());
}

TEST(RawTest, BrokenExpression)
{
    const std::string source = R"({% raw %}{{ x }{% endraw %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("{{ x }", result.c_str());
}

TEST(RawTest, BrokenTag)
{
    const std::string source = R"({% raw %}{% if im_broken }still work{% endraw %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("{% if im_broken }still work", result.c_str());
}

TEST(RawTest, ExtraSpaces)
{
    const std::string source = R"({% raw  %}abc{% endraw %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("abc", result.c_str());
}

TEST(RawTest, ExtraSpaces2)
{
    const std::string source = R"({% raw %}abc{%   endraw %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("abc", result.c_str());
}

TEST(RawTest, TrimPostRaw)
{
    const std::string source = R"({% raw -%}        abc{% endraw %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("abc", result.c_str());
}

TEST(RawTest, TrimRawEndRaw)
{
    const std::string source = R"({% raw -%}        abc     {%- endraw %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("abc", result.c_str());
}

TEST(RawTest, TrimPostEndRaw)
{
    const std::string source = R"({% raw %}abc{% endraw -%}               defg)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("abcdefg", result.c_str());
}

TEST(RawTest, TrimBeforeEndRaw)
{
    const std::string source = R"({% raw %}        abc     {%- endraw %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("        abc", result.c_str());
}

TEST(RawTest, TrimBeforeRaw)
{
    const std::string source = R"(         {%- raw %}        abc     {% endraw %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("        abc     ", result.c_str());
}

TEST(RawTest, ForRaw)
{
    const std::string source = R"({% for i in (0, 1, 2) -%}
    {%- raw %}{{ x }} {% endraw %}
    {%- endfor %})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("{{ x }} {{ x }} {{ x }} ", result.c_str());
}

TEST(RawTest, CommentRaw)
{
    const std::string source = R"({# {% raw %} {% endraw %} #})";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    const auto result = tpl.RenderAsString({}).value();
    std::cout << result << std::endl;
    EXPECT_STREQ("", result.c_str());
}
