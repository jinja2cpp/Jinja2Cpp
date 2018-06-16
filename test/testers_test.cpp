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
                            InputOutputPair{"0 is equalto(1)",                    "false"}
                            ));


INSTANTIATE_TEST_CASE_P(NeTest, TestersGenericTest, ::testing::Values(
                            InputOutputPair{"0 is ne(0)",                         "false"},
                            InputOutputPair{"0 is ne(1)",                         "true"},
                            InputOutputPair{"0.5 is ne(0.5)",                     "false"},
                            InputOutputPair{"0.5 is ne(1.5)",                     "true"},
                            InputOutputPair{"'0.5' is ne('0.5')",                 "false"},
                            InputOutputPair{"'0.5' is ne('1.5')",                 "true"},
                            InputOutputPair{"'Hello World' is ne('hello world')", "true"}
                            ));
