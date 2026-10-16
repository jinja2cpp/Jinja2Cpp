#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <thread>
#include <system_error>
#include <type_traits>
#include <utility>
#include <variant>
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
    // Jinja2 has `+%}` but no `+}}`: the `+` is the operator, missing its right operand
    std::string source = R"(Hello World {{ ' --' +}}
from Parser!)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    ASSERT_FALSE(parseRes.has_value());
    EXPECT_EQ(ErrorCode::UnexpectedToken, parseRes.error().GetCode());
}

TEST(BasicTests, TrimSpaces_3)
{
    std::string source = R"(Hello World {% if true +%} --{% endif +%})";

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

// The lexer finds the end of a tag only for end delimiters that cannot start inside a token;
// for the others, and for malformed tags, the splitter's own scan finds it (docs/tasks/0142)
TEST(BasicTests, TagEndsFoundWithoutTheLexer)
{
    struct Case
    {
        const char* varEnd;
        const char* blockEnd;
        const char* tpl;
        const char* expected;
    };
    const Case cases[] = {
        { "=>", "%}", "{{ x =>{% if x == 1 %}y{% endif %}", "1y" },
        { "*}", "%}", "{{ x ** 2 *}", "1" },
        { "end", "%}", "{{ x + 1 end", "2" },
        { "}}", "%}", "{{ 'a}}' ~ x }}", "a}}1" },
        { "}}", "%}", "{{ (x\n) }}", "1" },
    };
    for (const auto& c : cases)
    {
        TemplateEnv env;
        env.GetSettings().variableEndString = c.varEnd;
        env.GetSettings().blockEndString = c.blockEnd;
        Template tpl(&env);
        auto loaded = tpl.Load(c.tpl);
        ASSERT_TRUE(loaded) << c.tpl << ": " << ErrorToString(loaded.error());
        EXPECT_EQ(c.expected, tpl.RenderAsString(ValuesMap{ { "x", 1 } }).value()) << c.tpl;
    }
}

// Malformed tags fail to load as they did before the lexer found tag ends: no crash, no template
TEST(BasicTests, MalformedTagEnds)
{
    struct Case
    {
        const char* varEnd;
        const char* blockEnd;
        const char* tpl;
    };
    const Case cases[] = {
        // The start modifier is the first character of the end delimiter: the tag ends before its body
        { "-}", "%}", "{{-} 'q" },
        { "}}", "-%}", "{%-%} 'q" },
        { "+-", "%}", "{{+-} 'q" },
        { "-}", "%}", "{{-} x{{ 'q}}" },
        // The number ends at the end of the tag, in its exponent
        { "}}", "%}", "{{ 1e1e-}}" },
        { "}}", "%}", "{{ 1.5e1e+}}" },
        { "}}", "%}", "{% set z = 1e1e-%}" },
        // Never closed
        { "}}", "%}", "{{ a ; b" },
        { "}}", "%}", "{{ (a }} b" },
    };
    for (const auto& c : cases)
    {
        TemplateEnv env;
        env.GetSettings().variableEndString = c.varEnd;
        env.GetSettings().blockEndString = c.blockEnd;
        Template tpl(&env);
        EXPECT_FALSE(tpl.Load(c.tpl)) << c.tpl;
        TemplateW tplW(&env);
        EXPECT_FALSE(tplW.Load(std::wstring(c.tpl, c.tpl + std::strlen(c.tpl)))) << c.tpl;
    }

    // A long tag that never closes: the lexer gives up once it sees no end delimiter ahead
    std::string unclosed = "{{ a";
    for (int idx = 0; idx < 1000; ++idx)
    {
        unclosed += " xy";
    }
    Template tpl;
    auto result = tpl.Load(unclosed);
    ASSERT_FALSE(result);
    EXPECT_EQ(ErrorCode::ExpectedToken, result.error().GetCode());
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

// Text converted to newline_sequence lives with the template, not with its parser: it renders
// on every call, in a block too, for narrow and wide templates
TEST(BasicTests, NewlineSequenceTextOutlivesParse)
{
    TemplateEnv env;
    env.GetSettings().newlineSequence = "\r\n";
    Template tpl(&env);
    TemplateW tplW(&env);
    ASSERT_TRUE(tpl.Load("a\nb{% block x %}c\nd{% endblock %}e\n"));
    ASSERT_TRUE(tplW.Load(L"a\nb{% block x %}c\nd{% endblock %}e\n"));
    for (int i = 0; i != 2; ++i)
    {
        EXPECT_EQ("a\r\nbc\r\nde", tpl.RenderAsString(ValuesMap{}).value());
        EXPECT_EQ(L"a\r\nbc\r\nde", tplW.RenderAsString(ValuesMap{}).value());
    }
}

// Many blocks: a name defined twice is still an error, past the first few too
TEST(BasicTests, ManyBlocksRejectADuplicateName)
{
    std::string source;
    std::string expected;
    for (int n = 0; n != 40; ++n)
    {
        source += "{% block b" + std::to_string(n) + " %}" + std::to_string(n) + "{% endblock %}";
        expected += std::to_string(n);
    }
    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    EXPECT_EQ(expected, tpl.RenderAsString(ValuesMap{}).value());

    for (const char* name : { "b3", "b39" })
    {
        Template dup;
        EXPECT_FALSE(dup.Load(source + "{% block " + name + " %}{% endblock %}")) << name;
    }
}

// Value from any integral type stores int64_t (not bool), nullptr stores None, characters are refused (task 0067)
namespace
{
template<typename T>
void ExpectStoredAsInt(T val, const char* name)
{
    const Value v(val);
    ASSERT_TRUE(std::holds_alternative<int64_t>(v.GetData())) << name << ": index " << v.GetData().index();
    EXPECT_EQ(5, std::get<int64_t>(v.GetData())) << name;
    const Value assigned = val;
    EXPECT_TRUE(std::holds_alternative<int64_t>(assigned.GetData())) << name;
}
} // namespace

static_assert(!std::is_constructible_v<Value, char>);
static_assert(!std::is_constructible_v<Value, wchar_t>);
static_assert(!std::is_constructible_v<Value, char16_t>);
static_assert(!std::is_constructible_v<Value, char32_t>);
#ifdef __cpp_char8_t
static_assert(!std::is_constructible_v<Value, char8_t>);
#endif

TEST(BasicTests, ValueFromIntegralTypesStoresInt)
{
    ExpectStoredAsInt(static_cast<short>(5), "short");
    ExpectStoredAsInt(static_cast<unsigned short>(5), "unsigned short");
    ExpectStoredAsInt(5, "int");
    ExpectStoredAsInt(5u, "unsigned");
    ExpectStoredAsInt(5L, "long");
    ExpectStoredAsInt(5UL, "unsigned long");
    ExpectStoredAsInt(5LL, "long long");
    ExpectStoredAsInt(5ULL, "unsigned long long");
    ExpectStoredAsInt(static_cast<signed char>(5), "signed char");
    ExpectStoredAsInt(static_cast<unsigned char>(5), "unsigned char");
    ExpectStoredAsInt(static_cast<int8_t>(5), "int8_t");
    ExpectStoredAsInt(static_cast<uint16_t>(5), "uint16_t");
    ExpectStoredAsInt(static_cast<uint32_t>(5), "uint32_t");
    ExpectStoredAsInt(static_cast<int64_t>(5), "int64_t");
    ExpectStoredAsInt(static_cast<uint64_t>(5), "uint64_t");
    ExpectStoredAsInt(static_cast<size_t>(5), "size_t");
    ExpectStoredAsInt(static_cast<std::ptrdiff_t>(5), "ptrdiff_t");
    const unsigned lvalue = 5;
    ExpectStoredAsInt<const unsigned&>(lvalue, "const unsigned&");

    EXPECT_TRUE(std::holds_alternative<bool>(Value(true).GetData()));
    EXPECT_TRUE(std::holds_alternative<double>(Value(1.5f).GetData()));
    EXPECT_TRUE(std::holds_alternative<double>(Value(1.5).GetData()));
    EXPECT_TRUE(std::holds_alternative<double>(Value(1.5L).GetData()));
    EXPECT_TRUE(std::holds_alternative<int64_t>(Reflect(5LL).GetData()));

    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ u }} {{ s }} {{ u + 1 }} {{ n is none }}"));
    EXPECT_EQ("5 5 6 True", tpl.RenderAsString({ { "u", 5u }, { "s", static_cast<size_t>(5) }, { "n", nullptr } }).value());
}

