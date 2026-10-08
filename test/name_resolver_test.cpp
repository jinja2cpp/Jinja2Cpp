#include "../src/expression_evaluator.h"
#include "../src/load_settings.h"
#include "../src/name_resolver.h"
#include "../src/node_arena.h"
#include "../src/template_parser.h"

#include <jinja2cpp/template_env.h>

#include "gtest/gtest.h"

#include <string>

using namespace jinja2;

// Which names a render reads from slots (docs/design/0117-name-slots-plan.md, phase P1).
// Each name expression made inside a loop is listed in source order as name:slot, or
// name:- when it stays a lookup by name; names outside every loop are lookups and not listed

// The parser is internal to the library, so these tests run against the static one only
#ifndef JINJA2CPP_LINK_AS_SHARED
namespace
{
std::string ResolveNames(const std::string& source)
{
    NodeArena nodes;
    TemplateEnv env;
    detail::LoadSettings settings{ env.GetSettings() };
    TemplateParser<char> parser(&source, settings, &env, "test", nodes);
    auto result = parser.Parse();
    if (!result)
    {
        return "parse error";
    }
    std::string names;
    for (const auto& use : parser.GetNames().Uses())
    {
        const auto& name = nodes[use.first];
        if (!names.empty())
        {
            names += ' ';
        }
        names += std::string(name.GetName(nodes)) + ':' + (name.GetSlot().IsDynamic() ? std::string("-") : std::to_string(name.GetSlot().value));
    }
    return names;
}
} // namespace

struct NameResolverCase
{
    const char* source;
    const char* names;
};

class NameResolverTest : public testing::TestWithParam<NameResolverCase>
{
};

TEST_P(NameResolverTest, Resolve)
{
    const auto& param = GetParam();
    EXPECT_EQ(param.names, ResolveNames(param.source)) << param.source;
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(Slots, NameResolverTest, testing::Values(
    NameResolverCase{ "{% for x in xs %}{{ x }}{{ loop }}{{ y }}{% endfor %}{{ x }}", "x:1 loop:0 y:-" },
    NameResolverCase{ "{% for k, v in d %}{{ k }}{{ v }}{% endfor %}", "k:1 v:2" },
    NameResolverCase{ "{% for x in xs %}{% for y in x %}{{ x }}{{ y }}{{ loop }}{% endfor %}{% endfor %}", "x:1 x:1 y:3 loop:2" },
    NameResolverCase{ "{% for x in xs %}{% endfor %}{% for y in ys %}{{ y }}{% endfor %}", "y:1" },
    NameResolverCase{ "{% for x in xs if x > n %}{{ x }}{% endfor %}", "x:2 n:- x:1" },
    NameResolverCase{ "{% for x in xs if loop %}{{ x }}{% endfor %}", "loop:- x:1" },
    NameResolverCase{ "{% for x in xs %}{% for x in x %}{{ x }}{% endfor %}{% endfor %}", "x:1 x:3" },
    NameResolverCase{ "{% for x in xs %}{% set x %}a{% endset %}{{ x }}{% endfor %}", "x:-" },
    NameResolverCase{ "{% for x in xs %}{% set x = 1 %}{{ x }}{% endfor %}", "x:-" },
    NameResolverCase{ "{% for x in xs %}{{ x }}{% set x = 1 %}{% endfor %}", "x:-" },
    NameResolverCase{ "{% for x in xs %}{% with x = 1 %}{{ x }}{% endwith %}{{ x }}{% endfor %}", "x:- x:1" },
    NameResolverCase{ "{% for x in xs %}{{ x }}{% else %}{{ x }}{% endfor %}", "x:1" },
    NameResolverCase{ "{% for x in xs recursive %}{{ x }}{{ loop(x) }}{% endfor %}", "" },
    NameResolverCase{ "{% for x in xs recursive %}{% for y in x %}{{ y }}{% endfor %}{% endfor %}", "" },
    NameResolverCase{ "{% for x in xs %}{% macro m(a=x) %}{{ a }}{{ x }}{% endmacro %}{{ m() }}{% endfor %}", "a:0 x:- m:-" },
    NameResolverCase{ "{% macro m(a) %}{% for x in a %}{{ x }}{{ a }}{% endfor %}{% endmacro %}", "a:0 x:2 a:0" },
    NameResolverCase{ "{% for x in xs %}{% call m() %}{{ x }}{% endcall %}{% endfor %}", "x:-" },
    NameResolverCase{ "{% for x in xs %}{% block b %}{{ x }}{% endblock %}{% endfor %}", "" },
    NameResolverCase{ "{% for x in xs %}{% set ns.x = 1 %}{{ x }}{% endfor %}", "x:1" },
    NameResolverCase{ "{% for (a, (b, c)) in xs %}{{ a }}{{ b }}{{ c }}{% endfor %}", "a:1 b:2 c:3" },
    NameResolverCase{ "{% for x in xs %}{% import 'a' as x %}{{ x }}{% endfor %}", "x:-" },
    NameResolverCase{ "{% for x in xs %}{% from 'a' import y as x %}{{ x }}{% endfor %}", "x:-" },
    NameResolverCase{ "{% for x in xs %}{% for y in ys %}{{ y }}{% endfor %}{% for z in zs %}{{ z }}{% endfor %}{% endfor %}", "ys:- y:3 zs:- z:3" },
    NameResolverCase{ "{% macro m(a, b) %}{{ b }}{{ a }}{{ c }}{% endmacro %}", "b:1 a:0 c:-" },
    NameResolverCase{ "{% macro m(a) %}{{ caller() }}{{ varargs }}{{ kwargs }}{{ a }}{% endmacro %}", "caller:1 varargs:3 kwargs:2 a:0" },
    NameResolverCase{ "{% macro m(a) %}{{ a }}{% set a = 1 %}{{ a }}{% endmacro %}", "a:- a:-" },
    NameResolverCase{ "{% macro m(a) %}{% if a %}{% set a = 1 %}{% endif %}{{ a }}{% endmacro %}", "a:- a:-" },
    NameResolverCase{ "{% macro m(a) %}{% for x in a %}{% set a = x %}{% endfor %}{{ a }}{% endmacro %}", "a:0 x:2 a:0" },
    NameResolverCase{ "{% macro m(a) %}{% macro n(b) %}{{ a }}{{ b }}{% endmacro %}{{ n(a) }}{% endmacro %}", "a:- b:0 n:- a:0" },
    NameResolverCase{ "{% macro m(a) %}{% with b = a %}{{ a }}{{ b }}{% endwith %}{% endmacro %}", "a:0 a:0 b:-" },
    NameResolverCase{ "{% macro m(a=b, b=1) %}{{ a }}{{ b }}{% endmacro %}", "a:0 b:1" },
    NameResolverCase{ "{% call(item) m() %}{{ item }}{{ x }}{% endcall %}", "item:0 x:-" }));
// clang-format on
#endif
