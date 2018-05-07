#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"
#include "jinja2cpp/reflected_value.h"

using namespace jinja2;

TEST(ForLoopTest, IntegersLoop)
{
    std::string source = R"(
{% for i in its %}
a[{{i}}] = image[{{i}}];
{% endfor %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"its", ValuesList{0, 1, 2} }
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
a[0] = image[0];
a[1] = image[1];
a[2] = image[2];
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(ForLoopTest, InlineIntegersLoop)
{
    std::string source = R"(
{% for i in (0, 1, 2) %}
a[{{i}}] = image[{{i}}];
{% endfor %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
a[0] = image[0];
a[1] = image[1];
a[2] = image[2];
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(ForLoopTest, EmptyLoop)
{
    std::string source = R"(
{% for i in ints %}
a[{{i}}] = image[{{i}}];
{% endfor %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"ints", ValuesList()}
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(ForLoopTest, ReflectedIntegersLoop)
{
    std::string source = R"(
{% for i in its %}
a[{{i}}] = image[{{i}}];
{% endfor %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"its", Reflect(std::vector<int64_t>{0, 1, 2} ) }
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
a[0] = image[0];
a[1] = image[1];
a[2] = image[2];
)";
    EXPECT_EQ(expectedResult, result);
}

struct RangeParam
{
    std::string range;
    std::string result;
    friend std::ostream& operator << (std::ostream& os, const RangeParam& rp)
    {
        os << rp.range << " -> " << rp.result;
        return os;
    }
};

class RanageForLoopTest : public ::testing::TestWithParam<RangeParam>
{  
};

TEST_P(RanageForLoopTest, IntegersRangeLoop)
{
    auto& testParam = GetParam();
    std::string source = "{% for i in " + testParam.range + " %}{{i}},{% endfor %}";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

INSTANTIATE_TEST_CASE_P(RangeParamResolving, RanageForLoopTest, ::testing::Values(
        RangeParam{"range(3)", "0,1,2,"},
        RangeParam{"range(stop=3)", "0,1,2,"},
        RangeParam{"range(start=3)", ""},
        RangeParam{"range(5, start=2)", "2,3,4,"},
        RangeParam{"range(stop=5, 2)", "2,3,4,"},
        RangeParam{"range(stop=5, start=2)", "2,3,4,"},
        RangeParam{"range(1, 4)", "1,2,3,"},
        RangeParam{"range(0, 8, 2)", "0,2,4,6,"},
        RangeParam{"range(6, -2, -2)", "6,4,2,0,"},
        RangeParam{"range(0, 8, step=2)", "0,2,4,6,"},
        RangeParam{"range(0, stop=8, 2)", "0,2,4,6,"},
        RangeParam{"range(start=0, 8, 2)", "0,2,4,6,"},
        RangeParam{"range(start=0, 8, step=2)", "0,2,4,6,"},
        RangeParam{"range(0, stop=8, step=2)", "0,2,4,6,"},
        RangeParam{"range(start=0, 8, step=2)", "0,2,4,6,"}
        ));

TEST(ForLoopTest, LoopCycleLoop)
{
    std::string source = R"(
{% for i in range(5) %}
a[{{i}}] = image[{{loop.cycle(2, 4, 6)}}];
{% endfor %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
a[0] = image[2];
a[1] = image[4];
a[2] = image[6];
a[3] = image[2];
a[4] = image[4];
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(ForLoopTest, LoopCycle2Loop)
{
    std::string source = R"(
{% for i in range(5) %}
a[{{i}}] = image[{{loop.cycle("a", "b", "c")}}];
{% endfor %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
a[0] = image[a];
a[1] = image[b];
a[2] = image[c];
a[3] = image[a];
a[4] = image[b];
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(ForLoopTest, LoopVariable)
{
    std::string source = R"(
{% for i in its %}
length={{loop.length}}, index={{loop.index}}, index0={{loop.index0}}, first={{loop.first}}, last={{loop.last}}, previtem={{loop.previtem}}, nextitem={{loop.nextitem}};
{% endfor %}
)";

    Template mytemplate;
    ASSERT_TRUE(mytemplate.Load(source));

    ValuesMap params = {
        {"its", ValuesList{0, 1, 2} }
    };

    std::string result = mytemplate.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
length=3, index=1, index0=0, first=true, last=false, previtem=, nextitem=1;
length=3, index=2, index0=1, first=false, last=false, previtem=0, nextitem=2;
length=3, index=3, index0=2, first=false, last=true, previtem=1, nextitem=;
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(ForLoopTest, SimpleContainerLoop)
{
    std::string source = R"(
{% for i,name in images %}
a[{{i}}] = "{{name}}_{{loop.index0}}";
{% endfor %}
)";


    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"images", ValuesList{
                             ValuesMap{{"i", Value(1)}, {"name", "image1"}},
                             ValuesMap{{"i", Value(2)}, {"name", "image2"}},
                             ValuesMap{{"i", Value(3)}, {"name", "image3"}},
                         }
        }
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
a[1] = "image1_0";
a[2] = "image2_1";
a[3] = "image3_2";
)";

    EXPECT_EQ(expectedResult, result);
}

TEST(ForLoopTest, SimpleNestedLoop)
{
    std::string source = R"(
{% for i in outers %}a[{{i}}] = image[{{i}}];
{% for j in inners %}b[{{j}}] = image[{{j}}];
{% endfor %}
{% endfor %}
)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));

    ValuesMap params = {
        {"outers", ValuesList{0, 1, 2} },
        {"inners", ValuesList{0, 1}}
    };

    std::string result = tpl.RenderAsString(params);
    std::cout << "[" << result << "]" << std::endl;
    std::string expectedResult = R"DELIM(
a[0] = image[0];
b[0] = image[0];
b[1] = image[1];
a[1] = image[1];
b[0] = image[0];
b[1] = image[1];
a[2] = image[2];
b[0] = image[0];
b[1] = image[1];
)DELIM";

    EXPECT_EQ(expectedResult, result);
}