TEST(BasicTests, ValueFromNullptrIsNone)
{
    EXPECT_TRUE(Value(nullptr).IsNone());
    const Value v = nullptr;
    EXPECT_TRUE(v.IsNone());
}

// The 2.0 Value accessors (task 0072, docs/api-2.0.md section 2)
TEST(ValueApiTest, PredicatesNameTheStoredType)
{
    EXPECT_TRUE(Value().IsNone());
    EXPECT_TRUE(Value(true).IsBool());
    EXPECT_TRUE(Value(5).IsInt());
    EXPECT_TRUE(Value(1.5).IsDouble());
    EXPECT_TRUE(Value("s").IsString());
    EXPECT_TRUE(Value(L"s").IsWString());
    EXPECT_TRUE(Value(ValuesList{ 1 }).IsList());
    EXPECT_TRUE(Value(Reflect(std::vector<int>{ 1 })).IsList());
    EXPECT_TRUE(Value(ValuesMap{ { "k", 1 } }).IsMap());
    EXPECT_TRUE(Value(UserCallable()).IsCallable());

    EXPECT_FALSE(Value(5).IsDouble());
    EXPECT_FALSE(Value(true).IsInt());
    EXPECT_FALSE(Value(std::string_view("s")).IsString());
    EXPECT_FALSE(Value(1.5).IsNone());
}

