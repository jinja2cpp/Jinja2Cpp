#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"

#include "test_tools.h"

using namespace jinja2;

TEST(SetTest, SimpleSetTest)
{
    std::string source = R"(
{% set val = intValue %}
localVal: {{val}}
paramsVal: {{intValue}}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"intValue", 3},
        {"doubleValue", 12.123f},
        {"stringValue", "rain"},
        {"boolFalseValue", false},
        {"boolTrueValue", true},
    };

    std::string result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
localVal: 3
paramsVal: 3
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(SetTest, Tuple1AssignmentTest)
{
    std::string source = R"(
{% set firstName, lastName = emploee %}
firtsName: {{firstName}}
lastName: {{lastName}}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"emploee", ValuesMap{
             {"firstName", "John"},
             {"lastName", "Dow"}
         }},
    };

    std::string result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
firtsName: John
lastName: Dow
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(SetTest, Tuple2AssignmentTest)
{
    std::string source = R"(
{% set tuple = ("Hello", "World") %}
hello: {{tuple[0]}}
world: {{tuple[1]}}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
hello: Hello
world: World
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(SetTest, Tuple3AssignmentTest)
{
    std::string source = R"(
{% set tuple = ["Hello", "World"] %}
hello: {{tuple[0]}}
world: {{tuple[1]}}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
hello: Hello
world: World
)";
    EXPECT_EQ(expectedResult, result);
}


TEST(SetTest, Tuple4AssignmentTest)
{
    std::string source = R"(
{% set dict = {'hello' = "Hello", 'world' = "World"} %}
hello: {{dict.hello}}
world: {{dict.world}}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
hello: Hello
world: World
)";
    EXPECT_EQ(expectedResult, result);
}

using WithTest = TemplateEnvFixture;

TEST_F(WithTest, SimpleTest)
{
    auto result = Render(R"(
{% with inner = 42 %}
{{ inner }}
{%- endwith %}
)", {});
    EXPECT_EQ("\n42", result);
}

TEST_F(WithTest, MultiVarsTest)
{
    auto result = Render(R"(
{% with inner1 = 42, inner2 = 'Hello World' %}
{{ inner1 }}
{{ inner2 }}
{%- endwith %}
)", {});
    EXPECT_EQ("\n42\nHello World", result);
}

TEST_F(WithTest, ScopeTest1)
{
    auto result = Render(R"(
{{ outer }}
{% with inner = 42, outer = 'Hello World' %}
{{ inner }}
{{ outer }}
{%- endwith %}
{{ outer }}
)", {{"outer", "World Hello"}});
    EXPECT_EQ("\nWorld Hello\n42\nHello WorldWorld Hello\n", result);
}

TEST_F(WithTest, ScopeTest2)
{
    auto result = Render(R"(
{{ outer }}
{% with outer = 'Hello World', inner = outer %}
{{ inner }}
{{ outer }}
{%- endwith %}
{{ outer }}
)", {{"outer", "World Hello"}});
    EXPECT_EQ("\nWorld Hello\nWorld Hello\nHello WorldWorld Hello\n", result);
}

TEST_F(WithTest, ScopeTest3)
{
    auto result = Render(R"(
{{ outer }}
{% with outer = 'Hello World' %}
{% set inner = outer %}
{{ inner }}
{{ outer }}
{%- endwith %}
{{ outer }}
)", {{"outer", "World Hello"}});
    EXPECT_EQ("\nWorld Hello\nHello World\nHello WorldWorld Hello\n", result);
}

TEST_F(WithTest, ScopeTest4)
{
    auto result = Render(R"(
{% with inner1 = 42 %}
{% set inner2 = outer %}
{{ inner1 }}
{{ inner2 }}
{%- endwith %}
>> {{ inner1 }} <<
>> {{ inner2 }} <<
)", {{"outer", "World Hello"}});
    EXPECT_EQ("\n42\nWorld Hello>>  <<\n>>  <<\n", result);
}
