#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"
#include "test_tools.h"

using namespace jinja2;

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

SUBSTITUION_TEST_P(ExpressionSubstitutionTest)

INSTANTIATE_TEST_CASE_P(ConstantSubstitutionTest, ExpressionSubstitutionTest, ::testing::Values(
                            InputOutputPair{"",                  ""},
                            InputOutputPair{"'str1'",            "str1"},
                            InputOutputPair{"\"str1\"",          "str1"},
                            InputOutputPair{"100500",            "100500"},
                            InputOutputPair{"'100.555'",         "100.555"},
                            InputOutputPair{"true",              "true"},
                            InputOutputPair{"false",             "false"}
                            ));

INSTANTIATE_TEST_CASE_P(BasicValueSubstitutionTest, ExpressionSubstitutionTest, ::testing::Values(
                            InputOutputPair{"intValue",       "3"},
                            InputOutputPair{"doubleValue",    "12.123"},
                            InputOutputPair{"stringValue",    "rain"},
                            InputOutputPair{"boolTrueValue",  "true"},
                            InputOutputPair{"boolFalseValue", "false"}
                            ));

INSTANTIATE_TEST_CASE_P(IndexSubscriptionTest, ExpressionSubstitutionTest, ::testing::Values(
                            InputOutputPair{"intValue[0]",               ""},
                            InputOutputPair{"doubleValue[0]",            ""},
                            InputOutputPair{"stringValue[0]",            "r"},
                            InputOutputPair{"boolTrueValue[0]",          ""},
                            InputOutputPair{"boolFalseValue[0]",         ""},
                            InputOutputPair{"intList[-1]",               ""},
                            InputOutputPair{"intList[10]",               ""},
                            InputOutputPair{"intList[0]",                "9"},
                            InputOutputPair{"intList[9]",                "4"},
                            InputOutputPair{"intList[5]",                "2"},
                            InputOutputPair{"mapValue['intVal']",        "10"},
                            InputOutputPair{"mapValue['dblVal']",        "100.5"},
                            InputOutputPair{"mapValue['stringVal']",     "string100.5"},
                            InputOutputPair{"mapValue['boolValue']",     "true"},
                            InputOutputPair{"mapValue['intVAl']",        ""},
                            InputOutputPair{"reflectedVal['intValue']",  "0"},
                            InputOutputPair{"reflectedVal['dblValue']",  "0"},
                            InputOutputPair{"reflectedVal['boolValue']", "false"},
                            InputOutputPair{"reflectedVal['strValue']",  "test string 0"},
                            InputOutputPair{"reflectedVal['StrValue']",  ""}
                            ));

INSTANTIATE_TEST_CASE_P(DotSubscriptionTest, ExpressionSubstitutionTest, ::testing::Values(
                            InputOutputPair{"mapValue.intVal",        "10"},
                            InputOutputPair{"mapValue.dblVal",        "100.5"},
                            InputOutputPair{"mapValue.stringVal",     "string100.5"},
                            InputOutputPair{"mapValue.boolValue",     "true"},
                            InputOutputPair{"mapValue.intVAl",        ""},
                            InputOutputPair{"reflectedVal.intValue",  "0"},
                            InputOutputPair{"reflectedVal.dblValue",  "0"},
                            InputOutputPair{"reflectedVal.boolValue", "false"},
                            InputOutputPair{"reflectedVal.strValue",  "test string 0"},
                            InputOutputPair{"reflectedVal.StrValue",  ""}
                            ));


INSTANTIATE_TEST_CASE_P(ComplexSubscriptionTest, ExpressionSubstitutionTest, ::testing::Values(
//                            InputOutputPair{"mapValue.reflectedList[1]['intValue']",    "1"},
                            InputOutputPair{"reflectedVal.strValue[0]",        "t"}
                            ));