TEST(ValueApiTest, AsReturnsReferencesAndThrowsOnOtherTypes)
{
    Value str("text");
    EXPECT_EQ("text", str.AsString());
    str.AsString() += "!";
    EXPECT_EQ("text!", str.As<std::string>());
    EXPECT_EQ(&str.AsString(), &str.As<std::string>());
    EXPECT_THROW((void)str.AsInt(), std::bad_variant_access);
    EXPECT_THROW((void)Value(5).AsString(), std::bad_variant_access);

    EXPECT_EQ(5, Value(5).AsInt());
    EXPECT_EQ(1.5, Value(1.5).AsDouble());
    EXPECT_TRUE(Value(true).AsBool());
    EXPECT_EQ(L"w", Value(L"w").AsWString());

    Value list(ValuesList{ 1, 2 });
    list.AsList().emplace_back(3);
    EXPECT_EQ(3U, list.As<ValuesList>().size());
    EXPECT_EQ(&list.AsList(), &list.As<ValuesList>());
    // AsList is the ValuesList alternative only; a GenericList is reached through ToGenericList
    EXPECT_THROW((void)Value(Reflect(std::vector<int>{ 1 })).AsList(), std::bad_variant_access);

    Value map(ValuesMap{ { "k", 1 } });
    map.AsMap()["n"] = 2;
    EXPECT_EQ(2U, map.AsMap().size());

    UserCallable uc([](const UserCallableParams&) { return Value(7); }, {});
    const Value callable(uc);
    EXPECT_EQ(7, callable.AsCallable().callable(UserCallableParams{}).AsInt());
}

