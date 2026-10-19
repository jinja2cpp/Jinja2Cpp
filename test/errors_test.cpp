#include "test_tools.h"

#include "../src/template_impl.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/exception.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <chrono>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <iterator>
#include <limits>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <system_error>
#include <type_traits>
#include <utility>
#include <vector>

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
    EXPECT_EQ(std::wstring(L"noname.j2tpl:1:4: error: Template environment doesn't set\n{% extends 'module' %}\n---^-------"), ErrorToString(parseResult.error()));

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
    ASSERT_TRUE(tpl2.Load(R"({{ foo() }})"));
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
    ASSERT_TRUE(tpl2.Load(LR"({{ foo() }})"));
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

// A call that does not fit its filter fails where the filter runs, as in Jinja2; the message
// is kept in the template's tree (0118 P5b)
TEST(ErrorsFilterArgumentsTest, ReportedAtRender)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% if false %}{{ x | int(1, 2, 3, 4) }}{% endif %}{{ x | default(1, a_long_keyword_name=2) }}"));
    auto result = tpl.RenderAsString({ { "x", 1 } });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("noname.j2tpl:1:1: error: Unexpected exception occurred during template processing. Exception: default() got an unexpected keyword argument 'a_long_keyword_name'\n", ErrorToString(result.error()));

    TemplateW wide;
    ASSERT_TRUE(wide.Load(L"{{ x | int(1, 2, 3, 4) }}"));
    EXPECT_FALSE(wide.RenderAsString({ { "x", 1 } }).has_value());
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

// A long source line is cut to a window around the error column (task 0050)
TEST(ErrorsLongLineTest, LocationWindow)
{
    auto check = [](const std::string& source, const std::string& expectedDescr) {
        Template tpl;
        auto parseResult = tpl.Load(source);
        ASSERT_FALSE(parseResult.has_value());
        EXPECT_EQ(expectedDescr, parseResult.error().GetLocationDescr());

        TemplateW tplW;
        auto parseResultW = tplW.Load(ConvertString<std::wstring>(source));
        ASSERT_FALSE(parseResultW.has_value());
        EXPECT_EQ(ConvertString<std::wstring>(expectedDescr), parseResultW.error().GetLocationDescr());
    };

    const std::string err = "{{ ) }}";
    // 160 characters is still shown whole
    check(std::string(153, 'x') + err, std::string(153, 'x') + err + "\n" + std::string(153, ' ') + "---^-------");
    // Error near the end: head cut, the window ends with the line
    check(std::string(200, 'x') + err, "..." + std::string(113, 'x') + err + "\n" + std::string(116, ' ') + "---^-------");
    // Error near the start: tail cut
    check(err + std::string(200, 'y'), err + std::string(113, 'y') + "...\n---^-------");
    // Error in the middle: both ends cut, 40 characters before the column
    check(std::string(200, 'x') + err + std::string(200, 'y'),
          "..." + std::string(37, 'x') + err + std::string(76, 'y') + "...\n" + std::string(40, ' ') + "---^-------");
    // Leading whitespace of a cut line is not copied
    check(std::string(200, ' ') + err + std::string(200, 'y'),
          "..." + std::string(37, ' ') + err + std::string(76, 'y') + "...\n" + std::string(40, ' ') + "---^-------");
}

// Many errors on one long line used to cost O(errors * line length): 20 000 took about 30 s
TEST(ErrorsLongLineTest, ManyErrorsOnOneLine)
{
    std::string source;
    for (int i = 0; i < 40000; ++i)
    {
        source += "{% ( %}";
    }

    const auto start = std::chrono::steady_clock::now();
    Template tpl;
    auto parseResult = tpl.Load(source);
    const auto elapsed = std::chrono::steady_clock::now() - start;
    ASSERT_FALSE(parseResult.has_value());
    EXPECT_EQ(ErrorCode::UnexpectedToken, parseResult.error().GetCode());
    EXPECT_EQ(1u, parseResult.error().GetErrorLocation().line);
    EXPECT_EQ(4u, parseResult.error().GetErrorLocation().col);
    // Generous for sanitizer and Debug builds; the quadratic version takes minutes
    EXPECT_LT(elapsed, std::chrono::seconds(10));
}

TEST_P(ErrorsGenericTest, Test)
{
    const auto& testParam = GetParam();
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
    const auto& testParam = GetParam();
    std::string source = testParam.tpl;

    TemplateEnv env;
    TemplateW tpl(&env);
    auto parseResult = tpl.Load(jinja2::ConvertString<std::wstring>(source));
    ASSERT_FALSE(parseResult.has_value());

    auto result = ErrorToString(parseResult.error());
    std::wcout << result << std::endl;
    auto expectedResult = jinja2::ConvertString<std::wstring>(testParam.result);
    EXPECT_EQ(expectedResult, result);
}

