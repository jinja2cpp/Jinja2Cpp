#include <iostream>
#include <string>

#include "test_tools.h"
#include "jinja2cpp/template.h"
#include "jinja2cpp/user_callable.h"

using namespace jinja2;

struct TestersGenericTestTag;
using TestersGenericTest = InputOutputPairTest<TestersGenericTestTag>;


TEST_P(TestersGenericTest, Test)
{
    const auto& testParam = GetParam();
    std::string source = "{{ 'true' if " + testParam.tpl + " else 'false' }}";

    PerformBothTests(source, testParam.result);
}

INSTANTIATE_TEST_SUITE_P(EqTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is eq(0)",                         "true"},
                            InputOutputPair{"0 is eq(1)",                         "false"},
                            InputOutputPair{"0.5 is eq(0.5)",                     "true"},
                            InputOutputPair{"0.5 is eq(1.5)",                     "false"},
                            InputOutputPair{"'0.5' is eq('0.5')",                 "true"},
                            InputOutputPair{"'0.5' is eq('1.5')",                 "false"},
                            InputOutputPair{"'Hello World' is eq('hello world')", "false"},
                            InputOutputPair{"0 is equalto(1)",                    "false"},
                            InputOutputPair{"intList[0] is eq(intAsDoubleList[0])", "true"},
                            InputOutputPair{"intAsDoubleList[0] is eq(intList[0])", "true"}
                            ));


INSTANTIATE_TEST_SUITE_P(NeTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is ne(0)",                         "false"},
                            InputOutputPair{"0 is ne(1)",                         "true"},
                            InputOutputPair{"0.5 is ne(0.5)",                     "false"},
                            InputOutputPair{"0.5 is ne(1.5)",                     "true"},
                            InputOutputPair{"'0.5' is ne('0.5')",                 "false"},
                            InputOutputPair{"'0.5' is ne('1.5')",                 "true"},
                            InputOutputPair{"'Hello World' is ne('hello world')", "true"},
                            InputOutputPair{"intList[0] is ne(intAsDoubleList[0])", "false"},
                            InputOutputPair{"intAsDoubleList[0] is ne(intList[0])", "false"}
                            ));


INSTANTIATE_TEST_SUITE_P(GeTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is ge(0)",                         "true"},
                            InputOutputPair{"0 is ge(1)",                         "false"},
                            InputOutputPair{"1 is ge(0)",                         "true"},
                            InputOutputPair{"0.5 is ge(0.5)",                     "true"},
                            InputOutputPair{"0.5 is ge(1.5)",                     "false"},
                            InputOutputPair{"1.5 is ge(0.5)",                     "true"},
                            InputOutputPair{"'0.5' is ge('0.5')",                 "true"},
                            InputOutputPair{"'0.5' is ge('1.5')",                 "false"},
                            InputOutputPair{"'1.5' is ge('0.5')",                 "true"},
                            InputOutputPair{"'Hello World' is ge('hello world')", "false"},
                            InputOutputPair{"'hello world' is ge('Hello World')", "true"},
                            InputOutputPair{"intList[0] is ge(intAsDoubleList[0])", "true"},
                            InputOutputPair{"intList[0] is ge(intAsDoubleList[1])", "true"},
                            InputOutputPair{"intList[1] is ge(intAsDoubleList[0])", "false"},
                            InputOutputPair{"intAsDoubleList[0] is ge(intList[0])", "true"},
                            InputOutputPair{"intAsDoubleList[0] is ge(intList[1])", "true"},
                            InputOutputPair{"intAsDoubleList[1] is ge(intList[0])", "false"}
                            ));


