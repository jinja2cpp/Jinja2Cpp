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
                            InputOutputPair{"stringValue | list | first", "r"},
                            InputOutputPair{"reflectedIntVector | first", "9"}
                            ));

INSTANTIATE_TEST_CASE_P(Last, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4] | last", "4"},
                            InputOutputPair{"(1, 2, 3, 4) | last", "4"},
                            InputOutputPair{"intValue | last", ""},
                            InputOutputPair{"intList | last", "4"},
                            InputOutputPair{"stringValue | list | last", "n"},
                            InputOutputPair{"reflectedIntVector | last", "4"}
                            ));

INSTANTIATE_TEST_CASE_P(Length, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4, 5] | length", "5"},
                            InputOutputPair{"(1, 2, 3, 4, 5, 6) | length", "6"},
                            InputOutputPair{"intValue | length", ""},
                            InputOutputPair{"intList | length", "10"},
                            InputOutputPair{"stringValue | list | length", "4"},
                            InputOutputPair{"reflectedIntVector | length", "10"}
                            ));

INSTANTIATE_TEST_CASE_P(Min, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4, 5] | min", "1"},
                            InputOutputPair{"(1, 2, 3, 4, 5, 6) | min", "1"},
                            InputOutputPair{"intValue | min", ""},
                            InputOutputPair{"intList | min", "0"},
                            InputOutputPair{"stringValue | list | min", "a"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | min", "str1"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | min(true)", "Str6"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | min(case_sensitive=true)", "Str6"}
                            ));

INSTANTIATE_TEST_CASE_P(Max, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4, 5] | max", "5"},
                            InputOutputPair{"(1, 2, 3, 4, 5, 6) | max", "6"},
                            InputOutputPair{"intValue | max", ""},
                            InputOutputPair{"intList | max", "9"},
                            InputOutputPair{"stringValue | list | max", "r"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | max", "Str6"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | max(true)", "str5"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | max(case_sensitive=true)", "str5"}
                            ));

INSTANTIATE_TEST_CASE_P(Reverse, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['str1', 'str2', 'str3'] | reverse", "str3, str2, str1"},
                            InputOutputPair{"[3, 1, 2] | reverse",                "2, 1, 3"},
                            InputOutputPair{"reflectedIntVector | reverse",       "4, 5, 3, 6, 2, 7, 1, 8, 0, 9"}
                            ));

INSTANTIATE_TEST_CASE_P(Sum, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4, 5] | sum",   "15"},
                            InputOutputPair{"[] | sum(start=15)",      "15"},
                            InputOutputPair{"intValue | sum",          ""},
                            InputOutputPair{"intList | sum(start=10)", "55"},
                            InputOutputPair{"stringValue | list | sum","rain"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | sum",
                                                                       "str1str2str3str4str5Str6"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | sum(start='Hello')",
                                                                       "Hellostr1str2str3str4str5Str6"},
                            InputOutputPair{"reflectedList | sum(attribute='strValue')",
                                                                       "test string 0test string 1test string 2test string 3test string 4test string 5test string 6test string 7test string 8test string 9"}
                            ));

INSTANTIATE_TEST_CASE_P(Unique, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['str1', 'str2', 'str3'] | unique",                      "str1, str2, str3"},
                            InputOutputPair{"['str3', 'str1', 'str1'] | unique",                      "str3, str1"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | unique",                      "Str2, str1, str3"},
                            InputOutputPair{"['Str2', 'str2', 'str3'] | unique",                      "Str2, str3"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | unique(case_sensitive=true)", "Str2, str1, str3"},
                            InputOutputPair{"[3, 1, 2] | unique",                                     "3, 1, 2"},
                            InputOutputPair{"[3, 1, 2, 1, -2, 1, 10, 1, 6, 1, 5] | unique",           "3, 1, 2, -2, 10, 6, 5"},
                            InputOutputPair{"[3.0, 3, 1] | unique",                                   "3, 1"},
                            InputOutputPair{"reflectedList | unique(attribute='strValue') | map(attribute='strValue')",
                                                                                                      "test string 0, test string 1, test string 2, test string 3, test string 4, test string 5, test string 6, test string 7, test string 8, test string 9"},
                            InputOutputPair{"reflectedList | unique(attribute='boolValue') | map(attribute='strValue')",
                                                                                                      "test string 0, test string 1"}
                            ));

INSTANTIATE_TEST_CASE_P(Attr, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"{'key'='itemName', 'value'='itemValue'} | attr('key')", "itemName"},
                            InputOutputPair{"mapValue | attr('intVal')", "10"},
                            InputOutputPair{"mapValue | attr(name='dblVal')", "100.5"},
                            InputOutputPair{"mapValue | attr('stringVal')", "string100.5"},
                            InputOutputPair{"mapValue | attr('boolValue')", "true"},
                            InputOutputPair{"reflectedVal | attr('intValue')", "0"},
                            InputOutputPair{"filledReflectedPtrVal | attr('strValue')", "test string 0"}
                            ));

