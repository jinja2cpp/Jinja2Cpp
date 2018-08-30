#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"

using namespace jinja2;

TEST(BasicTests, PlainSingleLineTemplateProcessing)
{
    std::string source = "Hello World from Parser!";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = "Hello World from Parser!";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, PlainSingleLineTemplateProcessing_Wide)
{
    std::wstring source = L"Hello World from Parser!";

    TemplateW tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::wstring result = tpl.RenderAsString(ValuesMap{});
    std::wcout << result << std::endl;
    std::wstring expectedResult = L"Hello World from Parser!";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, PlainMultiLineTemplateProcessing)
{
    std::string source = R"(Hello World
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, InlineCommentsSkip)
{
    std::string source = "Hello World {#Comment to skip #}from Parser!";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = "Hello World from Parser!";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, SingleLineCommentsSkip)
{
    std::string source = R"(Hello World
{#Comment to skip #}
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, MultiLineCommentsSkip)
{
    std::string source = R"(Hello World
{#Comment to
skip #}
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, CommentedOutCodeSkip)
{
    std::string source = R"(Hello World
{#Comment to
            {{for}}
            {{endfor}}
skip #}
{#Comment to
             {%
 skip #}
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_1)
{
    std::string source = R"(Hello World
      {{- ' --' }}
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
 --
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
 --<
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_4)
{
    std::string source = R"(Hello World
      {%- set delim = ' --' %} {{+delim}}<
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
  --<
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(BasicTests, StripLSpaces_5)
{
    std::string source = R"(Hello World
      {%- set delim = ' --' %} {{-delim}}<
from Parser!)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
 --<
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
 --
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

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World
 --
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World> -- <
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

    std::string result = tpl.RenderAsString(ValuesMap{});
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

    std::string result = tpl.RenderAsString(ValuesMap{});
    std::cout << result << std::endl;
    std::string expectedResult = R"(Hello World --
> -- <
from Parser!)";
    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}