INSTANTIATE_TEST_SUITE_P(GtTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is gt(0)",                         "false"},
                            InputOutputPair{"0 is greaterthan(0)",                "false"},
                            InputOutputPair{"0 is gt(1)",                         "false"},
                            InputOutputPair{"1 is greaterthan(0)",                "true"},
                            InputOutputPair{"0.5 is gt(0.5)",                     "false"},
                            InputOutputPair{"0.5 is gt(1.5)",                     "false"},
                            InputOutputPair{"1.5 is gt(0.5)",                     "true"},
                            InputOutputPair{"'0.5' is gt('0.5')",                 "false"},
                            InputOutputPair{"'0.5' is gt('1.5')",                 "false"},
                            InputOutputPair{"'1.5' is gt('0.5')",                 "true"},
                            InputOutputPair{"'Hello World' is gt('hello world')", "false"},
                            InputOutputPair{"'hello world' is gt('Hello World')", "true"},
                            InputOutputPair{"intList[0] is gt(intAsDoubleList[0])", "false"},
                            InputOutputPair{"intList[0] is gt(intAsDoubleList[1])", "true"},
                            InputOutputPair{"intList[1] is gt(intAsDoubleList[0])", "false"},
                            InputOutputPair{"intAsDoubleList[0] is gt(intList[0])", "false"},
                            InputOutputPair{"intAsDoubleList[0] is gt(intList[1])", "true"},
                            InputOutputPair{"intAsDoubleList[1] is gt(intList[0])", "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(LeTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is le(0)",                         "true"},
                            InputOutputPair{"0 is le(1)",                         "true"},
                            InputOutputPair{"1 is le(0)",                         "false"},
                            InputOutputPair{"0.5 is le(0.5)",                     "true"},
                            InputOutputPair{"0.5 is le(1.5)",                     "true"},
                            InputOutputPair{"1.5 is le(0.5)",                     "false"},
                            InputOutputPair{"'0.5' is le('0.5')",                 "true"},
                            InputOutputPair{"'0.5' is le('1.5')",                 "true"},
                            InputOutputPair{"'1.5' is le('0.5')",                 "false"},
                            InputOutputPair{"'Hello World' is le('hello world')", "true"},
                            InputOutputPair{"'hello world' is le('Hello World')", "false"},
                            InputOutputPair{"intList[0] is le(intAsDoubleList[0])", "true"},
                            InputOutputPair{"intList[0] is le(intAsDoubleList[1])", "false"},
                            InputOutputPair{"intList[1] is le(intAsDoubleList[0])", "true"},
                            InputOutputPair{"intAsDoubleList[0] is le(intList[0])", "true"},
                            InputOutputPair{"intAsDoubleList[0] is le(intList[1])", "false"},
                            InputOutputPair{"intAsDoubleList[1] is le(intList[0])", "true"}
                            ));


INSTANTIATE_TEST_SUITE_P(LtTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is lt(0)",                         "false"},
                            InputOutputPair{"0 is lessthan(0)",                   "false"},
                            InputOutputPair{"0 is lessthan(1)",                   "true"},
                            InputOutputPair{"1 is lt(0)",                         "false"},
                            InputOutputPair{"0.5 is lt(0.5)",                     "false"},
                            InputOutputPair{"0.5 is lt(1.5)",                     "true"},
                            InputOutputPair{"1.5 is lt(0.5)",                     "false"},
                            InputOutputPair{"'0.5' is lt('0.5')",                 "false"},
                            InputOutputPair{"'0.5' is lt('1.5')",                 "true"},
                            InputOutputPair{"'1.5' is lt('0.5')",                 "false"},
                            InputOutputPair{"'Hello World' is lt('hello world')", "true"},
                            InputOutputPair{"'hello world' is lt('Hello World')", "false"},
                            InputOutputPair{"intList[0] is lt(intAsDoubleList[0])", "false"},
                            InputOutputPair{"intList[0] is lt(intAsDoubleList[1])", "false"},
                            InputOutputPair{"intList[1] is lt(intAsDoubleList[0])", "true"},
                            InputOutputPair{"intAsDoubleList[0] is lt(intList[0])", "false"},
                            InputOutputPair{"intAsDoubleList[0] is lt(intList[1])", "false"},
                            InputOutputPair{"intAsDoubleList[1] is lt(intList[0])", "true"}
                            ));

INSTANTIATE_TEST_SUITE_P(DefinedTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"antList is defined",              "false"},
                            InputOutputPair{"intList is defined",              "true"}
                            ));

INSTANTIATE_TEST_SUITE_P(UndefinedTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"antList is undefined",              "true"},
                            InputOutputPair{"intList is undefined",              "false"}
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(IterableTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is iterable",              "false"},
                            InputOutputPair{"'intList' is iterable",      "true"},
                            InputOutputPair{"false is iterable",          "false"},
                            InputOutputPair{"0.2 is iterable",            "false"},
                            InputOutputPair{"intValue is iterable",       "false"},
                            InputOutputPair{"stringValue is iterable",    "true"},
                            InputOutputPair{"doubleValue is iterable",    "false"},
                            InputOutputPair{"boolFalseValue is iterable", "false"},
                            InputOutputPair{"boolTrueValue is iterable",  "false"},
                            InputOutputPair{"[0, 1, 2] is iterable",      "true"},
                            InputOutputPair{"(0, 1, 2) is iterable",      "true"},
                            InputOutputPair{"{'name'='itemName', 'val'='itemValue'} is iterable",        "true"},
                            InputOutputPair{"mapValue is iterable",       "true"},
                            // InputOutputPair{"mapValue | first is iterable", "false"},
                            InputOutputPair{"intList is iterable",        "true"},
                            InputOutputPair{"reflectedVal is iterable",   "true"},
                            InputOutputPair{"reflectedList is iterable",  "true"}
                            ));
