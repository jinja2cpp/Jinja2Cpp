#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <string>

using namespace jinja2;

using MacroTest = BasicTemplateRenderer;

MULTISTR_TEST(MacroTest, SimpleMacro,
              R"(
{% macro test() %}
Hello World!
{% endmacro %}
{{ test() }}{{ test() }}
)",
              //-------------
              R"(


Hello World!

Hello World!
)")
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
)")
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest,
              OneParamRecursiveMacro,
              R"(
{% macro fib(param) %}{{ 1 if param == 1 else (fib(param - 1) | int + param) }}{% endmacro %}
{{ fib(10) }}
)",
              //-----------
              R"(

55)")
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
)")
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
)")
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, MacroVariables,
              R"(
{% macro test(param1, param2, param3='World') %}
name: {{ test.name }}
arguments: {{ test.arguments | pprint }}
varargs: {{ varargs | pprint }}
kwargs: {{ kwargs | pprint }}
{% endmacro %}
{{ test(1, 2, 3, 4, 6, extraValue=5) }}
)",
              //-----------
              R"(


name: test
arguments: ['param1', 'param2', 'param3']
varargs: [4, 6]
kwargs: {'extraValue': 5}
)")
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, SimpleCallMacro,
              R"(
{% macro test() %}
Hello World! -> {{ caller() }} <-
{% endmacro %}
{% call test() %}Message from caller{% endcall %}
)",
              //-----------------
              R"(


Hello World! -> Message from caller <-
)")
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, CallWithParamsAndSimpleMacro,
              R"(
{% macro test() %}
-> {{ caller('Hello World' | upper) }} <-
{% endmacro %}
{% call(message) test() %}{{ message }}{% endcall %}
)",
              //------------
              R"(


-> HELLO WORLD <-
)")
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
)")
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, MacroCallVariables,
              R"(
{% macro invoke() %}
arguments: {{ caller.arguments | pprint }}{{ caller(1, 2, 3, 4, 6, extraValue=5) }}{% endmacro %}
{% call (param1, param2, param3='World') invoke() %}
varargs: {{ varargs | pprint }}
kwargs: {{ kwargs | pprint }}
{% endcall %}
)",
              //--------------
              R"(


arguments: ['param1', 'param2', 'param3']
varargs: [4, 6]
kwargs: {'extraValue': 5}
)")
{
    params = PrepareTestData();
}

// Arguments bound in slots (0117 P2): a `set` in the body hides the argument from then on
MULTISTR_TEST(MacroTest, ParamReassignedBySet, R"({% macro m(a) %}{{ a }}{% set a = a + 1 %}{{ a }}{% endmacro %}{{ m(1) }}{{ m(5) }})",
              //--------------
              R"(1256)")
{
    params = PrepareTestData();
}

// A nested macro reads the outer one's argument by name, from the outer call
MULTISTR_TEST(MacroTest, NestedMacroReadsOuterParam, R"({% macro outer(a) %}{% macro inner(b) %}{{ a }}{{ b }}{% endmacro %}{{ inner(a + 1) }}{% endmacro %}{{ outer(1) }}{{ outer(3) }})",
              //--------------
              R"(1234)")
{
    params = PrepareTestData();
}

// Each call has a frame of its own: the arguments of the outer calls survive the inner ones
MULTISTR_TEST(MacroTest, RecursiveMacroKeepsParams, R"({% macro f(n) %}{{ n }}{% if n > 0 %}[{{ f(n - 1) }}]{{ n }}{% endif %}{% endmacro %}{{ f(2) }})",
              //--------------
              R"(2[1[0]1]2)")
{
    params = PrepareTestData();
}

// The call body sees the calling macro's argument and its own, where it is defined, not the
// called macro's (docs/tasks/0165)
MULTISTR_TEST(MacroTest, CallBodyParamsAndCallerParams, R"({% macro m(p) %}{{ p }}{{ caller(p * 2) }}{{ p }}{% endmacro %}{% macro w(a) %}{% call(x) m(a + 1) %}{{ a }}{{ x }}{% endcall %}{% endmacro %}{{ w(1) }})",
              //--------------
              R"(2142)")
{
    params = PrepareTestData();
}

