#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"
#include "test_tools.h"

using namespace jinja2;

using MacroTest = BasicTemplateRenderer;

MULTISTR_TEST(MacroTest, SimpleMacro,
R"(
{% macro test %}
Hello World!
{% endmacro %}
{{ test() }}{{ test() }}
)",
//-------------
R"(
Hello World!
Hello World!

)"
)
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, OneParamMacro,
R"(
{% macro test(param) %}
-->{{ param }}<--
{% endmacro %}
{{ test('Hello') }}{{ test(param='World!') }}
)",
//-----------
R"(
-->Hello<--
-->World!<--

)"
)
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, OneDefaultParamMacro,
R"(
{% macro test(param='Hello') %}
-->{{ param }}<--
{% endmacro %}
{{ test() }}{{ test('World!') }}
)",
//--------------
R"(
-->Hello<--
-->World!<--

)"
)
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, ClosureMacro,
R"(
{% macro test1(param) %}-->{{ param('Hello World') }}<--{% endmacro %}
{% macro test(param1) %}
{% set var='Some Value' %}
{% macro inner1(msg) %}{{var ~ param1}} -> {{msg}}{% endmacro %}
{% macro inner2(msg) %}{{msg | upper}}{% endmacro %}
-->{{ test1(inner1) }}<--
-->{{ test1(inner2) }}<--
{% endmacro %}
{{ test() }}{{ test('World!') }}
)",
//-----------
R"(
-->-->Some Value -> Hello World<--<--
-->-->HELLO WORLD<--<--
-->-->Some ValueWorld! -> Hello World<--<--
-->-->HELLO WORLD<--<--

)"
)
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, MacroVariables,
R"(
{% macro test(param1='Hello', param2, param3='World') %}
name: {{ name }}
arguments: {{ arguments | pprint }}
defaults: {{ defaults | pprint }}
varargs: {{ varargs | pprint }}
kwargs: {{ kwargs | pprint }}
{% endmacro %}
{{ test(1, 2, param3=3, 4, extraValue=5, 6) }}
)",
//-----------
R"(
name: test
arguments: ['param1', 'param2', 'param3']
defaults: ['Hello', none, 'World']
varargs: [4, 6]
kwargs: {'extraValue': 5}

)"
)
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, SimpleCallMacro,
R"(
{% macro test %}
Hello World! -> {{ caller() }} <-
{% endmacro %}
{% call test %}Message from caller{% endcall %}
)",
//-----------------
R"(
Hello World! -> Message from caller <-
)"
)
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, CallWithParamsAndSimpleMacro,
R"(
{% macro test %}
-> {{ caller('Hello World' | upper) }} <-
{% endmacro %}
{% call(message) test %}{{ message }}{% endcall %}
)",
//------------
R"(
-> HELLO WORLD <-
)"
)
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, CallWithParamsAndMacro,
R"(
{% macro test(msg) %}
{{ msg }} >>> -> {{ caller([msg]) }} <--> {{ caller([msg], 'upper') }} <-
{% endmacro %}
{% call(message, fName='lower') test('Hello World') %}{{ message | map(fName) | first }}{% endcall %}
)",
//-------------
R"(
Hello World >>> -> hello world <--> HELLO WORLD <-
)"
)
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, MacroCallVariables,
R"(
{% macro invoke() %}{{ caller(1, 2, param3=3, 4, extraValue=5, 6) }}{% endmacro %}
{% call (param1='Hello', param2, param3='World') invoke %}
name: {{ name }}
arguments: {{ arguments | pprint }}
defaults: {{ defaults | pprint }}
varargs: {{ varargs | pprint }}
kwargs: {{ kwargs | pprint }}
{% endcall %}
)",
//--------------
R"(
name: $call$
arguments: ['param1', 'param2', 'param3']
defaults: ['Hello', none, 'World']
varargs: [4, 6]
kwargs: {'extraValue': 5}
)"
)
{
    params = PrepareTestData();
}