TEST_P(ErrorsGenericExtensionsTest, Test)
{
    const auto& testParam = GetParam();
    std::string source = testParam.tpl;

    TemplateEnv env;
    env.GetSettings().extensions.doStatement = true;

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
    const auto& testParam = GetParam();
    std::string source = testParam.tpl;

    TemplateEnv env;
    env.GetSettings().extensions.doStatement = true;

    TemplateW tpl(&env);
    auto parseResult = tpl.Load(jinja2::ConvertString<std::wstring>(source));
    ASSERT_FALSE(parseResult.has_value());

    auto result = ErrorToString(parseResult.error());
    std::wcout << result << std::endl;
    auto expectedResult = jinja2::ConvertString<std::wstring>(testParam.result);
    EXPECT_EQ(expectedResult, result);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(BasicTest, ErrorsGenericTest, ::testing::Values(
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
                                            "noname.j2tpl:1:3: error: Unexpected token '<<End of block>>'. Expected: '}}'\n{{\n--^-------"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(BasicExpressionsTest, ErrorsGenericTest, ::testing::Values(
                            InputOutputPair{"{{ * or }}",
                                            "noname.j2tpl:1:4: error: Unexpected token: '*'\n{{ * or }}\n---^-------"},
                            InputOutputPair{"{{ 1 + }}",
                                            "noname.j2tpl:1:8: error: Unexpected token: '<<End of block>>'\n{{ 1 + }}\n    ---^-------"},
                            InputOutputPair{"{{ + [1,,] }}",
                                            "noname.j2tpl:1:9: error: Unexpected token: ','\n{{ + [1,,] }}\n     ---^-------"},
                            InputOutputPair{"{{ 1 + 2 + [1,,]}}",
                                            "noname.j2tpl:1:15: error: Unexpected token: ','\n{{ 1 + 2 + [1,,]}}\n           ---^-------"},
                            InputOutputPair{"{{ 1 + 2 and [1,,]}}",
                                            "noname.j2tpl:1:17: error: Unexpected token: ','\n{{ 1 + 2 and [1,,]}}\n             ---^-------"},
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
                                            "noname.j2tpl:1:23: error: Unexpected token '}'. Expected: ':'\n{{ 1 if 2 == 3 else {1} }}\n                   ---^-------"},
                            InputOutputPair{"{{ range(1, 3} }}",
                                            "noname.j2tpl:1:14: error: ')' expected\n{{ range(1, 3} }}\n          ---^-------"},
                            InputOutputPair{"{{ {1, 3, 5] }}",
                                            "noname.j2tpl:1:6: error: Unexpected token ','. Expected: ':'\n{{ {1, 3, 5] }}\n  ---^-------"},
                            InputOutputPair{"{{ {'key'} }}",
                                            "noname.j2tpl:1:10: error: Unexpected token '}'. Expected: ':'\n{{ {'key'} }}\n      ---^-------"},
                            InputOutputPair{"{{ {'key':} }}",
                                            "noname.j2tpl:1:11: error: Expected expression, got: '}'\n{{ {'key':} }}\n       ---^-------"},
                            InputOutputPair{"{{ {'key':,} }}",
                                            "noname.j2tpl:1:11: error: Expected expression, got: ','\n{{ {'key':,} }}\n       ---^-------"},
                            InputOutputPair{"{{ {'key'=1} }}",
                                            "noname.j2tpl:1:10: error: Unexpected token '='. Expected: ':'\n{{ {'key'=1} }}\n      ---^-------"},
                            InputOutputPair{"{{ {=1} }}",
                                            "noname.j2tpl:1:5: error: Expected expression, got: '='\n{{ {=1} }}\n ---^-------"},
                            InputOutputPair{"{{ {'key':1] }}",
                                            "noname.j2tpl:1:12: error: '}' expected\n{{ {'key':1] }}\n        ---^-------"},
                            InputOutputPair{"{{ {'key':1,,} }}",
                                            "noname.j2tpl:1:13: error: Expected expression, got: ','\n{{ {'key':1,,} }}\n         ---^-------"},
                            InputOutputPair{"{{ [1, 3, 5} }}",
                                            "noname.j2tpl:1:12: error: ']' expected\n{{ [1, 3, 5} }}\n        ---^-------"},
                            InputOutputPair{"{{ 0x }}",
                                            "noname.j2tpl:1:3: error: Parse error\n{{ 0x }}\n--^-------"},
                            InputOutputPair{"{{ 0b12 }}",
                                            "noname.j2tpl:1:7: error: Unexpected token '2'. Expected: '<<End of block>>'\n{{ 0b12 }}\n   ---^-------"},
                            InputOutputPair{"{{ 0_1 }}",
                                            "noname.j2tpl:1:3: error: Parse error\n{{ 0_1 }}\n--^-------"},
                            InputOutputPair{"{{ 00_1 }}",
                                            "noname.j2tpl:1:3: error: Parse error\n{{ 00_1 }}\n--^-------"},
                            InputOutputPair{"{{ 01 }}",
                                            "noname.j2tpl:1:3: error: Parse error\n{{ 01 }}\n--^-------"},
                            InputOutputPair{"{{ 1e+ }}",
                                            "noname.j2tpl:1:3: error: Parse error\n{{ 1e+ }}\n--^-------"},
                            InputOutputPair{"{{ .5 }}",
                                            "noname.j2tpl:1:4: error: Unexpected token: '.'\n{{ .5 }}\n---^-------"},
                            InputOutputPair{"{{ x[8( }}",
                                            "noname.j2tpl:1:9: error: Unexpected token: '<<End of block>>'\n{{ x[8( }}\n     ---^-------"},
                            InputOutputPair{"{{ 1__0 }}",
                                            "noname.j2tpl:1:5: error: Unexpected token '__0'. Expected: '<<End of block>>'\n{{ 1__0 }}\n ---^-------"},
                            InputOutputPair{"{{ [1, 3,,] }}",
                                            "noname.j2tpl:1:10: error: Unexpected token: ','\n{{ [1, 3,,] }}\n      ---^-------"},
                            InputOutputPair{"{{ (1, 3, 5} }}",
                                            "noname.j2tpl:1:12: error: ')' expected\n{{ (1, 3, 5} }}\n        ---^-------"},
                            InputOutputPair{"{{ value.'10' }}",
                                            "noname.j2tpl:1:11: error: Identifier expected\n{{ value.'10' }}\n       ---^-------"},
                            InputOutputPair{"{{ value[1 2] }}",
                                            "noname.j2tpl:1:12: error: ']' expected\n{{ value[1 2] }}\n        ---^-------"},
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
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(StatementsTest_1, ErrorsGenericTest, ::testing::Values(
                            InputOutputPair{"{% if %}",
                                            "noname.j2tpl:1:7: error: Expected expression, got: '<<End of block>>'\n{% if %}\n   ---^-------"},
                            InputOutputPair{"{% endif %}",
                                            "noname.j2tpl:1:4: error: Unexpected statement: 'endif'\n{% endif %}\n---^-------"},
                            InputOutputPair{"{% endfor %}",
                                            "noname.j2tpl:1:4: error: Unexpected statement: 'endfor'\n{% endfor %}\n---^-------"},
                            InputOutputPair{"{% else hello %}",
                                            "noname.j2tpl:1:4: error: Unexpected statement: 'else'\n{% else hello %}\n---^-------"},
                            InputOutputPair{"{% if a %}{% else hello %}",
                                            "noname.j2tpl:1:19: error: Expected end of statement, got: 'hello'\n{% if a %}{% else hello %}\n               ---^-------"},
                            InputOutputPair{"{% elif %}",
                                            "noname.j2tpl:1:4: error: Unexpected statement: 'elif'\n{% elif %}\n---^-------"},
                            InputOutputPair{"{% if a %}{% elif %}",
                                            "noname.j2tpl:1:19: error: Expected expression, got: '<<End of block>>'\n{% if a %}{% elif %}\n               ---^-------"},
                            InputOutputPair{"{% for %}",
                                            "noname.j2tpl:1:8: error: Identifier expected\n{% for %}\n    ---^-------"},
                            InputOutputPair{"{% for 10 in range(10) %}",
                                            "noname.j2tpl:1:8: error: Identifier expected\n{% for 10 in range(10) %}\n    ---^-------"},
                            InputOutputPair{"{% for i, 10 range(10)%}",
                                            "noname.j2tpl:1:11: error: Unexpected token '10'. Expected: '<<Identifier>>', 'in', ','\n{% for i, 10 range(10)%}\n       ---^-------"},
                            InputOutputPair{"{% for i on range(10)%}",
                                            "noname.j2tpl:1:10: error: Unexpected token 'on'. Expected: '<<Identifier>>', 'in', ','\n{% for i on range(10)%}\n      ---^-------"},
                            InputOutputPair{"{% for i in range(10,)%}",
                                            "noname.j2tpl:1:25: error: Unexpected token '<<End of block>>'. Expected: 'endfor'\n{% for i in range(10,)%}\n                     ---^-------"},
                            InputOutputPair{"{% for i in range(10) rec%}",
                                            "noname.j2tpl:1:23: error: Unexpected token 'rec'. Expected: 'if', 'recursive', '<<End of block>>'\n{% for i in range(10) rec%}\n                   ---^-------"},
                            InputOutputPair{"{% for i in range(10) endfor%}",
                                            "noname.j2tpl:1:23: error: Unexpected token 'endfor'. Expected: 'if', 'recursive', '<<End of block>>'\n{% for i in range(10) endfor%}\n                   ---^-------"},
                            InputOutputPair{"{% for i in range(10) if {key} %}",
                                            "noname.j2tpl:1:30: error: Unexpected token '}'. Expected: ':'\n{% for i in range(10) if {key} %}\n                          ---^-------"},
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
                            InputOutputPair{"{% set 10%}",
                                            "noname.j2tpl:1:8: error: Identifier expected\n{% set 10%}\n    ---^-------"},
                            InputOutputPair{"{% set i = {key] %}",
                                            "noname.j2tpl:1:16: error: Unexpected token ']'. Expected: ':'\n{% set i = {key] %}\n            ---^-------"},
                            InputOutputPair{"{% set id=10%}\n{% endset %}",
                                            "noname.j2tpl:2:4: error: Unexpected statement: 'endset'\n{% endset %}\n---^-------"},
                            InputOutputPair{"{% extends %}",
                                            "noname.j2tpl:1:12: error: Unexpected token: '<<End of block>>'\n{% extends %}\n        ---^-------"},
                            InputOutputPair{"{% for i in x %}{% extends 'a' %}{% endfor %}",
                                            "noname.j2tpl:1:20: error: Unexpected statement: 'extends'\n{% for i in x %}{% extends 'a' %}{% endfor %}\n                ---^-------"},
                            InputOutputPair{R"({% extends "/_layouts/default.html" }
                                              {% block content %}
                                              {% endblock %}
                                              )",
                                            "noname.j2tpl:1:37: error: Expected end of statement, got: '}'\n{% extends \"/_layouts/default.html\" }\n                                 ---^-------"},
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
                            InputOutputPair{"{% from 'foo' import bar baz %}",
                                            "noname.j2tpl:1:26: error: Unexpected token 'baz'. Expected: '<<End of block>>', ',', 'with', 'without'\n{% from 'foo' import bar baz %}\n                      ---^-------"},
                            InputOutputPair{"{% from 'foo' import _bar %}",
                                            "noname.j2tpl:1:22: error: Unexpected token: '_bar'\n{% from 'foo' import _bar %}\n                  ---^-------"},
                            InputOutputPair{"{% block a %}{% endblock %}{% block a %}{% endblock %}",
                                            "noname.j2tpl:1:31: error: Unexpected statement: 'block'\n{% block a %}{% endblock %}{% block a %}{% endblock %}\n                           ---^-------"},
                            InputOutputPair{"{% block a required %}x{% endblock %}",
                                            "noname.j2tpl:1:23: error: Unexpected token: 'x'\n{% block a required %}x{% endblock %}\n                   ---^-------"},
                            InputOutputPair{"{% block a required %}{{ x }}{% endblock %}",
                                            "noname.j2tpl:1:25: error: Unexpected token: ' x '\n{% block a required %}{{ x }}{% endblock %}\n                     ---^-------"},
                            InputOutputPair{"{% block a required %}{% if x %}{% endif %}{% endblock %}",
                                            "noname.j2tpl:1:26: error: Unexpected statement: 'if'\n{% block a required %}{% if x %}{% endif %}{% endblock %}\n                      ---^-------"},
                            InputOutputPair{"{% from 'foo' import %}",
                                            "noname.j2tpl:1:22: error: Unexpected token '<<End of block>>'. Expected: '<<Identifier>>'\n{% from 'foo' import %}\n                  ---^-------"},
                            InputOutputPair{"{% from 'foo' import bar, %}",
                                            "noname.j2tpl:1:27: error: Unexpected token '<<End of block>>'. Expected: '<<Identifier>>'\n{% from 'foo' import bar, %}\n                       ---^-------"},
                            InputOutputPair{"{% from 'foo' import bar,, with context %}",
                                            "noname.j2tpl:1:26: error: Unexpected token ','. Expected: '<<Identifier>>'\n{% from 'foo' import bar,, with context %}\n                      ---^-------"},
                            InputOutputPair{"{% from 'foo' import bar with context, %}",
                                            "noname.j2tpl:1:38: error: Expected end of statement, got: ','\n{% from 'foo' import bar with context, %}\n                                  ---^-------"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(StatementsTest_2, ErrorsGenericTest, ::testing::Values(
                            InputOutputPair{"{% block %}",
                                            "noname.j2tpl:1:10: error: Identifier expected\n{% block %}\n      ---^-------"},
                            InputOutputPair{"{% block 10 %}",
                                            "noname.j2tpl:1:10: error: Identifier expected\n{% block 10 %}\n      ---^-------"},
                            InputOutputPair{"{% block a scp %}",
                                            "noname.j2tpl:1:12: error: Unexpected token 'scp'. Expected: '<<End of block>>'\n{% block a scp %}\n        ---^-------"},
                            InputOutputPair{"{% block somename %}{% endblock 10 %}",
                                            "noname.j2tpl:1:33: error: Expected end of statement, got: '10'\n{% block somename %}{% endblock 10 %}\n                             ---^-------"},
                            InputOutputPair{"{% endblock %}{% endblock %}",
                                            "noname.j2tpl:1:4: error: Unexpected statement: 'endblock'\n{% endblock %}{% endblock %}\n---^-------"},
                            InputOutputPair{"{% macro 10 %}{% endmacro %}",
                                            "noname.j2tpl:1:10: error: Identifier expected\n{% macro 10 %}{% endmacro %}\n      ---^-------"},
                            InputOutputPair{"{% macro name(10) %}{% endmacro %}",
                                            "noname.j2tpl:1:15: error: Identifier expected\n{% macro name(10) %}{% endmacro %}\n           ---^-------"},
                            InputOutputPair{"{% macro name(=10) %}{% endmacro %}",
                                            "noname.j2tpl:1:15: error: Identifier expected\n{% macro name(=10) %}{% endmacro %}\n           ---^-------"},
                            InputOutputPair{"{% macro name name1 %}{% endmacro %}",
                                            "noname.j2tpl:1:15: error: Unexpected token 'name1'. Expected: '('\n{% macro name name1 %}{% endmacro %}\n           ---^-------"},
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
                                            "noname.j2tpl:1:14: error: Unexpected token 'name1'. Expected: '('\n{% call name name1 %}{% endcall %}\n          ---^-------"},
                            InputOutputPair{"{% call name(caller=1) %}{% endcall %}",
                                            "noname.j2tpl:1:14: error: Unexpected token: 'caller'\n{% call name(caller=1) %}{% endcall %}\n          ---^-------"},
                            InputOutputPair{"{% call name(a=f(caller=7), caller=1) %}{% endcall %}",
                                            "noname.j2tpl:1:29: error: Unexpected token: 'caller'\n{% call name(a=f(caller=7), caller=1) %}{% endcall %}\n                         ---^-------"},
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
                            InputOutputPair{"{% with %}{% endif %}",
                                            "noname.j2tpl:1:14: error: Unexpected statement: 'endif'\n{% with %}{% endif %}\n          ---^-------"},
                            InputOutputPair{"{% with a %}{% endif %}",
                                            "noname.j2tpl:1:11: error: Unexpected token '<<End of block>>'. Expected: '='\n{% with a %}{% endif %}\n       ---^-------"},
                            InputOutputPair{"{% with a 42 %}{% endif %}",
                                            "noname.j2tpl:1:11: error: Unexpected token '42'. Expected: '='\n{% with a 42 %}{% endif %}\n       ---^-------"},
                            InputOutputPair{"{% with a = %}{% endif %}",
                                            "noname.j2tpl:1:13: error: Unexpected token: '<<End of block>>'\n{% with a = %}{% endif %}\n         ---^-------"},
                            InputOutputPair{"{% with a = 42 b = 30 %}{% endif %}",
                                            "noname.j2tpl:1:16: error: Unexpected token 'b'. Expected: '<<End of block>>', ','\n{% with a = 42 b = 30 %}{% endif %}\n            ---^-------"},
                            InputOutputPair{"{% with a = 42, %}{% endif %}",
                                            "noname.j2tpl:1:22: error: Unexpected statement: 'endif'\n{% with a = 42, %}{% endif %}\n                  ---^-------"},
// FIXME:                            InputOutputPair{"{% with a = 42 %}",
//                                            "noname.j2tpl:1:4: error: Extension disabled\n{% do 'Hello World' %}\n---^-------"},
                            InputOutputPair{"{% with a = 42 %}{% endfor %}",
                                            "noname.j2tpl:1:21: error: Unexpected statement: 'endfor'\n{% with a = 42 %}{% endfor %}\n                 ---^-------"},
                            InputOutputPair{"{% if a == 42 %}{% endwith %}",
                                            "noname.j2tpl:1:20: error: Unexpected statement: 'endwith'\n{% if a == 42 %}{% endwith %}\n                ---^-------"},
                            InputOutputPair{"{{}}",
                                            "noname.j2tpl:1:3: error: Unexpected token: '<<End of block>>'\n{{}}\n--^-------"},
                            InputOutputPair{"{% raw %}{% raw %}{{ x }{% endraw %}{% endraw %}",
                                            "noname.j2tpl:1:37: error: Unexpected raw block end\n{% raw %}{% raw %}{{ x }{% endraw %}{% endraw %}\n                                 ---^-------"},
                            InputOutputPair{"{% raw %}",
                                            "noname.j2tpl:1:10: error: Expected end of raw block\n{% raw %}\n      ---^-------"},
                            InputOutputPair{"{{ 2 + 3 + {% raw %} }}", "noname.j2tpl:1:13: error: Expected expression, got: '%'\n{{ 2 + 3 + {% raw %} }}\n         ---^-------" },
    InputOutputPair{ "{% meta %}", "noname.j2tpl:1:11: error: Expected end of meta block\n{% meta %}\n       ---^-------" },
    InputOutputPair{ "{% endmeta %}", "noname.j2tpl:1:1: error: Unexpected meta block end\n{% endmeta %}\n^-------" }));

INSTANTIATE_TEST_SUITE_P(ExtensionStatementsTest, ErrorsGenericExtensionsTest, ::testing::Values(
                            InputOutputPair{"{% do %}",
                                            "noname.j2tpl:1:7: error: Unexpected token: '<<End of block>>'\n{% do %}\n   ---^-------"},
                            InputOutputPair{"{% do 1 + %}",
                                            "noname.j2tpl:1:11: error: Unexpected token: '<<End of block>>'\n{% do 1 + %}\n       ---^-------"}
                            ));
// clang-format on

// The error contract (docs/api-2.0.md 2.1, task 0253): types, codes and the Load/Render boundary
static_assert(static_cast<int>(ErrorCode::UserCallableException) == 15);
static_assert(static_cast<int>(ErrorCode::IntegerOutOfRange) == 16);
static_assert(static_cast<int>(ErrorCode::TemplateRuntimeError) == 17);
static_assert(static_cast<int>(ErrorCode::ResourceLimitExceeded) == 18);
static_assert(sizeof(ErrorInfo) == sizeof(void*));
static_assert(sizeof(ErrorInfoW) == sizeof(void*));
static_assert(std::is_nothrow_default_constructible_v<ErrorInfo>);
static_assert(std::is_nothrow_copy_constructible_v<ErrorInfo>);
static_assert(std::is_nothrow_move_constructible_v<ErrorInfo>);
static_assert(std::is_nothrow_copy_assignable_v<ErrorInfo>);
static_assert(std::is_nothrow_move_assignable_v<ErrorInfo>);
static_assert(noexcept(std::declval<const ErrorInfo&>().GetCode()));
static_assert(noexcept(std::declval<const ErrorInfo&>().GetErrorLocation()));
static_assert(noexcept(std::declval<const ErrorInfo&>().GetRelatedLocations()));
static_assert(noexcept(std::declval<const ErrorInfo&>().GetExtraParams()));
static_assert(noexcept(std::declval<const ErrorInfo&>().GetLocationDescr()));
static_assert(noexcept(std::declval<const ErrorInfo&>().GetException()));
static_assert(std::is_base_of_v<std::runtime_error, jinja2::Exception>);
static_assert(std::is_nothrow_copy_constructible_v<jinja2::Exception>);
static_assert(noexcept(Value(int{})));
static_assert(!noexcept(Value(uint64_t{})));
static_assert(std::is_error_code_enum_v<ErrorCode>);
static_assert(std::is_same_v<BadResultAccess<>, nonstd::bad_expected_access<ErrorInfo>>);
static_assert(std::is_same_v<BadResultAccess<wchar_t>, nonstd::bad_expected_access<ErrorInfoW>>);

namespace
{
template<typename Fn>
Result<std::string> RenderCalling(Fn&& fn)
{
    Template tpl;
    EXPECT_TRUE(tpl.Load("a{{ f() }}b").has_value());
    return tpl.RenderAsString({ { "f", MakeCallable(std::forward<Fn>(fn)) } });
}

struct HostError
{
    int value;
};
} // namespace

TEST(ErrorContractTest, NonStdExceptionFromCallableIsKept)
{
    auto result = RenderCalling([]() -> Value { throw HostError{ 42 }; });
    ASSERT_FALSE(result.has_value());
    const auto& error = result.error();
    EXPECT_EQ(ErrorCode::UnexpectedException, error.GetCode());
    EXPECT_EQ("unknown exception", error.GetExtraParams().at(0).AsString());
    ASSERT_TRUE(error.GetException());
    try
    {
        std::rethrow_exception(error.GetException());
    }
    catch (const HostError& e)
    {
        EXPECT_EQ(42, e.value);
    }
}

TEST(ErrorContractTest, StdExceptionFromCallableIsKept)
{
    auto result = RenderCalling([]() -> Value { throw std::invalid_argument("bad"); });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::UnexpectedException, result.error().GetCode());
    EXPECT_EQ("bad", result.error().GetExtraParams().at(0).AsString());
    EXPECT_THROW(std::rethrow_exception(result.error().GetException()), std::invalid_argument);
}

TEST(ErrorContractTest, BadAllocFromCallablePropagates)
{
    EXPECT_THROW(static_cast<void>(RenderCalling([]() -> Value { throw std::bad_alloc(); })), std::bad_alloc);
}

TEST(ErrorContractTest, LengthErrorIsResourceLimit)
{
    auto result = RenderCalling([]() -> Value { throw std::length_error("too long"); });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
    EXPECT_TRUE(result.error().GetException());
}

TEST(ErrorContractTest, HostExceptionCodePassesThrough)
{
    auto result = RenderCalling([]() -> Value { throw jinja2::Exception(ErrorCode::UndefinedError, "'x' is undefined"); });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::UndefinedError, result.error().GetCode());
    EXPECT_EQ("'x' is undefined", result.error().GetExtraParams().at(0).AsString());
    EXPECT_THROW(std::rethrow_exception(result.error().GetException()), jinja2::Exception);

    result = RenderCalling([]() -> Value { throw jinja2::Exception(ErrorCode::ResourceLimitExceeded, "budget spent"); });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
}

TEST(ErrorContractTest, ThrowingListAccessorIsKept)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for i in xs %}{{ i }}{% endfor %}").has_value());
    auto list = MakeGenericList([n = 0]() mutable -> std::optional<Value> {
        if (n++ == 1)
        {
            throw std::runtime_error("accessor failed");
        }
        return Value(n);
    });
    auto result = tpl.RenderAsString({ { "xs", list } });
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::UnexpectedException, result.error().GetCode());
    EXPECT_EQ("accessor failed", result.error().GetExtraParams().at(0).AsString());
    EXPECT_TRUE(result.error().GetException());
}

