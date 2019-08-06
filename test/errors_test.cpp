#include <iostream>
#include <string>

#include "test_tools.h"
#include "jinja2cpp/template.h"

using namespace jinja2;

struct ErrorsGenericTestTag;
struct ErrorsGenericExtensionTestTag;
using ErrorsGenericTest = InputOutputPairTest<ErrorsGenericTestTag>;
using ErrorsGenericExtensionsTest = InputOutputPairTest<ErrorsGenericExtensionTestTag>;

TEST_F(TemplateEnvFixture, EnvironmentAbsentErrorsTest)
{
    Template tpl1;
    auto parseResult = tpl1.Load("{% extends 'module' %}");
    ASSERT_FALSE(parseResult.has_value());

    EXPECT_EQ("noname.j2tpl:1:4: error: Template environment doesn't set\n{% extends 'module' %}\n---^-------", ErrorToString(parseResult.error()));

    parseResult = tpl1.Load("{% include 'module' %}");
    ASSERT_FALSE(parseResult.has_value());

    EXPECT_EQ("noname.j2tpl:1:4: error: Template environment doesn't set\n{% include 'module' %}\n---^-------", ErrorToString(parseResult.error()));

    parseResult = tpl1.Load("{% from 'module' %}");
    ASSERT_FALSE(parseResult.has_value());

    EXPECT_EQ("noname.j2tpl:1:4: error: Template environment doesn't set\n{% from 'module' %}\n---^-------", ErrorToString(parseResult.error()));

    parseResult = tpl1.Load("{% import 'module' %}");
    ASSERT_FALSE(parseResult.has_value());

    EXPECT_EQ("noname.j2tpl:1:4: error: Template environment doesn't set\n{% import 'module' %}\n---^-------", ErrorToString(parseResult.error()));
}

TEST_F(TemplateEnvFixture, EnvironmentAbsentErrorsTest_Wide)
{
    TemplateW tpl1;
    auto parseResult = tpl1.Load(L"{% extends 'module' %}");
    ASSERT_FALSE(parseResult.has_value());

    EXPECT_EQ(L"noname.j2tpl:1:4: error: Template environment doesn't set\n{% extends 'module' %}\n---^-------", ErrorToString(parseResult.error()));

    parseResult = tpl1.Load(L"{% include 'module' %}");
    ASSERT_FALSE(parseResult.has_value());

    EXPECT_EQ(L"noname.j2tpl:1:4: error: Template environment doesn't set\n{% include 'module' %}\n---^-------", ErrorToString(parseResult.error()));

    parseResult = tpl1.Load(L"{% from 'module' %}");
    ASSERT_FALSE(parseResult.has_value());

    EXPECT_EQ(L"noname.j2tpl:1:4: error: Template environment doesn't set\n{% from 'module' %}\n---^-------", ErrorToString(parseResult.error()));

    parseResult = tpl1.Load(L"{% import 'module' %}");
    ASSERT_FALSE(parseResult.has_value());

    EXPECT_EQ(L"noname.j2tpl:1:4: error: Template environment doesn't set\n{% import 'module' %}\n---^-------", ErrorToString(parseResult.error()));
}

TEST_F(TemplateEnvFixture, RenderErrorsTest)
{
    Template tpl1;
    auto renderResult = tpl1.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ("<unknown file>:1:1: error: Template not parsed\n", ErrorToString(renderResult.error()));

    Template tpl2;
    tpl2.Load(R"({{ foo() }})");
    renderResult = tpl2.RenderAsString({{"foo", MakeCallable([]() -> Value {throw std::runtime_error("Bang!"); })}});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ("noname.j2tpl:1:1: error: Unexpected exception occurred during template processing. Exception: Bang!\n", ErrorToString(renderResult.error()));

    Template tpl3(&m_env);
    auto parseResult = tpl3.Load("{% import name as name %}");
    EXPECT_TRUE(parseResult.has_value());
    if (!parseResult)
        std::cout << parseResult.error() << std::endl;
    renderResult = tpl3.RenderAsString({{"name", 10}});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ("noname.j2tpl:1:1: error: Invalid template name: 10\n", ErrorToString(renderResult.error()));
}

