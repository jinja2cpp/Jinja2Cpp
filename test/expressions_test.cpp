#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"

using namespace jinja2;

TEST(ExpressionsTest, BasicValueSubstitution)
{
    std::string source = R"(
IntValue: {{intValue}}
StringValue: {{stringValue}}
DoubleValue: {{doubleValue}}
BoolFalceValue: {{boolFalseValue}}
BoolTrueValue: {{boolTrueValue}}
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

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
IntValue: 3
StringValue: rain
DoubleValue: 12.123
BoolFalceValue: false
BoolTrueValue: true
)";

    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(ExpressionsTest, BasicConstantSubstitution)
{
    std::string source = R"(
{{ "Hello" }} {{ 'world' }}!!!
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Hello world!!!
)";

    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(ExpressionsTest, ConstantSubstitution)
{
    std::string source = R"(
SingleQuotedString={{ 'SingleQuotedString' }}
DoubleQuotedString={{ "DoubleQuotedString" }}
IntValue={{ 100500 }}
DoubleValue={{ 42.42 }}
BoolTrueValue={{ True }}
BoolFalseValue={{ False }}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
SingleQuotedString=SingleQuotedString
DoubleQuotedString=DoubleQuotedString
IntValue=100500
DoubleValue=42.42
BoolTrueValue=true
BoolFalseValue=false
)";

    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(ExpressionsTest, BinaryMathOperations)
{
    std::string source = R"(
{{ 1 + 10 }}
{{ 1 - 10}}
{{ 0.1 + 1 }}
{{ 0.1 - 10.5 }}
{{ 1 * 10 }}
{{ 7 / 3}}
{{ 7 // 3 }}
{{ 7 % intValue }}
{{ 3 ** 4 }}
{{ 10 ** -2 }}
{{ 10/10 + 2*5 }}
{{ 'Hello' + " " + 'World ' + stringValue }}
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

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
11
-9
1.1
-10.4
10
2.33333
2
1
81
0.01
11
Hello World rain
)";

    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(ExpressionFiltersTest, Join)
{
    std::string source = R"(
{{ ("Hello", 'world') | join }}!!!
{{ ("Hello", 'world') | join(', ') }}!!!
{{ ("Hello", 'world') | join(d = '; ') }}!!!
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Helloworld!!!
Hello, world!!!
Hello; world!!!
)";

    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(ExpressionsTest, IfExpression)
{
    std::string source = R"(
{{ intValue if intValue is eq(3) }}
{{ stringValue if intValue < 3 else doubleValue }}
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

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
3
12.123
)";

    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(ExpressionsTest, IndexExpression)
{
    std::string source = R"(
{{listValue[0]}}
{{dictValue.item1}}
{{dictValue["item1"]}}
{{listValue[dictValue.item2]}}
{{dictValue.item4[2]}}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"listValue", ValuesList{10, 20, 30, 40}},
        {"dictValue", ValuesMap{{"item1", 1}, {"item2", 2}, {"item3", 3}, {"item4", ValuesList{60, 70, 80}}}},
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
10
1
1
30
80
)";

    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