INSTANTIATE_TEST_CASE_P(Map, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"reflectedList | map(attribute='intValue')",       "0, 1, 2, 3, 4, 5, 6, 7, 8, 9"},
                            InputOutputPair{"[[0, 1], [1, 2], [2, 3], [3, 4]] | map('first')", "0, 1, 2, 3"},
                            InputOutputPair{"[['str1', 'Str2'], ['str2', 'Str3'], ['str3', 'Str4'], ['str4', 'Str5']] | map('min')",
                                                                                               "str1, str2, str3, str4"},
                            InputOutputPair{"[['str1', 'Str2'], ['str2', 'Str3'], ['str3', 'Str4'], ['str4', 'Str5']] | map('min', case_sensitive=true)",
                                                                                               "Str2, Str3, Str4, Str5"}
                            ));


INSTANTIATE_TEST_CASE_P(Reject, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['', 'str1', '', 'str2', '', 'str3', '', 'str4'] | reject", ", , , "},
                            InputOutputPair{"['_str1', 'str1', '_str2', 'str2', '_str3', 'str3', '_str4', 'str4'] | reject('startsWith', '_')",
                                                                                                         "str1, str2, str3, str4"},
                            InputOutputPair{"['_str1', 'str1', '_str2', 'str2', '_str3', 'str3', '_str4', 'str4'] | reject('startsWith', str='_')",
                                                                                                         "str1, str2, str3, str4"}
                            ));

INSTANTIATE_TEST_CASE_P(RejectAttr, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"reflectedList | rejectattr('boolValue') | map(attribute='strValue')",
                                                                                        "test string 0, test string 2, test string 4, test string 6, test string 8"},
                            InputOutputPair{"reflectedList | rejectattr(attribute='boolValue') | map(attribute='strValue')",
                                                                                        "test string 0, test string 2, test string 4, test string 6, test string 8"}
                            ));

INSTANTIATE_TEST_CASE_P(Select, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['', 'str1', '', 'str2', '', 'str3', '', 'str4'] | select", "str1, str2, str3, str4"},
                            InputOutputPair{"['_str1', 'str1', '_str2', 'str2', '_str3', 'str3', '_str4', 'str4'] | select('startsWith', '_')",
                                                                                                         "_str1, _str2, _str3, _str4"},
                            InputOutputPair{"['_str1', 'str1', '_str2', 'str2', '_str3', 'str3', '_str4', 'str4'] | select('startsWith', str='_')",
                                                                                                         "_str1, _str2, _str3, _str4"}
                            ));

INSTANTIATE_TEST_CASE_P(SelectAttr, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"reflectedList | selectattr('boolValue') | map(attribute='strValue')",
                                                                                        "test string 1, test string 3, test string 5, test string 7, test string 9"},
                            InputOutputPair{"reflectedList | selectattr(attribute='boolValue') | map(attribute='strValue')",
                                                                                        "test string 1, test string 3, test string 5, test string 7, test string 9"}
                            ));


INSTANTIATE_TEST_CASE_P(PPrint, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"10 | pprint", "10"},
                            InputOutputPair{"10.5 | pprint", "10.5"},
                            InputOutputPair{"intValue | pprint", "3"},
                            InputOutputPair{"stringValue | pprint", "'rain'"},
                            InputOutputPair{"boolFalseValue | pprint", "false"},
                            InputOutputPair{"boolTrueValue | pprint", "true"},
                            InputOutputPair{"intList | pprint", "[9, 0, 8, 1, 7, 2, 6, 3, 5, 4]"},
                            InputOutputPair{"{'key'='itemName'} | pprint", "{'key': 'itemName'}"}
                            ));


