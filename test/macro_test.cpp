#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/exception.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <exception>
#include <memory>
#include <stdexcept>
#include <string>
#include <string_view>

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
arguments: ('param1', 'param2', 'param3')
varargs: (4, 6)
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


arguments: ('param1', 'param2', 'param3')
varargs: (4, 6)
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

// A kept closure keeps the names it reads (docs/tasks/0375): `applymacro` names a macro by a
// string, directly, through `map`, or from a value, and so reads them all
MULTISTR_TEST(MacroTest, KeptClosureAppliesMacroByName, R"({% set ns = namespace(f=none) %}{% macro o(a) %}{% macro s(v) %}<{{ v }}{{ a }}>{% endmacro %}{% macro k() %}{{ 1|applymacro(macro='s') }}{{ [2]|map('applymacro', macro='s')|join }}{{ [3]|map('applymacro', macro=a)|join }}{% endmacro %}{% set ns.f = k %}{% endmacro %}{{ o('s') }}{{ ns.f() }})",
              //--------------
              R"(<1s><2s><3s>)")
{
    params = PrepareTestData();
}

// A recursive call does not see the names the outer call set
MULTISTR_TEST(MacroTest, RecursiveCallHidesTheOuterCallsSet, R"({% macro m(n) %}{% if n %}{% set v = 'A' %}{% endif %}[{{ v }}]{% if n %}{{ m(0) }}{% endif %}{% endmacro %}{{ m(1) }})",
              //--------------
              R"([A][])")
{
    params = PrepareTestData();
}

// Every default is evaluated by each call (docs/tasks/0166): a list default is a new list
// each time, even one the template folds to a constant or builds from a name
MULTISTR_TEST(MacroTest, MutableDefaultIsFreshPerCall, R"({% macro m(l=[], k=[x]) %}{% set _ = l.append(1) %}{% set _ = k.append(2) %}{{ l }}{{ k }}{% endmacro %}{% set x = 0 %}{{ m() }}{{ m() }})",
              //--------------
              R"([1][0, 2][1][0, 2])")
{
    params = PrepareTestData();
}

// A default that calls a macro runs once per call that needs it, and never at definition
MULTISTR_TEST(MacroTest, DefaultCallingAMacroRunsPerCall, R"({% set ns = namespace(n=0) %}{% macro bump() %}{% set ns.n = ns.n + 1 %}{{ ns.n }}{% endmacro %}{% macro m(a=bump()) %}{{ a }}{% endmacro %}{{ ns.n }}{{ m() }}{{ m('x') }}{{ m() }}{{ ns.n }})",
              //--------------
              R"(01x22)")
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

