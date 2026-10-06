#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <iostream>
#include <string>
#include <thread>
#include <vector>

using namespace jinja2;

using BasicMultiStrTest = BasicTemplateRenderer;


MULTISTR_TEST(BasicMultiStrTest, PlainSingleLineTemplateProcessing, R"(Hello World from Parser!)", R"(Hello World from Parser!)")
{
}

MULTISTR_TEST(BasicMultiStrTest, PlainMultiLineTemplateProcessing, R"(Hello World
from Parser!)", R"(Hello World
from Parser!)")
{
}

MULTISTR_TEST(BasicMultiStrTest, InlineCommentsSkip, "Hello World {#Comment to skip #}from Parser!", "Hello World from Parser!")
{
}

MULTISTR_TEST(BasicMultiStrTest, SingleLineCommentsSkip,
R"(Hello World
{#Comment to skip #}
from Parser!)",
R"(Hello World

from Parser!)")
{
}

MULTISTR_TEST(BasicMultiStrTest, MultiLineCommentsSkip,
R"(Hello World
{#Comment to
skip #}
from Parser!)",
R"(Hello World

from Parser!)")
{
}

MULTISTR_TEST(BasicMultiStrTest, CommentedOutCodeSkip,
R"(Hello World
{#Comment to
            {{for}}
            {{endfor}}
skip #}
{#Comment to
             {%
 skip #}
from Parser!)",
R"(Hello World


from Parser!)")
{
}

TEST(BasicTests, StripSpaces_None)
{
    std::string source = R"(Hello World
#
      {{ ' -expr- ' }}
#
      {% if true %}{{ ' -stmt- ' }}{% endif %}
#
      {# comment 1 #}{{ ' -comm- ' }}{# comment 2 #}
#
      i = {{ '-expr- ' }}
#
      i = {% if true %}{{ '-stmt- ' }}{% endif %}
#
      i = {# comment 1 #}{{ '-comm- ' }}{# comment 2 #}
#
{% for i in range(3) %}
   ####
   i = {{i}}
   ####
{% endfor %}
   ####
from Parser!)";

    TemplateEnv env;
    env.GetSettings().lstripBlocks = false;
    env.GetSettings().trimBlocks = false;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = "Hello World\n"
                                 "#\n"
                                 "       -expr- \n"
                                 "#\n"
                                 "       -stmt- \n"
                                 "#\n"
                                 "       -comm- \n"
                                 "#\n"
                                 "      i = -expr- \n"
                                 "#\n"
                                 "      i = -stmt- \n"
                                 "#\n"
                                 "      i = -comm- \n"
                                 "#\n"
                                 "\n"
                                 "   ####\n"
                                 "   i = 0\n"
                                 "   ####\n"
                                 "\n"
                                 "   ####\n"
                                 "   i = 1\n"
                                 "   ####\n"
                                 "\n"
                                 "   ####\n"
                                 "   i = 2\n"
                                 "   ####\n"
                                 "\n"
                                 "   ####\n"
                                 "from Parser!";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripSpaces_LStripBlocks)
{
    std::string source = R"(Hello World
#
      {{ ' -expr- ' }}
#
      {% if true %}{{ ' -stmt- ' }}{% endif %}
#
      {# comment 1 #}{{ ' -comm- ' }}{# comment 2 #}
#
      i = {{ '-expr- ' }}
#
      i = {% if true %}{{ '-stmt- ' }}{% endif %}
#
      i = {# comment 1 #}{{ '-comm- ' }}{# comment 2 #}
#
{% for i in range(3) %}
   ####
   i = {{i}}
   ####
{% endfor %}
   ####
from Parser!)";

    TemplateEnv env;
    env.GetSettings().lstripBlocks = true;
    env.GetSettings().trimBlocks = false;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = "Hello World\n"
                                 "#\n"
                                 "       -expr- \n"
                                 "#\n"
                                 " -stmt- \n"
                                 "#\n"
                                 " -comm- \n"
                                 "#\n"
                                 "      i = -expr- \n"
                                 "#\n"
                                 "      i = -stmt- \n"
                                 "#\n"
                                 "      i = -comm- \n"
                                 "#\n"
                                 "\n"
                                 "   ####\n"
                                 "   i = 0\n"
                                 "   ####\n"
                                 "\n"
                                 "   ####\n"
                                 "   i = 1\n"
                                 "   ####\n"
                                 "\n"
                                 "   ####\n"
                                 "   i = 2\n"
                                 "   ####\n"
                                 "\n"
                                 "   ####\n"
                                 "from Parser!";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripSpaces_TrimBlocks)
{
    std::string source = R"(Hello World
#
      {{ ' -expr- ' }}
#
      {% if true %}{{ ' -stmt- ' }}{% endif %}
#
      {# comment 1 #}{{ ' -comm- ' }}{# comment 2 #}
#
      i = {{ '-expr- ' }}
#
      i = {% if true %}{{ '-stmt- ' }}{% endif %}
#
      i = {# comment 1 #}{{ '-comm- ' }}{# comment 2 #}
#
{% for i in range(3) %}
   ####
   i = {{i}}
   ####
{% endfor %}
   ####
from Parser!)";

    TemplateEnv env;
    env.GetSettings().lstripBlocks = false;
    env.GetSettings().trimBlocks = true;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = "Hello World\n"
                                 "#\n"
                                 "       -expr- \n"
                                 "#\n"
                                 "       -stmt- #\n"
                                 "       -comm- #\n"
                                 "      i = -expr- \n"
                                 "#\n"
                                 "      i = -stmt- #\n"
                                 "      i = -comm- #\n"
                                 "   ####\n"
                                 "   i = 0\n"
                                 "   ####\n"
                                 "   ####\n"
                                 "   i = 1\n"
                                 "   ####\n"
                                 "   ####\n"
                                 "   i = 2\n"
                                 "   ####\n"
                                 "   ####\n"
                                 "from Parser!";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripSpaces_LStripBlocks_TrimBlocks)
{
    std::string source = R"(Hello World
#
      {{ ' -expr- ' }}
#
      {% if true %}{{ ' -stmt- ' }}{% endif %}
#
      {# comment 1 #}{{ ' -comm- ' }}{# comment 2 #}
#
      i = {{ '-expr- ' }}
#
      i = {% if true %}{{ '-stmt- ' }}{% endif %}
#
      i = {# comment 1 #}{{ '-comm- ' }}{# comment 2 #}
#
{% for i in range(3) %}
   ####
   i = {{i}}
   ####
{% endfor %}
   ####
from Parser!)";

    TemplateEnv env;
    env.GetSettings().lstripBlocks = true;
    env.GetSettings().trimBlocks = true;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = "Hello World\n"
                                 "#\n"
                                 "       -expr- \n"
                                 "#\n"
                                 " -stmt- #\n"
                                 " -comm- #\n"
                                 "      i = -expr- \n"
                                 "#\n"
                                 "      i = -stmt- #\n"
                                 "      i = -comm- #\n"
                                 "   ####\n"
                                 "   i = 0\n"
                                 "   ####\n"
                                 "   ####\n"
                                 "   i = 1\n"
                                 "   ####\n"
                                 "   ####\n"
                                 "   i = 2\n"
                                 "   ####\n"
                                 "   ####\n"
                                 "from Parser!";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_1)
{
    std::string source = R"(Hello World
      {{- ' --' }}
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World --
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_2)
{
    std::string source = R"(Hello World
      {{+ ' --' }}
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
       --
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_3)
{
    std::string source = R"(Hello World
      {%- set delim = ' --' %}{{delim}}<
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World --<
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_4)
{
    std::string source = "Hello World\t\t   \t" R"(
      {%- set delim = ' --' %} {{+delim}}<
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World  --<
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_5)
{
    std::string source = R"(Hello World{%- set delim = ' --' %} {{-delim}}<
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World --<
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_6)
{
    std::string source = R"(Hello World
      {%+ set delim = ' --' %} > {{-delim}}<
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
       > --<
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_7)
{
    std::string source = R"({{-'Hello' }} World
      {{- ' --' }}
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World --
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_8)
{
    std::string source = R"({{+'Hello' }} World
      {{- ' --' }}
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World --
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimSpaces_1)
{
    std::string source = R"(Hello World {{ ' --' -}}
from Parser!)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World  --from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimSpaces_2)
{
    std::string source = R"(Hello World {{ ' --' +}}
from Parser!)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World  --
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimSpaces_3)
{
    std::string source = R"(Hello World {{ ' --' +}})";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World  --)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimSpaces_4)
{
    std::string source = R"(Hello World {{ 'str1 ' -}}
            {{- 'str2 '-}}
            {{- 'str3 '-}}
            {{- 'str4 '-}}
from Parser!)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World str1 str2 str3 str4 from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimSpaces_5)
{
    std::string source = R"(Hello World {% set delim = ' -- ' -%} >{{delim}}<
from Parser!)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World > -- <
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimSpaces_6)
{
    std::string source = R"(Hello World {% set delim = ' -- ' +%} >{{delim}}<
from Parser!)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World  > -- <
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimSpaces_7)
{
    std::string source = R"(Hello World {% set delim = ' -- ' -%}
>{{delim}}<
from Parser!)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World > -- <
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimSpaces_8)
{
    std::string source = "Hello World{% set delim = ' -- ' +%}\n>{{delim}}<\nfrom Parser!";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
> -- <
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimStripSpacesMixed_1)
{
    std::string source = R"(Hello World -{% set delim = ' -- ' -%}
>{{delim}}<
from Parser!)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World -> -- <
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, TrimStripSpacesMixed_2)
{
    std::string source = R"(Hello World -{% set delim = ' -- ' -%}-
>{{delim}}<
from Parser!)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(ValuesMap{}).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World --
> -- <
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, EnvTestPreservesGlobalVar)
{
	jinja2::TemplateEnv tplEnv;
	tplEnv.AddGlobal("global_var", jinja2::Value("foo"));
	tplEnv.AddGlobal("global_fn", jinja2::MakeCallable([]() {
                return "bar";
     }));
    std::string result1;
	{
		jinja2::Template tpl(&tplEnv);
		ASSERT_TRUE(tpl.Load("Hello {{ global_var }} {{ global_fn() }}!!!"));
		result1 = tpl.RenderAsString(jinja2::ValuesMap{}).value();
	}
    std::string result2;
	{
		jinja2::Template tpl(&tplEnv);
		ASSERT_TRUE(tpl.Load("Hello {{ global_var }} {{ global_fn() }}!!!"));
		result2 = tpl.RenderAsString(jinja2::ValuesMap{}).value();
	}
    ASSERT_EQ(result1, result2);
}

