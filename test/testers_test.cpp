#include <iostream>
#include <string>

#include "test_tools.h"
#include "jinja2cpp/template.h"

using namespace jinja2;

struct TestersGenericTestTag;
using TestersGenericTest = InputOutputPairTest<TestersGenericTestTag>;


TEST_P(TestersGenericTest, Test)
{
    auto& testParam = GetParam();
    std::string source = "{{ 'true' if " + testParam.tpl + " else 'false'}}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(PrepareTestData());
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

INSTANTIATE_TEST_CASE_P(EqTest, TestersGenericTest, ::testing::Values(
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


INSTANTIATE_TEST_CASE_P(NeTest, TestersGenericTest, ::testing::Values(
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


INSTANTIATE_TEST_CASE_P(GeTest, TestersGenericTest, ::testing::Values(
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


INSTANTIATE_TEST_CASE_P(GtTest, TestersGenericTest, ::testing::Values(
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

INSTANTIATE_TEST_CASE_P(LeTest, TestersGenericTest, ::testing::Values(
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


INSTANTIATE_TEST_CASE_P(LtTest, TestersGenericTest, ::testing::Values(
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