TEST(ErrorContractTest, ExceptionRecordsOrigin)
{
    const unsigned line = __LINE__ + 1;
    const jinja2::Exception ex(ErrorCode::IntegerOutOfRange, std::string("big"));
    EXPECT_EQ(ErrorCode::IntegerOutOfRange, ex.Code());
    EXPECT_STREQ("big", ex.what());
    EXPECT_EQ(line, ex.Line());
    EXPECT_NE(nullptr, std::strstr(ex.File(), "errors_test.cpp")) << ex.File();
}

// A big integer is stored, not refused; one that reaches a template (until docs/tasks/0270) is an
// error the engine raises, so it carries no exception
TEST(ErrorContractTest, BigIntInTemplateIsIntegerOutOfRange)
{
    const auto big = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1;
    EXPECT_TRUE(Value(big).IsBigInt());

    const std::pair<const char*, Value> rows[] = { { "{{ n + 1 }}", Value(big) }, { "{{ n[0] }}", Reflect(std::vector<uint64_t>{ big }) } };
    for (const auto& [source, n] : rows)
    {
        Template tpl;
        ASSERT_TRUE(tpl.Load(source));
        const auto result = tpl.RenderAsString({ { "n", n } });
        ASSERT_FALSE(result.has_value()) << source;
        EXPECT_EQ(ErrorCode::IntegerOutOfRange, result.error().GetCode()) << source;
        EXPECT_FALSE(result.error().GetException()) << source;
    }
}