// clang-format on

INSTANTIATE_TEST_SUITE_P(MappingTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is mapping",              "false"},
                            InputOutputPair{"'intList' is mapping",      "false"},
                            InputOutputPair{"false is mapping",          "false"},
                            InputOutputPair{"0.2 is mapping",            "false"},
                            InputOutputPair{"intValue is mapping",       "false"},
                            InputOutputPair{"stringValue is mapping",    "false"},
                            InputOutputPair{"doubleValue is mapping",    "false"},
                            InputOutputPair{"boolFalseValue is mapping", "false"},
                            InputOutputPair{"boolTrueValue is mapping",  "false"},
                            InputOutputPair{"[0, 1, 2] is mapping",      "false"},
                            InputOutputPair{"(0, 1, 2) is mapping",      "false"},
                            InputOutputPair{"{'name'='itemName', 'val'='itemValue'} is mapping",        "true"},
                            InputOutputPair{"mapValue is mapping",       "true"},
                            // InputOutputPair{"mapValue | first is mapping", "true"}, ???
                            InputOutputPair{"intList is mapping",        "false"},
                            InputOutputPair{"reflectedVal is mapping",   "true"},
                            InputOutputPair{"reflectedList is mapping",  "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(NumberTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is number",              "true"},
                            InputOutputPair{"'intList' is number",      "false"},
                            InputOutputPair{"false is number",          "false"},
                            InputOutputPair{"0.2 is number",            "true"},
                            InputOutputPair{"intValue is number",       "true"},
                            InputOutputPair{"stringValue is number",    "false"},
                            InputOutputPair{"doubleValue is number",    "true"},
                            InputOutputPair{"boolFalseValue is number", "false"},
                            InputOutputPair{"boolTrueValue is number",  "false"},
                            InputOutputPair{"[0, 1, 2] is number",      "false"},
                            InputOutputPair{"(0, 1, 2) is number",      "false"},
                            InputOutputPair{"{'name'='itemName', 'val'='itemValue'} is number",        "false"},
                            InputOutputPair{"mapValue is number",       "false"},
                            // InputOutputPair{"mapValue | first is number", "true"}, ???
                            InputOutputPair{"intList is number",        "false"},
                            InputOutputPair{"reflectedVal is number",   "false"},
                            InputOutputPair{"reflectedList is number",  "false"}
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(SequenceTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is sequence",              "false"},
                            InputOutputPair{"'intList' is sequence",      "true"},
                            InputOutputPair{"false is sequence",          "false"},
                            InputOutputPair{"0.2 is sequence",            "false"},
                            InputOutputPair{"intValue is sequence",       "false"},
                            InputOutputPair{"stringValue is sequence",    "true"},
                            InputOutputPair{"doubleValue is sequence",    "false"},
                            InputOutputPair{"boolFalseValue is sequence", "false"},
                            InputOutputPair{"boolTrueValue is sequence",  "false"},
                            InputOutputPair{"[0, 1, 2] is sequence",      "true"},
                            InputOutputPair{"(0, 1, 2) is sequence",      "true"},
                            InputOutputPair{"{'name'='itemName', 'val'='itemValue'} is sequence",        "false"},
                            InputOutputPair{"mapValue is sequence",       "false"},
                            // InputOutputPair{"mapValue | first is sequence", "true"}, ???
                            InputOutputPair{"intList is sequence",        "true"},
                            InputOutputPair{"reflectedVal is sequence",   "false"},
                            InputOutputPair{"reflectedList is sequence",  "true"}
                            ));
// clang-format on


INSTANTIATE_TEST_SUITE_P(StringTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is string",              "false"},
                            InputOutputPair{"'intList' is string",      "true"},
                            InputOutputPair{"false is string",          "false"},
                            InputOutputPair{"0.2 is string",            "false"},
                            InputOutputPair{"intValue is string",       "false"},
                            InputOutputPair{"stringValue is string",    "true"},
                            InputOutputPair{"doubleValue is string",    "false"},
                            InputOutputPair{"boolFalseValue is string", "false"},
                            InputOutputPair{"boolTrueValue is string",  "false"},
                            InputOutputPair{"[0, 1, 2] is string",      "false"},
                            InputOutputPair{"(0, 1, 2) is string",      "false"},
                            InputOutputPair{"{'name'='itemName', 'val'='itemValue'} is string",        "false"},
                            InputOutputPair{"mapValue is string",       "false"},
                            // InputOutputPair{"mapValue | first is string", "true"}, ???
                            InputOutputPair{"intList is string",        "false"},
                            InputOutputPair{"reflectedVal is string",   "false"},
                            InputOutputPair{"reflectedList is string",  "false"}
                            ));


