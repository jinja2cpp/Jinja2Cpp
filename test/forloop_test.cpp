#include "test_tools.h"

#include "jinja2cpp/template.h"
#include "jinja2cpp/reflected_value.h"
#include "jinja2cpp/generic_list_impl.h"

#include <array>
#include <iostream>
#include <string>
#include <forward_list>

using namespace jinja2;

using ForLoopTest = BasicTemplateRenderer;

MULTISTR_TEST(ForLoopTest, IntegersLoop,
 R"(
{% for i in its %}
a[{{i}}] = image[{{i}}];
{% endfor %}
)",
//--------
R"(

a[0] = image[0];

a[1] = image[1];

a[2] = image[2];

)"
)
{
    params = {
        {"its", ValuesList{0, 1, 2} }
    };
}

MULTISTR_TEST(ForLoopTest, InlineIntegersLoop,
R"(
{% for i in (0, 1, 2) %}
a[{{i}}] = image[{{i}}];
{% endfor %}
)",
//---------
R"(

a[0] = image[0];

a[1] = image[1];

a[2] = image[2];

)"
)
{
}

MULTISTR_TEST(ForLoopTest, InnerVarsLoop,
    R"(
{% set var = 0 %}
{% for i in (0, 1, 2) %}
{% set var = var + i %}
a[{{i}}] = image[{{var}}];
{% endfor %}
)",
//---------
    R"(



a[0] = image[0];


a[1] = image[1];


a[2] = image[2];

)"
)
{
}

MULTISTR_TEST(ForLoopTest, EmptyLoop,
R"(
{% for i in ints %}
a[{{i}}] = image[{{i}}];
{% endfor %}
)",
//---------
R"(

)"
)
{
    params = {
        {"ints", ValuesList()}
    };
}

MULTISTR_TEST(ForLoopTest, ReflectedIntegersLoop,
R"(
{% for i in its %}
a[{{i}}] = image[{{i}}];
{% endfor %}
)",
//----------
R"(

a[0] = image[0];

a[1] = image[1];

a[2] = image[2];

)"
)
{
    params = {
        {"its", Reflect(std::vector<int64_t>{0, 1, 2} ) }
    };
}

struct RangeForLoopTesstTag {};
using RangeForLoopTest = InputOutputPairTest<RangeForLoopTesstTag>;

TEST_P(RangeForLoopTest, IntegersRangeLoop)
{
    auto& testParam = GetParam();
    std::string source = "{% for i in " + testParam.tpl + " %}{{i}},{% endfor %}";

    PerformBothTests(source, testParam.result, {});
}

INSTANTIATE_TEST_CASE_P(RangeParamResolving, RangeForLoopTest, ::testing::Values(
        InputOutputPair{"range(3)", "0,1,2,"},
        InputOutputPair{"range(stop=3)", "0,1,2,"},
        InputOutputPair{"range(start=3)", ""},
        InputOutputPair{"range(5, start=2)", "2,3,4,"},
        InputOutputPair{"range(stop=5, 2)", "2,3,4,"},
        InputOutputPair{"range(stop=5, start=2)", "2,3,4,"},
        InputOutputPair{"range(1, 4)", "1,2,3,"},
        InputOutputPair{"range(0, 8, 2)", "0,2,4,6,"},
        InputOutputPair{"range(6, -2, -2)", "6,4,2,0,"},
        InputOutputPair{"range(0, 8, step=2)", "0,2,4,6,"},
        InputOutputPair{"range(0, stop=8, 2)", "0,2,4,6,"},
        InputOutputPair{"range(start=0, 8, 2)", "0,2,4,6,"},
        InputOutputPair{"range(start=0, 8, step=2)", "0,2,4,6,"},
        InputOutputPair{"range(0, stop=8, step=2)", "0,2,4,6,"},
        InputOutputPair{"range(start=0, 8, step=2)", "0,2,4,6,"}
        ));

MULTISTR_TEST(ForLoopTest, LoopCycleLoop,
R"(
{% for i in range(5) %}
a[{{i}}] = image[{{loop.cycle(2, 4, 6)}}];
{% endfor %}
)",
//-----------
R"(

a[0] = image[2];

