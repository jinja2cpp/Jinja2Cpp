#include <iostream>
#include <string>

#include "test_tools.h"
#include "jinja2cpp/template.h"

using namespace jinja2;

SUBSTITUION_TEST_P(FilterGenericTest)

struct ListIteratorTestTag;
using ListIteratorTest = InputOutputPairTest<ListIteratorTestTag>;

struct GroupByTestTag;
using FilterGroupByTest = InputOutputPairTest<GroupByTestTag>;

struct ListSliceTestTag;
using ListSliceTest = InputOutputPairTest<ListSliceTestTag>;

TEST_P(ListIteratorTest, Test)
{
    auto& testParam = GetParam();
    std::string source = "{% for i in " + testParam.tpl + " %}{{i}}{{', ' if not loop.last}}{% endfor %}";

    PerformBothTests(source, testParam.result);
}

TEST_P(FilterGroupByTest, Test)
{
    auto& testParam = GetParam();

    jinja2::ValuesList testData;
    for (int n = 0; n < 10; ++ n)
    {
        TestStruct s;
        std::ostringstream str;
        std::wostringstream wstr;

        str << "test string " << n / 2;
        wstr << L"test string " << n;

        s.intValue = n / 2;
        s.dblValue = static_cast<double>(n / 2) / 2;
        s.boolValue = n % 2 == 1;
        s.strValue = str.str();
        s.wstrValue = wstr.str();

        testData.push_back(jinja2::Reflect(std::move(s)));
    }

    jinja2::ValuesMap params {{"testData", std::move(testData)}};

    std::string source = R"(
{% for grouper, list in )" + testParam.tpl + R"(
%}grouper: {{grouper | pprint }}
{%- for i in list %}
    {'intValue': {{i.intValue}}, 'dblValue': {{i.dblValue}}, 'boolValue': {{i.boolValue}}, 'strValue': '{{i.strValue}}', 'wstrValue': '<wchar_string>'}
{%- endfor %}
{% endfor %})";

    PerformBothTests(source, testParam.result, params);
}

using FilterGenericTestSingle = BasicTemplateRenderer;

MULTISTR_TEST(FilterGenericTestSingle, ApplyMacroTest,
R"(
{% macro test(str) %}{{ str | upper }}{% endmacro %}
{{ 'Hello World!' | applymacro(macro='test') }}
{{ ['str1', 'str2', 'str3'] | map('applymacro', macro='test') | join(', ') }}
)",
//-------------
R"(

HELLO WORLD!
STR1, STR2, STR3
)"
)
{
}

MULTISTR_TEST(FilterGenericTestSingle, ApplyMacroWithCallbackTest,
 R"(
{% macro joiner(list, delim) %}{{ list | map('applymacro', macro='caller') | join(delim) }}{% endmacro %}
{% call(item) joiner(['str1', 'str2', 'str3'], '->') %}{{item | upper}}{% endcall %}

)",
//--------
R"(

STR1->STR2->STR3

)"
)
{
}

TEST_P(ListSliceTest, Test)
{
    auto& testParam = GetParam();
    std::string source = "{{ " + testParam.tpl + " }}";

    PerformBothTests(source, testParam.result);
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
                            InputOutputPair{"mapValue | attr('intVal', default='99')", "10"},
                            InputOutputPair{"mapValue | attr('nonexistent', default='99')", "99"},
                            InputOutputPair{"mapValue | attr(name='dblVal')", "100.5"},
                            InputOutputPair{"mapValue | attr('stringVal')", "string100.5"},
                            InputOutputPair{"mapValue | attr('boolValue')", "true"},
                            InputOutputPair{"reflectedVal | attr('intValue')", "0"},
                            InputOutputPair{"filledReflectedPtrVal | attr('strValue')", "test string 0"}
                            ));

INSTANTIATE_TEST_CASE_P(Map, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"reflectedList | map(attribute='intValue')",       "0, 1, 2, 3, 4, 5, 6, 7, 8, 9"},
                            InputOutputPair{"reflectedList | map(attribute='intValue', default='99')",
                                                                                               "0, 1, 2, 3, 4, 5, 6, 7, 8, 9"},
                            InputOutputPair{"reflectedList | map(attribute='intEvenValue')",   "0, , 2, , 4, , 6, , 8, "},
                            InputOutputPair{"reflectedList | map(attribute='intEvenValue', default='99')",   
                                                                                               "0, 99, 2, 99, 4, 99, 6, 99, 8, 99"},                                                                   
                            InputOutputPair{"reflectedList | map(attribute='nonexistent')",    ", , , , , , , , , "},
                            InputOutputPair{"reflectedList | map(attribute='nonexistent', default='99')",    
                                                                                               "99, 99, 99, 99, 99, 99, 99, 99, 99, 99"},
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