// Out-parameters that point into the value refuse a temporary, which would leave them dangling
namespace
{
template<typename V, typename = void>
struct CanViewString : std::false_type
{
};
template<typename V>
struct CanViewString<V, std::void_t<decltype(std::declval<V>().AsString(std::declval<std::string_view&>()))>> : std::true_type
{
};
template<typename V, typename = void>
struct CanPointToList : std::false_type
{
};
template<typename V>
struct CanPointToList<V, std::void_t<decltype(std::declval<V>().AsList(std::declval<const ValuesList*&>()))>> : std::true_type
{
};
} // namespace
static_assert(CanViewString<const Value&>::value);
static_assert(CanViewString<Value&>::value);
static_assert(!CanViewString<Value>::value);
static_assert(CanPointToList<const Value&>::value);
static_assert(!CanPointToList<Value>::value);

TEST(ValueApiTest, NonThrowingAccessFillsOutOnlyOnMatch)
{
    const Value str("text");
    std::string_view view;
    ASSERT_TRUE(str.AsString(view));
    EXPECT_EQ("text", view);
    EXPECT_EQ(str.AsString().data(), view.data()); // no copy

    int64_t num = -1;
    EXPECT_FALSE(str.AsInt(num));
    EXPECT_EQ(-1, num);
    EXPECT_TRUE(Value(5).AsInt(num));
    EXPECT_EQ(5, num);

    double dbl = 0;
    EXPECT_TRUE(Value(2.5).AsDouble(dbl));
    EXPECT_EQ(2.5, dbl);
    bool flag = false;
    EXPECT_TRUE(Value(true).AsBool(flag));
    EXPECT_TRUE(flag);
    EXPECT_FALSE(Value(1).AsBool(flag));

    const Value wide(L"w");
    std::wstring_view wview;
    EXPECT_TRUE(wide.AsWString(wview));
    EXPECT_EQ(L"w", wview);
    EXPECT_FALSE(str.AsWString(wview));
    // A stored view is not a string: AsString(out) agrees with IsString
    const Value storedView(std::string_view("v"));
    EXPECT_FALSE(storedView.AsString(view));

    Value list(ValuesList{ 1 });
    const ValuesList* constList = nullptr;
    ASSERT_TRUE(std::as_const(list).AsList(constList));
    EXPECT_EQ(1U, constList->size());
    ValuesList* mutableList = nullptr;
    ASSERT_TRUE(list.AsList(mutableList));
    mutableList->emplace_back(2);
    EXPECT_EQ(2U, list.AsList().size());
    EXPECT_FALSE(str.AsList(constList));
    EXPECT_EQ(nullptr, constList);

    const ValuesMap* map = nullptr;
    const Value emptyMap(ValuesMap{});
    EXPECT_TRUE(emptyMap.AsMap(map));
    EXPECT_FALSE(list.AsMap(map));
    const UserCallable* callable = nullptr;
    const Value uc(UserCallable{});
    EXPECT_TRUE(uc.AsCallable(callable));
    EXPECT_FALSE(str.AsCallable(callable));

    static_assert(noexcept(str.AsString(view)));
    static_assert(noexcept(str.AsList(constList)));
    static_assert(noexcept(str.GetIf<std::string>()));
}

TEST(ValueApiTest, GetIfUnwrapsContainers)
{
    Value list(ValuesList{ 1 });
    ASSERT_NE(nullptr, list.GetIf<ValuesList>());
    EXPECT_EQ(&list.AsList(), list.GetIf<ValuesList>());
    // The stored alternative itself is reachable too
    EXPECT_NE(nullptr, list.GetIf<RecWrapper<ValuesList>>());
    EXPECT_EQ(nullptr, list.GetIf<ValuesMap>());
    EXPECT_EQ(nullptr, list.GetIf<std::string>());
    EXPECT_NE(nullptr, Value("s").GetIf<std::string>());
    EXPECT_NE(nullptr, std::as_const(list).GetIf<ValuesList>());
    EXPECT_NE(nullptr, Value(UserCallable()).GetIf<UserCallable>());
}