// A loop target named like an argument hides it only inside the loop; defaults see the arguments
MULTISTR_TEST(MacroTest, LoopTargetShadowsParam, R"({% macro m(a, b=a) %}{% for a in [7, 8] %}{{ a }}{{ b }}{% endfor %}{{ a }}{% endmacro %}{{ m(1) }})",
              //--------------
              R"(71811)")
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, KeywordArgsAndSpecialNames, R"({% macro m(a, b, c=3) %}{{ a }}{{ b }}{{ c }}{{ varargs | length }}{{ kwargs | length }}{% endmacro %}{{ m(b=2, a=1) }}{{ m(1, 2, 4, 5, d=6) }})",
              //--------------
              R"(1230012411)")
{
    params = PrepareTestData();
}

// Bodies resolve names where they are defined, not through their caller (docs/tasks/0165)
MULTISTR_TEST(MacroTest, NestedMacroReadsItsDefinitionSite, R"({% macro o(a) %}{% macro i() %}{{ a }}{% endmacro %}{% for a in [9] %}{{ i() }}{% endfor %}{{ i() }}{% endmacro %}{{ o(1) }}{{ o(2) }})",
              //--------------
              R"(1122)")
{
    params = PrepareTestData();
}

MULTISTR_TEST(MacroTest, MacroCalledFromAFilterBlock, R"({% set g = 'g' %}{% macro m() %}{{ g }}{% endmacro %}{% for g in ['L'] %}{% filter upper %}{{ m() }}{% endfilter %}{% endfor %})",
              //--------------
              R"(G)")
{
    params = PrepareTestData();
}

// A user callable's `*context` is the body's: what the macro sees where it is defined
MULTISTR_TEST(MacroTest, ContextCallableInABodySeesTheDefinitionSite, R"({% set g = 'G' %}{% macro m() %}{{ lookup(name='g') }}{% endmacro %}{% for g in ['L'] %}{{ m() }}{{ lookup(name='g') }}{% endfor %})",
              //--------------
              R"(GL)")
{
    params = PrepareTestData();
    params["lookup"] = jinja2::MakeCallable([](const jinja2::GenericMap& context, const std::string& name) { return context.GetValueByName(name); },
                                            jinja2::ArgInfo{ "*context" },
                                            jinja2::ArgInfo{ "name" });
}

// A recursive call does not see the names the outer call set
MULTISTR_TEST(MacroTest, RecursiveCallHidesTheOuterCallsSet, R"({% macro m(n) %}{% if n %}{% set v = 'A' %}{% endif %}[{{ v }}]{% if n %}{{ m(0) }}{% endif %}{% endmacro %}{{ m(1) }})",
              //--------------
              R"([A][])")
{
    params = PrepareTestData();
}

// A body that hands the context to a filter reads names by lookup: a call of it made inside a
// loop that cached the loop's names takes its own lookup cache epoch
TEST(MacroContextTest, ContextFilterInABodyCalledInsideALoop)
{
    TemplateEnv env;
    env.AddFilter("lookup", MakeCallable([](const std::string& name, const GenericMap& context) { return context.GetValueByName(name); }, ArgInfo{ "name" }, ArgInfo{ "*context" }));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set g = 'G' %}{% macro m(a) %}{{ a }}{{ a|lookup }}{% endmacro %}{% for g in ['L', 'M'] %}{{ g }}{{ m('g') }};{% endfor %}").has_value());
    EXPECT_EQ("LgG;MgG;", tpl.RenderAsString({}).value());
}

// A body that reads only its arguments takes no epoch of its own, called inside a loop that
// cached names of the same spelling
TEST(MacroContextTest, ArgumentsOnlyBodyCalledInsideALoop)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% macro m(x, y) %}[{{ x }}{% if y %}{{ y }}{% endif %}]{% endmacro %}{% for x in [1, 2] %}{% set y = 5 %}{{ x }}{{ y }}{{ m(x * 10) }}{{ m(x, y=x) }}{% endfor %}").has_value());
    EXPECT_EQ("15[10][11]25[20][22]", tpl.RenderAsString({}).value());
}
