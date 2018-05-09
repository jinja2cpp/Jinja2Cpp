#include <iostream>
#include <string>

#include "test_tools.h"
#include "jinja2cpp/template.h"

using namespace jinja2;

struct FilterGenericTestTag;
using FilterGenericTest = InputOutputPairTest<FilterGenericTestTag>;

struct ListIteratorTestTag;
using ListIteratorTest = InputOutputPairTest<ListIteratorTestTag>;

TEST_P(FilterGenericTest, Test)
{
    auto& testParam = GetParam();
    std::string source = "{{" + testParam.tpl + "}}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(PrepareTestData());
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

TEST_P(ListIteratorTest, Test)
{
    auto& testParam = GetParam();
    std::string source = "{% for i in " + testParam.tpl + "%}{{i}}{{', ' if not loop.last}}{% endfor %}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(PrepareTestData());
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

INSTANTIATE_TEST_CASE_P(StringJoin, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"['str1', 'str2', 'str3'] | join",                   "str1str2str3"},
                            InputOutputPair{"['str1', 'str2', 'str3'] | join(' ')",              "str1 str2 str3"},
                            InputOutputPair{"['str1', 'str2', 'str3'] | join(d='-')",            "str1-str2-str3"},
                            InputOutputPair{"reflectedList | join(d='-', 'strValue')",           "test string 0-test string 1-test string 2-test string 3-test string 4-test string 5-test string 6-test string 7-test string 8-test string 9"},
                            InputOutputPair{"reflectedList | join(attribute='strValue', '-')",   "test string 0-test string 1-test string 2-test string 3-test string 4-test string 5-test string 6-test string 7-test string 8-test string 9"},
                            InputOutputPair{"reflectedList | join(attribute='strValue', d='-')", "test string 0-test string 1-test string 2-test string 3-test string 4-test string 5-test string 6-test string 7-test string 8-test string 9"}
                            ));

INSTANTIATE_TEST_CASE_P(Sort, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['str1', 'str2', 'str3'] | sort",                                    "str1, str2, str3"},
                            InputOutputPair{"['str2', 'str1', 'str3'] | sort",                                    "str1, str2, str3"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | sort",                                    "str1, Str2, str3"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | sort(reverse=true)",                      "str3, Str2, str1"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | sort(case_sensitive=true)",               "Str2, str1, str3"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | sort(case_sensitive=true, reverse=true)", "str3, str1, Str2"},
                            InputOutputPair{"[3, 1, 2] | sort",                                                   "1, 2, 3"},
                            InputOutputPair{"reflectedIntVector | sort",                                          "0, 1, 2, 3, 4, 5, 6, 7, 8, 9"}
                            ));

INSTANTIATE_TEST_CASE_P(Default, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"intValue | default(0)", "3"},
                            InputOutputPair{"integerValue | default(0)", "0"},
                            InputOutputPair{"integerValue | d(0)", "0"},
                            InputOutputPair{"''|default('the string was empty', true)", "the string was empty"},
                            InputOutputPair{"''|default(default_value='the string was empty', boolean=true)", "the string was empty"},
                            InputOutputPair{"''|default('the string was empty', false)", ""},
                            InputOutputPair{"'Hello World!'|default('the string was empty', true)", "Hello World!"}
                            ));

INSTANTIATE_TEST_CASE_P(First, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4] | first", "1"},
                            InputOutputPair{"(1, 2, 3, 4) | first", "1"},
                            InputOutputPair{"intValue | first", ""},
                            InputOutputPair{"intList | first", "9"},
                            InputOutputPair{"stringValue | first", "r"},
                            InputOutputPair{"reflectedIntVector | first", "9"}
                            ));

INSTANTIATE_TEST_CASE_P(Last, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4] | last", "4"},
                            InputOutputPair{"(1, 2, 3, 4) | last", "4"},
                            InputOutputPair{"intValue | last", ""},
                            InputOutputPair{"intList | last", "4"},
                            InputOutputPair{"stringValue | last", "n"},
                            InputOutputPair{"reflectedIntVector | last", "4"}
                            ));
