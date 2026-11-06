#include "test_tools.h"

#include "../fuzz/prompt_escape_case.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/markup.h>
#include <jinja2cpp/prompt.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <optional>
#include <random>
#include <regex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace jinja2;

SUBSTITUTION_TEST_P(FilterGenericTest)

struct ListIteratorTestTag;
using ListIteratorTest = InputOutputPairTest<ListIteratorTestTag>;

struct GroupByTestTag;
using FilterGroupByTest = InputOutputPairTest<GroupByTestTag>;

struct ListSliceTestTag;
using ListSliceTest = InputOutputPairTest<ListSliceTestTag>;

TEST_P(ListIteratorTest, Test)
{
    const auto& testParam = GetParam();
    std::string source = "{% for i in " + testParam.tpl + " %}{{i}}{{', ' if not loop.last}}{% endfor %}";

    PerformBothTests(source, testParam.result);
}

TEST_P(FilterGroupByTest, Test)
{
    const auto& testParam = GetParam();

    jinja2::ValuesList testData;
    for (int n = 0; n < 10; ++ n)
    {
        TestStruct s;
        std::ostringstream str;
        std::wostringstream wstr;

        str << "test string " << n / 2;
        wstr << L"test string " << n;

        const auto half = n / 2;
        s.intValue = half;
        s.dblValue = static_cast<double>(half) / 2;
        s.boolValue = n % 2 == 1;
        s.strValue = str.str();
        s.wstrValue = wstr.str();

        testData.push_back(jinja2::Reflect(std::move(s)));
    }

    jinja2::ValuesMap params {{"testData", std::move(testData)}};

    std::string source = R"(
{% for group in )" + testParam.tpl + R"(
%}{% set grouper=group.grouper %}{% set list=group.list %}grouper: {{grouper | pprint }}
{%- for i in list %}
    {'intValue': {{i.intValue}}, 'dblValue': {{i.dblValue}}, 'boolValue': {{i.boolValue}}, 'strValue': '{{i.strValue}}', 'wstrValue': '<wchar_string>'}
{%- endfor %}
{% endfor %})";

    PerformBothTests(source, testParam.result, params);
}

using FilterGenericTestSingle = BasicTemplateRenderer;

MULTISTR_TEST(FilterGenericTestSingle, ApplyMacroTest,
              R"(
{% macro test(str, extra='') %}{{ (str ~ extra) | upper }}{% endmacro %}
{{ 'Hello World!' | applymacro(macro='test') }}
{{ 'Hello' | applymacro(' World!', macro='test') }}
{{ ['str1', 'str2', 'str3'] | map('applymacro', macro='test') | join(', ') }}
{{ ['str1', 'str2', 'str3'] | map('applymacro', '-', macro='test') | join(', ') }}
)",
              //-------------
              R"(

HELLO WORLD!
HELLO WORLD!
STR1, STR2, STR3
STR1-, STR2-, STR3-)")
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
)")
{
}

TEST_P(ListSliceTest, Test)
{
    const auto& testParam = GetParam();
    std::string source = "{{ " + testParam.tpl + " }}";

    PerformBothTests(source, testParam.result);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(StringJoin, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"['str1', 'str2', 'str3'] | join",                   "str1str2str3"},
                            InputOutputPair{"['str1', 'str2', 'str3'] | join(' ')",              "str1 str2 str3"},
                            InputOutputPair{"['str1', 'str2', 'str3'] | join(d='-')",            "str1-str2-str3"},
                            InputOutputPair{"reflectedList | join('-', attribute='strValue')",   "test string 0-test string 1-test string 2-test string 3-test string 4-test string 5-test string 6-test string 7-test string 8-test string 9"},
                            InputOutputPair{"reflectedList | join('-', attribute='strValue')",   "test string 0-test string 1-test string 2-test string 3-test string 4-test string 5-test string 6-test string 7-test string 8-test string 9"},
                            InputOutputPair{"reflectedList | join(attribute='strValue', d='-')", "test string 0-test string 1-test string 2-test string 3-test string 4-test string 5-test string 6-test string 7-test string 8-test string 9"}
                            ));
// clang-format on

INSTANTIATE_TEST_SUITE_P(Sort, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['str1', 'str2', 'str3'] | sort",                                    "str1, str2, str3"},
                            InputOutputPair{"['str2', 'str1', 'str3'] | sort",                                    "str1, str2, str3"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | sort",                                    "str1, Str2, str3"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | sort(reverse=true)",                      "str3, Str2, str1"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | sort(case_sensitive=true)",               "Str2, str1, str3"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | sort(case_sensitive=true, reverse=true)", "str3, str1, Str2"},
                            InputOutputPair{"[3, 1, 2] | sort",                                                   "1, 2, 3"},
                            InputOutputPair{"reflectedIntVector | sort",                                          "0, 1, 2, 3, 4, 5, 6, 7, 8, 9"}
                            ));

INSTANTIATE_TEST_SUITE_P(Default, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"intValue | default(0)", "3"},
                            InputOutputPair{"integerValue | default(0)", "0"},
                            InputOutputPair{"integerValue | d(0)", "0"},
                            InputOutputPair{"''|default('the string was empty', true)", "the string was empty"},
                            InputOutputPair{"''|default(default_value='the string was empty', boolean=true)", "the string was empty"},
                            InputOutputPair{"''|default('the string was empty', false)", ""},
                            InputOutputPair{"'Hello World!'|default('the string was empty', true)", "Hello World!"}
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(First, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4] | first", "1"},
                            InputOutputPair{"(1, 2, 3, 4) | first", "1"},
                            InputOutputPair{"intList | first", "9"},
                            InputOutputPair{"stringValue | list | first", "r"},
                            InputOutputPair{"reflectedIntVector | first", "9"}
                            ));

INSTANTIATE_TEST_SUITE_P(Last, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4] | last", "4"},
                            InputOutputPair{"(1, 2, 3, 4) | last", "4"},
                            InputOutputPair{"intList | last", "4"},
                            InputOutputPair{"stringValue | list | last", "n"},
                            InputOutputPair{"reflectedIntVector | last", "4"}
                            ));

INSTANTIATE_TEST_SUITE_P(Length, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4, 5] | length", "5"},
                            InputOutputPair{"(1, 2, 3, 4, 5, 6) | length", "6"},
                            InputOutputPair{"intList | length", "10"},
                            InputOutputPair{"stringValue | list | length", "4"},
                            InputOutputPair{"reflectedIntVector | length", "10"}
                            ));

INSTANTIATE_TEST_SUITE_P(Min, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4, 5] | min", "1"},
                            InputOutputPair{"(1, 2, 3, 4, 5, 6) | min", "1"},
                            InputOutputPair{"intList | min", "0"},
                            InputOutputPair{"stringValue | list | min", "a"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | min", "str1"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | min(true)", "Str6"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | min(case_sensitive=true)", "Str6"}
                            ));

INSTANTIATE_TEST_SUITE_P(Max, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4, 5] | max", "5"},
                            InputOutputPair{"(1, 2, 3, 4, 5, 6) | max", "6"},
                            InputOutputPair{"intList | max", "9"},
                            InputOutputPair{"stringValue | list | max", "r"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | max", "Str6"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | max(true)", "str5"},
                            InputOutputPair{"('str1', 'str2', 'str3', 'str4', 'str5', 'Str6') | max(case_sensitive=true)", "str5"}
                            ));
// clang-format on