TEST_F(TemplateEnvFixture, RenderErrorsTest_Wide)
{
    TemplateW tpl1;
    auto renderResult = tpl1.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ(L"<unknown file>:1:1: error: Template not parsed\n", ErrorToString(renderResult.error()));

    TemplateW tpl2;
    tpl2.Load(LR"({{ foo() }})");
    renderResult = tpl2.RenderAsString({ {"foo", MakeCallable([]() -> Value {throw std::runtime_error("Bang!"); })} });
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ(L"noname.j2tpl:1:1: error: Unexpected exception occurred during template processing. Exception: Bang!\n", ErrorToString(renderResult.error()));

    TemplateW tpl3(&m_env);
    auto parseResult = tpl3.Load(L"{% import name as name %}");
    EXPECT_TRUE(parseResult.has_value());
    if (!parseResult)
        std::wcout << parseResult.error() << std::endl;
    renderResult = tpl3.RenderAsString({ {"name", 10} });
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ(L"noname.j2tpl:1:1: error: Invalid template name: 10\n", ErrorToString(renderResult.error()));
}

TEST_F(TemplateEnvFixture, ErrorPropagationTest)
{
    AddFile("module", "{% for %}");
    Template tpl1(&m_env);
    auto parseResult = tpl1.Load("{% extends 'module' %}");
    ASSERT_TRUE(parseResult.has_value());
    auto renderResult = tpl1.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ("module:1:8: error: Identifier expected\n{% for %}\n    ---^-------", ErrorToString(renderResult.error()));

    Template tpl2(&m_env);
    parseResult = tpl2.Load("{% include 'module' %}");
    ASSERT_TRUE(parseResult.has_value());
    renderResult = tpl2.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ("module:1:8: error: Identifier expected\n{% for %}\n    ---^-------", ErrorToString(renderResult.error()));

    Template tpl3(&m_env);
    parseResult = tpl3.Load("{% from 'module' import name %}");
    ASSERT_TRUE(parseResult.has_value());
    renderResult = tpl3.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ("module:1:8: error: Identifier expected\n{% for %}\n    ---^-------", ErrorToString(renderResult.error()));

    Template tpl4(&m_env);
    parseResult = tpl4.Load("{% import 'module' as module %}");
    ASSERT_TRUE(parseResult.has_value());
    renderResult = tpl4.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ("module:1:8: error: Identifier expected\n{% for %}\n    ---^-------", ErrorToString(renderResult.error()));
}

TEST_F(TemplateEnvFixture, ErrorPropagationTest_Wide)
{
    AddFile("module", "{% for %}");
    TemplateW tpl1(&m_env);
    auto parseResult = tpl1.Load(L"{% extends 'module' %}");
    ASSERT_TRUE(parseResult.has_value());
    auto renderResult = tpl1.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ(L"module:1:8: error: Identifier expected\n{% for %}\n    ---^-------", ErrorToString(renderResult.error()));

    TemplateW tpl2(&m_env);
    parseResult = tpl2.Load(L"{% include 'module' %}");
    ASSERT_TRUE(parseResult.has_value());
    renderResult = tpl2.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ(L"module:1:8: error: Identifier expected\n{% for %}\n    ---^-------", ErrorToString(renderResult.error()));

    TemplateW tpl3(&m_env);
    parseResult = tpl3.Load(L"{% from 'module' import name %}");
    ASSERT_TRUE(parseResult.has_value());
    renderResult = tpl3.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ(L"module:1:8: error: Identifier expected\n{% for %}\n    ---^-------", ErrorToString(renderResult.error()));

    TemplateW tpl4(&m_env);
    parseResult = tpl4.Load(L"{% import 'module' as module %}");
    ASSERT_TRUE(parseResult.has_value());
    renderResult = tpl4.RenderAsString({});
    ASSERT_FALSE(renderResult.has_value());

    EXPECT_EQ(L"module:1:8: error: Identifier expected\n{% for %}\n    ---^-------", ErrorToString(renderResult.error()));
}

