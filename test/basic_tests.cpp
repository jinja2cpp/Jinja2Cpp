#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"
#include "test_tools.h"

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
		tpl.Load("Hello {{ global_var }} {{ global_fn() }}!!!");
		result1 = tpl.RenderAsString(jinja2::ValuesMap{}).value();
	}
    std::string result2;
	{
		jinja2::Template tpl(&tplEnv);
		tpl.Load("Hello {{ global_var }} {{ global_fn() }}!!!");
		result2 = tpl.RenderAsString(jinja2::ValuesMap{}).value();
	}
    ASSERT_EQ(result1, result2);
}

MULTISTR_TEST(BasicMultiStrTest, LiteralWithEscapeCharacters, R"({{ 'Hello\t\nWorld\n\twith\nescape\tcharacters!' }})", "Hello\t\nWorld\n\twith\nescape\tcharacters!")
{
}
