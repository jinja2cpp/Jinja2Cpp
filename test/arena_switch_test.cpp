#include "test_tools.h"

#include <jinja2cpp/value.h>

#include <string>

// Code of one template running inside another's render resolves its parse-tree handles
// through its own template's tree (0118 P4). Expected outputs are Python Jinja2's

class ArenaSwitchTest : public TemplateEnvFixture
{
protected:
    void SetUp() override
    {
        TemplateEnvFixture::SetUp();

        AddFile("base", "[{% block a %}A{% endblock %}|{% block b %}B{% endblock %}|{{ self.a() }}]");
        AddFile("child", R"({% extends "base" %}{% block a %}a{{ super() }}{% endblock %}{% block b %}{{ self.a() }}{% endblock %})");
        AddFile("wrapper", "{% macro wrap() %}<{{ caller() }}>{% endmacro %}");
        AddFile("includer", R"({% macro inc() %}{% include "header" %}{% endmacro %})");
        AddFile("header", "[{{ 1 + 1 }}]");
        AddFile("walker", "{% macro walk(l, items) %}{{ l(items) }}{% endmacro %}");
        AddFile("measurer", "{% macro len(l) %}{{ l.length }}{% endmacro %}");
        AddFile("macro_user", "{{ m() }}");
    }
};

TEST_F(ArenaSwitchTest, BlocksAndSelfAcrossTheChain)
{
    EXPECT_EQ("[aA|aA|aA]", Render(R"({% extends "child" %})"));
}

TEST_F(ArenaSwitchTest, CallerIntoAnImportedMacro)
{
    EXPECT_EQ("<6><6>", Render(R"({% from "wrapper" import wrap %}{% for x in [1,2] %}{% call wrap() %}{{ 3 * 2 }}{% endcall %}{% endfor %})"));
}

TEST_F(ArenaSwitchTest, IncludeInsideAnImportedMacro)
{
    EXPECT_EQ("[2][2]", Render(R"({% from "includer" import inc %}{% for x in [1,2] %}{{ inc() }}{% endfor %})"));
}

TEST_F(ArenaSwitchTest, RecursiveLoopCalledFromAnImportedMacro)
{
    const jinja2::ValuesMap params{ { "tree",
                                      jinja2::ValuesList{ jinja2::ValuesMap{ { "n", 1 },
                                                                             { "c", jinja2::ValuesList{ jinja2::ValuesMap{ { "n", 2 }, { "c", jinja2::ValuesList{ jinja2::ValuesMap{ { "n", 3 } } } } } } } },
                                                          jinja2::ValuesMap{ { "n", 4 } } } } };
    EXPECT_EQ("1(2(3))4",
              Render(R"({% from "walker" import walk %}{% for i in tree recursive %}{{ i.n }}{% if i.c %}({{ walk(loop, i.c) }}){% endif %}{% endfor %})", params));
}

// loop.length runs the loop's filter over the rest of the items while the macro's code runs
TEST_F(ArenaSwitchTest, FilteredLoopMeasuredInAnImportedMacro)
{
    EXPECT_EQ("333", Render(R"({% from "measurer" import len %}{% for x in [1,2,3,4,5] if x is odd %}{{ len(loop) }}{% endfor %})"));
    // A recursive loop keeps its names in scopes, not slots. Jinja2 rejects a filtered
    // recursive loop (divergence 0091), and Jinja2C++ takes `recursive` before `if`
    EXPECT_EQ("333", Render(R"({% from "measurer" import len %}{% for x in [1,2,3,4,5] recursive if x % 2 %}{{ len(loop) }}{% endfor %})"));
}

TEST_F(ArenaSwitchTest, MacroCalledFromAnInclude)
{
    EXPECT_EQ("M6", Render(R"({% macro m() %}M{{ 2*3 }}{% endmacro %}{% for x in [1] %}{% include "macro_user" %}{% endfor %})"));
}