MULTISTR_TEST(BasicMultiStrTest, LiteralWithEscapeCharacters, R"({{ 'Hello\t\nWorld\n\twith\nescape\tcharacters!' }})", "Hello\t\nWorld\n\twith\nescape\tcharacters!")
{
}

TEST(BasicTests, HashLineStatementPrefix)
{
    TemplateEnv env;
    env.GetSettings().lineStatementPrefix = "#";
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("# for i in range(2)\n{{ i }}\n# endfor\n"));
    EXPECT_EQ("0\n1\n", tpl.RenderAsString(ValuesMap{}).value());
}

TEST(BasicTests, CustomDelimitersInErrorMessages)
{
    TemplateEnv env;
    env.GetSettings().variableStartString = "<<";
    env.GetSettings().variableEndString = ">>";
    Template tpl(&env);
    auto result = tpl.Load("<< x");
    ASSERT_FALSE(result);
    EXPECT_EQ("noname.j2tpl:1:5: error: Unexpected token '<<End of block>>'. Expected: '>>'\n<< x\n ---^-------", ErrorToString(result.error()));
}

// Begin delimiters with different first characters, and those characters alone in the text
TEST(BasicTests, MixedDelimiterStarts)
{
    TemplateEnv env;
    env.GetSettings().variableStartString = "<<";
    env.GetSettings().variableEndString = ">>";
    env.GetSettings().blockStartString = "[%";
    env.GetSettings().blockEndString = "%]";
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("a < b [ c {{ x }} <<x>> [% if 1 %]y[% endif %]{# c #}<"));
    EXPECT_EQ("a < b [ c {{ x }} 1 y<", tpl.RenderAsString(ValuesMap{ { "x", 1 } }).value());
}

