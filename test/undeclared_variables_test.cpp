#include "test_tools.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <string_view>
#include <vector>

using namespace jinja2;

// Template::FindUndeclaredVariables against jinja2.meta.find_undeclared_variables
// (docs/tasks/0236). The expected names are Jinja2 3.1's, with the do, loopcontrols and i18n
// extensions and null translations installed
namespace
{
// The expected names separated by spaces: a table of literals initialises without throwing
struct UndeclaredCase
{
    const char* source;
    const char* names;
};

std::vector<std::string> SplitNames(std::string_view names)
{
    std::vector<std::string> result;
    while (!names.empty())
    {
        const auto end = std::min(names.find(' '), names.size());
        result.emplace_back(names.substr(0, end));
        names.remove_prefix(std::min(end + 1, names.size()));
    }
    return result;
}

const UndeclaredCase undeclaredCases[] = {
    // clang-format off
    { "Hello", "" },
    { "{{ x }}", "x" },
    { "{{ x }}{{ x }}{{ y }}", "x y" },
    { "{% set foo = 42 %}{{ bar + foo }}", "bar" },
    { "{{ x }}{% set x = 1 %}", "x" },
    { "{% set x = x + 1 %}", "x" },
    { "{% set a, b = c %}{{ a }}{{ b }}", "c" },
    { "{% set ns = namespace(a=1) %}{% set ns.a = t %}{{ ns.a }}", "t" },
    { "{% set blk %}{{ g }}{% endset %}{{ blk }}", "g" },
    { "{% set x %}{% set inner = 1 %}{% endset %}{{ inner }}", "inner" },
    { "{% if c %}{% set y = 1 %}{% endif %}", "c y" },
    { "{% if c %}{% set y = 1 %}{% endif %}{{ y }}", "c y" },
    { "{% if c %}{% set y = 1 %}{% else %}{% set y = 2 %}{% endif %}{{ y }}", "c y" },
    { "{% if c %}{% set y = 1 %}{{ y }}{% endif %}", "c y" },
    { "{% set y = 0 %}{% if c %}{% set y = 1 %}{% endif %}{{ y }}", "c" },
    { "{% if a %}{{ b }}{% elif c %}{{ d }}{% else %}{{ e }}{% endif %}", "a b c d e" },
    { "{{ a if b else c }}", "a b c" },
    { "{% for i in items %}{{ i }}{{ loop.index }}{{ z }}{% endfor %}", "items z" },
    { "{% for i in items if i > lim %}{{ i }}{% else %}{{ i }}{% endfor %}", "i items lim" },
    { "{% for i in a %}{{ loop.index }}{% else %}{{ loop }}{% endfor %}", "a loop" },
    { "{% for i in a if j %}{% endfor %}", "a j" },
    { "{% for a, b in pairs %}{{ a }}{{ b }}{% endfor %}{{ a }}", "a pairs" },
    { "{% for i in items %}{% set j = i %}{% endfor %}{{ j }}", "items j" },
    { "{% for i in a %}{{ x }}{% endfor %}{% set x = 1 %}", "a" },
    { "{% for i in a %}{% if c %}{% set k = 1 %}{% endif %}{% endfor %}", "a c k" },
    { "{% for i in i %}{{ i }}{% endfor %}", "i" },
    { "{% for x in x %}{% for x in x %}{{ x }}{% endfor %}{% endfor %}", "x" },
    { "{% for i in x recursive %}{{ loop(i.children) }}{% endfor %}", "x" },
    { "{% for k, v in d.items() %}{{ k }}{% endfor %}", "d" },
    { "{% macro m(a, b=d) %}{{ a }}{{ b }}{{ e }}{{ caller() }}{{ varargs }}{{ kwargs }}{% endmacro %}{{ m(1) }}", "d e" },
    { "{% macro m(a=a) %}{% endmacro %}", "" },
    { "{% macro m(a) %}{% set a = 2 %}{{ a }}{% endmacro %}", "" },
    { "{% macro m() %}{{ outer }}{% endmacro %}{% set outer = 1 %}", "" },
    { "{% macro m() %}{{ m() }}{% endmacro %}", "" },
    { "{% macro m() %}{{ loop }}{{ self }}{{ super }}{% endmacro %}", "loop super" },
    { "{% macro m() %}{% if c %}{% set k = 1 %}{% endif %}{% endmacro %}", "c k" },
    { "{% call(u) m2(v) %}{{ u }}{{ w }}{% endcall %}", "m2 v w" },
    { "{% call m() %}{{ caller }}{% endcall %}", "m" },
    { "{{ self }}{{ super }}{{ loop }}{{ caller }}{{ varargs }}", "caller loop super varargs" },
    { "{% block b %}{{ super() }}{{ self.b() }}{% endblock %}", "" },
    { "{% set x = 1 %}{% block b %}{{ x }}{% endblock %}", "x" },
    { "{% block b %}{% set q = 1 %}{{ q }}{% endblock %}{{ q }}", "q" },
    { "{% block b %}{% block c %}{{ v }}{% endblock %}{% set v = 1 %}{% endblock %}", "v" },
    { "{% with p = r %}{{ p }}{{ s }}{% endwith %}", "r s" },
    { "{% with %}{% set w = 1 %}{% endwith %}{{ w }}", "w" },
    { "{% with a = 1, b = a %}{% endwith %}", "a" },
    { "{% filter upper %}{{ f }}{% endfilter %}", "f" },
    { "{% filter upper %}{% set z = 1 %}{% endfilter %}{{ z }}", "z" },
    { "{% filter replace(a, \"\") %}{% endfilter %}{% set a = 1 %}", "a" },
    { "{% set x | replace(a, \"\") %}{% endset %}{% set a = 1 %}", "" },
    { "{% for i in l %}{% set x | replace(a, \"\") %}{% endset %}{% set a = 1 %}{% endfor %}", "l" },
    { "{% autoescape true %}{{ ae }}{% endautoescape %}", "ae" },
    { "{{ x|default(y) }}{{ h.attr }}{{ k[\"key\"] }}", "h k x y" },
    { "{{ x.y.z(c, a=b) }}", "b c x" },
    { "{{ [1, 2]|map(attribute=a)|list }}{{ x is defined }}", "a x" },
    { "{{ range(3) }}{{ dict() }}{{ lipsum() }}{{ cycler(1) }}{{ joiner() }}{{ namespace() }}", "" },
    { "{{ true }}{{ none }}{{ True }}{{ None }}", "" },
    { "{% raw %}{{ notvar }}{% endraw %}{# {{ c }} #}", "" },
    { "{% include \"inc.html\" %}", "" },
    { "{% include [\"a\", n] ignore missing %}{% extends layout %}", "layout n" },
    { "{% extends \"base.html\" %}{% block b %}{{ q }}{% endblock %}", "q" },
    { "{% import \"m.html\" as mm %}{{ mm.x }}{{ mm2 }}", "mm2" },
    { "{% from \"m.html\" import a as b, c %}{{ b }}{{ c }}{{ a }}", "a" },
    { "{% do lst.append(1) %}", "lst" },
    { "{% for i in a %}{% if i %}{% break %}{% endif %}{{ v }}{% endfor %}", "a v" },
    { "{% trans %}hi {{ user }}{% endtrans %}", "user" },
    { "{% trans count=n %}one{% pluralize %}many {{ count }}{% endtrans %}", "n" },
    { "{{ _('x') }}{{ gettext('y') }}", "" },
    { "{% trans user %}hi {{ user }}{% endtrans %}", "user" },
    { "{% trans user, n %}{{ user }} one{% pluralize n %}{{ user }} many{% endtrans %}", "n user" },
    { "{% trans count %}one{% pluralize %}many{% endtrans %}", "count" },
    { "{% for user in users %}{% trans %}{{ user }}{% endtrans %}{% endfor %}{% trans user %}{{ user }}{% endtrans %}{% set user = w %}", "user users w" },
    { "{% for d in b %}{% else %}{% set f %}x{% endset %}{% endfor %}", "b" },
    { "{% for d in b %}{% else %}{% set f = 1 %}{% set g %}{{ f }}{% endset %}{% endfor %}", "b" },
    { "{% macro b(x=caller, y=varargs, z=kwargs) %}{% endmacro %}", "caller kwargs varargs" },
    { "{% macro b(x=varargs, y=kwargs) %}{{ varargs }}{% endmacro %}", "kwargs" },
    { "{% macro b(x=caller) %}{% if q %}{{ caller() }}{% endif %}{% endmacro %}", "q" },
    { "{% macro b(x=caller) %}{% macro c() %}{{ caller }}{% endmacro %}{% endmacro %}", "" },
    { "{% call(x=caller) b() %}{{ caller }}{% endcall %}", "b" },
    { "{% macro n(a=kwargs) %}{% block q %}{% macro k() %}{{ kwargs }}{% endmacro %}{% endblock %}{% endmacro %}", "kwargs" },
    { "{% macro n(a=kwargs) %}{% block q %}{{ kwargs }}{% endblock %}{% endmacro %}", "kwargs" },
    // clang-format on
};

void EnableExtensions(TemplateEnv& env)
{
    env.GetSettings().extensions.doStatement = true;
    env.GetSettings().extensions.loopControls = true;
    env.GetSettings().extensions.i18n = true;
}
} // namespace