a[1] = image[4];

a[2] = image[6];

a[3] = image[2];

a[4] = image[4];

)"
)
{
}

MULTISTR_TEST(ForLoopTest, LoopCycle2Loop,
R"(
{% for i in range(5) %}
a[{{i}}] = image[{{loop.cycle("a", "b", "c")}}];
{% endfor %}
)",
//--------
R"(

a[0] = image[a];

a[1] = image[b];

a[2] = image[c];

a[3] = image[a];

a[4] = image[b];

)"
)
{
}

MULTISTR_TEST(ForLoopTest, LoopWithIf,
R"(
{% for i in range(10) if i is even %}
a[{{i}}] = image[{{i}}];
{% endfor %}
)",
//---------
R"(

a[0] = image[0];

a[2] = image[2];

a[4] = image[4];

a[6] = image[6];

a[8] = image[8];

)"
)
{
}

MULTISTR_TEST(ForLoopTest, LoopWithElse,
R"(
{% for i in idx%}
a[{{i}}] = image[{{i}}];
{% else %}
No indexes given
{% endfor %}
{% for i in range(0)%}
a[{{i}}] = image[{{i}}];
{% else %}
No indexes given
{% endfor %}
)",
//-------
R"(

No indexes given


No indexes given

)"
)
{
}

MULTISTR_TEST(ForLoopTest, LoopVariableWithIf,
R"(
{% for i in its if i is even%}
{{i}} length={{loop.length}}, index={{loop.index}}, index0={{loop.index0}}, first={{loop.first}}, last={{loop.last}}, previtem={{loop.previtem}}, nextitem={{loop.nextitem}};
{% endfor %}
)",
//-----------
R"(

0 length=3, index=1, index0=0, first=true, last=false, previtem=, nextitem=2;

2 length=3, index=2, index0=1, first=false, last=false, previtem=0, nextitem=4;

4 length=3, index=3, index0=2, first=false, last=true, previtem=2, nextitem=;

)"
)
{
    params = {
        {"its", ValuesList{0, 1, 2, 3, 4} }
    };
}

MULTISTR_TEST(ForLoopTest, LoopVariable,
R"(
{% for i in its %}
length={{loop.length}}, index={{loop.index}}, index0={{loop.index0}}, first={{loop.first}}, last={{loop.last}}, previtem={{loop.previtem}}, nextitem={{loop.nextitem}};
{% endfor %}
)",
//--------------
R"(

length=3, index=1, index0=0, first=true, last=false, previtem=, nextitem=1;

length=3, index=2, index0=1, first=false, last=false, previtem=0, nextitem=2;

length=3, index=3, index0=2, first=false, last=true, previtem=1, nextitem=;

)"
)
{
    params = {
        {"its", ValuesList{0, 1, 2} }
    };
}

MULTISTR_TEST(ForLoopTest, SimpleContainerLoop,
R"(
{% for i,name in images %}
a[{{i}}] = "{{name}}_{{loop.index0}}";
{% endfor %}
)",
//---------
R"(

a[1] = "image1_0";

a[2] = "image2_1";

a[3] = "image3_2";

)")
{
    params = {
        {"images", ValuesList{
                             ValuesMap{{"i", Value(1)}, {"name", "image1"}},
                             ValuesMap{{"i", Value(2)}, {"name", "image2"}},
                             ValuesMap{{"i", Value(3)}, {"name", "image3"}},
                         }
        }
    };
}

MULTISTR_TEST(ForLoopTest, SimpleNestedLoop,
R"(
{% for i in outers %}a[{{i}}] = image[{{i}}];
{% for j in inners %}b[{{j}}] = image[{{j}}];
{% endfor %}
{% endfor %}
)",
//-----------
R"(
a[0] = image[0];
b[0] = image[0];
b[1] = image[1];

a[1] = image[1];
b[0] = image[0];
b[1] = image[1];

a[2] = image[2];
b[0] = image[0];
b[1] = image[1];


)")
{
    params = {
        {"outers", ValuesList{0, 1, 2} },
        {"inners", ValuesList{0, 1}}
    };
}