TEST(ValueApiTest, ToGenericViewsCoverBothRepresentations)
{
    const Value list(ValuesList{ 1, "a" });
    const GenericList fromValues = list.ToGenericList();
    ASSERT_TRUE(fromValues.IsValid());
    ASSERT_EQ(2U, fromValues.GetSize().value());
    EXPECT_EQ(1, fromValues.begin()->AsInt());
    EXPECT_EQ(2U, Value(Reflect(std::vector<int>{ 1, 2 })).ToGenericList().GetSize().value());
    EXPECT_FALSE(Value(5).ToGenericList().IsValid());

    const Value map(ValuesMap{ { "k", 2 } });
    const GenericMap fromMap = map.ToGenericMap();
    ASSERT_NE(nullptr, fromMap.GetAccessor());
    EXPECT_EQ(2, fromMap.GetValueByName("k").AsInt());
    EXPECT_EQ(nullptr, Value(5).ToGenericMap().GetAccessor());

    // From a temporary the view owns the container
    const GenericList owned = Value(ValuesList{ 1, 2, 3 }).ToGenericList();
    EXPECT_EQ(3U, owned.GetSize().value());
    EXPECT_EQ("[1, 2, 3]", Value(owned).ToString());
    const GenericMap ownedMap = Value(ValuesMap{ { "a", 1 } }).ToGenericMap();
    EXPECT_EQ(1, ownedMap.GetValueByName("a").AsInt());
}

TEST(ValueApiTest, MovedFromContainerIsAbsent)
{
    Value list(ValuesList{ 1 });
    const Value moved(std::move(list));
    EXPECT_EQ(1U, moved.AsList().size());
    // NOLINTNEXTLINE(bugprone-use-after-move,clang-analyzer-cplusplus.Move): the moved-from state is the test
    EXPECT_EQ(nullptr, list.GetIf<ValuesList>());
    const ValuesList* out = nullptr;
    EXPECT_FALSE(list.AsList(out)); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move)
}

TEST(ValueApiTest, VisitUnwrapsContainers)
{
    auto kind = [](const auto& item) -> std::string {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, ValuesList>)
        {
            return "list of " + std::to_string(item.size());
        }
        else if constexpr (std::is_same_v<T, ValuesMap>)
        {
            return "map";
        }
        else if constexpr (std::is_same_v<T, UserCallable>)
        {
            return "callable";
        }
        else if constexpr (std::is_same_v<T, int64_t>)
        {
            return "int";
        }
        else
        {
            return "other";
        }
    };
    EXPECT_EQ("list of 2", Visit(kind, Value(ValuesList{ 1, 2 })));
    EXPECT_EQ("map", Visit(kind, Value(ValuesMap{})));
    EXPECT_EQ("callable", Visit(kind, Value(UserCallable())));
    EXPECT_EQ("int", Visit(kind, Value(5)));

    Value list(ValuesList{ 1 });
    Visit(
        [](auto& item) {
            if constexpr (std::is_same_v<std::decay_t<decltype(item)>, ValuesList>)
            {
                item.emplace_back(2);
            }
        },
        list);
    EXPECT_EQ(2U, list.AsList().size());
}