TEST(ErrorContractTest, ErrorCodeInterop)
{
    std::error_code ec = ErrorCode::TemplateNotFound;
    EXPECT_EQ(&jinja2::error_category(), &ec.category());
    EXPECT_STREQ("jinja2cpp", ec.category().name());
    EXPECT_EQ(6, ec.value());
    EXPECT_EQ("template not found", ec.message());
    EXPECT_TRUE(ec == std::errc::no_such_file_or_directory);
    EXPECT_TRUE(std::error_code(ErrorCode::FileNotFound) == std::errc::no_such_file_or_directory);
    EXPECT_TRUE(std::error_code(ErrorCode::ResourceLimitExceeded) == std::errc::value_too_large);
    EXPECT_TRUE(std::error_code(ErrorCode::IntegerOutOfRange) == std::errc::value_too_large);
    EXPECT_TRUE(std::error_code(ErrorCode::InvalidTemplateName) == std::errc::invalid_argument);
    EXPECT_FALSE(std::error_code(ErrorCode::UndefinedError) == std::errc::invalid_argument);
    EXPECT_TRUE(std::error_code(ErrorCode::UndefinedError) == std::error_code(ErrorCode::UndefinedError).default_error_condition());
    EXPECT_EQ("integer out of range", std::error_code(ErrorCode::IntegerOutOfRange).message());
}