INSTANTIATE_TEST_SUITE_P(InTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 in (2, 1, 0)",             "true"},
                            InputOutputPair{"0 in (1, 2, 3)",             "false"},
                            InputOutputPair{"0 in intList",               "true"},
                            InputOutputPair{"1000 in intList",            "false"},
                            InputOutputPair{"'string9' in stringList",    "true"},
                            InputOutputPair{"'string90' in stringList",   "false"},
                            InputOutputPair{"'string' in 'a big string'", "true"},
                            InputOutputPair{"'a big string' in 'substr'",  "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(EvenTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is even",              "true"},
                            InputOutputPair{"11 is even",             "false"},
                            InputOutputPair{"11.6 is even",           "false"},
                            InputOutputPair{"11.0 is even",           "false"},
                            InputOutputPair{"12.6 is even",           "false"},
                            InputOutputPair{"12.0 is even",           "true"},
                            InputOutputPair{"'intList' is even",      "false"},
                            InputOutputPair{"false is even",          "false"},
                            InputOutputPair{"0.2 is even",            "false"},
                            InputOutputPair{"intValue is even",       "false"},
                            InputOutputPair{"stringValue is even",    "false"},
                            InputOutputPair{"doubleValue is even",    "false"},
                            InputOutputPair{"boolFalseValue is even", "false"},
                            InputOutputPair{"boolTrueValue is even",  "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(OddTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is odd",              "false"},
                            InputOutputPair{"11 is odd",             "true"},
                            InputOutputPair{"11.6 is odd",           "false"},
                            InputOutputPair{"11.0 is odd",           "true"},
                            InputOutputPair{"12.6 is odd",           "false"},
                            InputOutputPair{"12.0 is odd",           "false"},
                            InputOutputPair{"'intList' is odd",      "false"},
                            InputOutputPair{"false is odd",          "false"},
                            InputOutputPair{"0.2 is odd",            "false"},
                            InputOutputPair{"intValue is odd",       "true"},
                            InputOutputPair{"stringValue is odd",    "false"},
                            InputOutputPair{"doubleValue is odd",    "false"},
                            InputOutputPair{"boolFalseValue is odd", "false"},
                            InputOutputPair{"boolTrueValue is odd",  "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(LowerTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is lower",              "false"},
                            InputOutputPair{"11.6 is lower",           "false"},
                            InputOutputPair{"'intList' is lower",      "false"},
                            InputOutputPair{"'intlist' is lower",      "true"},
                            InputOutputPair{"'intlist1235' is lower",  "true"},
                            InputOutputPair{"'intList1235' is lower",  "false"},
                            InputOutputPair{"false is lower",          "false"},
                            InputOutputPair{"0.2 is lower",            "false"},
                            InputOutputPair{"intValue is lower",       "false"},
                            InputOutputPair{"stringValue is lower",    "true"},
                            InputOutputPair{"doubleValue is lower",    "false"},
                            InputOutputPair{"boolFalseValue is lower", "false"},
                            InputOutputPair{"boolTrueValue is lower",  "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(UpperTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is upper",              "false"},
                            InputOutputPair{"11.6 is upper",           "false"},
                            InputOutputPair{"'intList' is upper",      "false"},
                            InputOutputPair{"'INTLIST' is upper",      "true"},
                            InputOutputPair{"'INTLIST1235' is upper",  "true"},
                            InputOutputPair{"'intList1235' is upper",  "false"},
                            InputOutputPair{"false is upper",          "false"},
                            InputOutputPair{"0.2 is upper",            "false"},
                            InputOutputPair{"intValue is upper",       "false"},
                            InputOutputPair{"stringValue is upper",    "false"},
                            InputOutputPair{"doubleValue is upper",    "false"},
                            InputOutputPair{"boolFalseValue is upper", "false"},
                            InputOutputPair{"boolTrueValue is upper",  "false"}
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(BooleanTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"true is boolean",          "true"},
                            InputOutputPair{"boolFalseValue is boolean", "true"},
                            InputOutputPair{"0 is boolean",             "false"},
                            InputOutputPair{"none is boolean",          "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(TrueFalseTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"true is true",            "true"},
                            InputOutputPair{"1 is true",               "false"},
                            InputOutputPair{"boolTrueValue is true",   "true"},
                            InputOutputPair{"false is true",           "false"},
                            InputOutputPair{"false is false",          "true"},
                            InputOutputPair{"0 is false",              "false"},
                            InputOutputPair{"none is false",           "false"},
                            InputOutputPair{"boolFalseValue is false", "true"}
                            ));