MULTISTR_TEST(ForLoopTest, RecursiveLoop,
R"(
{%set items=[
    {'name'='root1', 'children'=[
            {'name'='child1_1'},
            {'name'='child1_2'},
            {'name'='child1_3'}
        ]},
    {'name'='root2', 'children'=[
            {'name'='child2_1'},
            {'name'='child2_2'},
            {'name'='child2_3'}
        ]},
    {'name'='root3', 'children'=[
            {'name'='child3_1'},
            {'name'='child3_2'},
            {'name'='child3_3'}
        ]}
    ] %}
{% for i in items recursive %}{{i.name}}({{ loop.depth }}-{{ loop.depth0 }}) -> {{loop(i.children)}}{% endfor %}
)",
//---------
"\n\nroot1(1-0) -> child1_1(2-1) -> child1_2(2-1) -> child1_3(2-1) -> root2(1-0) -> child2_1(2-1) -> child2_2(2-1) -> child2_3(2-1) -> root3(1-0) -> child3_1(2-1) -> child3_2(2-1) -> child3_3(2-1) -> \n"
)
{
}

MULTISTR_TEST(ForLoopTest, GenericListTest_Generator,
R"(
{{ input[0] | pprint }}
{% for i in input %}>{{ i }}<{% endfor %}
{% for i in input %}>{{ i }}<{% else %}<empty>{% endfor %}
)",
//----------
R"(
none
>10<>20<>30<>40<>50<>60<>70<>80<>90<
<empty>
)"
)
{
    params = {
        {"input", jinja2::MakeGenericList([cur = 10]() mutable -> nonstd::optional<Value> {
            if (cur > 90)
                return nonstd::optional<Value>();

            auto tmp = cur;
            cur += 10;
            return Value(tmp);
        }) }
    };
}

using ForLoopTestSingle = SubstitutionTestBase;

TEST_F(ForLoopTestSingle, GenericListTest_InputIterator)
{
    std::string source = R"(
{{ input[0] | pprint }}
{% for i in input %}>{{ i }}<{% endfor %}
{% for i in input %}>{{ i }}<{% else %}<empty>{% endfor %}
)";
    std::string sampleStr("10 20 30 40 50 60 70 80 90");
    std::istringstream is(sampleStr);

    ValuesMap params = {
        {"input", jinja2::MakeGenericList(std::istream_iterator<int>(is), std::istream_iterator<int>()) }
    };

    std::string expectedResult = R"(
none
>10<>20<>30<>40<>50<>60<>70<>80<>90<
<empty>
)";

    BasicTemplateRenderer::ExecuteTest<jinja2::Template>(source, expectedResult, params, "Narrow version");
}

TEST_F(ForLoopTestSingle, GenericListTest_ForwardIterator)
{
    std::string source = R"(
{{ input[0] | pprint }}
{% for i in input %}>{{ i }}<{% endfor %}
{% for i in input %}>{{ i }}<{% else %}<empty>{% endfor %}
)";
    std::forward_list<int> sampleList{10, 20, 30, 40, 50, 60, 70, 80, 90};

    ValuesMap params = {
        {"input", jinja2::MakeGenericList(begin(sampleList), end(sampleList)) }
    };

    std::string expectedResult = R"(
none
>10<>20<>30<>40<>50<>60<>70<>80<>90<
>10<>20<>30<>40<>50<>60<>70<>80<>90<
)";

    PerformBothTests(source, expectedResult, params);
}

TEST_F(ForLoopTestSingle, GenericListTest_RandomIterator)
{
    std::string source = R"(
{{ input[0] | pprint }}
{% for i in input %}>{{ i }}<{% endfor %}
{% for i in input %}>{{ i }}<{% else %}<empty>{% endfor %}
)";
    std::array<int, 9> sampleList{10, 20, 30, 40, 50, 60, 70, 80, 90};

    ValuesMap params = {
        {"input", jinja2::MakeGenericList(begin(sampleList), end(sampleList)) }
    };

    std::string expectedResult = R"(
10
>10<>20<>30<>40<>50<>60<>70<>80<>90<
>10<>20<>30<>40<>50<>60<>70<>80<>90<
)";

    PerformBothTests(source, expectedResult, params);
}