// ToString prints as {{ value }} does, Python str() (decision 3b)
TEST(ValueApiTest, ToStringPrintsAsTheTemplateDoes)
{
    const ValuesList values{ 5,
                             1.5,
                             true,
                             Value(),
                             "text",
                             L"wide",
                             ValuesList{ 1, "a", Value(), 2.0 },
                             ValuesMap{ { "k", 2 }, { "a", ValuesList{ true } } },
                             Reflect(std::vector<int>{ 1, 2 }),
                             1e100,
                             -0.0,
                             Value(ValuesMap{ { "a b", 1 }, { "", ValuesList{ L"w" } } }).ToGenericMap(),
                             ValuesList{} };
    const std::vector<std::string> expected{ "5", "1.5", "True", "None", "text", "wide", "[1, 'a', None, 2.0]",
                                             "{'k': 2, 'a': [True]}", "[1, 2]", "1e+100", "-0.0",
                                             "{'a b': 1, '': ['w']}", "[]" };
    ASSERT_EQ(expected.size(), values.size());
    for (size_t idx = 0; idx < values.size(); ++idx)
    {
        const Value& val = values[idx];
        EXPECT_EQ(expected[idx], val.ToString()) << idx;
        EXPECT_EQ(expected[idx], ToString(val)) << idx;
        EXPECT_EQ(ConvertString<std::wstring>(expected[idx]), val.ToWString()) << idx;
        EXPECT_EQ(ConvertString<std::wstring>(expected[idx]), ToWString(val)) << idx;

        Template tpl;
        ASSERT_TRUE(tpl.Load("{{ v }}"));
        EXPECT_EQ(tpl.RenderAsString({ { "v", val } }).value(), val.ToString()) << idx;
    }
    EXPECT_EQ("5", ToString(5));
    EXPECT_EQ("0", ToString(0)); // a number, not a null pointer, with string_helpers.h included
    EXPECT_EQ("None", ToString(nullptr));
    EXPECT_EQ("", Value(UserCallable()).ToString());
}

TEST(ValueApiTest, ToStringAppendsAndToCharsFillsABuffer)
{
    std::string out = "n=";
    Value(42).ToString(out);
    Value(ValuesList{ 1 }).ToString(out);
    EXPECT_EQ("n=42[1]", out);
    std::wstring wout = L"n=";
    Value(42).ToWString(wout);
    EXPECT_EQ(L"n=42", wout);

    char buf[8];
    auto res = Value(12345).ToChars(std::begin(buf), std::end(buf));
    EXPECT_EQ(std::errc(), res.ec);
    EXPECT_EQ("12345", std::string(buf, res.ptr));

    res = Value("too long for eight").ToChars(std::begin(buf), std::end(buf));
    EXPECT_EQ(std::errc::value_too_large, res.ec);
    EXPECT_EQ(std::end(buf), res.ptr);

    res = Value("12345678").ToChars(std::begin(buf), std::end(buf));
    EXPECT_EQ(std::errc(), res.ec);
    EXPECT_EQ(std::end(buf), res.ptr);
    const Value none;
    static_assert(noexcept(none.ToChars(nullptr, nullptr)));
}

#ifdef __cpp_lib_format
TEST(ValueApiTest, FormatterPrintsAsToString)
{
    EXPECT_EQ("5 [1, 'a'] None", std::format("{} {} {}", Value(5), Value(ValuesList{ 1, "a" }), Value()));
    EXPECT_EQ("   True", std::format("{:>7}", Value(true)));
}
#endif

TEST(ValueApiTest, StringConversionsBetweenEncodings)
{
    EXPECT_EQ("abc", ToString(std::wstring(L"abc")));
    EXPECT_EQ("abc", ToString(std::wstring_view(L"abc")));
    EXPECT_EQ("abc", ToString(std::string("abc")));
    EXPECT_EQ("abc", ToString("abc"));
    EXPECT_EQ("abc", ToString(L"abc"));
    EXPECT_EQ(L"abc", ToWString(std::string("abc")));
    EXPECT_EQ(L"abc", ToWString(std::string_view("abc")));
    EXPECT_EQ(L"abc", ToWString("abc"));
    EXPECT_EQ(L"abc", ToWString(L"abc"));
}

static_assert(std::is_same_v<std::variant_alternative_t<0, Value::ValueData>, NoneValue>);
static_assert(!std::is_convertible_v<NoneValue, int>);
static_assert(!std::is_nothrow_constructible_v<Value, ValuesList&&>);
