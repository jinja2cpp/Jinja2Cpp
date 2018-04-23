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