INSTANTIATE_TEST_CASE_P(GroupBy, FilterGroupByTest, ::testing::Values(
                            InputOutputPair{"testData | groupby('intValue')", R"(
grouper: 0
    {'intValue': 0, 'dblValue': 0, 'boolValue': false, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 0, 'dblValue': 0, 'boolValue': true, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
grouper: 1
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': false, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': true, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
grouper: 2
    {'intValue': 2, 'dblValue': 1, 'boolValue': false, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1, 'boolValue': true, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
grouper: 3
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': false, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': true, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
grouper: 4
    {'intValue': 4, 'dblValue': 2, 'boolValue': false, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2, 'boolValue': true, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
)"
                                },
                            InputOutputPair{"testData | groupby('dblValue')", R"(
grouper: 0
    {'intValue': 0, 'dblValue': 0, 'boolValue': false, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 0, 'dblValue': 0, 'boolValue': true, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
grouper: 0.5
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': false, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': true, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
grouper: 1
    {'intValue': 2, 'dblValue': 1, 'boolValue': false, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1, 'boolValue': true, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
grouper: 1.5
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': false, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': true, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
grouper: 2
    {'intValue': 4, 'dblValue': 2, 'boolValue': false, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2, 'boolValue': true, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
)"
                                },
                            InputOutputPair{"testData | groupby('strValue')", R"(
grouper: 'test string 0'
    {'intValue': 0, 'dblValue': 0, 'boolValue': false, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 0, 'dblValue': 0, 'boolValue': true, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
grouper: 'test string 1'
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': false, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': true, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
grouper: 'test string 2'
    {'intValue': 2, 'dblValue': 1, 'boolValue': false, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1, 'boolValue': true, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
grouper: 'test string 3'
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': false, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': true, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
grouper: 'test string 4'
    {'intValue': 4, 'dblValue': 2, 'boolValue': false, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2, 'boolValue': true, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
)"
                                },
                            InputOutputPair{"testData | groupby('boolValue')", R"(
grouper: false
    {'intValue': 0, 'dblValue': 0, 'boolValue': false, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': false, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1, 'boolValue': false, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': false, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2, 'boolValue': false, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
grouper: true
    {'intValue': 0, 'dblValue': 0, 'boolValue': true, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': true, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1, 'boolValue': true, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': true, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2, 'boolValue': true, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
)"
                                }
                            ));

INSTANTIATE_TEST_CASE_P(DictSort, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort | pprint", "['key': 'itemName', 'Value': 'ItemValue']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(by='value') | pprint", "['key': 'itemName', 'Value': 'ItemValue']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(reverse=true) | pprint", "['Value': 'ItemValue', 'key': 'itemName']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(reverse=true, by='value') | pprint", "['Value': 'ItemValue', 'key': 'itemName']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(case_sensitive=true) | pprint", "['Value': 'ItemValue', 'key': 'itemName']"},
                            InputOutputPair{"{'key'='itemName', 'Value'='ItemValue'} | dictsort(case_sensitive=true, reverse=true) | pprint", "['key': 'itemName', 'Value': 'ItemValue']"},
                            InputOutputPair{"simpleMapValue | dictsort | pprint", "['boolValue': true, 'dblVal': 100.5, 'intVal': 10, 'stringVal': 'string100.5']"},
    InputOutputPair{
      "reflectedVal | dictsort | pprint",
      "['basicCallable': <callable>, 'boolValue': false, 'dblValue': 0, 'getInnerStruct': <callable>, 'getInnerStructValue': <callable>, 'innerStruct': "
      "{'strValue': 'Hello World!'}, 'innerStructList': [{'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, "
      "{'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, "
      "{'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}], 'intEvenValue': 0, 'intValue': 0, 'strValue': 'test string 0', 'strViewValue': 'test "
      "string 0', 'tmpStructList': [{'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, "
      "{'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, {'strValue': 'Hello World!'}, "
      "{'strValue': 'Hello World!'}], 'wstrValue': 'test string 0', 'wstrViewValue': 'test string 0']" }));

INSTANTIATE_TEST_CASE_P(UrlEncode, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'Hello World' | urlencode", "Hello+World"},
                            // InputOutputPair{"'Hello World\xD0\x9C\xD0\xBA' | urlencode", "Hello+World%D0%9C%D0%BA"},
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
                            InputOutputPair{"{'name'='itemName', 'val'='itemValue'} | list | sort | pprint", "['name', 'val']"}
                            ));

INSTANTIATE_TEST_CASE_P(Trim, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'string' | trim | pprint", "'string'"},
                            InputOutputPair{"'    string' | trim | pprint", "'string'"},
                            InputOutputPair{"'string    ' | trim | pprint", "'string'"},
                            InputOutputPair{"'    string     ' | trim | pprint", "'string'"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));

INSTANTIATE_TEST_CASE_P(Title, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'string' | title | pprint", "'String'"},
                            InputOutputPair{"'1234string' | title | pprint", "'1234string'"},
                            InputOutputPair{"'hello world' | title | pprint", "'Hello World'"},
                            InputOutputPair{"'hello123ooo, world!' | title | pprint", "'Hello123ooo, World!'"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));

INSTANTIATE_TEST_CASE_P(Upper, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'string' | upper | pprint", "'STRING'"},
                            InputOutputPair{"'1234string' | upper | pprint", "'1234STRING'"},
                            InputOutputPair{"'hello world' | upper | pprint", "'HELLO WORLD'"},
                            InputOutputPair{"'hello123ooo, world!' | upper | pprint", "'HELLO123OOO, WORLD!'"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));


INSTANTIATE_TEST_CASE_P(Lower, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'String' | lower | pprint", "'string'"},
                            InputOutputPair{"'1234String' | lower | pprint", "'1234string'"},
                            InputOutputPair{"'Hello World' | lower | pprint", "'hello world'"},
                            InputOutputPair{"'Hello123OOO, World!' | lower | pprint", "'hello123ooo, world!'"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));


INSTANTIATE_TEST_CASE_P(WordCount, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'string' | wordcount", "1"},
                            InputOutputPair{"'1234string' | wordcount", "1"},
                            InputOutputPair{"'hello world' | wordcount", "2"},
                            InputOutputPair{"'hello123ooo, world!' | wordcount", "2"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));

INSTANTIATE_TEST_CASE_P(Replace, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'Hello World' | replace('Hello', 'Goodbye') | pprint", "'Goodbye World'"},
                            InputOutputPair{"'Hello World' | replace(old='l', new='L') | pprint", "'HeLLo WorLd'"},
                            InputOutputPair{"'Hello World' | replace(old='l', new='L', 2) | pprint", "'HeLLo World'"},
                            InputOutputPair{"'Hello World' | replace('l', 'L', count=1) | pprint", "'HeLlo World'"}
                            ));

INSTANTIATE_TEST_CASE_P(Truncate, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'foo bar baz qux' | truncate(6, leeway=0) | pprint", "'foo...'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(6, true) | pprint", "'foo ba...'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(11, true) | pprint", "'foo bar baz qux'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(11, true, leeway=0) | pprint", "'foo bar baz...'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(9) | pprint", "'foo bar baz...'"},
                            InputOutputPair{"'VeryVeryVeryLongWord' | truncate(3) | pprint", "'...'"},
                            InputOutputPair{"'VeryVeryVeryLongWord' | truncate(16) | pprint", "'VeryVeryVeryLongWord'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(6, end=' >>', leeway=0) | pprint", "'foo >>'"}
                            ));

INSTANTIATE_TEST_CASE_P(Capitalize, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'String' | capitalize | pprint", "'String'"},
                            InputOutputPair{"'string' | capitalize | pprint", "'String'"},
                            InputOutputPair{"'1234string' | capitalize | pprint", "'1234string'"},
                            InputOutputPair{"'1234String' | capitalize | pprint", "'1234string'"},
                            InputOutputPair{"'Hello World' | capitalize | pprint", "'Hello world'"},
                            InputOutputPair{"' Hello World' | capitalize | pprint", "' hello world'"},
                            InputOutputPair{"'Hello123OOO, World!' | capitalize | pprint", "'Hello123ooo, world!'"}
                            ));

INSTANTIATE_TEST_CASE_P(Escape, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'' | escape | pprint", "''"},
                            InputOutputPair{"'abcd&><efgh' | escape | pprint", "'abcd&amp;&gt;&lt;efgh'"},
                            InputOutputPair{"'\\\"\\'' | escape | pprint", "'&#34;&#39;'"}
                            ));

INSTANTIATE_TEST_CASE_P(Batch, FilterGenericTest, ::testing::Values(
                            InputOutputPair{
                                "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16] | batch(linecount=3) | pprint",
                                "[[1, 2, 3, 4, 5, 6], [7, 8, 9, 10, 11, none], [12, 13, 14, 15, 16, none]]"
                            },
                            InputOutputPair{
                                "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16] | batch(3, 0) | pprint",
                                "[[1, 2, 3, 4, 5, 6], [7, 8, 9, 10, 11, 0], [12, 13, 14, 15, 16, 0]]"
                            },
                            InputOutputPair{
                                "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17] | batch(3, -1) | pprint",
                                "[[1, 2, 3, 4, 5, 6], [7, 8, 9, 10, 11, 12], [13, 14, 15, 16, 17, -1]]"
                            },
                            InputOutputPair{"[1, 2, 3] | batch(0) | pprint", "none"},
                            InputOutputPair{"[1, 2, 3, 4] | batch(2) | pprint", "[[1, 2], [3, 4]]"},
                            InputOutputPair{"'some string' | batch(0) | pprint", "none"},
                            InputOutputPair{"[] | batch(0) | pprint", "none"}
                            ));

INSTANTIATE_TEST_CASE_P(Format, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'Hello {}!' | format('World') ", "Hello World!"},
                            InputOutputPair{"'{1} {0}!' | format('World', 'Hello') | pprint", "'Hello World!'"},
                            InputOutputPair{"'{}' | format(1024) | pprint", "'1024'"},
                            InputOutputPair{"'{}' | format([1, 'a'])", "[1, 'a']"},
                            InputOutputPair{"'{}' | format({'a'=1})", "{'a': 1}"},
                            InputOutputPair{"'{}' | format(stringValue)","rain"},
                            InputOutputPair{"'{}' | format(wstringValue)", "  hello world "},
                            InputOutputPair{"'{:07d}' | format(1024) | pprint", "'0001024'"},
                            InputOutputPair{"'{0:.2f}' | format(13.949999988079071) | pprint", "'13.95'"},
                            InputOutputPair{"'{0:.15f}' | format(13.949999988079071) | pprint", "'13.949999988079071'"},
                            InputOutputPair{"'{0:.2f} != {1:02d}' | format(13.949999988079071, 7) | pprint", "'13.95 != 07'"},
                            InputOutputPair{"'PI = {pi:.2f}' | format(pi=3.1415) | pprint", "'PI = 3.14'"},
                            InputOutputPair{"'ONE = {one:02d}, PI = {pi:.2f}' | format(pi=3.1415, one=1) | pprint", "'ONE = 01, PI = 3.14'"},
                            InputOutputPair{"'Hello {name}!' | format(name='World')", "Hello World!"},
                            InputOutputPair{"'Hello {array}!' | format(array=[1, 2, 3])", "Hello [1, 2, 3]!"},
                            InputOutputPair{"'Hello {boolean}!' | format(boolean=True)", "Hello true!"},
                            InputOutputPair{"'Hello {empty}!' | format(empty=nonexistent)", "Hello none!"}
                        ));