// A StopIteration out of a call the template makes gives undefined, as Jinja2's Context.call makes it
// (docs/tasks/0479). Each expectation is Jinja2 3.1.6's, rendered with the filters `stop` and `fail` (raising
// StopIteration and ValueError), the test `stop`, the globals `f` and `stopfn`, and the templates `inc`, `imp` and
// `bad`; an error row names the code the failure maps to
namespace
{
struct TestStop
    : std::runtime_error
    , StopIteration
{
    TestStop()
        : std::runtime_error("stop")
    {
    }
};

struct StopCase
{
    const char* source;
    UndefinedPolicy policy;
    const char* expected;
};

void FillStopEnv(TemplateEnv& env)
{
    auto files = std::make_shared<MemoryFileSystem>();
    files->AddFile("inc", "{{ 1|stop }}");
    files->AddFile("imp", "{% macro y() %}a{{ 1|stop }}b{% endmacro %}");
    files->AddFile("bad", "{{ 1|stop }}");
    env.AddFilesystemHandler({}, files);
    env.GetSettings().extensions.loopControls = true;
    env.AddFilter("stop", MakeCallable([](const Value&) -> Value { throw TestStop(); }, ArgInfo{ "v" }));
    env.AddFilter("fail", MakeCallable([](const Value&) -> Value { throw std::runtime_error("boom"); }, ArgInfo{ "v" }));
    env.AddTest("stop", MakeCallable([](const Value&) -> Value { throw TestStop(); }, ArgInfo{ "v" }));
    env.AddGlobal("f", MakeCallable([](const Value&) { return std::string("F"); }, ArgInfo{ "v" }));
    env.AddGlobal("stopfn", MakeCallable([]() -> Value { throw TestStop(); }));
}

template<typename Tpl, typename CharT>
void CheckStopCase(const StopCase& param)
{
    TemplateEnv env;
    FillStopEnv(env);
    env.GetSettings().undefinedPolicy = param.policy;
    Tpl tpl(&env);
    ASSERT_TRUE(tpl.Load(ConvertString<std::basic_string<CharT>>(std::string(param.source)))) << param.source;
    auto result = tpl.RenderAsString(ValuesMap{});
    const std::string expected = param.expected;
    if (expected.rfind("error: ", 0) == 0)
    {
        ASSERT_FALSE(result.has_value()) << param.source;
        const auto code = result.error().GetCode();
        const std::string name = expected.substr(7);
        if (name == "UndefinedError")
        {
            EXPECT_EQ(ErrorCode::UndefinedError, code) << param.source;
        }
        else if (name == "TemplateRuntimeError")
        {
            EXPECT_EQ(ErrorCode::TemplateRuntimeError, code) << param.source;
        }
        else
        {
            EXPECT_EQ(ErrorCode::UserCallableException, code) << param.source;
            // What the host threw stays kept
            EXPECT_TRUE(result.error().GetException()) << param.source;
        }
        return;
    }
    ASSERT_TRUE(result.has_value()) << param.source << ": " << ToString(result.error().ToString());
    EXPECT_EQ(ConvertString<std::basic_string<CharT>>(expected), result.value()) << param.source;
}

class MacroStopTest : public ::testing::TestWithParam<StopCase>
{
};
} // namespace

TEST_P(MacroStopTest, Narrow)
{
    CheckStopCase<Template, char>(GetParam());
}

