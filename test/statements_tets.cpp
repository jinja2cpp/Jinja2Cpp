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
"\n\n42\n"
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
"\n\n42\nHello World\n"
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
"\nWorld Hello\n\n42\nHello World\nWorld Hello\n"
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
"\nWorld Hello\n\nWorld Hello\nHello World\nWorld Hello\n"
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
"\nWorld Hello\n\n\nHello World\nHello World\nWorld Hello\n"
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
"\n\n\n42\nWorld Hello\n>>  <<\n>>  <<\n"
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
    EXPECT_STREQ("\n\n    THIS TEXT BECOMES UPPERCASE\n\n", result.c_str());
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
    EXPECT_STREQ("\n9+8+7+6+5+4+3+2+1+0\n", result.c_str());
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
    EXPECT_STREQ("\n\n|\n11222333445556677890\n|\n", result.c_str());
}

TEST(SetBlockStatement, MoreVars)
{
    const std::string source = R"(
{% set foo1,foo2,foo3,foo4,foo5 %}
11222333445556677890
{% endset %}
|{{foo1}}|
|{{foo2}}|
|{{foo5}}|
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    const auto result = tpl.RenderAsString({}).value();
    EXPECT_STREQ("\n\n|\n11222333445556677890\n|\n|\n11222333445556677890\n|\n|\n11222333445556677890\n|\n", result.c_str());
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
    EXPECT_STREQ("\n\n|9+8+7+6+5+4+3+2+1+0|\n", result.c_str());
}

TEST(SetBlockStatement, MoreVarsFiltered)
{
    const std::string source = R"(
{% set foo1,foo2,foo3,foo4,foo5 | trim | list | sort(reverse=true) | unique | join("+") %}
11222333445556677890
{% endset %}
|{{foo1}}|
|{{foo2}}|
|{{foo5}}|
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    const auto result = tpl.RenderAsString({}).value();
    EXPECT_STREQ("\n\n|9+8+7+6+5+4+3+2+1+0|\n|9+8+7+6+5+4+3+2+1+0|\n|9+8+7+6+5+4+3+2+1+0|\n", result.c_str());
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
    EXPECT_STREQ("\n\n    This is a raw text {{ 2 + 2 }}\n\n", result.c_str());
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
    EXPECT_STREQ("\n\n    <ul>\n    {% for item in seq %}\n        <li>{{ item }}</li>\n    {% endfor %}\n    </ul>\n", result.c_str());
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

using ForTest = BasicTemplateRenderer;

MULTISTR_TEST(ForTest, ForKeyValueInDictSorted,
R"(
{% set d = {'a'=1,'b'=2} %}
{% for k, v in d|dictsort %}
{{ k }}: {{ v }}
{%- endfor %}
)",
//------------
R"(


a: 1
b: 2
)"
)
{
}
