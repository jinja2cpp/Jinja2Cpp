#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"

using namespace jinja2;

constexpr int Iterations = 10000;

#if !defined(NDEBUG) || defined(_DEBUG)
#define PerfTests DISABLED_PerfTests
#else
#define PerfTests PerfTests
#endif

TEST(PerfTests, PlainText)
{
    std::string source = "Hello World from Parser!";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    jinja2::ValuesMap params;

    auto renderResult = tpl.RenderAsString(params);
    if (!renderResult)
    {
        std::cout << "Render error: " << renderResult.error() << std::endl;
        ASSERT_FALSE(true);
    }
    std::cout << renderResult.value() << std::endl;
    std::string result;
    for (int n = 0; n < Iterations * 100; ++ n)
        result = tpl.RenderAsString(params).value();

    std::cout << result << std::endl;
}

TEST(PerfTests, SimpleSubstituteText)
{
    std::string source = "{{ message }} from Parser!";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    jinja2::ValuesMap params = {{"message", "Hello World!"}};

    std::cout << tpl.RenderAsString(params).value() << std::endl;
    std::string result;
    for (int n = 0; n < Iterations * 100; ++ n)
        result = tpl.RenderAsString(params).value();

    std::cout << result << std::endl;
}

TEST(PerfTests, ValueSubstituteText)
{
    std::string source = "{{ message }} from Parser!";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    jinja2::ValuesMap params = {{"message", 100500}};

    std::cout << tpl.RenderAsString(params).value() << std::endl;
    std::string result;
    for (int n = 0; n < Iterations * 100; ++ n)
        result = tpl.RenderAsString(params).value();

    std::cout << result << std::endl;
}

TEST(PerfTests, SimpleSubstituteFilterText)
{
    std::string source = "{{ message | upper }} from Parser!";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    jinja2::ValuesMap params = {{"message", "Hello World!"}};

    std::cout << tpl.RenderAsString(params).value() << std::endl;
    std::string result;
    for (int n = 0; n < Iterations * 100; ++ n)
        result = tpl.RenderAsString(params).value();

    std::cout << result << std::endl;
}

TEST(PerfTests, DoubleSubstituteText)
{
    std::string source = "{{ message }} from Parser! - {{number}}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    jinja2::ValuesMap params = {{"message", "Hello World!"},
                                {"number", 10}};

    std::cout << tpl.RenderAsString(params).value() << std::endl;
    std::string result;
    for (int n = 0; n < Iterations * 100; ++ n)
        result = tpl.RenderAsString(params).value();

    std::cout << result << std::endl;
}

TEST(PerfTests, ForLoopText)
{
    std::string source = "{% for i in range(20)%} {{i}} {%endfor%}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    jinja2::ValuesMap params = {};

    auto renderResult = tpl.RenderAsString(params);
    if (!renderResult)
    {
        std::cout << "Render error: " << renderResult.error() << std::endl;
        ASSERT_FALSE(true);
    }
    std::cout << renderResult.value() << std::endl;
    std::string result;
    for (int n = 0; n < Iterations * 20; ++ n)
        tpl.RenderAsString(params);

    std::cout << result << std::endl;
}

TEST(PerfTests, ForLoopParamText)
{
    std::string source = "{% for i in range(num)%} {{i}} {%endfor%}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    jinja2::ValuesMap params = {{"num", 20}};

    std::cout << tpl.RenderAsString(params).value() << std::endl;
    std::string result;
    for (int n = 0; n < Iterations * 20; ++ n)
        result = tpl.RenderAsString(params).value();

    std::cout << result << std::endl;
}

TEST(PerfTests, ForLoopIndexText)
{
    std::string source = "{% for i in range(20)%} {{i ~ '-' ~ loop.index}} {%endfor%}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    jinja2::ValuesMap params = {};

    std::cout << tpl.RenderAsString(params).value() << std::endl;
    std::string result;
    for (int n = 0; n < Iterations * 20; ++ n)
        result = tpl.RenderAsString(params).value();

    std::cout << result << std::endl;
}

TEST(PerfTests, ForLoopIfText)
{
    std::string source = "{% for i in range(20) if i is odd %} {{i}} {%endfor%}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    jinja2::ValuesMap params = {};

    std::cout << tpl.RenderAsString(params).value() << std::endl;
    std::string result;
    for (int n = 0; n < Iterations * 20; ++ n)
        result = tpl.RenderAsString(params).value();

    std::cout << result << std::endl;
}