TEST_P(MacroStopTest, Wide)
{
    CheckStopCase<TemplateW, wchar_t>(GetParam());
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(Jinja2Oracle, MacroStopTest, ::testing::Values(
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m().x }}", UndefinedPolicy::Default, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m().x }}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m().x }}", UndefinedPolicy::Debug, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m().x }}", UndefinedPolicy::Chainable, "" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m() is defined }}", UndefinedPolicy::Default, "False" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m() is defined }}", UndefinedPolicy::Strict, "False" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m() is defined }}", UndefinedPolicy::Debug, "False" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m() is defined }}", UndefinedPolicy::Chainable, "False" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m()|upper }}]", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m()|upper }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m()|upper }}]", UndefinedPolicy::Debug, "[{{ UNDEFINED VALUE PRINTED: VALUE WAS UNDEFINED BECAUSE A CALLABLE RAISED A STOPITERATION EXCEPTION }}]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m()|upper }}]", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{% set x = m() %}[{{ x }}]", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{% set x = m() %}[{{ x }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{% set x = m() %}[{{ x }}]", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{% set x = m() %}[{{ x }}]", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% macro c() %}<{{ caller() }}>{% endmacro %}{% call c() %}x{{ 1|stop }}{% endcall %}", UndefinedPolicy::Default, "<>" },
    StopCase{ "{% macro c() %}<{{ caller() }}>{% endmacro %}{% call c() %}x{{ 1|stop }}{% endcall %}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro c() %}<{{ caller() }}>{% endmacro %}{% call c() %}x{{ 1|stop }}{% endcall %}", UndefinedPolicy::Debug, "<{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}>" },
    StopCase{ "{% macro c() %}<{{ caller() }}>{% endmacro %}{% call c() %}x{{ 1|stop }}{% endcall %}", UndefinedPolicy::Chainable, "<>" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{% call m() %}x{% endcall %}]", UndefinedPolicy::Default, "error: TemplateRuntimeError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{% call m() %}x{% endcall %}]", UndefinedPolicy::Strict, "error: TemplateRuntimeError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{% call m() %}x{% endcall %}]", UndefinedPolicy::Debug, "error: TemplateRuntimeError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{% call m() %}x{% endcall %}]", UndefinedPolicy::Chainable, "error: TemplateRuntimeError" },
    StopCase{ "{% macro d(a=1|stop) %}{{ a }}{% endmacro %}[{{ d() }}]", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% macro d(a=1|stop) %}{{ a }}{% endmacro %}[{{ d() }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro d(a=1|stop) %}{{ a }}{% endmacro %}[{{ d() }}]", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% macro d(a=1|stop) %}{{ a }}{% endmacro %}[{{ d() }}]", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% macro i() %}{{ 1|stop }}{% endmacro %}{% macro o() %}a{{ i() }}b{% endmacro %}[{{ o() }}]", UndefinedPolicy::Default, "[ab]" },
    StopCase{ "{% macro i() %}{{ 1|stop }}{% endmacro %}{% macro o() %}a{{ i() }}b{% endmacro %}[{{ o() }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro i() %}{{ 1|stop }}{% endmacro %}{% macro o() %}a{{ i() }}b{% endmacro %}[{{ o() }}]", UndefinedPolicy::Debug, "[a{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}b]" },
    StopCase{ "{% macro i() %}{{ 1|stop }}{% endmacro %}{% macro o() %}a{{ i() }}b{% endmacro %}[{{ o() }}]", UndefinedPolicy::Chainable, "[ab]" },
    StopCase{ "{% macro t() %}x{{ 1 is stop }}{% endmacro %}[{{ t() }}]", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% macro t() %}x{{ 1 is stop }}{% endmacro %}[{{ t() }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro t() %}x{{ 1 is stop }}{% endmacro %}[{{ t() }}]", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% macro t() %}x{{ 1 is stop }}{% endmacro %}[{{ t() }}]", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m() ~ 'z' }}]", UndefinedPolicy::Default, "[z]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m() ~ 'z' }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m() ~ 'z' }}]", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}z]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}[{{ m() ~ 'z' }}]", UndefinedPolicy::Chainable, "[z]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{% for x in [1] %}[{{ m() }}]{% endfor %}", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{% for x in [1] %}[{{ m() }}]{% endfor %}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{% for x in [1] %}[{{ m() }}]{% endfor %}", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{% for x in [1] %}[{{ m() }}]{% endfor %}", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% macro m(caller=none) %}{{ caller() }}{% endmacro %}[{% call m() %}{{1|stop}}{% endcall %}]", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% macro m(caller=none) %}{{ caller() }}{% endmacro %}[{% call m() %}{{1|stop}}{% endcall %}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m(caller=none) %}{{ caller() }}{% endmacro %}[{% call m() %}{{1|stop}}{% endcall %}]", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% macro m(caller=none) %}{{ caller() }}{% endmacro %}[{% call m() %}{{1|stop}}{% endcall %}]", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m() is undefined }}", UndefinedPolicy::Default, "True" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m() is undefined }}", UndefinedPolicy::Strict, "True" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m() is undefined }}", UndefinedPolicy::Debug, "True" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m() is undefined }}", UndefinedPolicy::Chainable, "True" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m()|default('D') }}", UndefinedPolicy::Default, "D" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m()|default('D') }}", UndefinedPolicy::Strict, "D" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m()|default('D') }}", UndefinedPolicy::Debug, "D" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}b{% endmacro %}{{ m()|default('D') }}", UndefinedPolicy::Chainable, "D" },
    StopCase{ "{{ f(1|stop) }}", UndefinedPolicy::Default, "error: UserCallableException" },
    StopCase{ "{{ f(1|stop) }}", UndefinedPolicy::Strict, "error: UserCallableException" },
    StopCase{ "{{ f(1|stop) }}", UndefinedPolicy::Debug, "error: UserCallableException" },
    StopCase{ "{{ f(1|stop) }}", UndefinedPolicy::Chainable, "error: UserCallableException" },
    StopCase{ "[{{ stopfn() }}]", UndefinedPolicy::Default, "[]" },
    StopCase{ "[{{ stopfn() }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "[{{ stopfn() }}]", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "[{{ stopfn() }}]", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{{ stopfn().x }}", UndefinedPolicy::Default, "error: UndefinedError" },
    StopCase{ "{{ stopfn().x }}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{{ stopfn().x }}", UndefinedPolicy::Debug, "error: UndefinedError" },
    StopCase{ "{{ stopfn().x }}", UndefinedPolicy::Chainable, "" },
    StopCase{ "{{ 1|stop }}", UndefinedPolicy::Default, "error: UserCallableException" },
    StopCase{ "{{ 1|stop }}", UndefinedPolicy::Strict, "error: UserCallableException" },
    StopCase{ "{{ 1|stop }}", UndefinedPolicy::Debug, "error: UserCallableException" },
    StopCase{ "{{ 1|stop }}", UndefinedPolicy::Chainable, "error: UserCallableException" },
    StopCase{ "A{{ stopfn() }}B{{ stopfn() is defined }}", UndefinedPolicy::Default, "ABFalse" },
    StopCase{ "A{{ stopfn() }}B{{ stopfn() is defined }}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "A{{ stopfn() }}B{{ stopfn() is defined }}", UndefinedPolicy::Debug, "A{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}BFalse" },
    StopCase{ "A{{ stopfn() }}B{{ stopfn() is defined }}", UndefinedPolicy::Chainable, "ABFalse" },
    StopCase{ "{% macro m() %}a{{ 1|fail }}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Default, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{{ 1|fail }}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Strict, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{{ 1|fail }}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Debug, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{{ 1|fail }}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Chainable, "error: UserCallableException" },
    StopCase{ "{% for i in [[1]] recursive %}<{% if i is iterable %}{{ loop(i) }}{% else %}{{ i|stop }}{% endif %}>{% endfor %}", UndefinedPolicy::Default, "<>" },
    StopCase{ "{% for i in [[1]] recursive %}<{% if i is iterable %}{{ loop(i) }}{% else %}{{ i|stop }}{% endif %}>{% endfor %}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% for i in [[1]] recursive %}<{% if i is iterable %}{{ loop(i) }}{% else %}{{ i|stop }}{% endif %}>{% endfor %}", UndefinedPolicy::Debug, "<{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}>" },
    StopCase{ "{% for i in [[1]] recursive %}<{% if i is iterable %}{{ loop(i) }}{% else %}{{ i|stop }}{% endif %}>{% endfor %}", UndefinedPolicy::Chainable, "<>" },
    StopCase{ "{% import 'imp' as x %}[{{ x.y() }}]", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% import 'imp' as x %}[{{ x.y() }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% import 'imp' as x %}[{{ x.y() }}]", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% import 'imp' as x %}[{{ x.y() }}]", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% from 'imp' import y %}[{{ y() }}]", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% from 'imp' import y %}[{{ y() }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% from 'imp' import y %}[{{ y() }}]", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% from 'imp' import y %}[{{ y() }}]", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% macro m() %}a{% include 'inc' %}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Default, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% include 'inc' %}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Strict, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% include 'inc' %}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Debug, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% include 'inc' %}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Chainable, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% import 'bad' as b %}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Default, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% import 'bad' as b %}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Strict, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% import 'bad' as b %}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Debug, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% import 'bad' as b %}b{% endmacro %}[{{ m() }}]", UndefinedPolicy::Chainable, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% block b %}{{ 1|stop }}{% endblock %}z{% endmacro %}[{{ m() }}]", UndefinedPolicy::Default, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% block b %}{{ 1|stop }}{% endblock %}z{% endmacro %}[{{ m() }}]", UndefinedPolicy::Strict, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% block b %}{{ 1|stop }}{% endblock %}z{% endmacro %}[{{ m() }}]", UndefinedPolicy::Debug, "error: UserCallableException" },
    StopCase{ "{% macro m() %}a{% block b %}{{ 1|stop }}{% endblock %}z{% endmacro %}[{{ m() }}]", UndefinedPolicy::Chainable, "error: UserCallableException" },
    StopCase{ "{% block b %}{% if flag %}{{ 1|stop }}{% endif %}{% endblock %}{% set flag = true %}{% macro m() %}x{{ self.b() }}y{% endmacro %}[{{ m() }}]", UndefinedPolicy::Default, "error: UserCallableException" },
    StopCase{ "{% block b %}{% if flag %}{{ 1|stop }}{% endif %}{% endblock %}{% set flag = true %}{% macro m() %}x{{ self.b() }}y{% endmacro %}[{{ m() }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% block b %}{% if flag %}{{ 1|stop }}{% endif %}{% endblock %}{% set flag = true %}{% macro m() %}x{{ self.b() }}y{% endmacro %}[{{ m() }}]", UndefinedPolicy::Debug, "error: UserCallableException" },
    StopCase{ "{% block b %}{% if flag %}{{ 1|stop }}{% endif %}{% endblock %}{% set flag = true %}{% macro m() %}x{{ self.b() }}y{% endmacro %}[{{ m() }}]", UndefinedPolicy::Chainable, "error: UserCallableException" },
    StopCase{ "{% macro m() %}{% set v = 1 %}{% for q in [1,2] %}{{ q }}{{ 1|stop }}{% endfor %}{% endmacro %}{% set v = 0 %}{% for q in [7] %}[{{ m() }}{{ v }}{{ q }}]{% endfor %}", UndefinedPolicy::Default, "[07]" },
    StopCase{ "{% macro m() %}{% set v = 1 %}{% for q in [1,2] %}{{ q }}{{ 1|stop }}{% endfor %}{% endmacro %}{% set v = 0 %}{% for q in [7] %}[{{ m() }}{{ v }}{{ q }}]{% endfor %}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}{% set v = 1 %}{% for q in [1,2] %}{{ q }}{{ 1|stop }}{% endfor %}{% endmacro %}{% set v = 0 %}{% for q in [7] %}[{{ m() }}{{ v }}{{ q }}]{% endfor %}", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}07]" },
    StopCase{ "{% macro m() %}{% set v = 1 %}{% for q in [1,2] %}{{ q }}{{ 1|stop }}{% endfor %}{% endmacro %}{% set v = 0 %}{% for q in [7] %}[{{ m() }}{{ v }}{{ q }}]{% endfor %}", UndefinedPolicy::Chainable, "[07]" },
    StopCase{ "{% macro m(x) %}{{ x }}{{ 1|stop }}{% endmacro %}{% set x = 'o' %}[{{ m('i') }}{{ x }}]{{ m('j') }}", UndefinedPolicy::Default, "[o]" },
    StopCase{ "{% macro m(x) %}{{ x }}{{ 1|stop }}{% endmacro %}{% set x = 'o' %}[{{ m('i') }}{{ x }}]{{ m('j') }}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m(x) %}{{ x }}{{ 1|stop }}{% endmacro %}{% set x = 'o' %}[{{ m('i') }}{{ x }}]{{ m('j') }}", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}o]{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}" },
    StopCase{ "{% macro m(x) %}{{ x }}{{ 1|stop }}{% endmacro %}{% set x = 'o' %}[{{ m('i') }}{{ x }}]{{ m('j') }}", UndefinedPolicy::Chainable, "[o]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}{% endmacro %}[{{ m()|length }}]", UndefinedPolicy::Default, "[0]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}{% endmacro %}[{{ m()|length }}]", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}{% endmacro %}[{{ m()|length }}]", UndefinedPolicy::Debug, "[0]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}{% endmacro %}[{{ m()|length }}]", UndefinedPolicy::Chainable, "[0]" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}{% endmacro %}{% filter upper %}x{{ m() }}y{% endfilter %}", UndefinedPolicy::Default, "XY" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}{% endmacro %}{% filter upper %}x{{ m() }}y{% endfilter %}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}{% endmacro %}{% filter upper %}x{{ m() }}y{% endfilter %}", UndefinedPolicy::Debug, "X{{ UNDEFINED VALUE PRINTED: VALUE WAS UNDEFINED BECAUSE A CALLABLE RAISED A STOPITERATION EXCEPTION }}Y" },
    StopCase{ "{% macro m() %}a{{ 1|stop }}{% endmacro %}{% filter upper %}x{{ m() }}y{% endfilter %}", UndefinedPolicy::Chainable, "XY" },
    StopCase{ "{% macro m() %}{% set ns = namespace(a=1) %}{% for i in [1] %}{{ 1|stop }}{% endfor %}{% endmacro %}{% for i in [1,2,3] %}{{ i }}{{ m() }}{% if loop.last %}!{% endif %}{% endfor %}", UndefinedPolicy::Default, "123!" },
    StopCase{ "{% macro m() %}{% set ns = namespace(a=1) %}{% for i in [1] %}{{ 1|stop }}{% endfor %}{% endmacro %}{% for i in [1,2,3] %}{{ i }}{{ m() }}{% if loop.last %}!{% endif %}{% endfor %}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}{% set ns = namespace(a=1) %}{% for i in [1] %}{{ 1|stop }}{% endfor %}{% endmacro %}{% for i in [1,2,3] %}{{ i }}{{ m() }}{% if loop.last %}!{% endif %}{% endfor %}", UndefinedPolicy::Debug, "1{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}2{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}3{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}!" },
    StopCase{ "{% macro m() %}{% set ns = namespace(a=1) %}{% for i in [1] %}{{ 1|stop }}{% endfor %}{% endmacro %}{% for i in [1,2,3] %}{{ i }}{{ m() }}{% if loop.last %}!{% endif %}{% endfor %}", UndefinedPolicy::Chainable, "123!" },
    StopCase{ "{% macro m() %}{% for i in [1] %}{% if true %}{{ i }}{{ 1|stop }}{% endif %}{% endfor %}{% endmacro %}{% for i in [1,2] %}{% if i == 1 %}{% continue %}{% endif %}[{{ m() }}]{% endfor %}", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% macro m() %}{% for i in [1] %}{% if true %}{{ i }}{{ 1|stop }}{% endif %}{% endfor %}{% endmacro %}{% for i in [1,2] %}{% if i == 1 %}{% continue %}{% endif %}[{{ m() }}]{% endfor %}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% macro m() %}{% for i in [1] %}{% if true %}{{ i }}{{ 1|stop }}{% endif %}{% endfor %}{% endmacro %}{% for i in [1,2] %}{% if i == 1 %}{% continue %}{% endif %}[{{ m() }}]{% endfor %}", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% macro m() %}{% for i in [1] %}{% if true %}{{ i }}{{ 1|stop }}{% endif %}{% endfor %}{% endmacro %}{% for i in [1,2] %}{% if i == 1 %}{% continue %}{% endif %}[{{ m() }}]{% endfor %}", UndefinedPolicy::Chainable, "[]" },
    StopCase{ "{% autoescape true %}{% macro m() %}<b>{{ 1|stop }}{% endmacro %}[{{ m() }}]{% endautoescape %}", UndefinedPolicy::Default, "[]" },
    StopCase{ "{% autoescape true %}{% macro m() %}<b>{{ 1|stop }}{% endmacro %}[{{ m() }}]{% endautoescape %}", UndefinedPolicy::Strict, "error: UndefinedError" },
    StopCase{ "{% autoescape true %}{% macro m() %}<b>{{ 1|stop }}{% endmacro %}[{{ m() }}]{% endautoescape %}", UndefinedPolicy::Debug, "[{{ undefined value printed: value was undefined because a callable raised a StopIteration exception }}]" },
    StopCase{ "{% autoescape true %}{% macro m() %}<b>{{ 1|stop }}{% endmacro %}[{{ m() }}]{% endautoescape %}", UndefinedPolicy::Chainable, "[]" }));
