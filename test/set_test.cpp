#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"

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

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
localVal: 3
paramsVal: 3
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(SetTest, TupleAssignmentTest)
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

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
firtsName: John
lastName: Dow
)";
    EXPECT_EQ(expectedResult, result);
}