INSTANTIATE_TEST_CASE_P(DictSort, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort | pprint", "['key': 'itemName', 'Value': 'ItemValue']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(by='value') | pprint", "['key': 'itemName', 'Value': 'ItemValue']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(reverse=true) | pprint", "['Value': 'ItemValue', 'key': 'itemName']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(reverse=true, by='value') | pprint", "['Value': 'ItemValue', 'key': 'itemName']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(case_sensitive=true) | pprint", "['Value': 'ItemValue', 'key': 'itemName']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(case_sensitive=true, reverse=true) | pprint", "['key': 'itemName', 'Value': 'ItemValue']"},
                            InputOutputPair{"simpleMapValue | dictsort | pprint", "['boolValue': true, 'dblVal': 100.5, 'intVal': 10, 'stringVal': 'string100.5']"},
                            InputOutputPair{"reflectedVal | dictsort | pprint", "['boolValue': false, 'dblValue': 0, 'intValue': 0, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>']"}
                            ));

INSTANTIATE_TEST_CASE_P(UrlEncode, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'Hello World' | urlencode", "Hello+World"},
                            InputOutputPair{"'! # $ & ( ) * + , / : ; = ? @ [ ] %' | urlencode", "%21+%23+%24+%26+%28+%29+%2A+%2B+%2C+%2F+%3A+%3B+%3D+%3F+%40+%5B+%5D+%25"}
                            ));

INSTANTIATE_TEST_CASE_P(Abs, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"10 | abs", "10"},
                            InputOutputPair{"-10 | abs", "10"},
                            InputOutputPair{"10.5 | abs", "10.5"},
                            InputOutputPair{"-10.5 | abs", "10.5"},
                            InputOutputPair{"'10' | abs", ""}
                            ));

INSTANTIATE_TEST_CASE_P(Round, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"10 | round", "10"},
                            InputOutputPair{"10 | round(1)", "10"},
                            InputOutputPair{"10.5 | round", "11"},
                            InputOutputPair{"10.4 | round", "10"},
                            InputOutputPair{"10.6 | round", "11"},
                            InputOutputPair{"-10.5 | round", "-11"},
                            InputOutputPair{"-10.4 | round", "-10"},
                            InputOutputPair{"-10.6 | round", "-11"},
                            InputOutputPair{"10.5 | round(method='ceil')", "11"},
                            InputOutputPair{"10.5 | round(method='floor')", "10"},
                            InputOutputPair{"-10.5 | round(method='ceil')", "-11"},
                            InputOutputPair{"-10.5 | round(method='floor')", "-10"},
                            InputOutputPair{"10.44 | round(1)", "10.4"},
                            InputOutputPair{"10.46 | round(precision=1)", "10.5"}
                            ));

INSTANTIATE_TEST_CASE_P(Convert, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"10 | int", "10"},
                            InputOutputPair{"10 | float", "10"},
                            InputOutputPair{"10.5 | int", "10"},
                            InputOutputPair{"'10.4' | float | pprint", "10.4"},
                            InputOutputPair{"'100;4' | float(10.4) | pprint", "10.4"},
                            InputOutputPair{"'100;4' | int(10) | pprint", "10"},
                            InputOutputPair{"'100' | int(10) | pprint", "100"},
                            InputOutputPair{"'0x100' | int(10, 0) | pprint", "256"},
                            InputOutputPair{"'0100' | int(10, 0) | pprint", "64"},
                            InputOutputPair{"'100' | int(10, base=10) | pprint", "100"},
                            InputOutputPair{"'100' | int(10, base=2) | pprint", "4"},
                            InputOutputPair{"'100' | int(10, base=8) | pprint", "64"},
                            InputOutputPair{"'100' | int(10, base=16) | pprint", "256"},
                            InputOutputPair{"'100' | list | pprint", "['1', '0', '0']"},
                            InputOutputPair{"{'name'='itemName', 'val'='itemValue'} | list | pprint", "['name': 'itemName', 'val': 'itemValue']"}
                            ));

INSTANTIATE_TEST_CASE_P(Trim, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'string' | trim | pprint", "'string'"},
                            InputOutputPair{"'    string' | trim | pprint", "'string'"},
                            InputOutputPair{"'string    ' | trim | pprint", "'string'"},
                            InputOutputPair{"'    string     ' | trim | pprint", "'string'"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));
