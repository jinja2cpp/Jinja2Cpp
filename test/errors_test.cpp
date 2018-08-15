#include <iostream>
#include <string>

#include "test_tools.h"
#include "jinja2cpp/template.h"

using namespace jinja2;

struct ErrorsGenericTestTag;
using ErrorsGenericTest = InputOutputPairTest<ErrorsGenericTestTag>;


TEST_P(ErrorsGenericTest, Test)
{
    auto& testParam = GetParam();
    std::string source = testParam.tpl;

    Template tpl;
    auto parseResult = tpl.Load(source);
    EXPECT_TRUE(parseResult.HasError());

    std::ostringstream errorDescr;
    errorDescr << parseResult;
    std::string result = errorDescr.str();
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

INSTANTIATE_TEST_CASE_P(BasicTest, ErrorsGenericTest, ::testing::Values(
                            InputOutputPair{"{{}}",                         ""},
                            InputOutputPair{"{{ ) }}",                         ""},
                            InputOutputPair{"{% %}",                         ""},
                            InputOutputPair{"{% if %}",                         ""},
                            InputOutputPair{"{% endif %}",                         ""},
                            InputOutputPair{"Hello World!\n    {% if %}",                         ""},
                            InputOutputPair{"Hello World!\n\t{% if %}",                         ""},
                            InputOutputPair{"{{",                         ""},
                            InputOutputPair{"}}",                         ""}
                            ));