TEST_P(ErrorsGenericTest, Test)
{
    auto& testParam = GetParam();
    std::string source = testParam.tpl;

    TemplateEnv env;
    Template tpl(&env);
    auto parseResult = tpl.Load(source);
    ASSERT_FALSE(parseResult.has_value());

    auto result = ErrorToString(parseResult.error());
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

TEST_P(ErrorsGenericTest, Test_Wide)
{
    auto& testParam = GetParam();
    std::string source = testParam.tpl;

    TemplateEnv env;
    TemplateW tpl(&env);
    auto parseResult = tpl.Load(jinja2::ConvertString<std::wstring>(source));
    ASSERT_FALSE(parseResult.has_value());

    auto result = ErrorToString(parseResult.error());
    std::wcout << result << std::endl;
    std::wstring expectedResult = jinja2::ConvertString<std::wstring>(testParam.result);
    EXPECT_EQ(expectedResult, result);
}

TEST_P(ErrorsGenericExtensionsTest, Test)
{
    auto& testParam = GetParam();
    std::string source = testParam.tpl;

    TemplateEnv env;
    env.GetSettings().extensions.Do = true;

    Template tpl(&env);
    auto parseResult = tpl.Load(source);
    ASSERT_FALSE(parseResult.has_value());

    auto result = ErrorToString(parseResult.error());
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

TEST_P(ErrorsGenericExtensionsTest, Test_Wide)
{
    auto& testParam = GetParam();
    std::string source = testParam.tpl;

    TemplateEnv env;
    env.GetSettings().extensions.Do = true;

    TemplateW tpl(&env);
    auto parseResult = tpl.Load(jinja2::ConvertString<std::wstring>(source));
    ASSERT_FALSE(parseResult.has_value());

    auto result = ErrorToString(parseResult.error());
    std::wcout << result << std::endl;
    std::wstring expectedResult = jinja2::ConvertString<std::wstring>(testParam.result);
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

INSTANTIATE_TEST_CASE_P(StatementsTest_1, ErrorsGenericTest, ::testing::Values(
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
                                            "noname.j2tpl:1:23: error: Unexpected token 'rec'. Expected: 'if', 'recursive', '<<End of block>>'\n{% for i in range(10) rec%}\n                   ---^-------"},
                            InputOutputPair{"{% for i in range(10) endfor%}",
                                            "noname.j2tpl:1:23: error: Unexpected token 'endfor'. Expected: 'if', 'recursive', '<<End of block>>'\n{% for i in range(10) endfor%}\n                   ---^-------"},
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
                            InputOutputPair{"{% import %}",
                                            "noname.j2tpl:1:11: error: Unexpected token: '<<End of block>>'\n{% import %}\n       ---^-------"},
                            InputOutputPair{"{% import 'foo' %}",
                                            "noname.j2tpl:1:17: error: Unexpected token '<<End of block>>'. Expected: 'as'\n{% import 'foo' %}\n             ---^-------"},
                            InputOutputPair{"{% import 'foo' as %}",
                                            "noname.j2tpl:1:20: error: Unexpected token '<<End of block>>'. Expected: '<<Identifier>>'\n{% import 'foo' as %}\n                ---^-------"},
                            InputOutputPair{"{% import 'foo', as %}",
                                            "noname.j2tpl:1:16: error: Unexpected token ','. Expected: 'as'\n{% import 'foo', as %}\n            ---^-------"},
                            InputOutputPair{"{% import 'foo', %}",
                                            "noname.j2tpl:1:16: error: Unexpected token ','. Expected: 'as'\n{% import 'foo', %}\n            ---^-------"},
                            InputOutputPair{"{% import 'foo', bar %}",
                                            "noname.j2tpl:1:16: error: Unexpected token ','. Expected: 'as'\n{% import 'foo', bar %}\n            ---^-------"},
                            InputOutputPair{"{% from 'foo' import, %}",
                                            "noname.j2tpl:1:21: error: Unexpected token ','. Expected: '<<Identifier>>'\n{% from 'foo' import, %}\n                 ---^-------"},
                            InputOutputPair{"{% from 'foo' import %}",
                                            "noname.j2tpl:1:22: error: Unexpected token '<<End of block>>'. Expected: '<<Identifier>>'\n{% from 'foo' import %}\n                  ---^-------"},
                            InputOutputPair{"{% from 'foo' import bar, %}",
                                            "noname.j2tpl:1:27: error: Unexpected token '<<End of block>>'. Expected: '<<Identifier>>'\n{% from 'foo' import bar, %}\n                       ---^-------"},
                            InputOutputPair{"{% from 'foo' import bar,, with context %}",
                                            "noname.j2tpl:1:26: error: Unexpected token ','. Expected: '<<Identifier>>'\n{% from 'foo' import bar,, with context %}\n                      ---^-------"},
                            InputOutputPair{"{% from 'foo' import bar with context, %}",
                                            "noname.j2tpl:1:38: error: Expected end of statement, got: ','\n{% from 'foo' import bar with context, %}\n                                  ---^-------"}
                            ));

INSTANTIATE_TEST_CASE_P(StatementsTest_2, ErrorsGenericTest, ::testing::Values(
                            InputOutputPair{"{% block %}",
                                            "noname.j2tpl:1:10: error: Identifier expected\n{% block %}\n      ---^-------"},
                            InputOutputPair{"{% block 10 %}",
                                            "noname.j2tpl:1:10: error: Identifier expected\n{% block 10 %}\n      ---^-------"},
                            InputOutputPair{"{% block a scp %}",
                                            "noname.j2tpl:1:12: error: Unexpected token 'scp'. Expected: 'scoped'\n{% block a scp %}\n        ---^-------"},
                            InputOutputPair{"{% block somename %}{% endblock 10 %}",
                                            "noname.j2tpl:1:33: error: Unexpected token '10'. Expected: '<<Identifier>>', '<<End of block>>'\n{% block somename %}{% endblock 10 %}\n                             ---^-------"},
                            InputOutputPair{"{% endblock %}{% endblock %}",
                                            "noname.j2tpl:1:4: error: Unexpected statement: 'endblock'\n{% endblock %}{% endblock %}\n---^-------"},
                            InputOutputPair{"{% macro 10 %}{% endmacro %}",
                                            "noname.j2tpl:1:10: error: Identifier expected\n{% macro 10 %}{% endmacro %}\n      ---^-------"},
                            InputOutputPair{"{% macro name(10) %}{% endmacro %}",
                                            "noname.j2tpl:1:15: error: Identifier expected\n{% macro name(10) %}{% endmacro %}\n           ---^-------"},
                            InputOutputPair{"{% macro name(=10) %}{% endmacro %}",
                                            "noname.j2tpl:1:15: error: Identifier expected\n{% macro name(=10) %}{% endmacro %}\n           ---^-------"},
                            InputOutputPair{"{% macro name name1 %}{% endmacro %}",
                                            "noname.j2tpl:1:15: error: Unexpected token: 'name1'\n{% macro name name1 %}{% endmacro %}\n           ---^-------"},
                            InputOutputPair{"{% macro name() name1 %}{% endmacro %}",
                                            "noname.j2tpl:1:17: error: Expected end of statement, got: 'name1'\n{% macro name() name1 %}{% endmacro %}\n             ---^-------"},
                            InputOutputPair{"{% macro name(param) name1 %}{% endmacro %}",
                                            "noname.j2tpl:1:22: error: Expected end of statement, got: 'name1'\n{% macro name(param) name1 %}{% endmacro %}\n                  ---^-------"},
                            InputOutputPair{"{% macro name(param=*) %}{% endmacro %}",
                                            "noname.j2tpl:1:21: error: Unexpected token: '*'\n{% macro name(param=*) %}{% endmacro %}\n                 ---^-------"},
                            InputOutputPair{"{% block b %}{% endmacro %}",
                                            "noname.j2tpl:1:17: error: Unexpected statement: 'endmacro'\n{% block b %}{% endmacro %}\n             ---^-------"},
                            InputOutputPair{"{% call 10 %}{% endcall %}",
                                            "noname.j2tpl:1:9: error: Unexpected token: '10'\n{% call 10 %}{% endcall %}\n     ---^-------"},
                            InputOutputPair{"{% call name(=10) %}{% endcall %}",
                                            "noname.j2tpl:1:14: error: Unexpected token: '='\n{% call name(=10) %}{% endcall %}\n          ---^-------"},
                            InputOutputPair{"{% call(10) %}{% endcall %}",
                                            "noname.j2tpl:1:9: error: Identifier expected\n{% call(10) %}{% endcall %}\n     ---^-------"},
                            InputOutputPair{"{% call (=10) name %}{% endcall %}",
                                            "noname.j2tpl:1:10: error: Identifier expected\n{% call (=10) name %}{% endcall %}\n      ---^-------"},
                            InputOutputPair{"{% call name name1 %}{% endcall %}",
                                            "noname.j2tpl:1:14: error: Expected end of statement, got: 'name1'\n{% call name name1 %}{% endcall %}\n          ---^-------"},
                            InputOutputPair{"{% call name() name1 %}{% endcall %}",
                                            "noname.j2tpl:1:16: error: Expected end of statement, got: 'name1'\n{% call name() name1 %}{% endcall %}\n            ---^-------"},
                            InputOutputPair{"{% call name(param) name1 %}{% endcall %}",
                                            "noname.j2tpl:1:21: error: Expected end of statement, got: 'name1'\n{% call name(param) name1 %}{% endcall %}\n                 ---^-------"},
                            InputOutputPair{"{% call name(param=*) %}{% endcall %}",
                                            "noname.j2tpl:1:20: error: Unexpected token: '*'\n{% call name(param=*) %}{% endcall %}\n                ---^-------"},
                            InputOutputPair{"{% block b %}{% endcall %}",
                                            "noname.j2tpl:1:17: error: Unexpected statement: 'endcall'\n{% block b %}{% endcall %}\n             ---^-------"},
                            InputOutputPair{"{% do 'Hello World' %}",
                                            "noname.j2tpl:1:4: error: Extension disabled\n{% do 'Hello World' %}\n---^-------"},
                            InputOutputPair{"{% with %}{% endif }",
                                            "noname.j2tpl:1:9: error: Identifier expected\n{% with %}{% endif }\n     ---^-------"},
                            InputOutputPair{"{% with a %}{% endif }",
                                            "noname.j2tpl:1:11: error: Unexpected token '<<End of block>>'. Expected: '='\n{% with a %}{% endif }\n       ---^-------"},
                            InputOutputPair{"{% with a 42 %}{% endif }",
                                            "noname.j2tpl:1:11: error: Unexpected token '42'. Expected: '='\n{% with a 42 %}{% endif }\n       ---^-------"},
                            InputOutputPair{"{% with a = %}{% endif }",
                                            "noname.j2tpl:1:13: error: Unexpected token: '<<End of block>>'\n{% with a = %}{% endif }\n         ---^-------"},
                            InputOutputPair{"{% with a = 42 b = 30 %}{% endif }",
                                            "noname.j2tpl:1:16: error: Unexpected token 'b'. Expected: '<<End of block>>', ','\n{% with a = 42 b = 30 %}{% endif }\n            ---^-------"},
                            InputOutputPair{"{% with a = 42, %}{% endif }",
                                            "noname.j2tpl:1:22: error: Unexpected statement: 'endif'\n{% with a = 42, %}{% endif }\n                  ---^-------"},
// FIXME:                            InputOutputPair{"{% with a = 42 %}",
//                                            "noname.j2tpl:1:4: error: Extension disabled\n{% do 'Hello World' %}\n---^-------"},
                            InputOutputPair{"{% with a = 42 %}{% endfor %}",
                                            "noname.j2tpl:1:21: error: Unexpected statement: 'endfor'\n{% with a = 42 %}{% endfor %}\n                 ---^-------"},
                            InputOutputPair{"{% if a == 42 %}{% endwith %}",
                                            "noname.j2tpl:1:20: error: Unexpected statement: 'endwith'\n{% if a == 42 %}{% endwith %}\n                ---^-------"},
                            InputOutputPair{"{{}}",
                                            "noname.j2tpl:1:3: error: Unexpected token: '<<End of block>>'\n{{}}\n--^-------"}
                            ));

INSTANTIATE_TEST_CASE_P(ExtensionStatementsTest, ErrorsGenericExtensionsTest, ::testing::Values(
                            InputOutputPair{"{% do %}",
                                            "noname.j2tpl:1:7: error: Unexpected token: '<<End of block>>'\n{% do %}\n   ---^-------"},
                            InputOutputPair{"{% do 1 + %}",
                                            "noname.j2tpl:1:11: error: Unexpected token: '<<End of block>>'\n{% do 1 + %}\n       ---^-------"}
                            ));