// clang-format on

namespace
{
class CountingSink final : public OutputSink
{
public:
    void Write(std::string_view text) override { output.append(text); }
    std::string output;
};
} // namespace

// Jinja2 buffers a macro's output and drops it when the call stops; a string takes its output back in full, a sink
// only while the output has not left the buffer (docs/tasks/0479)
TEST(MacroStopOutputTest, StringTargetTakesTheOutputBack)
{
    TemplateEnv env;
    FillStopEnv(env);
    Template tpl(&env);
    TemplateW wide(&env);
    for (const int count : { 10, 127, 128, 600, 5000 })
    {
        const std::string source = "[{% macro m() %}{{ 'x' * " + std::to_string(count) + " }}{{ 1|stop }}{% endmacro %}{{ m() }}]{{ 'y' * " + std::to_string(count) + " }}[{{ m() }}]";
        ASSERT_TRUE(tpl.Load(source)) << count;
        EXPECT_EQ("[]" + std::string(static_cast<size_t>(count), 'y') + "[]", tpl.RenderAsString(ValuesMap{}).value()) << count;
        ASSERT_TRUE(wide.Load(ConvertString<std::wstring>(source))) << count;
        EXPECT_EQ(ConvertString<std::wstring>("[]" + std::string(static_cast<size_t>(count), 'y') + "[]"), wide.RenderAsString(ValuesMap{}).value()) << count;
    }
    // A long fragment before the macro, and one between the macro's pieces
    ASSERT_TRUE(tpl.Load("{{ 'a' * 700 }}{% macro m() %}p{{ 'b' * 300 }}q{{ 1|stop }}{% endmacro %}{{ m() }}z"));
    EXPECT_EQ(std::string(700, 'a') + "z", tpl.RenderAsString(ValuesMap{}).value());
}