TEST(UndeclaredVariablesTest, MatchesJinja2Meta)
{
    TemplateEnv env;
    EnableExtensions(env);
    for (const auto& c : undeclaredCases)
    {
        const auto expected = SplitNames(c.names);
        Template tpl(&env);
        ASSERT_TRUE(tpl.Load(c.source)) << c.source;
        auto names = tpl.FindUndeclaredVariables();
        ASSERT_TRUE(names) << c.source;
        EXPECT_EQ(expected, names.value()) << c.source;

        TemplateW wide(&env);
        ASSERT_TRUE(wide.Load(ConvertString<std::wstring>(std::string(c.source)))) << c.source;
        auto wideNames = wide.FindUndeclaredVariables();
        ASSERT_TRUE(wideNames) << c.source;
        EXPECT_EQ(expected, wideNames.value()) << c.source;
    }
}

TEST(UndeclaredVariablesTest, LeavesOutEnvironmentGlobals)
{
    TemplateEnv env;
    EnableExtensions(env);
    env.AddGlobal("site", Value("example"));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ site }}{{ page }}{{ range(2)|list }}"));
    EXPECT_EQ(std::vector<std::string>{ "page" }, tpl.FindUndeclaredVariables().value());

    // Without the i18n extension `gettext` is an ordinary name
    Template plain;
    ASSERT_TRUE(plain.Load("{{ gettext('x') }}{{ range(2)|list }}"));
    EXPECT_EQ(std::vector<std::string>{ "gettext" }, plain.FindUndeclaredVariables().value());
}

