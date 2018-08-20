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
    EXPECT_FALSE(parseResult.has_value());

    std::ostringstream errorDescr;
    errorDescr << parseResult.error();
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
                            InputOutputPair{"{{ + [1,] }}",
                                            "noname.j2tpl:1:9: error: Unexpected token: ']'\n{{ + [1,] }}\n     ---^-------"},
                            InputOutputPair{"{{ 1 + 2 + [1,]}}",
                                            "noname.j2tpl:1:15: error: Unexpected token: ']'\n{{ 1 + 2 + [1,]}}\n           ---^-------"},
                            InputOutputPair{"{{ 1 + 2 and [1,]}}",
                                            "noname.j2tpl:1:17: error: Unexpected token: ']'\n{{ 1 + 2 and [1,]}}\n             ---^-------"},
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
                            InputOutputPair{"{{ 1 if 2 == 3 else {1} }}",
                                            "noname.j2tpl:1:22: error: String expected\n{{ 1 if 2 == 3 else {1} }}\n                  ---^-------"},
                            InputOutputPair{"{{ 1 if 2 is equalto(10,) else 2 }}",
                                            "noname.j2tpl:1:25: error: Unexpected token: ')'\n{{ 1 if 2 is equalto(10,) else 2 }}\n                     ---^-------"},
                            InputOutputPair{"{{ range(1, 3, ) }}",
                                            "noname.j2tpl:1:16: error: Unexpected token: ')'\n{{ range(1, 3, ) }}\n            ---^-------"},
                            InputOutputPair{"{{ range(1, 3} }}",
                                            "noname.j2tpl:1:14: error: ')' expected\n{{ range(1, 3} }}\n          ---^-------"},
                            InputOutputPair{"{{ {1, 3, 5] }}",
                                            "noname.j2tpl:1:5: error: String expected\n{{ {1, 3, 5] }}\n ---^-------"},
                            InputOutputPair{"{{ {'key'} }}",
                                            "noname.j2tpl:1:10: error: Unexpected token '}'. Expected: '='\n{{ {'key'} }}\n      ---^-------"},
                            InputOutputPair{"{{ {'key'=} }}",
                                            "noname.j2tpl:1:11: error: Expected expression, got: '}'\n{{ {'key'=} }}\n       ---^-------"},
                            InputOutputPair{"{{ {'key'=,} }}",
                                            "noname.j2tpl:1:11: error: Expected expression, got: ','\n{{ {'key'=,} }}\n       ---^-------"},
                            InputOutputPair{"{{ {=1} }}",
                                            "noname.j2tpl:1:5: error: String expected\n{{ {=1} }}\n ---^-------"},
                            InputOutputPair{"{{ {'key'=1] }}",
                                            "noname.j2tpl:1:12: error: '}' expected\n{{ {'key'=1] }}\n        ---^-------"},
                            InputOutputPair{"{{ {'key'=1,} }}",
                                            "noname.j2tpl:1:13: error: String expected\n{{ {'key'=1,} }}\n         ---^-------"},
                            InputOutputPair{"{{ [1, 3, 5} }}",
                                            "noname.j2tpl:1:12: error: ']' expected\n{{ [1, 3, 5} }}\n        ---^-------"},
                            InputOutputPair{"{{ [1, 3,] }}",
                                            "noname.j2tpl:1:10: error: Unexpected token: ']'\n{{ [1, 3,] }}\n      ---^-------"},
                            InputOutputPair{"{{ (1, 3, 5} }}",
                                            "noname.j2tpl:1:14: error: ')' expected\n{{ (1, 3, 5} }}\n          ---^-------"},
                            InputOutputPair{"{{ value.'10' }}",
                                            "noname.j2tpl:1:11: error: Identifier expected\n{{ value.'10' }}\n       ---^-------"},
                            InputOutputPair{"{{ value[1,] }}",
                                            "noname.j2tpl:1:11: error: ']' expected\n{{ value[1,] }}\n       ---^-------"},
                            InputOutputPair{"{{ value[1} }}",
                                            "noname.j2tpl:1:11: error: ']' expected\n{{ value[1} }}\n       ---^-------"},
                            InputOutputPair{"{{ value[=] }}",
                                            "noname.j2tpl:1:10: error: Unexpected token: '='\n{{ value[=] }}\n      ---^-------"},
                            InputOutputPair{"{{ value[1] | map(arr=) }}",
                                            "noname.j2tpl:1:23: error: Unexpected token: ')'\n{{ value[1] | map(arr=) }}\n                   ---^-------"},
                            InputOutputPair{"{{ value[1] | map(,) }}",
                                            "noname.j2tpl:1:19: error: Unexpected token: ','\n{{ value[1] | map(,) }}\n               ---^-------"},
                            InputOutputPair{"{{}}",
                                            "noname.j2tpl:1:3: error: Unexpected token: '<<End of block>>'\n{{}}\n--^-------"}
                            ));