INSTANTIATE_TEST_SUITE_P(NumberKindTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"intValue is integer",    "true"},
                            InputOutputPair{"doubleValue is integer", "false"},
                            InputOutputPair{"true is integer",        "false"},
                            InputOutputPair{"'1' is integer",         "false"},
                            InputOutputPair{"doubleValue is float",   "true"},
                            InputOutputPair{"1.0 is float",           "true"},
                            InputOutputPair{"intValue is float",      "false"},
                            InputOutputPair{"intValue is odd and doubleValue is float", "true"}
                            ));

INSTANTIATE_TEST_SUITE_P(DivisibleByTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"9 is divisibleby 3",     "true"},
                            InputOutputPair{"9 is divisibleby(4)",    "false"},
                            InputOutputPair{"-9 is divisibleby(3)",   "true"},
                            InputOutputPair{"7.5 is divisibleby(2.5)", "true"},
                            InputOutputPair{"7.5 is divisibleby(2)",  "false"},
                            InputOutputPair{"true is divisibleby(1)", "true"},
                            InputOutputPair{"intValue is not divisibleby(2)", "true"}
                            ));

INSTANTIATE_TEST_SUITE_P(CallableTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"range is callable",     "true"},
                            InputOutputPair{"intValue is callable",  "false"},
                            InputOutputPair{"'range' is callable",   "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(SameAsTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"none is sameas none",        "true"},
                            InputOutputPair{"none is sameas false",       "false"},
                            InputOutputPair{"intValue is sameas 3",       "true"},
                            InputOutputPair{"1 is sameas true",           "false"},
                            InputOutputPair{"1 is sameas 1.0",            "false"},
                            InputOutputPair{"intList is sameas intList",  "true"},
                            InputOutputPair{"intList is sameas intAsDoubleList", "false"},
                            InputOutputPair{"simpleMapValue is sameas simpleMapValue", "true"},
                            InputOutputPair{"[1] is sameas [1]",          "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(FilterTestNameTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"'upper' is filter",    "true"},
                            InputOutputPair{"'replace' is filter",  "true"},
                            InputOutputPair{"'nope' is filter",     "false"},
                            InputOutputPair{"'odd' is filter",      "false"},
                            InputOutputPair{"1 is filter",          "false"},
                            InputOutputPair{"'odd' is test",        "true"},
                            InputOutputPair{"'divisibleby' is test", "true"},
                            InputOutputPair{"'upper' is test",      "true"},
                            InputOutputPair{"'nope' is test",       "false"}
                            ));

INSTANTIATE_TEST_SUITE_P(EscapedTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"stringValue is escaped", "false"},
                            InputOutputPair{"1 is escaped",           "false"}
                            ));
// clang-format on

TEST(TestersTest, UnknownTestIsAnError)
{
    // Jinja2 fails at compile time; Jinja2C++ resolves user testers at render time
    for (std::string source : { "{{ 1 is nonexistent }}", "{{ [1, 2]|select('nonexistent')|list }}", "{{ 1 is intValue }}" })
    {
        Template tpl;
        ASSERT_TRUE(tpl.Load(source)) << source;
        auto result = tpl.RenderAsString(PrepareTestData());
        ASSERT_FALSE(result) << source;
        EXPECT_NE(std::string::npos, result.error().ToString().find("No test named")) << source << ": " << result.error().ToString();
    }

    Template tpl;
    ASSERT_TRUE(tpl.Load("{% if false %}{{ 1 is nonexistent }}{% endif %}ok"));
    EXPECT_EQ("ok", tpl.RenderAsString(PrepareTestData()).value());
}

TEST(TestersTest, UserCallablesCountAsTestsAndFilters)
{
    ValuesMap params{ { "mytest", MakeCallable([](const std::string& val) { return val.empty(); }, ArgInfo{ "val" }) } };

    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ 'T' if 'mytest' is test else 'F' }}{{ 'T' if 'mytest' is filter else 'F' }}{{ 'T' if '' is mytest else 'F' }}"));
    EXPECT_EQ("TTT", tpl.RenderAsString(params).value());
}
