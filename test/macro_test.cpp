#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"
#include "test_tools.h"

using namespace jinja2;

TEST(MacroTest, SimpleMacro)
{
    std::string source = R"(
{% macro test %}
Hello World!
{% endmacro %}
{{ test() }}{{ test() }}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Hello World!
Hello World!

)";
    EXPECT_EQ(expectedResult, result);
}

TEST(MacroTest, OneParamMacro)
{
    std::string source = R"(
{% macro test(param) %}
-->{{ param }}<--
{% endmacro %}
{{ test('Hello') }}{{ test(param='World!') }}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
-->Hello<--
-->World!<--

)";
    EXPECT_EQ(expectedResult, result);
}

TEST(MacroTest, OneDefaultParamMacro)
{
    std::string source = R"(
{% macro test(param='Hello') %}
-->{{ param }}<--
{% endmacro %}
{{ test() }}{{ test('World!') }}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
-->Hello<--
-->World!<--

)";
    EXPECT_EQ(expectedResult, result);
}

TEST(MacroTest, ClosureMacro)
{
    std::string source = R"(
{% macro test1(param) %}-->{{ param('Hello World') }}<--{% endmacro %}
{% macro test(param1) %}
{% set var='Some Value' %}
{% macro inner1(msg) %}{{var ~ param1}} -> {{msg}}{% endmacro %}
{% macro inner2(msg) %}{{msg | upper}}{% endmacro %}
-->{{ test1(inner1) }}<--
-->{{ test1(inner2) }}<--
{% endmacro %}
{{ test() }}{{ test('World!') }}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
-->-->Some Value -> Hello World<--<--
-->-->HELLO WORLD<--<--
-->-->Some ValueWorld! -> Hello World<--<--
-->-->HELLO WORLD<--<--

)";
    EXPECT_EQ(expectedResult, result);
}

TEST(MacroTest, MacroVariables)
{
    std::string source = R"(
{% macro test(param1='Hello', param2, param3='World') %}
name: {{ name }}
arguments: {{ arguments | pprint }}
defaults: {{ defaults | pprint }}
varargs: {{ varargs | pprint }}
kwargs: {{ kwargs | pprint }}
{% endmacro %}
{{ test(1, 2, param3=3, 4, extraValue=5, 6) }}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
name: test
arguments: ['param1', 'param2', 'param3']
defaults: ['Hello', none, 'World']
varargs: [4, 6]
kwargs: {'extraValue': 5}

)";
    EXPECT_EQ(expectedResult, result);
}

TEST(MacroTest, SimpleCallMacro)
{
    std::string source = R"(
{% macro test %}
Hello World! -> {{ caller() }} <-
{% endmacro %}
{% call test %}Message from caller{% endcall %}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Hello World! -> Message from caller <-
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(MacroTest, CallWithParamsAndSimpleMacro)
{
    std::string source = R"(
{% macro test %}
-> {{ caller('Hello World' | upper) }} <-
{% endmacro %}
{% call(message) test %}{{ message }}{% endcall %}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
-> HELLO WORLD <-
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(MacroTest, CallWithParamsAndMacro)
{
    std::string source = R"(
{% macro test(msg) %}
{{ msg }} >>> -> {{ caller([msg]) }} <--> {{ caller([msg], 'upper') }} <-
{% endmacro %}
{% call(message, fName='lower') test('Hello World') %}{{ message | map(fName) | first }}{% endcall %}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Hello World >>> -> hello world <--> HELLO WORLD <-
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(MacroTest, MacroCallVariables)
{
    std::string source = R"(
{% macro invoke() %}{{ caller(1, 2, param3=3, 4, extraValue=5, 6) }}{% endmacro %}
{% call (param1='Hello', param2, param3='World') invoke %}
name: {{ name }}
arguments: {{ arguments | pprint }}
defaults: {{ defaults | pprint }}
varargs: {{ varargs | pprint }}
kwargs: {{ kwargs | pprint }}
{% endcall %}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
name: $call$
arguments: ['param1', 'param2', 'param3']
defaults: ['Hello', none, 'World']
varargs: [4, 6]
kwargs: {'extraValue': 5}
)";
    EXPECT_EQ(expectedResult, result);
}