TEST(MacroStopOutputTest, SinkTakesTheOutputBackWhileItIsBuffered)
{
    TemplateEnv env;
    FillStopEnv(env);
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("[{% macro m(n) %}{{ 'x' * n }}{{ 1|stop }}{% endmacro %}{{ m(n) }}]"));
    for (const bool eachPiece : { false, true })
    {
        RenderOptions options;
        options.writeEachPiece = eachPiece;
        CountingSink shortSink;
        ASSERT_TRUE(tpl.RenderTo(shortSink, ValuesMap{ { "n", 10 } }, options));
        EXPECT_EQ("[]", shortSink.output);
        // The macro's output left the buffer: the render fails as it did before the call stopped
        CountingSink longSink;
        auto result = tpl.RenderTo(longSink, ValuesMap{ { "n", 600 } }, options);
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(ErrorCode::UserCallableException, result.error().GetCode());
        EXPECT_THROW(std::rethrow_exception(result.error().GetException()), TestStop);
    }
}

// What a stopped macro left in a scope is gone: the code after it sees the names it saw before
TEST(MacroStopOutputTest, ContextIsRestored)
{
    TemplateEnv env;
    FillStopEnv(env);
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% macro m(x) %}{% set v = x %}{% for q in [1, 2] %}{% set w = q %}{{ q }}{{ 1|stop }}{% endfor %}{% endmacro %}"
                         "{% set v = 'o' %}{% for q in 'ab' %}[{{ m(q) }}{{ v }}{{ q }}{{ w }}]{% endfor %}{{ v }}"));
    EXPECT_EQ("[oa][ob]o", tpl.RenderAsString(ValuesMap{}).value());
}
