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

TEST(PerfTests, DISABLED_TestMatsuhiko)
{
    std::string source = R"(
<!doctype html>
<html>
  <head>
    <title>{{ page_title }}</title>
  </head>
  <body>
    <div class="header">
      <h1>{{ page_title }}</h1>
    </div>
    <ul class="navigation">
    {% for href, caption in [
        {'href'='index.html', 'caption'='Index'},
        {'href'='downloads.html', 'caption'='Downloads'},
        {'href'='products.html', 'caption'='Products'}
      ] %}
      <li><a href="{{ href }}">{{ caption }}</a></li>
    {% endfor %}
    </ul>
    <div class="table">
      <table>
      {% for row in table %}
        <tr>
        {% for cell in row|list %}
          <td>{{ cell }}</td>
        {% endfor %}
        </tr>
      {% endfor %}
      </table>
    </div>
  </body>
</html>
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    jinja2::ValuesMap params = {};
    params["page_title"] = "mitsuhiko's benchmark";
    // jinja2::ValuesMap dictEntry = {{"a", 1}, {"b", 2}, {"c", 3}, {"d", 4}, {"e", 5}, {"f", 6}, {"g",7}, {"h", 8}, {"i", 9}, {"j", 10}};
    jinja2::ValuesList dictEntry = {"a", "b", "c", "d", "e", "f", "g", "h",  "i",  "j"};
    jinja2::ValuesList table;
    for (int n = 0; n < 1000; ++ n)
        table.push_back(jinja2::Value(dictEntry));
    params["table"] = std::move(table);

//    std::cout << tpl.RenderAsString(params).value() << std::endl;
    std::string result;
    for (int n = 0; n < 5000; ++ n)
        tpl.RenderAsString(params).value();

//    std::cout << result << std::endl;
}