TEST(BasicTests, EmptyDelimiterKeepsDefault)
{
    TemplateEnv env;
    env.GetSettings().variableStartString.clear();
    TemplateW tpl(&env);
    ASSERT_TRUE(tpl.Load(L"{{ 1 }}"));
    EXPECT_EQ(L"1", tpl.RenderAsString(ValuesMap{}).value());
}

TEST(BasicTests, SettingsCompareDelimiters)
{
    Settings lhs;
    Settings rhs;
    EXPECT_TRUE(lhs == rhs);
    rhs.blockEndString = "%>";
    EXPECT_TRUE(lhs != rhs);
    rhs = lhs;
    rhs.lineCommentPrefix = "##";
    EXPECT_TRUE(lhs != rhs);
}

// Templates share the settings of their environment; changing them, also through the reference GetSettings returns,
// affects the templates made afterwards only
TEST(BasicTests, SettingsChangedBetweenTemplates)
{
    TemplateEnv env;
    Template before(&env);
    env.GetSettings().variableStartString = "<<";
    env.GetSettings().variableEndString = ">>";
    Template after(&env);
    TemplateW afterW(&env);
    ASSERT_TRUE(before.Load("{{ x }} <<x>>"));
    ASSERT_TRUE(after.Load("{{ x }} <<x>>"));
    ASSERT_TRUE(afterW.Load(L"{{ x }} <<x>>"));
    EXPECT_EQ("1 <<x>>", before.RenderAsString(ValuesMap{ { "x", 1 } }).value());
    EXPECT_EQ("{{ x }} 1", after.RenderAsString(ValuesMap{ { "x", 1 } }).value());
    EXPECT_EQ(L"{{ x }} 1", afterW.RenderAsString(ValuesMap{ { "x", 1 } }).value());

    env.SetSettings(Settings());
    Template reset(&env);
    ASSERT_TRUE(reset.Load("{{ x }} <<x>>"));
    EXPECT_EQ("1 <<x>>", reset.RenderAsString(ValuesMap{ { "x", 1 } }).value());

    env.GetSettings().trimBlocks = true;
    Template trimmed(&env);
    ASSERT_TRUE(trimmed.Load("{% if true %}\nx{% endif %}"));
    EXPECT_EQ("x", trimmed.RenderAsString(ValuesMap{}).value());
}