TEST(ErrorContractTest, ErrorInfoSharesItsPayload)
{
    const ErrorInfo empty;
    EXPECT_EQ(ErrorCode::Unspecified, empty.GetCode());
    EXPECT_TRUE(empty.GetExtraParams().empty());
    EXPECT_TRUE(empty.GetLocationDescr().empty());
    EXPECT_FALSE(empty.GetException());

    ErrorInfo::Data data;
    data.code = ErrorCode::UnexpectedException;
    data.srcLoc = { "t.j2", 2, 3 };
    data.relatedLocs.push_back({ "u.j2", 4, 5 });
    data.extraParams.emplace_back(std::string("boom"));
    data.exception = std::make_exception_ptr(HostError{ 7 });
    ErrorInfo error(std::move(data));

    ErrorInfo copy = error;
    EXPECT_EQ(&error.GetExtraParams(), &copy.GetExtraParams());
    EXPECT_EQ(error.GetException(), copy.GetException());
    ErrorInfo moved = std::move(copy);
    EXPECT_EQ(ErrorCode::Unspecified, copy.GetCode()); // NOLINT(bugprone-use-after-move)
    EXPECT_EQ(error.GetException(), moved.GetException());
    copy = moved;
    EXPECT_EQ(2u, copy.GetErrorLocation().line);
    moved = ErrorInfo();
    EXPECT_EQ(ErrorCode::UnexpectedException, copy.GetCode());
    auto& self = copy;
    copy = std::move(self);
    EXPECT_EQ(ErrorCode::UnexpectedException, copy.GetCode());

    // The wide conversion keeps every field
    auto wide = ErrorConverter<ErrorInfoW, ErrorInfo>::Convert(error);
    EXPECT_EQ(ErrorCode::UnexpectedException, wide.GetCode());
    ASSERT_EQ(1u, wide.GetRelatedLocations().size());
    EXPECT_EQ("u.j2", wide.GetRelatedLocations()[0].fileName);
    EXPECT_EQ(error.GetException(), wide.GetException());
}

