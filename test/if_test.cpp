#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"

using namespace jinja2;

TEST(IfTest, IfTrueTest)
{
    std::string source = R"(
{% if TrueVal %}
Hello from Jinja template!
{% endif %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"TrueVal", true},
        {"FalseVal", true},
    };

    std::string result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Hello from Jinja template!
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(IfTest, IfFalseTest)
{
    std::string source = R"(
{% if FalseVal %}
Hello from Jinja template!
{% else %}
Else branch triggered!
{% endif %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"TrueVal", true},
        {"FalseVal", false},
    };

    std::string result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Else branch triggered!
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(IfTest, ElseIfTrueTest)
{
    std::string source = R"(
{% if FalseVal %}
Hello from Jinja template!
{% elif FalseVal %}
ElseIf 1 branch triggered!
{% elif TrueVal %}
ElseIf 2 branch triggered!
{% else %}
ElseIf branch triggered!
{% endif %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"TrueVal", true},
        {"FalseVal", false},
    };

    std::string result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
ElseIf 2 branch triggered!
)";
    EXPECT_EQ(expectedResult, result);
}