INSTANTIATE_TEST_SUITE_P(Reverse, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['str1', 'str2', 'str3'] | reverse", "str3, str2, str1"},
                            InputOutputPair{"[3, 1, 2] | reverse",                "2, 1, 3"},
                            InputOutputPair{"reflectedIntVector | reverse",       "4, 5, 3, 6, 2, 7, 1, 8, 0, 9"}
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(Sum, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"[1, 2, 3, 4, 5] | sum",   "15"},
                            InputOutputPair{"[] | sum(start=15)",      "15"},
                            InputOutputPair{"intList | sum(start=10)", "55"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(Unique, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['str1', 'str2', 'str3'] | unique",                      "str1, str2, str3"},
                            InputOutputPair{"['str3', 'str1', 'str1'] | unique",                      "str3, str1"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | unique",                      "Str2, str1, str3"},
                            InputOutputPair{"['Str2', 'str2', 'str3'] | unique",                      "Str2, str3"},
                            InputOutputPair{"['Str2', 'str1', 'str3'] | unique(case_sensitive=true)", "Str2, str1, str3"},
                            InputOutputPair{"[3, 1, 2] | unique",                                     "3, 1, 2"},
                            InputOutputPair{"[3, 1, 2, 1, -2, 1, 10, 1, 6, 1, 5] | unique",           "3, 1, 2, -2, 10, 6, 5"},
                            InputOutputPair{"[3.0, 3, 1] | unique",                                   "3.0, 1"},
                            InputOutputPair{"reflectedList | unique(attribute='strValue') | map(attribute='strValue')",
                                                                                                      "test string 0, test string 1, test string 2, test string 3, test string 4, test string 5, test string 6, test string 7, test string 8, test string 9"},
                            InputOutputPair{"reflectedList | unique(attribute='boolValue') | map(attribute='strValue')",
                                                                                                      "test string 0, test string 1"},
                            // A reflected struct is a host object, which Python hashes; a dict is not
                            InputOutputPair{"reflectedList | unique | map(attribute='strValue')",
                                                                                                      "test string 0, test string 1, test string 2, test string 3, test string 4, test string 5, test string 6, test string 7, test string 8, test string 9"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(Attr, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"{'key': 'itemName', 'value': 'itemValue'} | attr('key')", ""},
                            InputOutputPair{"mapValue | attr('intVal')", ""},
                            InputOutputPair{"mapValue | attr('intVal', default='99')", "99"},
                            InputOutputPair{"mapValue | attr('nonexistent', default='99')", "99"},
                            InputOutputPair{"mapValue | attr(name='dblVal')", ""},
                            InputOutputPair{"mapValue | attr('stringVal')", ""},
                            InputOutputPair{"mapValue | attr('boolValue')", ""},
                            InputOutputPair{"reflectedVal | attr('intValue')", "0"},
                            InputOutputPair{"filledReflectedPtrVal | attr('strValue')", "test string 0"}
                            ));
// clang-format on

INSTANTIATE_TEST_SUITE_P(Map, ListIteratorTest, ::testing::Values(
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


INSTANTIATE_TEST_SUITE_P(Reject, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['', 'str1', '', 'str2', '', 'str3', '', 'str4'] | reject", ", , , "},
                            InputOutputPair{"['_str1', 'str1', '_str2', 'str2', '_str3', 'str3', '_str4', 'str4'] | reject('startsWith', '_')",
                                                                                                         "str1, str2, str3, str4"},
                            InputOutputPair{"['_str1', 'str1', '_str2', 'str2', '_str3', 'str3', '_str4', 'str4'] | reject('startsWith', str='_')",
                                                                                                         "str1, str2, str3, str4"}
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(RejectAttr, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"reflectedList | rejectattr('boolValue') | map(attribute='strValue')",
                                                                                        "test string 0, test string 2, test string 4, test string 6, test string 8"},
                            InputOutputPair{"reflectedList | rejectattr('boolValue', attribute='boolValue') | map(attribute='strValue')",
                                                                                        "test string 0, test string 2, test string 4, test string 6, test string 8"}
                            ));

INSTANTIATE_TEST_SUITE_P(Select, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"['', 'str1', '', 'str2', '', 'str3', '', 'str4'] | select", "str1, str2, str3, str4"},
                            InputOutputPair{"['_str1', 'str1', '_str2', 'str2', '_str3', 'str3', '_str4', 'str4'] | select('startsWith', '_')",
                                                                                                         "_str1, _str2, _str3, _str4"},
                            InputOutputPair{"['_str1', 'str1', '_str2', 'str2', '_str3', 'str3', '_str4', 'str4'] | select('startsWith', str='_')",
                                                                                                         "_str1, _str2, _str3, _str4"}
                            ));

INSTANTIATE_TEST_SUITE_P(SelectAttr, ListIteratorTest, ::testing::Values(
                            InputOutputPair{"reflectedList | selectattr('boolValue') | map(attribute='strValue')",
                                                                                        "test string 1, test string 3, test string 5, test string 7, test string 9"},
                            InputOutputPair{"reflectedList | selectattr('boolValue', attribute='boolValue') | map(attribute='strValue')",
                                                                                        "test string 1, test string 3, test string 5, test string 7, test string 9"}
                            ));
// clang-format on


// clang-format off
INSTANTIATE_TEST_SUITE_P(PPrint, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"10 | pprint", "10"},
                            InputOutputPair{"10.5 | pprint", "10.5"},
                            InputOutputPair{"intValue | pprint", "3"},
                            InputOutputPair{"stringValue | pprint", "'rain'"},
                            InputOutputPair{"boolFalseValue | pprint", "False"},
                            InputOutputPair{"boolTrueValue | pprint", "True"},
                            InputOutputPair{"none | pprint", "None"},
                            InputOutputPair{"[none, true, false] | pprint", "[None, True, False]"},
                            InputOutputPair{"() | pprint", "()"},
                            InputOutputPair{"(1,) | pprint", "(1,)"},
                            InputOutputPair{"(1, (2, 3)) | pprint", "(1, (2, 3))"},
                            InputOutputPair{"{'a': (none, false)} | dictsort | pprint", "[('a', (None, False))]"},
                            InputOutputPair{"intList | pprint", "[9, 0, 8, 1, 7, 2, 6, 3, 5, 4]"},
                            InputOutputPair{"{'key': 'itemName'} | pprint", "{'key': 'itemName'}"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(GroupBy, FilterGroupByTest, ::testing::Values(
                            InputOutputPair{"testData | groupby('intValue')", R"(
grouper: 0
    {'intValue': 0, 'dblValue': 0.0, 'boolValue': False, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 0, 'dblValue': 0.0, 'boolValue': True, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
grouper: 1
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': False, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': True, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
grouper: 2
    {'intValue': 2, 'dblValue': 1.0, 'boolValue': False, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1.0, 'boolValue': True, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
grouper: 3
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': False, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': True, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
grouper: 4
    {'intValue': 4, 'dblValue': 2.0, 'boolValue': False, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2.0, 'boolValue': True, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
)"
                                },
                            InputOutputPair{"testData | groupby('dblValue')", R"(
grouper: 0.0
    {'intValue': 0, 'dblValue': 0.0, 'boolValue': False, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 0, 'dblValue': 0.0, 'boolValue': True, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
grouper: 0.5
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': False, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': True, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
grouper: 1.0
    {'intValue': 2, 'dblValue': 1.0, 'boolValue': False, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1.0, 'boolValue': True, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
grouper: 1.5
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': False, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': True, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
grouper: 2.0
    {'intValue': 4, 'dblValue': 2.0, 'boolValue': False, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2.0, 'boolValue': True, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
)"
                                },
                            InputOutputPair{"testData | groupby('strValue')", R"(
grouper: 'test string 0'
    {'intValue': 0, 'dblValue': 0.0, 'boolValue': False, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 0, 'dblValue': 0.0, 'boolValue': True, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
grouper: 'test string 1'
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': False, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': True, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
grouper: 'test string 2'
    {'intValue': 2, 'dblValue': 1.0, 'boolValue': False, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1.0, 'boolValue': True, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
grouper: 'test string 3'
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': False, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': True, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
grouper: 'test string 4'
    {'intValue': 4, 'dblValue': 2.0, 'boolValue': False, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2.0, 'boolValue': True, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
)"
                                },
                            InputOutputPair{"testData | groupby('boolValue')", R"(
grouper: False
    {'intValue': 0, 'dblValue': 0.0, 'boolValue': False, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': False, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1.0, 'boolValue': False, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': False, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2.0, 'boolValue': False, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
grouper: True
    {'intValue': 0, 'dblValue': 0.0, 'boolValue': True, 'strValue': 'test string 0', 'wstrValue': '<wchar_string>'}
    {'intValue': 1, 'dblValue': 0.5, 'boolValue': True, 'strValue': 'test string 1', 'wstrValue': '<wchar_string>'}
    {'intValue': 2, 'dblValue': 1.0, 'boolValue': True, 'strValue': 'test string 2', 'wstrValue': '<wchar_string>'}
    {'intValue': 3, 'dblValue': 1.5, 'boolValue': True, 'strValue': 'test string 3', 'wstrValue': '<wchar_string>'}
    {'intValue': 4, 'dblValue': 2.0, 'boolValue': True, 'strValue': 'test string 4', 'wstrValue': '<wchar_string>'}
)"
                                }
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(DictSort, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"{'key': 'itemName', 'Value': 'ItemValue'} | dictsort | pprint", "[('key', 'itemName'), ('Value', 'ItemValue')]"},
                            InputOutputPair{"{'key': 'itemName', 'Value': 'ItemValue'} | dictsort(by='value') | pprint", "[('key', 'itemName'), ('Value', 'ItemValue')]"},
                            InputOutputPair{"{'key': 'itemName', 'Value': 'ItemValue'} | dictsort(reverse=true) | pprint", "[('Value', 'ItemValue'), ('key', 'itemName')]"},
                            InputOutputPair{"{'key': 'itemName', 'Value': 'ItemValue'} | dictsort(reverse=true, by='value') | pprint", "[('Value', 'ItemValue'), ('key', 'itemName')]"},
                            InputOutputPair{"{'key': 'itemName', 'Value': 'ItemValue'} | dictsort(case_sensitive=true) | pprint", "[('Value', 'ItemValue'), ('key', 'itemName')]"},
                            InputOutputPair{"{'key': 'itemName', 'Value': 'ItemValue'} | dictsort(case_sensitive=true, reverse=true) | pprint", "[('key', 'itemName'), ('Value', 'ItemValue')]"},
                            InputOutputPair{"simpleMapValue | dictsort | pprint", "[('boolValue', True),\n"
                                                                                  " ('dblVal', 100.5),\n"
                                                                                  " ('intVal', 10),\n"
                                                                                  " ('stringVal', 'string100.5')]"},
    InputOutputPair{
      "reflectedVal | dictsort | pprint",
      "[('basicCallable', <callable>),\n"
      " ('boolValue', False),\n"
      " ('dblValue', 0.0),\n"
      " ('getInnerStruct', <callable>),\n"
      " ('getInnerStructValue', <callable>),\n"
      " ('innerStruct', {'strValue': 'Hello World!'}),\n"
      " ('innerStructList',\n"
      "  [{'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'}]),\n"
      " ('intEvenValue', 0),\n"
      " ('intValue', 0),\n"
      " ('strValue', 'test string 0'),\n"
      " ('strViewValue', 'test string 0'),\n"
      " ('tmpStructList',\n"
      "  [{'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'},\n"
      "   {'strValue': 'Hello World!'}]),\n"
      " ('wstrValue', 'test string 0'),\n"
      " ('wstrViewValue', 'test string 0')]" }));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(UrlEncode, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'Hello World' | urlencode", "Hello%20World"},
                            // InputOutputPair{"'Hello World\xD0\x9C\xD0\xBA' | urlencode", "Hello+World%D0%9C%D0%BA"},
                            InputOutputPair{"'! # $ & ( ) * + , / : ; = ? @ [ ] %' | urlencode", "%21%20%23%20%24%20%26%20%28%20%29%20%2A%20%2B%20%2C%20/%20%3A%20%3B%20%3D%20%3F%20%40%20%5B%20%5D%20%25"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(Abs, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"10 | abs", "10"},
                            InputOutputPair{"-10 | abs", "10"},
                            InputOutputPair{"10.5 | abs", "10.5"},
                            InputOutputPair{"-10.5 | abs", "10.5"},
                            InputOutputPair{"true | abs", "1"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(Round, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"10 | round", "10"},
                            InputOutputPair{"10 | round(1)", "10"},
                            InputOutputPair{"10.5 | round", "10.0"},
                            InputOutputPair{"10.4 | round", "10.0"},
                            InputOutputPair{"10.6 | round", "11.0"},
                            InputOutputPair{"-10.5 | round", "-10.0"},
                            InputOutputPair{"-10.4 | round", "-10.0"},
                            InputOutputPair{"-10.6 | round", "-11.0"},
                            InputOutputPair{"10.5 | round(method='ceil')", "11.0"},
                            InputOutputPair{"10.5 | round(method='floor')", "10.0"},
                            InputOutputPair{"-10.5 | round(method='ceil')", "-10.0"},
                            InputOutputPair{"-10.5 | round(method='floor')", "-11.0"},
                            InputOutputPair{"10.44 | round(1)", "10.4"},
                            InputOutputPair{"10.46 | round(precision=1)", "10.5"},
                            InputOutputPair{"9223372036854775800 | round(-1)", "9223372036854775800"},
                            InputOutputPair{"-9223372036854775800 | round(-1)", "-9223372036854775800"},
                            InputOutputPair{"9223372036854775807 | round(-18)", "9000000000000000000"},
                            InputOutputPair{"5 | round(-19)", "0"},
                            InputOutputPair{"-5000000000000000000 | round(-19)", "0"},
                            InputOutputPair{"5000000000000000000 | round(-19)", "0"},
                            InputOutputPair{"9000000000000000000 | round(-20)", "0"},
                            InputOutputPair{"25 | round(-1)", "20"},
                            InputOutputPair{"5 | round(300, 'floor')", "5.0"},
                            InputOutputPair{"5 | round(309, 'ceil')", "5.0"},
                            InputOutputPair{"2.5 | round(300, 'floor')", "2.5"},
                            InputOutputPair{"0.1 | round(17, 'floor')", "0.1"},
                            InputOutputPair{"0.1 | round(17, 'ceil')", "0.1"},
                            InputOutputPair{"1.7 | round(25, 'floor')", "1.7000000000000002"},
                            InputOutputPair{"123456.789 | round(30, 'floor')", "123456.78900000002"},
                            InputOutputPair{"5 | round(-5, 'floor')", "0.0"},
                            InputOutputPair{"5 | round(-5, 'ceil')", "99999.99999999999"},
                            InputOutputPair{"15 | round(-1, 'ceil')", "20.0"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(Convert, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"10 | int", "10"},
                            InputOutputPair{"'1 2' | int", "0"},
                            InputOutputPair{"10 | float", "10.0"},
                            InputOutputPair{"10.5 | int", "10"},
                            InputOutputPair{"'10.4' | float | pprint", "10.4"},
                            InputOutputPair{"'100;4' | float(10.4) | pprint", "10.4"},
                            InputOutputPair{"'100;4' | int(10) | pprint", "10"},
                            InputOutputPair{"'100' | int(10) | pprint", "100"},
                            InputOutputPair{"'0x100' | int(10, 0) | pprint", "256"},
                            InputOutputPair{"'0100' | int(10, 0) | pprint", "100"},
                            InputOutputPair{"'100' | int(10, base=10) | pprint", "100"},
                            InputOutputPair{"'100' | int(10, base=2) | pprint", "4"},
                            InputOutputPair{"'100' | int(10, base=8) | pprint", "64"},
                            InputOutputPair{"'100' | int(10, base=16) | pprint", "256"},
                            InputOutputPair{"'100' | list | pprint", "['1', '0', '0']"},
                            InputOutputPair{"{'name': 'itemName', 'val': 'itemValue'} | list | sort | pprint", "['name', 'val']"}
                            ));
// clang-format on

INSTANTIATE_TEST_SUITE_P(Trim, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'string' | trim | pprint", "'string'"},
                            InputOutputPair{"'    string' | trim | pprint", "'string'"},
                            InputOutputPair{"'string    ' | trim | pprint", "'string'"},
                            InputOutputPair{"'    string     ' | trim | pprint", "'string'"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));

INSTANTIATE_TEST_SUITE_P(Title, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'string' | title | pprint", "'String'"},
                            InputOutputPair{"'1234string' | title | pprint", "'1234string'"},
                            InputOutputPair{"'hello world' | title | pprint", "'Hello World'"},
                            InputOutputPair{"'hello123ooo, world!' | title | pprint", "'Hello123ooo, World!'"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));

INSTANTIATE_TEST_SUITE_P(Upper, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'string' | upper | pprint", "'STRING'"},
                            InputOutputPair{"'1234string' | upper | pprint", "'1234STRING'"},
                            InputOutputPair{"'hello world' | upper | pprint", "'HELLO WORLD'"},
                            InputOutputPair{"'hello123ooo, world!' | upper | pprint", "'HELLO123OOO, WORLD!'"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));


INSTANTIATE_TEST_SUITE_P(Lower, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'String' | lower | pprint", "'string'"},
                            InputOutputPair{"'1234String' | lower | pprint", "'1234string'"},
                            InputOutputPair{"'Hello World' | lower | pprint", "'hello world'"},
                            InputOutputPair{"'Hello123OOO, World!' | lower | pprint", "'hello123ooo, world!'"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));


INSTANTIATE_TEST_SUITE_P(WordCount, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'string' | wordcount", "1"},
                            InputOutputPair{"'1234string' | wordcount", "1"},
                            InputOutputPair{"'hello world' | wordcount", "2"},
                            InputOutputPair{"'hello123ooo, world!' | wordcount", "2"}/*,
                            InputOutputPair{"wstringValue | trim", "'hello world'"}*/
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(Replace, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'Hello World' | replace('Hello', 'Goodbye') | pprint", "'Goodbye World'"},
                            InputOutputPair{"'Hello World' | replace(old='l', new='L') | pprint", "'HeLLo WorLd'"},
                            InputOutputPair{"'Hello World' | replace('l', 'L', count=2) | pprint", "'HeLLo World'"},
                            InputOutputPair{"'Hello World' | replace('l', 'L', count=1) | pprint", "'HeLlo World'"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(Truncate, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'foo bar baz qux' | truncate(6, leeway=0) | pprint", "'foo...'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(6, true) | pprint", "'foo...'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(11, true) | pprint", "'foo bar baz qux'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(11, true, leeway=0) | pprint", "'foo bar ...'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(9) | pprint", "'foo...'"},
                            InputOutputPair{"'VeryVeryVeryLongWord' | truncate(3) | pprint", "'...'"},
                            InputOutputPair{"'VeryVeryVeryLongWord' | truncate(16) | pprint", "'VeryVeryVeryLongWord'"},
                            InputOutputPair{"'foo bar baz qux' | truncate(6, end=' >>', leeway=0) | pprint", "'foo >>'"}
                            ));
// clang-format on

INSTANTIATE_TEST_SUITE_P(Capitalize, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'String' | capitalize | pprint", "'String'"},
                            InputOutputPair{"'string' | capitalize | pprint", "'String'"},
                            InputOutputPair{"'1234string' | capitalize | pprint", "'1234string'"},
                            InputOutputPair{"'1234String' | capitalize | pprint", "'1234string'"},
                            InputOutputPair{"'Hello World' | capitalize | pprint", "'Hello world'"},
                            InputOutputPair{"' Hello World' | capitalize | pprint", "' hello world'"},
                            InputOutputPair{"'Hello123OOO, World!' | capitalize | pprint", "'Hello123ooo, world!'"}
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(Escape, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'' | escape | pprint", "Markup('')"},
                            InputOutputPair{"'abcd&><efgh' | escape | pprint", "Markup('abcd&amp;&gt;&lt;efgh')"},
                            InputOutputPair{"'\\\"\\'' | escape | pprint", "Markup('&#34;&#39;')"}
                            ));
// clang-format on

// clang-format off
// Escaping of strings longer than the per-thread buffer (4096 bytes) takes another path
INSTANTIATE_TEST_SUITE_P(EscapeLong, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"('<' * 4097) | escape | length", "16388"},
                            InputOutputPair{"('a<' * 2100) | escape | length", "10500"},
                            InputOutputPair{"('a<' * 2100) | escape | replace('a&lt;', '') | pprint", "''"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(Batch, FilterGenericTest, ::testing::Values(
                            InputOutputPair{
                                "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16] | batch(linecount=3) | list | pprint",
                                "[[1, 2, 3], [4, 5, 6], [7, 8, 9], [10, 11, 12], [13, 14, 15], [16]]"
                            },
                            InputOutputPair{
                                "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16] | batch(3, 0) | list | pprint",
                                "[[1, 2, 3], [4, 5, 6], [7, 8, 9], [10, 11, 12], [13, 14, 15], [16, 0, 0]]"
                            },
                            InputOutputPair{
                                "[1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17] | batch(3, -1) | list | pprint",
                                "[[1, 2, 3], [4, 5, 6], [7, 8, 9], [10, 11, 12], [13, 14, 15], [16, 17, -1]]"
                            },
                            InputOutputPair{"[1, 2, 3] | batch(0) | list | pprint", "[[], [1, 2, 3]]"},
                            InputOutputPair{"[1, 2, 3, 4] | batch(2) | list | pprint", "[[1, 2], [3, 4]]"},
                            InputOutputPair{"'some string' | batch(0) | list | pprint", "[[], ['s', 'o', 'm', 'e', ' ', 's', 't', 'r', 'i', 'n', 'g']]"},
                            InputOutputPair{"[] | batch(0) | list | pprint", "[]"},
                            InputOutputPair{"nope | batch(2) | list | pprint", "[]"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(Format, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'Hello %s!' | format('World') ", "Hello World!"},
                            InputOutputPair{"'%s %s!' | format('Hello', 'World') | pprint", "'Hello World!'"},
                            InputOutputPair{"'%s' | format(1024) | pprint", "'1024'"},
                            InputOutputPair{"'%s' | format([1, 'a'])", "[1, 'a']"},
                            InputOutputPair{"'%s' | format({'a': 1})", "{'a': 1}"},
                            InputOutputPair{"'%s' | format(stringValue)","rain"},
                            InputOutputPair{"'%s' | format(wstringValue)", "  hello world "},
                            InputOutputPair{"'%07d' | format(1024) | pprint", "'0001024'"},
                            InputOutputPair{"'%.2f' | format(13.949999988079071) | pprint", "'13.95'"},
                            InputOutputPair{"'%.15f' | format(13.949999988079071) | pprint", "'13.949999988079071'"},
                            InputOutputPair{"'%.2f != %02d' | format(13.949999988079071, 7) | pprint", "'13.95 != 07'"},
                            InputOutputPair{"'PI = %(pi).2f' | format(pi=3.1415) | pprint", "'PI = 3.14'"},
                            InputOutputPair{"'ONE = %(one)02d, PI = %(pi).2f' | format(pi=3.1415, one=1) | pprint", "'ONE = 01, PI = 3.14'"},
                            InputOutputPair{"'Hello %(name)s!' | format(name='World')", "Hello World!"},
                            InputOutputPair{"'Hello %(array)s!' | format(array=[1, 2, 3])", "Hello [1, 2, 3]!"},
                            InputOutputPair{"'Hello %(boolean)s!' | format(boolean=True)", "Hello True!"},
                            InputOutputPair{"'x' | format()", "x"},
                            InputOutputPair{"'x' | format(a=1)", "x"},
                            InputOutputPair{"'{}' | format()", "{}"},
                            InputOutputPair{"'%d' | format(1e30)", "1000000000000000019884624838656"},
                            InputOutputPair{"'%d' | format(-1e30)", "-1000000000000000019884624838656"},
                            InputOutputPair{"'%5.3d|%+d|%05d|%-24d|' | format(1e19, 1e19, -1e19, 1e19)", "10000000000000000000|+10000000000000000000|-10000000000000000000|10000000000000000000    |"},
                            InputOutputPair{"'%i|%u' | format(2e25, 9223372036854775808.0)", "20000000000000001811939328|9223372036854775808"},
                            InputOutputPair{"'%a' | format('x')", "'x'"}
                        ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(ListSlice, ListSliceTest, ::testing::Values(
                            InputOutputPair{"[] | slice(3) | list | pprint",                                "[[], [], []]"},
                            InputOutputPair{"[1, 2, 3] | slice(3) | list | pprint",                         "[[1], [2], [3]]"},
                            InputOutputPair{"[1, 2, 3] | slice(3, 0) | list | pprint",                      "[[1, 0], [2, 0], [3, 0]]"},
                            InputOutputPair{"[1, 2] | slice(3) | list | pprint",                            "[[1], [2], []]"},
                            InputOutputPair{"[1, 2] | slice(3, 0) | list | pprint",                         "[[1], [2], [0]]"},
                            InputOutputPair{"[1, 2, 3, 4, 5, 6, 7, 8, 9] | slice(3) | list | pprint",       "[[1, 2, 3], [4, 5, 6], [7, 8, 9]]"},
                            InputOutputPair{"[1, 2, 3, 4, 5, 6, 7, 8, 9] | slice(3, 0) | list | pprint",    "[[1, 2, 3, 0], [4, 5, 6, 0], [7, 8, 9, 0]]"},
                            InputOutputPair{"[1, 2, 3, 4, 5, 6, 7] | slice(3) | list | pprint",             "[[1, 2, 3], [4, 5], [6, 7]]"},
                            InputOutputPair{"[1, 2, 3, 4, 5, 6, 7] | slice(3, 0) | list | pprint",          "[[1, 2, 3], [4, 5, 0], [6, 7, 0]]"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(Striptags, FilterGenericTest, ::testing::Values(
                            InputOutputPair{ "' Hello  World ' | striptags | pprint", "'Hello World'" },
                            InputOutputPair{ "'foo <bar> baz <qux>' | striptags | pprint", "'foo baz'" },
                            InputOutputPair{"'ab&cd&amp;&gt;&lt;efgh' | striptags | pprint", "'ab&cd&><efgh'"},
                            InputOutputPair{"'<em>Foo &amp; Bar</em>' | striptags | pprint", "'Foo & Bar'"},
                            InputOutputPair{"'&amp;&apos;&gt;&lt;&quot;&#39;\"&#34;' | striptags | pprint", "'&\\'><\"\\'\"\"'"},
                            InputOutputPair{"'&#34;&#39;' | striptags | pprint", "'\"\\''"}));
// clang-format on


// clang-format off
INSTANTIATE_TEST_SUITE_P(Center, FilterGenericTest, ::testing::Values(
                            InputOutputPair{" 'x' | center | pprint", "('                                       '\n"
                                                                      " 'x                                        ')"},
                            InputOutputPair{" 'x' | center(width=5) | pprint", "'  x  '"},
                            InputOutputPair{" 'x' | center(width=0) | pprint", "'x'"},
                            InputOutputPair{" '  x' | center(width=5) | pprint", "'   x '"}
                            ));

// C++-only filters (docs/tasks/0233): identifier case conversion, C++ string-literal escaping
// and block-style YAML
INSTANTIATE_TEST_SUITE_P(Camelize, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'device_type' | camelize", "DeviceType"},
                            InputOutputPair{"'device_type' | camelize(upper_first=false)", "deviceType"},
                            InputOutputPair{"'HTTPServer' | camelize", "HttpServer"},
                            InputOutputPair{"'XMLHttpRequest' | camelize(false)", "xmlHttpRequest"},
                            InputOutputPair{"'__FOO_BAR__' | camelize", "FooBar"},
                            InputOutputPair{"'some-name with  spaces.x' | camelize", "SomeNameWithSpacesX"},
                            InputOutputPair{"'vector3d_type' | camelize", "Vector3dType"},
                            InputOutputPair{"'' | camelize | pprint", "''"},
                            InputOutputPair{"42 | camelize | pprint", "'42'"}
                            ));

INSTANTIATE_TEST_SUITE_P(Underscorize, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'DeviceType' | underscorize", "device_type"},
                            InputOutputPair{"'HTTPServer' | underscorize", "http_server"},
                            InputOutputPair{"'getHTTPResponseCode' | underscorize", "get_http_response_code"},
                            InputOutputPair{"'Vector3D' | underscorize", "vector3_d"},
                            InputOutputPair{"'already_snake' | underscorize", "already_snake"},
                            InputOutputPair{"'  Some Name-here ' | underscorize", "some_name_here"},
                            InputOutputPair{"'__init__' | underscorize", "init"},
                            InputOutputPair{"'deviceType' | camelize | underscorize", "device_type"}
                            ));

INSTANTIATE_TEST_SUITE_P(EscapeCpp, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"'say \"hi\"' | escapecpp", "say \\\"hi\\\""},
                            InputOutputPair{"'a\\\\b' | escapecpp", "a\\\\b"},
                            InputOutputPair{"'tab\\tnl\\ncr\\r' | escapecpp", "tab\\tnl\\ncr\\r"},
                            InputOutputPair{"\"it's ok\" | escapecpp", "it's ok"},
                            InputOutputPair{"12 | escapecpp", "12"}
                            ));

INSTANTIATE_TEST_SUITE_P(ToYaml, FilterGenericTest, ::testing::Values(
                            InputOutputPair{"{'a': 1, 'b': [1, 2], 'c': {'d': none, 'e': 'x y'}} | toyaml",
                                            "a: 1\nb:\n  - 1\n  - 2\nc:\n  d: null\n  e: x y"},
                            InputOutputPair{"{1: 'a', (1, 2): 'b', none: 'c'} | toyaml", "\"1\": a\n\"(1, 2)\": b\nNone: c"},
                            InputOutputPair{"[1, 'two', [3, 4], {'k': true}, [], {}] | toyaml",
                                            "- 1\n- two\n- - 3\n  - 4\n- k: true\n- []\n- {}"},
                            InputOutputPair{"[{'a': 1, 'b': {'c': 2}}] | toyaml", "- a: 1\n  b:\n    c: 2"},
                            InputOutputPair{"{'items': [{'id': 1}, {'id': 2}, {'n': 3}], 'e': []} | toyaml", "items:\n  - id: 1\n  - id: 2\n  - \"n\": 3\ne: []"},
                            InputOutputPair{"['yes', '', '1.5', ' x', 'x ', 'a: b', 'a#b', 'Null', '-x', 'a\\nb', 'q\"\\\\'] | toyaml",
                                            "- \"yes\"\n- \"\"\n- \"1.5\"\n- \" x\"\n- \"x \"\n- \"a: b\"\n- \"a#b\"\n- \"Null\"\n- \"-x\"\n- \"a\\nb\"\n- \"q\\\"\\\\\""},
                            InputOutputPair{"['plain text', 'path/to-file.txt', 'f(x) + 1', '_x'] | toyaml",
                                            "- plain text\n- path/to-file.txt\n- f(x) + 1\n- _x"},
                            InputOutputPair{"[1.5, 1e20, -2.0, false] | toyaml", "- 1.5\n- 1.0e+20\n- -2.0\n- false"},
                            InputOutputPair{"{'a': 1} | dictsort | toyaml", "- - a\n  - 1"},
                            InputOutputPair{"'plain' | toyaml", "plain"},
                            InputOutputPair{"none | toyaml", "null"},
                            InputOutputPair{"{} | toyaml", "{}"},
                            InputOutputPair{"[] | toyaml", "[]"}
                            ));
// clang-format on
struct XmlAttr : ::testing::Test
{
    template<typename CharT>
    using AttributeSet = std::set<std::basic_string<CharT>>;

    template <typename CharT>
    AttributeSet<CharT> ExtractAttributeSet(const std::basic_string<CharT> &attributeString)
    {
        using String = std::basic_string<CharT>;
        using Regex = std::basic_regex<CharT>;
        using RegexTokenIterator = std::regex_token_iterator<typename String::const_iterator>;

        AttributeSet<CharT> result;
        const Regex pattern(ConvertString<String>(std::string(R"((\S+=["].*?["]))")));
        std::copy(RegexTokenIterator(attributeString.begin(), attributeString.end(), pattern, 0),
                  RegexTokenIterator(),
                  std::inserter(result, result.begin()));

        return result;
    }

    template<typename TemplateType, typename StringType>
    void PerformXmlAttrTest(const StringType& source, const StringType& expectedResult, const jinja2::ValuesMap& params)
    {
        TemplateType tpl;
        ASSERT_TRUE(tpl.Load(source));
        const auto result = tpl.RenderAsString(params).value();

        const auto resultAttributes = ExtractAttributeSet(result);
        const auto expectedAttributes = ExtractAttributeSet(expectedResult);

        EXPECT_EQ(resultAttributes, expectedAttributes);
    }

    void PerformBothXmlAttrTests(const std::string& source, const std::string& expectedResult, const jinja2::ValuesMap& params)
    {
        PerformXmlAttrTest<Template>(source, expectedResult, params);
        PerformXmlAttrTest<TemplateW>(ConvertString<std::wstring>(source), ConvertString<std::wstring>(expectedResult), params);
    }

    template<typename TemplateType, typename StringType>
    bool ParseTemplate(const StringType& source, const jinja2::ValuesMap& params = {})
    {
        TemplateType tpl;
        if (!tpl.Load(source))
            return false;

        return tpl.RenderAsString(params).has_value();
    }

    void PerformNegativeTest(const std::string& source, const jinja2::ValuesMap& params = {})
    {
        EXPECT_FALSE(ParseTemplate<Template>(source, params));
        EXPECT_FALSE(ParseTemplate<TemplateW>(ConvertString<std::wstring>(source), params));
    }
};

TEST_F(XmlAttr, FixtureValidation)
{
    constexpr auto source = R"(a="1" b="str" c="k:'a'")";
    const auto attributes = ExtractAttributeSet(std::string(source));

    ASSERT_EQ(attributes.size(), 3);
    ASSERT_TRUE(attributes.find(R"(a="1")") != attributes.end());
    ASSERT_TRUE(attributes.find(R"(b="str")") != attributes.end());
    ASSERT_TRUE(attributes.find(R"(c="k:'a'")") != attributes.end());

    ASSERT_TRUE(attributes.find(R"(z="1")") == attributes.end());
}

TEST_F(XmlAttr, SerializeFlatMap)
{
    constexpr auto source = "{{ {'foo': 42, 'bar': 1.35, 'bool': true, 'blub:blub': '<?>'}|xmlattr }}";
    constexpr auto expectedResult = R"(foo="42" bar="1.35" bool="True" blub:blub="&lt;?&gt;")";

    PerformBothXmlAttrTests(source, expectedResult, {});
}

TEST_F(XmlAttr, SerializeNestedMap)
{
    constexpr auto source = "{{ { 'foo': {'bar': '\"&<>'}, 'l': [1, 2, 3], 'blub:blub': '<?>' }|xmlattr }}";
    constexpr auto expectedResult = R"(foo="{&#39;bar&#39;: &#39;&#34;&amp;&lt;&gt;&#39;}" l="[1, 2, 3]" blub:blub="&lt;?&gt;")";

    PerformBothXmlAttrTests(source, expectedResult, {});
}

TEST_F(XmlAttr, FilterCanBeAppliedToMapOnly)
{
    using namespace std::string_literals;
    PerformNegativeTest("{{ [1, 2, 3]|xmlattr }}"s);
    PerformNegativeTest("{{ 10|xmlattr }}"s);
    PerformNegativeTest("{{ 1.2|xmlattr }}"s);
    PerformNegativeTest("{{ 'string'|xmlattr }}"s);
}

struct TestValues
{
    std::string strValue;
    std::wstring wstrValue;
};

namespace jinja2
{
template<>
struct TypeReflection<TestValues> : TypeReflected<TestValues>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            { "str_view",
              [](const TestValues& obj) {
                  return jinja2::Reflect(std::string_view(obj.strValue));
              } },
            { "wstr_view",
              [](const TestValues& obj) {
                  return jinja2::Reflect(std::wstring_view(obj.wstrValue));
              } },
            { "callable",
              [](const TestValues& /*obj*/) {
                  return jinja2::MakeCallable([]() {
                      return 0;
                  });
              } },
            { "none",
              [](const TestValues& /*obj*/) {
                  return jinja2::NoneValue();
              } },
        };

        return accessors;
    }
};

} // namespace jinja2

TEST_F(XmlAttr, SerializeMapWithStringViewsAndNoneSerializebleValues)
{
    constexpr auto source = "{{ obj|xmlattr }}";
    constexpr auto expectedResult = R"(str_view="string" wstr_view="wstring")";

    TestValues testValues {"string", L"wstring"};
    ValuesMap params{ { "obj",  jinja2::Reflect(testValues)  }};

    PerformBothXmlAttrTests(source, expectedResult, params);
}

// Case filters at the edges of the ASCII letter ranges and around non-ASCII text: ASCII letters
// change, other characters stay as they are (0048), and title splits words on Unicode
// whitespace too. Wide sources are written out: the shared fixture widens templates byte by
// byte.
TEST(CaseFilters, AsciiAndNonAsciiText)
{
    const std::pair<std::string, std::string> cases[] = {
        { "{{ '@AZ[`az{' | upper }}", "@AZ[`AZ{" },
        { "{{ '@AZ[`az{' | lower }}", "@az[`az{" },
        { "{{ 'xY@[`{' | capitalize }}|{{ '' | capitalize }}", "Xy@[`{|" },
        { "{{ 'a-b(c{d[e<f g\th]i)j}k>l_m@' | title }}|{{ '' | title }}", "A-B(C{D[E<F G\tH]i)j}k>l_m@|" },
        { "{{ 'a\xE2\x86\x92xb' | upper }}", "A\xE2\x86\x92XB" },
        { "{{ 'A\xE2\x86\x92XB' | lower }}", "a\xE2\x86\x92xb" },
        { "{{ '\xE2\x86\x92xAb' | capitalize }}", "\xE2\x86\x92xab" },
        { "{{ 'oNE\xE3\x80\x80tWO\xE2\x86\x92thrEE \xE2\x86\x92x' | title }}", "One\xE3\x80\x80Two\xE2\x86\x92three \xE2\x86\x92x" },
        // Full Unicode case mappings and Final_Sigma (0048); octal escapes cannot swallow a following letter
        { "{{ '\307\206a'|capitalize }}|{{ '\303\237a'|upper }}|{{ '\316\221\316\243 \316\221\316\243\316\221'|lower }}|{{ '\316\221\316\243 b\316\243'|title }}|{{ '\304\260'|lower|length }}|{{ '\316\221\316\243.'|capitalize }}|{{ '\357\254\201x \307\206'|title }}", "\307\205a|SSA|\316\261\317\202 \316\261\317\203\316\261|\316\221\317\203 B\317\203|2|\316\221\317\202.|FIx \307\204" },
        { "{{ '\303\251a'|upper }}|{{ '\303\211A'|title }}|{{ '\303\211A'|lower }}|{{ '\303\251a'|capitalize }}", "\303\211A|\303\211a|\303\251a|\303\211a" },
        // The HTML5 named references of html.unescape, longest legacy prefix, dropped numeric ones
        { "[{{ '&hearts; &frac12; &amp x &ampx &notit; &#x1F600; &#0; &#128; &Eacute &copyright'|striptags }}]", "[\xE2\x99\xA5 \xC2\xBD & x &x \xC2\xACit; \xF0\x9F\x98\x80 \xEF\xBF\xBD \xE2\x82\xAC \xC3\x89 \xC2\xA9right]" },
        { "[{{ '&#1; &#x7f; &#xFFFE; &#xD800; &#65 &AMP &NotEqualTilde; &acE; &amp\xC3\xA9'|striptags }}]", "[   \xEF\xBF\xBD A & \xE2\x89\x82\xCC\xB8 \xE2\x88\xBE\xCC\xB3 &\xC3\xA9]" },
    };
    const std::pair<std::wstring, std::wstring> wideCases[] = {
        { L"{{ '@AZ[`az{' | upper }}", L"@AZ[`AZ{" },
        { L"{{ '@AZ[`az{' | lower }}", L"@az[`az{" },
        { L"{{ 'xY@[`{' | capitalize }}|{{ '' | capitalize }}", L"Xy@[`{|" },
        { L"{{ 'a-b(c{d[e<f g\th]i)j}k>l_m@' | title }}|{{ '' | title }}", L"A-B(C{D[E<F G\tH]i)j}k>l_m@|" },
        { L"{{ 'a\x2192xb' | upper }}", L"A\x2192XB" },
        { L"{{ 'A\x2192XB' | lower }}", L"a\x2192xb" },
        { L"{{ '\x2192xAb' | capitalize }}", L"\x2192xab" },
        { L"{{ 'oNE\x3000tWO\x2192thrEE \x2192x' | title }}", L"One\x3000Two\x2192three \x2192x" },
        { L"{{ '\u01C6a'|capitalize }}|{{ '\u00DFa'|upper }}|{{ '\u0391\u03A3 \u0391\u03A3\u0391'|lower }}|{{ '\u0391\u03A3 b\u03A3'|title }}|{{ '\u0130'|lower|length }}|{{ '\u0391\u03A3.'|capitalize }}|{{ '\uFB01x \u01C6'|title }}", L"\u01C5a|SSA|\u03B1\u03C2 \u03B1\u03C3\u03B1|\u0391\u03C3 B\u03C3|2|\u0391\u03C2.|FIx \u01C4" },
        { L"{{ '\u00E9a'|upper }}|{{ '\u00C9A'|title }}|{{ '\u00C9A'|lower }}|{{ '\u00E9a'|capitalize }}", L"\u00C9A|\u00C9a|\u00E9a|\u00C9a" },
        { L"[{{ '&hearts; &frac12; &amp x &ampx &notit; &#x1F600; &#0; &#128; &Eacute &copyright'|striptags }}]", L"[\u2665 \u00BD & x &x \u00ACit; \U0001F600 \uFFFD \u20AC \u00C9 \u00A9right]" },
        { L"[{{ '&#1; &#x7f; &#xFFFE; &#xD800; &#65 &AMP &NotEqualTilde; &acE; &amp\u00E9'|striptags }}]", L"[   \uFFFD A & \u2242\u0338 \u223E\u0333 &\u00E9]" },
    };
    for (const auto& [source, expected] : cases)
    {
        Template tpl;
        ASSERT_TRUE(tpl.Load(source).has_value()) << source;
        EXPECT_EQ(expected, tpl.RenderAsString({}).value()) << source;
    }
    for (const auto& [source, expected] : wideCases)
    {
        TemplateW tpl;
        ASSERT_TRUE(tpl.Load(source).has_value());
        EXPECT_EQ(expected, tpl.RenderAsString({}).value());
    }
}

namespace
{
// The output, or the error's text from the exception on
std::string RenderOrError(const std::string& source, const ValuesMap& params)
{
    Template tpl;
    auto loaded = tpl.Load(source);
    if (!loaded)
    {
        return "load error: " + ErrorToString(loaded.error());
    }
    auto result = tpl.RenderAsString(params);
    if (result)
    {
        return *result;
    }
    // The engine's runtime error (Python's TypeError, ValueError): written as before it had a code of its own
    if (result.error().GetCode() == ErrorCode::TemplateRuntimeError)
    {
        return "Exception: " + result.error().GetExtraParams().at(0).AsString() + "\n";
    }
    auto text = ErrorToString(result.error());
    auto pos = text.find("Exception: ");
    // Without the source line the error is located at
    return pos == std::string::npos ? text : text.substr(pos, text.find('\n', pos) + 1 - pos);
}
} // namespace

// A literal format is parsed at Load (task 0128); it must give what a format read from a
// variable gives, error messages included
TEST(PercentFormatTest, LiteralFormatMatchesVariableFormat)
{
    const char* formats[] = { "%s:%d", "a%%b%s", "%%", "x%%", "%%%s", "", "plain", "%(a)s %(b)05.1f %%", "%(a)*d", "%*d|%-*.*f",
                              "%.*s", "50%", "%(a", "%(a)", "%5", "%-5.", "%y", "%5%", "%-%", "%s %s", "%s", "%(a(b))s", "%ld|%c", "\xE2\x86\x92%s\xE2\x86\x92" };
    // The right operand of %, and the same values as format() arguments
    const std::pair<const char*, const char*> args[] = {
        { "('x', 1)", "'x', 1" },
        { "(5, 3, 8, 2, 3.14159)", "5, 3, 8, 2, 3.14159" },
        { "()", "" },
        { "1", "1" },
        { "{'a': 1, 'b': 2}", "a=1, b=2" },
        { "('x',)", "'x'" },
    };
    for (const char* format : formats)
    {
        for (const auto& [operand, callArgs] : args)
        {
            ValuesMap params{ { "f", std::string(format) } };
            std::string literal = std::string("'") + format + "'";
            EXPECT_EQ(RenderOrError("{{ f % " + std::string(operand) + " }}", params), RenderOrError("{{ " + literal + " % " + operand + " }}", params))
                << format << " % " << operand;
            EXPECT_EQ(RenderOrError("{{ f | format(" + std::string(callArgs) + ") }}", params), RenderOrError("{{ " + literal + " | format(" + callArgs + ") }}", params))
                << format << " | format(" << callArgs << ")";
        }
    }
}

TEST(PercentFormatTest, LiteralFormatErrors)
{
    EXPECT_EQ("Exception: unsupported format character 'y' (0x79) at index 4\n", RenderOrError("{{ 'ab %y' % 1 }}", {}));
    EXPECT_EQ("Exception: unsupported format character 'y' (0x79) at index 4\n", RenderOrError("{{ 'ab %y' | format(1) }}", {}));
    EXPECT_EQ("Exception: not enough arguments for format string\n", RenderOrError("{{ '%s %s' % ('a',) }}", {}));
    EXPECT_EQ("Exception: format requires a mapping\n", RenderOrError("{{ '%(a' % 1 }}", {}));
    EXPECT_EQ("Exception: incomplete format key\n", RenderOrError("{{ '%(a' % {'a': 1} }}", {}));
    EXPECT_EQ("Exception: incomplete format\n", RenderOrError("{{ '%5' % 1 }}", {}));
    EXPECT_EQ("Exception: not enough arguments for format string\n", RenderOrError("{{ '%*' % () }}", {}));
    // A NUL after '%' is a conversion character, not the end of the directive
    const std::string nulFormat("%\0", 2);
    EXPECT_EQ("Exception: not enough arguments for format string\n", RenderOrError("{{ '" + nulFormat + "' % () }}", {}));
    EXPECT_EQ("Exception: not enough arguments for format string\n", RenderOrError("{{ f % () }}", { { "f", nulFormat } }));
    EXPECT_EQ(RenderOrError("{{ f % 1 }}", { { "f", nulFormat } }), RenderOrError("{{ '" + nulFormat + "' % 1 }}", {}));
    EXPECT_EQ(RenderOrError("{{ f | format(1) }}", { { "f", nulFormat } }), RenderOrError("{{ '" + nulFormat + "' | format(1) }}", {}));
    EXPECT_NE(std::string::npos, RenderOrError("{{ f % 1 }}", { { "f", nulFormat } }).find("unsupported format character"));
    EXPECT_EQ("x-1|y-2", RenderOrError("{% for v in [1, 2] %}{{ '%s-%d' % (('x', 'y')[loop.index0], v) }}{{ '|' if not loop.last }}{% endfor %}", {}));
}

namespace
{
// The text of the error a template raises, without the position prefix
std::string RenderError(const std::string& source, const ValuesMap& params = {})
{
    auto text = RenderOrError(source, params);
    return text;
}

ValuesMap UsersParams()
{
    return ValuesMap{ { "users", ValuesList{ ValuesMap{ { "name", "a" } }, ValuesMap{ { "name", "b" }, { "addr", ValuesMap{ { "zip", "1" } } } } } } };
}
} // namespace

// Task 0177: filters that iterate, take len() of or call methods on their input refuse
// None, bools and numbers with Python's message; an undefined stays lenient
TEST(FilterErrors, NonIterableInputLikePython)
{
    EXPECT_EQ("Exception: 'NoneType' object is not iterable\n", RenderError("{{ none|first }}"));
    EXPECT_EQ("Exception: 'bool' object is not iterable\n", RenderError("{{ true|first }}"));
    EXPECT_EQ("Exception: 'int' object is not reversible\n", RenderError("{{ 5|last }}"));
    EXPECT_EQ("Exception: object of type 'int' has no len()\n", RenderError("{{ 5|length }}"));
    EXPECT_EQ("Exception: object of type 'int' has no len()\n", RenderError("{{ 5|count }}"));
    EXPECT_EQ("Exception: object of type 'NoneType' has no len()\n", RenderError("{{ none|random }}"));
    EXPECT_EQ("Exception: argument must be iterable\n", RenderError("{{ 5|reverse }}"));
    EXPECT_EQ("Exception: 'int' object is not iterable\n", RenderError("{{ 5|sum }}"));
    EXPECT_EQ("Exception: 'int' object is not iterable\n", RenderError("{{ 5|min }}"));
    EXPECT_EQ("Exception: 'float' object is not iterable\n", RenderError("{{ 1.5|max }}"));
    EXPECT_EQ("Exception: 'NoneType' object is not iterable\n", RenderError("{{ none|sort }}"));
    EXPECT_EQ("Exception: 'int' object is not iterable\n", RenderError("{{ 5|list }}"));
    EXPECT_EQ("Exception: 'int' object is not iterable\n", RenderError("{{ 5|groupby('a') }}"));
    EXPECT_EQ("Exception: 'int' object is not iterable\n", RenderError("{{ 5|map(attribute='a')|list }}"));
    EXPECT_EQ("Exception: 'NoneType' object has no attribute 'items'\n", RenderError("{{ none|dictsort }}"));
    EXPECT_EQ("Exception: 'int' object has no attribute 'items'\n", RenderError("{{ 5|dictsort }}"));
    EXPECT_EQ("Exception: 'list' object has no attribute 'items'\n", RenderError("{{ [1]|dictsort }}"));
    EXPECT_EQ("Exception: 'str' object has no attribute 'items'\n", RenderError("{{ 'ab'|dictsort }}"));
    EXPECT_EQ("Exception: unsupported operand type(s) for +=: 'NoneType' and 'str'\n", RenderError("{{ none|indent }}"));
    EXPECT_EQ("Exception: unsupported operand type(s) for +=: 'int' and 'str'\n", RenderError("{{ 5|indent }}"));
    EXPECT_EQ("Exception: 'list' object has no attribute 'splitlines'\n", RenderError("{{ [1]|indent }}"));
    EXPECT_EQ("|0|[]", RenderError("{{ nope|first }}|{{ nope|length }}|{{ nope|list }}"));
    // map, select and reject start with `if value:`: a falsy input is an empty result
    EXPECT_EQ("[]|[]|[]|[]|[]|[]", RenderError("{{ none|map(attribute='a')|list }}|{{ false|map(attribute='a')|list }}|{{ 0|map('upper')|list }}|"
                                               "{{ 0.0|select('odd')|list }}|{{ none|reject('odd')|list }}|{{ none|selectattr('a')|list }}"));
    EXPECT_EQ("Exception: 'bool' object is not iterable\n", RenderError("{{ true|map(attribute='a')|list }}"));
    EXPECT_EQ("Exception: 'int' object is not iterable\n", RenderError("{{ 5|select('odd')|list }}"));
}

// Task 0048: filters raise where Python does (item 10 and the format and round leftovers)
TEST(FilterErrors, TypeErrorsLikePython)
{
    EXPECT_EQ("Exception: Object of type Undefined is not JSON serializable\n", RenderError("{{ nope|tojson }}"));
    EXPECT_EQ("Exception: Object of type Undefined is not JSON serializable\n", RenderError("{{ [nope]|tojson }}"));
    EXPECT_EQ("Exception: Object of type Undefined is not JSON serializable\n", RenderError("{{ {'a': nope}|tojson }}"));
    EXPECT_EQ("Exception: can't multiply sequence by non-int of type 'float'\n", RenderError("{{ [1]|tojson(1.5) }}"));
    EXPECT_EQ("[\n 1\n]", RenderError("{{ [1]|tojson(true) }}"));
    EXPECT_EQ("[\n  1\n]", RenderError("{{ [1]|tojson(2) }}"));
    EXPECT_EQ("[1]", RenderError("{{ [1]|tojson(none) }}"));
    EXPECT_EQ("Exception: 'NoneType' object is not iterable\n", RenderError("{{ none|batch(2)|list }}"));
    EXPECT_EQ("Exception: 'int' object is not iterable\n", RenderError("{{ 5|batch(2)|list }}"));
    EXPECT_EQ("Exception: 'NoneType' object is not iterable\n", RenderError("{{ none|slice(3)|list }}"));
    EXPECT_EQ("Exception: 'int' object is not iterable\n", RenderError("{{ 1|slice(3)|list }}"));
    EXPECT_EQ("[[], [], []]", RenderError("{{ nope|slice(3)|list }}"));
    EXPECT_EQ("Exception: not all arguments converted during string formatting\n", RenderError("{{ 'x'|format(1) }}"));
    EXPECT_EQ("Exception: not all arguments converted during string formatting\n", RenderError("{{ '{}'|format(1) }}"));
    EXPECT_EQ("Exception: cannot convert float NaN to integer\n", RenderError("{{ '%d'|format(1e999 - 1e999) }}"));
    EXPECT_EQ("Exception: cannot convert float infinity to integer\n", RenderError("{{ '%d'|format(1e999) }}"));
    EXPECT_EQ("Exception: %x format: an integer is required, not float\n", RenderError("{{ '%x'|format(1e30) }}"));
}

TEST(FilterErrors, DottedAttributePathWithMissingParent)
{
    for (const char* source : { "{{ users|map(attribute='addr.zip')|list }}", "{{ users|sort(attribute='addr.zip')|list }}", "{{ users|sum(attribute='addr.zip') }}",
                                "{{ users|groupby('addr.zip')|list }}", "{{ users|unique(attribute='addr.zip')|list }}", "{{ users|selectattr('addr.zip')|list }}" })
    {
        EXPECT_NE(std::string::npos, RenderError(source, UsersParams()).find("'dict object' has no attribute 'addr'")) << source;
    }
    EXPECT_EQ("['d', '1']", RenderError("{{ users|map(attribute='addr.zip', default='d')|list }}", UsersParams()));
    EXPECT_EQ("['a', 'b']", RenderError("{{ users|map(attribute='name')|list }}", UsersParams()));
    EXPECT_EQ("2", RenderError("{{ users|map(attribute='nope')|list|length }}", UsersParams()));
}

TEST(FilterErrors, RoundBeyondInt64)
{
    // Beyond int64 the result is a big integer, rounded in decimal (task 0270)
    EXPECT_EQ("9223372036854775810|10000000000000000000|-9223372036854775810",
              RenderOrError("{{ 9223372036854775807|round(-1) }}|{{ 9000000000000000000|round(-19) }}|{{ -9223372036854775807|round(-1) }}", {}));
    EXPECT_EQ("Exception: rounded value too large to represent\n", RenderError("{{ 1.797e308|round(-308) }}"));
    EXPECT_EQ("Exception: int too large to convert to float\n", RenderError("{{ 2.5|round(309, 'floor') }}"));
    EXPECT_EQ("Exception: cannot convert float infinity to integer\n", RenderError("{{ 1e308|round(2, 'floor') }}"));
    EXPECT_EQ("Exception: float division by zero\n", RenderError("{{ 5|round(-400, 'floor') }}"));
}

// Non-ASCII text through the narrow engine; the wide engine is covered by the parity case
TEST(FilterUnicode, IntAndFloatReadUnicodeDigitsAndSpaces)
{
    EXPECT_EQ("12|12.0|10|12.5|1000", RenderOrError("{{ '\331\241\331\242'|int }}|{{ '\357\274\221\357\274\222'|float }}|{{ ' 1_0\343\200\200'|int }}|{{ ' 12.5 '|float }}|{{ '1_000'|int }}", {}));
    EXPECT_EQ("42|5|-1|18|100.0|12.5|12", RenderOrError("{{ '\331\244\331\242'|int }}|{{ '\343\200\200\357\274\225\343\200\200'|int }}|{{ '-\357\274\221'|int }}|{{ '\357\274\221\357\274\222'|int(base=16) }}|{{ '\331\241e\331\242'|float }}|{{ '\331\241\331\242.\331\245'|float }}|{{ '\360\235\237\217\360\235\237\220'|int }}", {}));
    EXPECT_EQ("0|7|0|1.0 kB", RenderOrError("{{ '\303\251'|int }}|{{ '1\303\251'|float(7) }}|{{ '1 2'|int }}|{{ '\331\241\331\240\331\240\331\240'|filesizeformat }}", {}));
}

// Found by the 0048 verifier: Python checks tojson's indent only for non-strings, round(x, None)
// is an int, and int()/float() strip only ASCII whitespace that Py_ISSPACE knows
TEST(FilterErrors, VerifierLeftovers)
{
    EXPECT_EQ("\"a\"|\"b\"", RenderOrError("{{ 'a'|tojson(1.5) }}|{{ s|tojson(1.5) }}", { { "s", "b" } }));
    EXPECT_NE(RenderOrError("{{ 1|tojson(1.5) }}", {}).find("can't multiply sequence"), std::string::npos);
    EXPECT_EQ("2|2|5|-4|2.0", RenderOrError("{{ 1.5|round(None) }}|{{ 2.5|round(None) }}|{{ 5|round(None) }}|{{ -3.5|round(None) }}|{{ 1.5|round }}", {}));
    EXPECT_NE(RenderOrError("{{ 1.5|round(None, 'floor') }}", {}).find("NoneType"), std::string::npos);
    // An integer beyond int64 is a big integer (task 0270)
    EXPECT_EQ("1000000000000000052504760255204420248704468581108159154915854115511802457988908195786371375080447864043704443832883878176942523235360430575644792184786706982848387200926575803737830233794788090059368953234970799945081119038967640880074652742780142494579258788820056842838115669472196386865459400540160", RenderOrError("{{ 1e300|round(None) }}", {}));
    // \x1c-\x1f as variables: an octal escape before a digit is MSVC C4125
    const std::string fileSep = std::string(1, '\x1c') + "5";
    const std::string unitSep = std::string(1, '\x1f') + "5";
    EXPECT_EQ("0|0.0|5|5.0", RenderOrError("{{ fs|int }}|{{ us|float }}|{{ '\v5\t'|int }}|{{ '\f5\r'|float }}", { { "fs", fileSep }, { "us", unitSep } }));
}

// docs/tasks/0233: the C++-only filters on non-ASCII text, control characters and values
// YAML has no form for; toxml was removed and is an unknown filter, as in Jinja2
TEST(CppOnlyFilters, UnicodeControlsAndErrors)
{
    EXPECT_EQ("Stra\303\237eName|\303\244pfel_birnen|\303\204pfel", RenderOrError("{{ 'stra\303\237e_name'|camelize }}|{{ '\303\204pfelBirnen'|underscorize }}|{{ '\303\244pfel'|camelize }}", {}));
    // Joined pieces: a hex escape would swallow the b and the 1 that follow it
    const std::string controls = std::string("a\x01") + "b\x7f\x1b" + "1";
    EXPECT_EQ("a\\001b\\177\\0331|\303\251", RenderOrError("{{ s|escapecpp }}|{{ '\303\251'|escapecpp }}", { { "s", controls } }));
    EXPECT_EQ("- \346\227\245\346\234\254\n- \"\\x01\\e\\0\"\n- \"\\u2028\"\n- .nan\n- -.inf", RenderOrError("{{ ['\346\227\245\346\234\254', s, '\342\200\250', 'nan'|float, '-inf'|float]|toyaml }}", { { "s", std::string("\x01\x1b\0", 3) } }));
    EXPECT_NE(RenderOrError("{{ namespace(a=1)|toyaml }}", {}).find("not YAML serializable"), std::string::npos);
    EXPECT_NE(RenderOrError("{{ {'a': missing}|toyaml }}", {}).find("Object of type Undefined is not YAML serializable"), std::string::npos);
    // Malformed UTF-8: a long run of continuation bytes was an out-of-range shift (UBSan)
    const std::string tails = std::string("a") + std::string(40, '\x80');
    EXPECT_EQ(tails + "|\"a\"", RenderOrError("{{ s|toyaml }}|{{ s|tojson }}", { { "s", tails } }));
    // Lazy sequences are iterated once; a key over 255 bytes is an explicit "? key"
    EXPECT_EQ("- 0\n- 2|r:\n  - 0\n  - 1", RenderOrError("{{ range(0, 3, 2)|toyaml }}|{{ {'r': range(2)}|toyaml }}", {}));
    // A generator with no end stops at the size cap, in toyaml and tojson alike
    const std::string chunk(size_t{ 1 } << 20, 'x');
    auto endless = [&chunk] { return MakeGenericList([&chunk]() -> std::optional<Value> { return Value(chunk); }); };
    EXPECT_NE(RenderOrError("{{ g|toyaml }}", { { "g", endless() } }).find("a YAML document of length"), std::string::npos);
    EXPECT_NE(RenderOrError("{{ g|tojson }}", { { "g", endless() } }).find("a JSON document of length"), std::string::npos);
    const std::string longKey(300, 'k');
    EXPECT_EQ("? " + longKey + "\n: 1\n" + "short:\n  - ? " + longKey + "\n    :\n      - 2", RenderOrError("{{ {k: 1, 'short': [{k: [2]}]}|toyaml }}", { { "k", longKey } }));

    Template tpl;
    auto loaded = tpl.Load("{{ {'a': 1}|toxml }}");
    ASSERT_FALSE(loaded.has_value());
    EXPECT_EQ(ErrorCode::UnknownFilter, loaded.error().GetCode());
}

TEST(FilterPprint, ReadsSinglePassListsOnce)
{
    // pprint measures a value before it prints it: a host generator is read once, nested or not
    auto counting = [](int last) {
        return MakeGenericList([n = 0, last]() mutable -> std::optional<Value> { return n < last ? std::optional<Value>(Value(int64_t{ ++n })) : std::nullopt; });
    };
    EXPECT_EQ("[1, 2, 3]", RenderOrError("{{ g|pprint }}", { { "g", counting(3) } }));
    EXPECT_EQ("{'g': [1, 2, 3]}", RenderOrError("{{ {'g': g}|pprint }}", { { "g", counting(3) } }));
    std::string wrapped = "[1";
    for (int n = 2; n <= 30; ++n)
    {
        wrapped += ",\n " + std::to_string(n);
    }
    EXPECT_EQ(wrapped + "]", RenderOrError("{{ g|pprint }}", { { "g", counting(30) } }));
}

TEST(FilterPprint, CapsItsResult)
{
    // A wide key indents each line of its value by its width: the result stops at the cap
    EXPECT_NE(RenderOrError("{{ {('x' * 1000000): range(1000)|list}|pprint }}", {}).find("a pprint result of length"), std::string::npos);
    // A string of line breaks becomes a piece per line: the pieces stop at the cap too
    EXPECT_NE(RenderOrError("{{ ('\\n' * 67108864)|pprint }}", {}).find("a pprint result of length"), std::string::npos);
    // urlize checks each extra scheme as it reads it
    EXPECT_NE(RenderOrError("{{ 'x'|urlize(extra_schemes=range(1000000000)) }}", {}), "x");
}

TEST(FilterUnicode, PercentAEscapesNonAscii)
{
    EXPECT_EQ("'\\xe9'", RenderOrError("{{ '%a'|format('\303\251') }}", {}));
    EXPECT_EQ("['\\xe9', 'x']", RenderOrError("{{ '%a'|format(['\303\251', 'x']) }}", {}));
    EXPECT_EQ("'\\xe9'|'\\xdf'|'\\U0001f600'", RenderOrError("{{ '%5a|%-6a|%a'|format('\303\251', '\303\237', '\360\237\230\200') }}", {}));
    EXPECT_EQ("'\\u65e5\\u672c'|'\346\227\245\346\234\254'", RenderOrError("{{ '%a|%r'|format('\346\227\245\346\234\254', '\346\227\245\346\234\254') }}", {}));
}

// docs/tasks/0358: neutralize and fence, with the specials `<|im_start|>`, `<|im_end|>` and `[INST]` and the tag `document`
// registered on the environment. The expected text is written out by hand from docs/cpp-only-filters.md
namespace
{
PromptEscapeOptions PromptTestOptions()
{
    PromptEscapeOptions options;
    options.specialTokens = { "<|im_start|>", "<|im_end|>", "[INST]" };
    options.tags = { "document" };
    return options;
}

struct PromptRow
{
    std::string tpl;
    std::string result;
    bool autoescape = false;
    ValuesMap params = {};
    AutoescapeMode mode = AutoescapeMode::Html;

    friend std::ostream& operator<<(std::ostream& os, const PromptRow& row) { return os << row.tpl << " -> " << row.result; }
};

void ConfigurePromptEnv(TemplateEnv& env, bool autoescape, AutoescapeMode mode = AutoescapeMode::Html)
{
    env.GetSettings().autoescape = autoescape;
    env.GetSettings().autoescapeMode = mode;
    env.GetSettings().promptEscaper = PromptEscaper(PromptTestOptions());
}

// The output, or "Exception: " and the message of the error
std::string PromptRender(const std::string& source, const ValuesMap& params = {}, bool autoescape = false, const Settings::Limits& limits = {})
{
    TemplateEnv env;
    ConfigurePromptEnv(env, autoescape);
    env.GetSettings().limits = limits;
    Template tpl(&env);
    auto loaded = tpl.Load(source);
    if (!loaded)
    {
        return "load error: " + ErrorToString(loaded.error());
    }
    auto result = tpl.RenderAsString(params);
    if (result)
    {
        return *result;
    }
    if (result.error().GetCode() == ErrorCode::TemplateRuntimeError)
    {
        return "Exception: " + result.error().GetExtraParams().at(0).AsString();
    }
    return "error " + std::to_string(static_cast<int>(result.error().GetCode()));
}
} // namespace

struct PromptFilterTestTag;
using PromptFilterTest = ::testing::TestWithParam<PromptRow>;

TEST_P(PromptFilterTest, Narrow)
{
    const auto& row = GetParam();
    TemplateEnv env;
    ConfigurePromptEnv(env, row.autoescape, row.mode);
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load(row.tpl).has_value());
    auto result = tpl.RenderAsString(row.params);
    ASSERT_TRUE(result.has_value()) << ErrorToString(result.error());
    EXPECT_EQ(row.result, *result);
}

TEST_P(PromptFilterTest, Wide)
{
    const auto& row = GetParam();
    TemplateEnv env;
    ConfigurePromptEnv(env, row.autoescape, row.mode);
    TemplateW tpl(&env);
    ASSERT_TRUE(tpl.Load(ConvertString<std::wstring>(row.tpl)).has_value());
    auto result = tpl.RenderAsString(row.params);
    ASSERT_TRUE(result.has_value()) << ConvertString<std::string>(ErrorToString(result.error()));
    EXPECT_EQ(ConvertString<std::wstring>(row.result), *result);
}

INSTANTIATE_TEST_SUITE_P(
    Neutralize,
    PromptFilterTest,
    ::testing::Values(
        // The pattern is broken after its first code point, whatever the case of its ASCII letters
        PromptRow{ "{{ '<|im_start|>x'|neutralize }}", "< |im_start|>x" },
        PromptRow{ "{{ '<|IM_START|>'|neutralize }}", "< |IM_START|>" },
        PromptRow{ "{{ '[INST] [inst] [Inst]'|neutralize }}", "[ INST] [ inst] [ Inst]" },
        PromptRow{ "{{ 'plain text'|neutralize }}", "plain text" },
        PromptRow{ "{{ ''|neutralize }}", "" },
        // Overlapping matches: matching resumes one code point after a match start
        PromptRow{ "{{ '<|<|im_start|>'|neutralize }}", "<|< |im_start|>" },
        PromptRow{ "{{ '<|im_start|im_end|>'|neutralize }}", "<|im_start|im_end|>" },
        PromptRow{ "{{ '<|im_end|><|im_end|>'|neutralize }}", "< |im_end|>< |im_end|>" },
        // A tag is `<name` or `</name` and a boundary: `>`, `/`, a space, a tab, CR, LF or the end
        PromptRow{ "{{ '</document ><Document>'|neutralize }}", "< /document >< Document>" },
        PromptRow{ "{{ '<documentation>'|neutralize }}", "<documentation>" },
        PromptRow{ "{{ '<document'|neutralize }}|{{ '<DOCUMENT/>'|neutralize }}|{{ '<document\\t'|neutralize }}", "< document|< DOCUMENT/>|< document\t" },
        PromptRow{ "{{ 'document>'|neutralize }}", "document>" },
        // The edges: two neutralized values side by side cannot form a special
        PromptRow{ "{{ 'a<|im_'|neutralize ~ 'start|>'|neutralize }}", "a<|im_  start|>" },
        PromptRow{ "{{ '<'|neutralize }}|{{ '>x'|neutralize }}|{{ 'x]'|neutralize }}|{{ ']x'|neutralize }}", "< | >x|x]| ]x" },
        PromptRow{ "{{ 'a</doc'|neutralize }}|{{ 'a<'|neutralize }}", "a</doc |a< " },
        PromptRow{ "{{ 'a['|neutralize ~ 'INST]'|neutralize }}", "a[  INST]" },
        // Separators go after whole code points: ü is two bytes, the smiley two UTF-16 units in a 16-bit wchar_t
        PromptRow{ "{{ '\xC3\xBC<|im_end|>'|neutralize }}", "\xC3\xBC< |im_end|>" },
        PromptRow{ "{{ '\xF0\x9F\x98\x80<|im_end|>'|neutralize }}", "\xF0\x9F\x98\x80< |im_end|>" },
        PromptRow{ "{{ '\xF0\x9F\x98\x80z'|neutralize(specials=['\xF0\x9F\x98\x80z']) }}", "\xF0\x9F\x98\x80 z" },
        PromptRow{ "{{ '\xC3\xBC' ~ 'x'|neutralize(specials=['\xC3\xBC' 'x']) }}", "\xC3\xBC x" },
        // Arguments replace the environment's options for the call
        PromptRow{ "{{ 'a[[x <|im_start|>'|neutralize(specials=['[[']) }}", "a[ [x <|im_start|>" },
        PromptRow{ "{{ '<document>'|neutralize(tags=[]) }}|{{ '<document>'|neutralize(tags=['doc', 'document']) }}", "<document>|< document>" },
        PromptRow{ "{{ '<|im_start|>'|neutralize(specials=[]) }}", "<|im_start|>" },
        PromptRow{ "{{ '<|im_end|>'|neutralize(separator='-') }}", "<-|im_end|>" },
        PromptRow{ "{{ '<|im_end|>'|neutralize(separator='\xE2\x81\xA0') }}", "<\xE2\x81\xA0|im_end|>" },
        PromptRow{ "{{ '<|im_end|>'|neutralize(none, none, none) }}", "< |im_end|>" },
        PromptRow{ "{% for s in ['ab', 'bc'] %}{{ 'abc'|neutralize(specials=[s]) }}|{% endfor %}", "a bc|ab c|" },
        PromptRow{ "{{ 'abc'|neutralize(['ab']) }}", "a bc" },
        // Any value is its str()
        PromptRow{ "{{ 123|neutralize }}|{{ missing|neutralize }}|{{ none|neutralize }}", "123||None" },
        // Markup in, Markup out; a plain string stays plain
        PromptRow{ "{{ (x|safe|neutralize) is escaped }}|{{ (x|neutralize) is escaped }}|{{ x|safe|neutralize }}", "True|False|< |im_start|>", false, { { "x", "<|im_start|>" } } },
        PromptRow{ "{{ x|safe|neutralize }}|{{ (x|safe|neutralize) is escaped }}|{{ (x|neutralize) is escaped }}", "< |im_start|>|True|False", true, { { "x", "<|im_start|>" } } },
        PromptRow{ "{{ x|neutralize }}", "&lt; |im_start|&gt;", true, { { "x", "<|im_start|>" } } },
        // fence: 3 backticks or one more than the longest run in the text
        PromptRow{ "{{ 'a```b'|fence }}", "````\na```b\n````" },
        PromptRow{ "{{ '````'|fence('py') }}", "`````py\n````\n`````" },
        PromptRow{ "{{ ''|fence }}", "```\n\n```" },
        PromptRow{ "{{ 'a`b``c`'|fence }}", "```\na`b``c`\n```" },
        PromptRow{ "{{ t|fence }}|{{ t|fence(lang='md') }}", "```\nabc\n```|```md\nabc\n```", false, { { "t", "abc\n" } } },
        PromptRow{ "{{ 'x'|fence('c++') }}|{{ 'x'|fence('C#') }}|{{ 'x'|fence('a-b_c.1') }}", "```c++\nx\n```|```C#\nx\n```|```a-b_c.1\nx\n```" },
        PromptRow{ "{{ 12|fence }}", "```\n12\n```" },
        // fence escapes first: neutralized with autoescape off, HTML-escaped with it on, Markup not at all
        PromptRow{ "{{ '<|im_end|>'|fence }}", "```\n< |im_end|>\n```" },
        PromptRow{ "{{ x|safe|fence }}|{{ (x|fence) is escaped }}", "```\n<|im_end|>\n```|False", false, { { "x", "<|im_end|>" } } },
        PromptRow{ "{{ '<b>'|fence }}|{{ (x|fence) is escaped }}", "```\n&lt;b&gt;\n```|True", true, { { "x", "y" } } },
        PromptRow{ "{{ '<|im_end|>'|fence }}", "```\n&lt;|im_end|&gt;\n```", true },
        PromptRow{ "{{ x|safe|fence }}", "```\n<b>\n```", true, { { "x", "<b>" } } },
        PromptRow{ "{{ x|safe|fence }}", "```\n<b>\n```", true, { { "x", "<b>" } } },
        PromptRow{ "{{ x|fence }}", "```\n&lt;a href=&#34;`\n```", true, { { "x", "<a href=\"`" } } }));

// docs/tasks/0358, PR 2: AutoescapeMode::Prompt. Autoescape is on and neutralizes; Markup means prompt-safe, and the
// template's string literals are Markup. x holds a special, y another, a and b the halves of one
namespace
{
PromptRow PromptModeRow(std::string tpl, std::string result, ValuesMap params = {}, bool autoescape = true)
{
    params.emplace("x", "<|im_start|>");
    params.emplace("y", "[INST]");
    params.emplace("a", "<|im_");
    params.emplace("b", "start|>");
    params.emplace("h", "<b>&");
    return PromptRow{ std::move(tpl), std::move(result), autoescape, std::move(params), AutoescapeMode::Prompt };
}
} // namespace

INSTANTIATE_TEST_SUITE_P(
    PromptMode,
    PromptFilterTest,
    ::testing::Values(
        // {{ }} neutralizes what is not Markup; no HTML entities
        PromptModeRow("{{ x }}|{{ y }}|{{ h }}", "< |im_start|>|[ INST]|<b>&"),
        PromptModeRow("{{ 5 }}|{{ none }}|{{ [x] }}|{{ missing }}", "5|None|['< |im_start|>']|"),
        // |safe and host Markup pass
        PromptModeRow("{{ x|safe }}|{{ m }}", "<|im_start|>|<|im_end|>", { { "m", Markup("<|im_end|>") } }),
        // Literals are prompt-safe: their specials stay, the value's are broken
        PromptModeRow("{{ '<|im_start|>' }}{{ '<|im_start|>' ~ 'user' }}", "<|im_start|><|im_start|>user"),
        PromptModeRow("{{ '<|im_start|>' ~ x ~ '<|im_end|>' }}", "<|im_start|>< |im_start|><|im_end|>"),
        PromptModeRow("{{ '<|im_start|>' + x + '<|im_end|>' }}", "<|im_start|>< |im_start|><|im_end|>"),
        PromptModeRow("{{ '<|im_start|>' ~ y ~ x }}", "<|im_start|>[ INST]< |im_start|>"),
        PromptModeRow("{% autoescape t %}{{ '<|im_start|>' ~ x }}{% endautoescape %}", "<|im_start|>< |im_start|>", { { "t", true } }),
        // format, %, join
        PromptModeRow("{{ '{}<|im_end|>'.format(x) }}", "< |im_start|><|im_end|>"),
        PromptModeRow("{{ '%s<|im_end|>'|format(x) }}|{{ '%s<|im_end|>' % x }}|{{ '%.3s' % x }}", "< |im_start|><|im_end|>|< |im_start|><|im_end|>|< |"),
        PromptModeRow("{{ '<|im_end|>'.join([x, y]) }}|{{ [x, '<|im_end|>']|join(', ') }}", "< |im_start|><|im_end|>[ INST]|< |im_start|>, <|im_end|>"),
        // Macro output and set blocks keep the template's specials and break the value's
        PromptModeRow("{% macro m(v) %}<|im_start|>{{ v }}<|im_end|>{% endmacro %}{{ m(x) }}|{{ m(x) ~ x }}", "<|im_start|>< |im_start|><|im_end|>|<|im_start|>< |im_start|><|im_end|>< |im_start|>"),
        PromptModeRow("{% set s %}<|im_start|>{{ x }}{% endset %}{{ s }}|{{ s ~ y }}", "<|im_start|>< |im_start|>|<|im_start|>< |im_start|>[ INST]"),
        PromptModeRow("{% set s = '<|im_start|>' ~ x %}{{ s }}", "<|im_start|>< |im_start|>"),
        // {% autoescape false %} writes the value as it is
        PromptModeRow("{% autoescape false %}{{ x }}{% endautoescape %}|{{ x }}", "<|im_start|>|< |im_start|>"),
        // escape, e and forceescape neutralize
        PromptModeRow("{{ x|e }}|{{ x|escape }}|{{ x|forceescape }}|{{ h|e }}", "< |im_start|>|< |im_start|>|< |im_start|>|<b>&"),
        PromptModeRow("{{ '<|im_end|>'|e }}|{{ '<|im_end|>'|forceescape }}|{{ x|e|e }}", "<|im_end|>|< |im_end|>|< |im_start|>"),
        // The HTML of urlize, xmlattr and tojson is neutralized
        PromptModeRow("{{ u|urlize }}", "see <a href=\"http://x.io\" rel=\"noopener\">http://x.io</a> &lt;|im_end|&gt; [ INST]", { { "u", "see http://x.io <|im_end|> [INST]" } }),
        PromptModeRow("{{ {'a': y}|xmlattr }}|{{ {'a': x}|xmlattr }}", " a=\"[ INST]\"| a=\"&lt;|im_start|&gt;\""),
        PromptModeRow("{{ y|tojson }}|{{ x|tojson }}|{{ [y]|tojson }}", "\"[ INST]\"|\"\\u003c|im_start|\\u003e\"|[\"[ INST]\"]"),
        PromptModeRow("{{ y|tojson }}", "\"[ INST]\"", {}, false),
        // replace, truncate and upper on a value
        PromptModeRow("{{ x|replace('im', 'IM') }}|{{ x|upper }}", "< |IM_start|>|< |IM_START|>"),
        // A value is replaced as a str and escaped after: deleting from neutralized text could re-form a special
        PromptModeRow("{{ c|replace('\\n', '') }}|{{ (c|replace('\\n', '<|im_end|>')) is escaped }}", "< |im_start|>|False", { { "c", "<|im_\nstart|>" } }),
        PromptModeRow("{{ '<|im_end|>x'|replace('x', y) }}", "<|im_end|>[ INST]"),
        PromptModeRow("{{ ('aaaa bbbb cccc dddd eeee ffff'|safe)|truncate(12, true, y) }}", "aaaa b[ INST]"),
        // Values side by side cannot form a special; joined as plain text first, they are escaped as one
        PromptModeRow("{{ a }}{{ b }}|{{ a ~ b }}|{{ a ~ '' ~ b }}|{{ a ~ '<' ~ b }}", "<|im_  start|>|< |im_start|>|<|im_  start|>|<|im_ < start|>"),
        // is escaped, neutralize and fence
        PromptModeRow("{{ x is escaped }}|{{ (x|neutralize) is escaped }}|{{ '<|im_end|>' is escaped }}|{{ (x|e) is escaped }}", "False|True|True|True"),
        PromptModeRow("{{ x|neutralize }}|{{ x|fence }}|{{ (x|fence) is escaped }}", "< |im_start|>|```\n< |im_start|>\n```|True"),
        PromptModeRow("{{ x|safe|neutralize }}", "< |im_start|>"),
        // Literals that are Markup still compare, index and look up as strings
        PromptModeRow("{{ y == '[INST]' }}|{{ {'k': x}['k'] }}|{% set d = {'role': 'user'} %}{{ d.role }}|{{ 'abc'[1] }}|{{ 'a' in 'cat' }}", "True|< |im_start|>|user|b|True"),
        PromptModeRow("{% for m in ms %}{% if m.role == 'user' %}<|im_start|>{{ m.role }}\n{{ m.content }}<|im_end|>{% endif %}{% endfor %}",
                      "<|im_start|>user\n[ INST]< |im_end|><|im_end|>",
                      { { "ms", ValuesList{ ValuesMap{ { "role", "user" }, { "content", "[INST]<|im_end|>" } }, ValuesMap{ { "role", "system" }, { "content", "x" } } } } }),
        // Autoescape off: {{ }} is raw, literals are still Markup, neutralize gives a plain string
        PromptModeRow("{{ x }}|{{ (x|neutralize) is escaped }}|{{ x|neutralize }}|{{ '<|im_end|>' is escaped }}", "<|im_start|>|False|< |im_start|>|True", {}, false)));

namespace
{
// The inputs of the fixed-seed sample of fuzz_prompt_escape.cpp's generator
std::vector<std::vector<std::uint8_t>> PromptEscapeSample(int count)
{
    std::mt19937 random(358); // NOLINT(bugprone-random-generator-seed): a fixed sample, the same in every run
    std::uniform_int_distribution<int> byte(0, 255);
    std::uniform_int_distribution<std::size_t> length(8, 160);
    std::vector<std::vector<std::uint8_t>> inputs;
    for (int idx = 0; idx < count; ++idx)
    {
        auto& input = inputs.emplace_back(length(random));
        for (auto& unit : input)
        {
            unit = static_cast<std::uint8_t>(byte(random));
        }
    }
    return inputs;
}
} // namespace

TEST(PromptEscapeProperty, NoSpecialOfTheDataReachesTheOutput)
{
    // Random inputs choose templates of escaping constructs and data with private-use specials, rendered narrow and
    // wide in prompt mode
    int rendered = 0;
    constexpr int cases = 400;
    for (const auto& input : PromptEscapeSample(cases))
    {
        const auto escapeCase = jinja2_fuzz::MakePromptEscapeCase(input.data(), input.size());
        const auto narrow = jinja2_fuzz::RenderPromptEscapeCase<char>(escapeCase);
        const auto wide = jinja2_fuzz::RenderPromptEscapeCase<wchar_t>(escapeCase);
        ASSERT_NE(jinja2_fuzz::PromptEscapeResult::Leaked, narrow) << escapeCase.source;
        ASSERT_NE(jinja2_fuzz::PromptEscapeResult::Leaked, wide) << escapeCase.source;
        EXPECT_EQ(narrow, wide) << escapeCase.source;
        rendered += narrow == jinja2_fuzz::PromptEscapeResult::Clean ? 1 : 0;
    }
    // Most generated templates render, so the property is checked on real output
    EXPECT_GT(rendered, cases / 2);
}

TEST(PromptEscapeProperty, TheGeneratorCanLeakWithoutTheEscaper)
{
    // The same sample in HTML mode lets specials through: the generated data does reach the output
    int leaked = 0;
    for (const auto& input : PromptEscapeSample(400))
    {
        const auto escapeCase = jinja2_fuzz::MakePromptEscapeCase(input.data(), input.size());
        TemplateEnv env;
        env.GetSettings().autoescape = true;
        Template tpl(&env);
        if (!tpl.Load(escapeCase.source))
        {
            continue;
        }
        const auto result = tpl.RenderAsString(escapeCase.params);
        if (!result)
        {
            continue;
        }
        for (const auto& special : escapeCase.specials)
        {
            if (result->find(special) != std::string::npos)
            {
                ++leaked;
                break;
            }
        }
    }
    EXPECT_GT(leaked, 20);
}

TEST(PromptFilter, ArgumentErrors)
{
    // The separator must occur in no pattern: `|` is in the specials, `/` in the closing tag, `D` in `<document`
    for (const auto* separator : { "'|'", "''", "'/'", "'D'", "'<|im_start|>'", "1", "none|string" })
    {
        // "None" shares N and o with [INST] and <document (ASCII case ignored)
        const auto text = PromptRender(std::string("{{ 'x'|neutralize(separator=") + separator + ") }}");
        EXPECT_NE(std::string::npos, text.find("Exception: neutralize(): ")) << separator << " gave " << text;
    }
    EXPECT_NE(std::string::npos, PromptRender("{{ 'x'|neutralize(specials='ab') }}").find("specials must be a list of strings"));
    EXPECT_NE(std::string::npos, PromptRender("{{ 'x'|neutralize(tags=[1]) }}").find("tags must be a list of strings"));
    // lang is [A-Za-z0-9_+.#-]*
    for (const auto* lang : { "'a`b'", "'a b'", "'a\\nb'", "'a\\\"b'", "'\xC3\xBC'", "1", "['x']" })
    {
        const auto text = PromptRender(std::string("{{ 'x'|fence(") + lang + ") }}");
        EXPECT_NE(std::string::npos, text.find("Exception: fence(): ")) << lang << " gave " << text;
    }
    EXPECT_NE(std::string::npos, PromptRender("{{ 'x'|fence(lang='a`b') }}").find("Exception: fence(): "));
    EXPECT_EQ("Exception: fence(): lang may hold only letters, digits and _+.#-", PromptRender("{{ x|fence(lang) }}", { { "x", "t" }, { "lang", "a\nb" } }));
    // Too many or unknown arguments are reported as for any filter
    EXPECT_NE(std::string::npos, PromptRender("{{ 'x'|neutralize(1, 2, 3, 4) }}").find("Exception"));
    EXPECT_NE(std::string::npos, PromptRender("{{ 'x'|fence(1, 2) }}").find("Exception"));
    EXPECT_NE(std::string::npos, PromptRender("{{ 'x'|fence(nope='c') }}").find("Exception"));
}

TEST(PromptFilter, WithoutPatternsNothingChanges)
{
    // The default environment has no specials: the filters do not break anything, and fence only wraps
    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ '<|im_start|>[INST]</document>'|neutralize }}|{{ '<|im_start|>'|fence }}").has_value());
    auto result = tpl.RenderAsString({});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ("<|im_start|>[INST]</document>|```\n<|im_start|>\n```", *result);
}

TEST(PromptFilter, ArgumentsOfSeveralRendersOnOneTemplate)
{
    // The escaper built from the arguments is cached in the filter; a different argument builds another
    TemplateEnv env;
    ConfigurePromptEnv(env, false);
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ 'abc'|neutralize(specials=s) }}").has_value());
    for (const char* special : { "ab", "bc", "ab", "abc", "ab" })
    {
        auto result = tpl.RenderAsString({ { "s", ValuesList{ special } } });
        ASSERT_TRUE(result.has_value()) << result.error().ToString();
        const std::string expected = std::string(special) == "ab" ? "a bc" : std::string(special) == "bc" ? "ab c"
                                                                                                          : "a bc";
        EXPECT_EQ(expected, *result) << special;
    }
}

TEST(PromptFilter, EnvironmentOptionsAreTheDefaults)
{
    TemplateEnv env;
    PromptEscapeOptions options;
    options.specialTokens = { "<<" };
    options.separator = "~";
    env.GetSettings().promptEscaper = PromptEscaper(options);
    Template tpl(&env);
    // Only the specials are replaced: the separator of the environment is kept
    auto loaded = tpl.Load("{{ 'a<<b'|neutralize }}|{{ 'a<<b'|neutralize(specials=['<<b']) }}|{{ 'a<<b'|neutralize(separator='.') }}");
    ASSERT_TRUE(loaded.has_value()) << loaded.error().ToString();
    auto result = tpl.RenderAsString({});
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    EXPECT_EQ("a<~<b|a<~<b|a<.<b", *result);
}

TEST(PromptFilter, SizeLimits)
{
    // A text of `<` with the special `<<` grows to 2n + 1 characters: a separator after each of the first n - 1, one for
    // the start edge and one for the end edge (the first and the last `<`)
    constexpr std::size_t size = std::size_t{ 1 } << 20;
    PromptEscapeOptions options;
    options.specialTokens = { "<<" };
    const std::string text(size, '<');
    for (const bool wide : { false, true })
    {
        TemplateEnv env;
        env.GetSettings().promptEscaper = PromptEscaper(options);
        const std::string source = "{{ x|neutralize|length }}";
        auto render = [&](std::uint64_t limit) -> std::string {
            env.GetSettings().limits.maxSequenceSize = limit;
            if (wide)
            {
                TemplateW tpl(&env);
                if (!tpl.Load(ConvertString<std::wstring>(source)))
                {
                    return "load error";
                }
                auto result = tpl.RenderAsString({ { "x", text } });
                return result ? ConvertString<std::string>(*result) : "error " + std::to_string(static_cast<int>(result.error().GetCode()));
            }
            Template tpl(&env);
            if (!tpl.Load(source))
            {
                return "load error";
            }
            auto result = tpl.RenderAsString({ { "x", text } });
            return result ? *result : "error " + std::to_string(static_cast<int>(result.error().GetCode()));
        };
        const auto expectedLength = std::to_string((2 * size) + 1);
        EXPECT_EQ(expectedLength, render((2 * size) + 1)) << wide;
        EXPECT_EQ("error " + std::to_string(static_cast<int>(ErrorCode::ResourceLimitExceeded)), render(2 * size)) << wide;
        EXPECT_EQ("error " + std::to_string(static_cast<int>(ErrorCode::ResourceLimitExceeded)), render(size + (size / 2))) << wide;
        // The built-in cap of 2^26 holds a longer text; no limit set
        EXPECT_EQ(expectedLength, render(0)) << wide;
    }
    // A pattern list from a host generator obeys the cap while it is read, before the options are checked
    {
        TemplateEnv env;
        env.GetSettings().limits.maxSequenceSize = 100;
        Template tpl(&env);
        ASSERT_TRUE(tpl.Load("{{ 'x'|neutralize(specials=g) }}").has_value());
        auto endless = MakeGenericList([n = int64_t{ 0 }]() mutable -> std::optional<Value> { return Value("t" + std::to_string(n++)); });
        auto result = tpl.RenderAsString({ { "g", endless } });
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
    }
    // fence has its own check
    TemplateEnv env;
    env.GetSettings().limits.maxSequenceSize = size;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ x|fence }}").has_value());
    auto result = tpl.RenderAsString({ { "x", text } });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
}

TEST(PromptEscaper, IsValid)
{
    PromptEscapeOptions options = PromptTestOptions();
    EXPECT_TRUE(PromptEscaper::IsValid(PromptEscapeOptions{}));
    EXPECT_TRUE(PromptEscaper::IsValid(options));
    options.separator = "\xE2\x81\xA0";
    EXPECT_TRUE(PromptEscaper::IsValid(options));
    PromptEscapeOptions tagsOnly;
    tagsOnly.tags = { "document" };
    tagsOnly.separator = ">";
    EXPECT_TRUE(PromptEscaper::IsValid(tagsOnly)) << "`>` is in no pattern: tags are `<name` and `</name`, the boundary is not part of them";
    // Empty, or in a pattern; the case of ASCII letters does not matter, as it does not for matching
    for (const char* separator : { "", "|", "<", "/", "_", "INST", "inst", "S", "s", "D", "ocu", "</d", "<|", "t|>" })
    {
        options.separator = separator;
        EXPECT_FALSE(PromptEscaper::IsValid(options)) << '"' << separator << '"';
    }
    options.separator = " ";
    options.specialTokens.emplace_back("[ X]");
    EXPECT_FALSE(PromptEscaper::IsValid(options));
    // Empty patterns are ignored and need nothing
    options = PromptEscapeOptions{};
    options.specialTokens = { "" };
    options.tags = { "" };
    EXPECT_TRUE(PromptEscaper::IsValid(options));
    EXPECT_FALSE(PromptEscaper::IsValid([] {
        PromptEscapeOptions bad;
        bad.separator.clear();
        return bad;
    }()));
}

TEST(PromptEscaper, SeparatorSharesNoCharacterWithAPattern)
{
    // "ab" is in no pattern as a whole, but its "b" would complete the special "by" after the insertion in "xyz"
    PromptEscapeOptions options;
    options.specialTokens = { "xyz", "by" };
    options.separator = "ab";
    EXPECT_FALSE(PromptEscaper::IsValid(options));
    options.separator = "Q";
    EXPECT_TRUE(PromptEscaper::IsValid(options));
    options.specialTokens.emplace_back(257, 'x');
    EXPECT_FALSE(PromptEscaper::IsValid(options)) << "a pattern longer than 256 bytes";
    options.specialTokens.back().resize(256);
    EXPECT_TRUE(PromptEscaper::IsValid(options));
    // Malformed UTF-8 would become U+FFFD in the wide patterns only: a separator \xFF there would equal the special's U+FFFD
    {
        PromptEscapeOptions bad;
        bad.specialTokens = { "\xEF\xBF\xBD"
                              "x" };
        bad.separator = "\xFF";
        EXPECT_FALSE(PromptEscaper::IsValid(bad));
        bad.separator = " ";
        EXPECT_TRUE(PromptEscaper::IsValid(bad));
        bad.specialTokens = { "a\xC0\xAF"
                              "b" };
        EXPECT_FALSE(PromptEscaper::IsValid(bad)) << "overlong form";
        bad.specialTokens = { "ab" };
        bad.tags = { "d\xED\xA0\x80" };
        EXPECT_FALSE(PromptEscaper::IsValid(bad)) << "surrogate";
    }
    // A special of one code point cannot be broken by a separator after its first code point
    for (const char* single : { "x", "\xEE\x80\x80" })
    {
        PromptEscapeOptions one;
        one.specialTokens = { single };
        EXPECT_FALSE(PromptEscaper::IsValid(one)) << single;
    }
}

TEST(PromptEscaper, SeparatorAfterATagNameCountsAsABoundary)
{
    // The special "ab" gets a space after its "a", which would make "<a" the tag <a followed by a boundary
    PromptEscapeOptions options;
    options.specialTokens = { "ab" };
    options.tags = { "a" };
    const PromptEscaper escaper(options);
    EXPECT_EQ("< a b", escaper.Neutralize(std::string_view("<ab")));
    EXPECT_EQ(L"< a b", escaper.Neutralize(std::wstring_view(L"<ab")));
    EXPECT_EQ("xa b", escaper.Neutralize(std::string_view("xab")));
}

TEST(PromptEscaper, ConstructorThrowsForInvalidOptions)
{
    PromptEscapeOptions options = PromptTestOptions();
    options.separator = "|";
    EXPECT_THROW(PromptEscaper{ options }, std::invalid_argument);
    options.separator.clear();
    EXPECT_THROW(PromptEscaper{ options }, std::invalid_argument);
    options.separator = " ";
    EXPECT_NO_THROW(PromptEscaper{ options });
}

TEST(PromptEscaper, EqualityIsByIdentityOfTheCompiledPatterns)
{
    const PromptEscaper none;
    const PromptEscaper none2;
    EXPECT_EQ(none, none2);
    const PromptEscaper first(PromptTestOptions());
    const PromptEscaper copy = first; // NOLINT(performance-unnecessary-copy-initialization): the copy is what the test compares
    const PromptEscaper second(PromptTestOptions());
    EXPECT_EQ(first, copy);
    EXPECT_NE(first, second);
    EXPECT_NE(first, none);
    EXPECT_TRUE(first != second);

    Settings lhs;
    Settings rhs;
    EXPECT_EQ(lhs, rhs);
    lhs.promptEscaper = first;
    EXPECT_NE(lhs, rhs);
    rhs.promptEscaper = copy;
    EXPECT_EQ(lhs, rhs);
    rhs.promptEscaper = second;
    EXPECT_NE(lhs, rhs);
}

TEST(PromptEscaper, OptionsAreKept)
{
    const PromptEscaper none;
    EXPECT_TRUE(none.Options().specialTokens.empty());
    EXPECT_TRUE(none.Options().tags.empty());
    EXPECT_EQ(" ", none.Options().separator);
    EXPECT_EQ("<|im_start|>x", none.Neutralize("<|im_start|>x"));
    EXPECT_EQ(L"<|im_start|>x", none.Neutralize(L"<|im_start|>x"));

    const PromptEscaper escaper(PromptTestOptions());
    EXPECT_EQ(PromptTestOptions().specialTokens, escaper.Options().specialTokens);
    EXPECT_EQ(PromptTestOptions().tags, escaper.Options().tags);
}

TEST(PromptEscaper, NeutralizeNarrowAndWide)
{
    const PromptEscaper escaper(PromptTestOptions());
    struct Case
    {
        std::wstring text;
        std::wstring expected;
    };
    const std::vector<Case> cases = {
        { L"", L"" },
        { L"hello", L"hello" },
        { L"<|im_start|>user", L"< |im_start|>user" },
        { L"<|IM_END|><|Im_End|>", L"< |IM_END|>< |Im_End|>" },
        { L"<|<|im_start|>", L"<|< |im_start|>" },
        { L"a<|im_", L"a<|im_ " },
        { L"start|>", L" start|>" },
        { L"<", L"< " },
        { L">", L" >" },
        { L"]", L" ]" },
        { L"[INST]", L"[ INST]" },
        { L"[INST", L"[INST " },
        { L"INST]", L" INST]" },
        { L"</DOCUMENT>", L"< /DOCUMENT>" },
        { L"<documents>", L"<documents>" },
        { L"</document", L"< /document" },
        { L"x<docum", L"x<docum " },
        // Non-ASCII text and a code point beyond the BMP before and inside the pattern
        { L"ü<|im_end|>", L"ü< |im_end|>" },
        { L"\U0001F600<|im_end|>\U0001F600", L"\U0001F600< |im_end|>\U0001F600" },
        { L"\U0001F600[INST]", L"\U0001F600[ INST]" },
    };
    for (const auto& c : cases)
    {
        EXPECT_EQ(c.expected, escaper.Neutralize(std::wstring_view(c.text))) << ConvertString<std::string>(c.text);
        EXPECT_EQ(ConvertString<std::string>(c.expected), escaper.Neutralize(std::string_view(ConvertString<std::string>(c.text)))) << ConvertString<std::string>(c.text);
    }

    // A special that starts with a code point beyond the BMP (a surrogate pair in a 16-bit wchar_t) and one with a
    // multi-byte first character: the separator follows the whole code point
    PromptEscapeOptions options;
    options.specialTokens = { "\xF0\x9F\x98\x80"
                              "ab",
                              "\xEF\xBC\x9C"
                              "|x|"
                              "\xEF\xBC\x9E" };
    const PromptEscaper wideFirst(options);
    EXPECT_EQ("\xF0\x9F\x98\x80 ab", wideFirst.Neutralize("\xF0\x9F\x98\x80"
                                                          "ab"));
    EXPECT_EQ(L"\U0001F600 ab", wideFirst.Neutralize(L"\U0001F600ab"));
    EXPECT_EQ("\xEF\xBC\x9C |x|\xEF\xBC\x9E", wideFirst.Neutralize("\xEF\xBC\x9C|x|\xEF\xBC\x9E"));
    EXPECT_EQ(L"＜ |x|＞", wideFirst.Neutralize(L"＜|x|＞"));
    // Edges that start or end inside a multi-unit pattern
    EXPECT_EQ(" ab\xEF\xBC\x9C ", wideFirst.Neutralize("ab\xEF\xBC\x9C"));
}

TEST(PromptEscaper, MalformedUtf8PassesThrough)
{
    const PromptEscaper escaper(PromptTestOptions());
    // Truncated sequences, lone continuation bytes and bytes that never occur: only the separators are added, the
    // bytes around them are not touched
    EXPECT_EQ("\xF0\x9F< |im_end|>\xC3", escaper.Neutralize("\xF0\x9F<|im_end|>\xC3"));
    EXPECT_EQ("\x80\x80\xFF\xFE", escaper.Neutralize("\x80\x80\xFF\xFE"));
    EXPECT_EQ("\xE2\x82[ INST]\xE2", escaper.Neutralize("\xE2\x82[INST]\xE2"));
    EXPECT_EQ("\xC3", escaper.Neutralize("\xC3"));

    // Bytes drawn from an alphabet of the pattern's characters and every non-ASCII value, no space: the output holds no
    // pattern, and without the separators it is the input
    const std::string alphabet = "<|>/[]_INSTimdoucenrtasp\x80\x81\xBF\xC3\xE2\xF0\xFF";
    const std::vector<std::string> patterns = { "<|im_start|>", "<|im_end|>", "[inst]", "<document", "</document" };
    std::uint32_t state = 12345;
    auto next = [&state] {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    };
    for (int round = 0; round != 3000; ++round)
    {
        std::string text;
        const auto length = next() % 48;
        for (std::uint32_t idx = 0; idx != length; ++idx)
        {
            text += alphabet[next() % alphabet.size()];
        }
        // Sometimes a whole special in the middle
        if (round % 3 == 0)
        {
            text.insert(next() % (text.size() + 1), patterns[next() % patterns.size()] + (round % 2 ? ">" : ""));
        }
        const auto out = escaper.Neutralize(text);
        std::string lowered;
        std::string stripped;
        for (const char ch : out)
        {
            lowered += static_cast<char>(ch >= 'A' && ch <= 'Z' ? ch - 'A' + 'a' : ch);
            if (ch != ' ')
            {
                stripped += ch;
            }
        }
        ASSERT_EQ(text, stripped) << round;
        for (const auto& pattern : patterns)
        {
            // `<document` and `</document` need no boundary to be found here: they are found when one follows
            const auto at = lowered.find(pattern);
            if (at == std::string::npos)
            {
                continue;
            }
            const bool isTag = pattern.find("document") != std::string::npos;
            const bool boundary = at + pattern.size() == lowered.size() || std::string(">/ \t\r\n").find(lowered[at + pattern.size()]) != std::string::npos;
            ASSERT_TRUE(isTag && !boundary) << "found " << pattern << " in " << lowered << " from " << text;
        }
    }
}