TEST(UndeclaredVariablesTest, NotLoaded)
{
    Template tpl;
    auto names = tpl.FindUndeclaredVariables();
    ASSERT_FALSE(names);
    EXPECT_EQ(ErrorCode::TemplateNotParsed, names.error().GetCode());

    // A failed Load keeps the template loaded before it
    ASSERT_TRUE(tpl.Load("{{ a }}"));
    ASSERT_FALSE(tpl.Load("{{ a "));
    EXPECT_EQ(std::vector<std::string>{ "a" }, tpl.FindUndeclaredVariables().value());
}

// The names of a trans block's messages are read where the block is, so they may resolve to
// the slots of a loop or macro around it (docs/tasks/0236)
TEST(UndeclaredVariablesTest, TransNamesRenderFromSlots)
{
    TemplateEnv env;
    EnableExtensions(env);
    Template loop(&env);
    ASSERT_TRUE(loop.Load("{% for user in ['a', 'b'] %}{% trans %}hi {{ user }};{% endtrans %}{% endfor %}"));
    EXPECT_EQ("hi a;hi b;", loop.RenderAsString({}).value());
    // A bare variable of one block and a message name of another read the same name
    Template mixed(&env);
    ASSERT_TRUE(mixed.Load("{% for user in users %}{% trans %}{{ user }}{% endtrans %}{% endfor %}{% trans user %}{{ user }}{% endtrans %}{% set user = w %}"));
    EXPECT_EQ("abU", mixed.RenderAsString({ { "user", "U" }, { "users", ValuesList{ "a", "b" } }, { "w", "W" } }).value());
    // A set block in a loop's else is a store of the else's frame, read before it from outside
    Template loopElse(&env);
    ASSERT_TRUE(loopElse.Load("{% for d in [] %}{% else %}{{ f }}{% set f %}x{% endset %}{{ f }}{% endfor %}"));
    EXPECT_EQ("Fx", loopElse.RenderAsString({ { "f", "F" } }).value());
    Template macro(&env);
    ASSERT_TRUE(macro.Load("{% macro m(user) %}{% trans %}hi {{ user }};{% endtrans %}{% endmacro %}{{ m('c') }}"));
    EXPECT_EQ("hi c;", macro.RenderAsString({}).value());
}

// A set block's filters run in the block's frame, so a store after the block hides the outer
// name from them as in Jinja2, while a filter block's arguments read the enclosing frame
TEST(UndeclaredVariablesTest, SetBlockFiltersReadBlockFrame)
{
    Template setBlock;
    ASSERT_TRUE(setBlock.Load("{% set x | replace(a, '') %}xax{% endset %}{{ x }}{% set a = 'Z' %}"));
    EXPECT_EQ("xax", setBlock.RenderAsString({ { "a", "a" } }).value());
    Template filterBlock;
    ASSERT_TRUE(filterBlock.Load("{% filter replace(a, '') %}xax{% endfilter %}{% set a = 'Z' %}"));
    EXPECT_EQ("xx", filterBlock.RenderAsString({ { "a", "a" } }).value());
}