INSTANTIATE_TEST_CASE_P(ListSlice, ListSliceTest, ::testing::Values(
                            InputOutputPair{"1 | slice(3) | pprint",                                 "none"},
                            InputOutputPair{"[] | slice(3) | pprint",                                "[]"},
                            InputOutputPair{"[1, 2, 3] | slice(3) | pprint",                         "[[1, 2, 3]]"},
                            InputOutputPair{"[1, 2, 3] | slice(3, 0) | pprint",                      "[[1, 2, 3]]"},
                            InputOutputPair{"[1, 2] | slice(3) | pprint",                            "[[1, 2]]"},
                            InputOutputPair{"[1, 2] | slice(3, 0) | pprint",                         "[[1, 2, 0]]"},
                            InputOutputPair{"[1, 2, 3, 4, 5, 6, 7, 8, 9] | slice(3) | pprint",       "[[1, 2, 3], [4, 5, 6], [7, 8, 9]]"},
                            InputOutputPair{"[1, 2, 3, 4, 5, 6, 7, 8, 9] | slice(3, 0) | pprint",    "[[1, 2, 3], [4, 5, 6], [7, 8, 9]]"},
                            InputOutputPair{"[1, 2, 3, 4, 5, 6, 7] | slice(3) | pprint",             "[[1, 2, 3], [4, 5, 6], [7]]"},
                            InputOutputPair{"[1, 2, 3, 4, 5, 6, 7] | slice(3, 0) | pprint",          "[[1, 2, 3], [4, 5, 6], [7, 0, 0]]"}
                            ));

INSTANTIATE_TEST_CASE_P(Striptags, FilterGenericTest, ::testing::Values(
                            InputOutputPair{ "' Hello  World ' | striptags | pprint", "'Hello World'" },
                            InputOutputPair{ "'foo <bar> baz <qux>' | striptags | pprint", "'foo baz'" },
                            InputOutputPair{"'ab&cd&amp;&gt;&lt;efgh' | striptags | pprint", "'ab&cd&><efgh'"},
                            InputOutputPair{"'<em>Foo &amp; Bar</em>' | striptags | pprint", "'Foo & Bar'"},
                            InputOutputPair{"'&amp;&apos;&gt;&lt;&quot;&#39;\"&#34;' | striptags | pprint", "'&\'><\"\'\"\"'"},
                            InputOutputPair{"'&#34;&#39;' | striptags | pprint", "'\"\''"}));

