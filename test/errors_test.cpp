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
                            InputOutputPair{"{{}}",
                                            "noname.j2tpl:1:3: error: Unexpected token: '<<End of block>>'\n{{}}\n--^-------"},
                            InputOutputPair{"{{ ) }}",
                                            "noname.j2tpl:1:4: error: Unexpected token: ')'\n{{ ) }}\n---^-------"},
                            InputOutputPair{"{% %}",
                                            "noname.j2tpl:1:4: error: Unexpected token: '<<End of block>>'\n{% %}\n---^-------"},
                            InputOutputPair{"{% if %}",
                                            "noname.j2tpl:1:7: error: Expected expression, got: '<<End of block>>'\n{% if %}\n   ---^-------"},
                            InputOutputPair{"{% endif %}",
                                            "noname.j2tpl:1:4: error: Unexpected statement: 'endif'\n{% endif %}\n---^-------"},
                            InputOutputPair{"Hello World!\n    {% if %}",
                                            "noname.j2tpl:2:11: error: Expected expression, got: '<<End of block>>'\n    {% if %}\n       ---^-------"},
                            InputOutputPair{"Hello World!\n\t{% if %}",
                                            "noname.j2tpl:2:8: error: Expected expression, got: '<<End of block>>'\n\t{% if %}\n\t   ---^-------"},
                            InputOutputPair{"{{",
                                            "noname.j2tpl:1:3: error: Unexpected token: '<<End of block>>'\n{{\n--^-------"},
                            InputOutputPair{"}}",
                                            "noname.j2tpl:1:1: error: Unexpected expression block end\n}}\n^-------"}
                            ));

INSTANTIATE_TEST_CASE_P(BasicExpressionsTest, ErrorsGenericTest, ::testing::Values(
                            InputOutputPair{"{{ * or }}",
                                            "noname.j2tpl:1:4: error: Unexpected token: '*'\n{{ * or }}\n---^-------"},
                            InputOutputPair{"{{ 1 + }}",
                                            "noname.j2tpl:1:8: error: Unexpected token: '<<End of block>>'\n{{ 1 + }}\n    ---^-------"},
                            InputOutputPair{"{{ 1 or * }}",
                                            "noname.j2tpl:1:9: error: Unexpected token: '*'\n{{ 1 or * }}\n     ---^-------"},
                            InputOutputPair{"{{ 1 and * }}",
                                            "noname.j2tpl:1:10: error: Unexpected token: '*'\n{{ 1 and * }}\n      ---^-------"},
                            InputOutputPair{"{{ 1 not - }}",
                                            "noname.j2tpl:1:6: error: Unexpected token 'not'. Expected: '<<End of block>>'\n{{ 1 not - }}\n  ---^-------"},
                            InputOutputPair{"{{ 1 not * }}",
                                            "noname.j2tpl:1:6: error: Unexpected token 'not'. Expected: '<<End of block>>'\n{{ 1 not * }}\n  ---^-------"},
                            InputOutputPair{"{{ 1 | }}",
                                            "noname.j2tpl:1:8: error: Identifier expected\n{{ 1 | }}\n    ---^-------"},
                            InputOutputPair{"{{ 1 if }}",
                                            "noname.j2tpl:1:9: error: Unexpected token: '<<End of block>>'\n{{ 1 if }}\n     ---^-------"},
                            InputOutputPair{"{{ 1 if else 2 }}",
                                            "noname.j2tpl:1:9: error: Unexpected token: 'else'\n{{ 1 if else 2 }}\n     ---^-------"},
                            InputOutputPair{"{{ 1 if 2 is 3 else 2 }}",
                                            "noname.j2tpl:1:14: error: Identifier expected\n{{ 1 if 2 is 3 else 2 }}\n          ---^-------"},
                            InputOutputPair{"{{ 1 if 2 is equalto(10,) else 2 }}",
                                            "noname.j2tpl:1:25: error: Unexpected token: ')'\n{{ 1 if 2 is equalto(10,) else 2 }}\n                     ---^-------"},
                            InputOutputPair{"{{ range(1, 3, ) }}",
                                            "noname.j2tpl:1:16: error: Unexpected token: ')'\n{{ range(1, 3, ) }}\n            ---^-------"},
                            InputOutputPair{"{{ {1, 3, 5] }}",
                                            "noname.j2tpl:1:5: error: String expected\n{{ {1, 3, 5] }}\n ---^-------"},
                            InputOutputPair{"{{ [1, 3, 5} }}",
                                            "noname.j2tpl:1:12: error: Unexpected token: '}'\n{{ [1, 3, 5} }}\n        ---^-------"},
                            InputOutputPair{"{{ (1, 3, 5} }}",
                                            "noname.j2tpl:1:14: error: ')' expected\n{{ (1, 3, 5} }}\n          ---^-------"},
                            InputOutputPair{"{{}}",
                                            "noname.j2tpl:1:3: error: Unexpected token: '<<End of block>>'\n{{}}\n--^-------"}
                            ));