TEST(ErrorContractTest, LoadFromMissingFileFails)
{
    Template tpl;
    auto result = tpl.LoadFromFile("no-such-dir/no-such-template.j2");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::FileNotFound, result.error().GetCode());
    EXPECT_EQ("no-such-dir/no-such-template.j2", result.error().GetExtraParams().at(0).AsString());

    TemplateW tplW;
    auto resultW = tplW.LoadFromFile("no-such-dir/no-such-template.j2");
    ASSERT_FALSE(resultW.has_value());
    EXPECT_EQ(ErrorCode::FileNotFound, resultW.error().GetCode());
}

namespace
{
class ThrowingFileSystem : public IFilesystemHandler
{
public:
    [[nodiscard]] CharFileStreamPtr OpenStream(const std::string& /*name*/) const override { throw std::runtime_error("disk on fire"); }
    [[nodiscard]] WCharFileStreamPtr OpenWStream(const std::string& /*name*/) const override { throw HostError{ 1 }; }
    [[nodiscard]] std::optional<std::chrono::system_clock::time_point> GetLastModificationDate(const std::string& /*name*/) const override
    {
        return {};
    }
};
} // namespace

TEST(ErrorContractTest, ThrowingFilesystemHandlerIsKept)
{
    TemplateEnv env;
    env.AddFilesystemHandler(std::string(), std::make_shared<ThrowingFileSystem>());
    auto result = env.LoadTemplate("a.j2");
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::UnexpectedException, result.error().GetCode());
    EXPECT_EQ("disk on fire", result.error().GetExtraParams().at(0).AsString());
    EXPECT_THROW(std::rethrow_exception(result.error().GetException()), std::runtime_error);

    auto resultW = env.LoadTemplateW("a.j2");
    ASSERT_FALSE(resultW.has_value());
    EXPECT_EQ(ErrorCode::UnexpectedException, resultW.error().GetCode());
    EXPECT_THROW(std::rethrow_exception(resultW.error().GetException()), HostError);

    // Through an include, at render time
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% include 'a.j2' %}").has_value());
    auto rendered = tpl.RenderAsString({});
    ASSERT_FALSE(rendered.has_value());
    EXPECT_TRUE(rendered.error().GetException());
}

TEST(ErrorContractTest, ToCharsReportsHostFailures)
{
    auto list = MakeGenericList([]() -> std::optional<Value> { throw std::runtime_error("accessor failed"); });
    const Value value(list);
    char buffer[32];
    const auto result = value.ToChars(std::begin(buffer), std::end(buffer));
    EXPECT_EQ(std::errc::invalid_argument, result.ec);
}