// A finalize callable edited in place keeps its identity; the templates made afterwards still see the edit
TEST(BasicTests, FinalizeEditedInPlaceBetweenTemplates)
{
    TemplateEnv env;
    env.GetSettings().finalize = UserCallable([](const UserCallableParams& params) { return params["value"]; }, { ArgInfo("value", true) });
    Template before(&env);
    env.GetSettings().finalize.callable = [](const UserCallableParams&) { return Value("A"); };
    Template after(&env);
    ASSERT_TRUE(before.Load("{{ 1 }}"));
    ASSERT_TRUE(after.Load("{{ 1 }}"));
    EXPECT_EQ("1", before.RenderAsString(ValuesMap{}).value());
    EXPECT_EQ("A", after.RenderAsString(ValuesMap{}).value());
}

// Templates made at the same time from several threads share one copy of the settings
TEST(BasicTests, TemplatesMadeConcurrently)
{
    TemplateEnv env;
    env.GetSettings().blockStartString = "<%";
    env.GetSettings().blockEndString = "%>";
    std::vector<int> failures(4);
    std::vector<std::thread> threads;
    threads.reserve(failures.size());
    for (auto& failed : failures)
    {
        threads.emplace_back([&env, &failed] {
            for (int i = 0; i != 100; ++i)
            {
                Template tpl(&env);
                auto rendered = tpl.Load("<% for i in range(2) %>{{ i }}<% endfor %>") ? tpl.RenderAsString(ValuesMap{}) : nonstd::make_unexpected(ErrorInfo());
                failed += !rendered || rendered.value() != "01";
            }
        });
    }
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_EQ(std::vector<int>(4), failures);
}