INSTANTIATE_TEST_CASE_P(StatementsTest, ErrorsGenericTest, ::testing::Values(
                            InputOutputPair{"{% if %}",
                                            "noname.j2tpl:1:7: error: Expected expression, got: '<<End of block>>'\n{% if %}\n   ---^-------"},
                            InputOutputPair{"{% endif %}",
                                            "noname.j2tpl:1:4: error: Unexpected statement: 'endif'\n{% endif %}\n---^-------"},
                            InputOutputPair{"{% endfor %}",
                                            "noname.j2tpl:1:4: error: Unexpected statement: 'endfor'\n{% endfor %}\n---^-------"},
                            InputOutputPair{"{% else hello %}",
                                            "noname.j2tpl:1:9: error: Expected end of statement, got: 'hello'\n{% else hello %}\n     ---^-------"},
                            InputOutputPair{"{% elif %}",
                                            "noname.j2tpl:1:9: error: Expected expression, got: '<<End of block>>'\n{% elif %}\n     ---^-------"},
                            InputOutputPair{"{% for %}",
                                            "noname.j2tpl:1:8: error: Identifier expected\n{% for %}\n    ---^-------"},
                            InputOutputPair{"{% for 10 in range(10) %}",
                                            "noname.j2tpl:1:8: error: Identifier expected\n{% for 10 in range(10) %}\n    ---^-------"},
                            InputOutputPair{"{% for i, 10 range(10)%}",
                                            "noname.j2tpl:1:11: error: Unexpected token '10'. Expected: '<<Identifier>>', 'in', ','\n{% for i, 10 range(10)%}\n       ---^-------"},
                            InputOutputPair{"{% for i on range(10)%}",
                                            "noname.j2tpl:1:10: error: Unexpected token 'on'. Expected: '<<Identifier>>', 'in', ','\n{% for i on range(10)%}\n      ---^-------"},
                            InputOutputPair{"{% for i in range(10,)%}",
                                            "noname.j2tpl:1:22: error: Unexpected token: ')'\n{% for i in range(10,)%}\n                  ---^-------"},
                            InputOutputPair{"{% for i in range(10) rec%}",
                                            "noname.j2tpl:1:23: error: Unexpected token 'rec'. Expected: 'recursive', 'if'\n{% for i in range(10) rec%}\n                   ---^-------"},
                            InputOutputPair{"{% for i in range(10) endfor%}",
                                            "noname.j2tpl:1:23: error: Unexpected token 'endfor'. Expected: 'if', '<<End of block>>'\n{% for i in range(10) endfor%}\n                   ---^-------"},
                            InputOutputPair{"{% for i in range(10) if {key} %}",
                                            "noname.j2tpl:1:27: error: String expected\n{% for i in range(10) if {key} %}\n                       ---^-------"},
                            InputOutputPair{"{% for i in range(10) if true else hello %}",
                                            "noname.j2tpl:1:31: error: Expected end of statement, got: 'else'\n{% for i in range(10) if true else hello %}\n                           ---^-------"},
                            InputOutputPair{"{% for i in range(10) %}\n{% endif %}",
                                            "noname.j2tpl:2:4: error: Unexpected statement: 'endif'\n{% endif %}\n---^-------"},
                            InputOutputPair{"{% if true %}\n{% for i in range(10) %}\n{% endif %}",
                                            "noname.j2tpl:3:4: error: Unexpected statement: 'endif'\n{% endif %}\n---^-------"},
                            InputOutputPair{"{% if true %}\n{% endfor %}",
                                            "noname.j2tpl:2:4: error: Unexpected statement: 'endfor'\n{% endfor %}\n---^-------"},
                            InputOutputPair{"{% set %}",
                                            "noname.j2tpl:1:8: error: Identifier expected\n{% set %}\n    ---^-------"},
                            InputOutputPair{"{% set id%}",
                                            "noname.j2tpl:1:10: error: This feature has not been supported yet\n{% set id%}\n      ---^-------"},
                            InputOutputPair{"{% set 10%}",
                                            "noname.j2tpl:1:8: error: Identifier expected\n{% set 10%}\n    ---^-------"},
                            InputOutputPair{"{% set i = {key] %}",
                                            "noname.j2tpl:1:13: error: String expected\n{% set i = {key] %}\n         ---^-------"},
                            InputOutputPair{"{% set id=10%}\n{% endset %}",
                                            "noname.j2tpl:2:4: error: This feature has not been supported yet\n{% endset %}\n---^-------"},
                            InputOutputPair{"{% extends %}",
                                            "noname.j2tpl:1:12: error: Unexpected token '<<End of block>>'. Expected: '<<Identifier>>', '<<String>>'\n{% extends %}\n        ---^-------"},
                            InputOutputPair{"{% extends 10 %}",
                                            "noname.j2tpl:1:12: error: Unexpected token '10'. Expected: '<<Identifier>>', '<<String>>'\n{% extends 10 %}\n        ---^-------"},
                            InputOutputPair{"{% block %}",
                                            "noname.j2tpl:1:10: error: Identifier expected\n{% block %}\n      ---^-------"},
                            InputOutputPair{"{% block 10 %}",
                                            "noname.j2tpl:1:10: error: Identifier expected\n{% block 10 %}\n      ---^-------"},
                            InputOutputPair{"{% block a scp %}",
                                            "noname.j2tpl:1:12: error: Unexpected token 'scp'. Expected: 'scoped'\n{% block a scp %}\n        ---^-------"},
                            InputOutputPair{"{{}}",
                                            "noname.j2tpl:1:3: error: Unexpected token: '<<End of block>>'\n{{}}\n--^-------"}
                            ));
