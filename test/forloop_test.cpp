#include "test_tools.h"

#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <array>
#include <cstdint>
#include <forward_list>
#include <iterator>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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
)")
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
)")
{
}

MULTISTR_TEST(ForLoopTest, TupleUnpackLoop,
              R"(
{% for a, b in [(0, 1), (1, 2), (2, 3)] %}
a[{{a}}] = image[{{b}}];
{% endfor %}
)",
              //---------
              R"(

a[0] = image[1];

a[1] = image[2];

a[2] = image[3];
)")
{
}

// A pair unpacked into two names goes straight into their slots (docs/tasks/0088)
// clang-format off
MULTISTR_TEST(ForLoopTest, PairUnpackLoop,
              R"({% for k, v in {'b': 2, 'a': 1}|dictsort %}{{ k }}={{ v }}{{ loop.previtem }};{% endfor %}|{% for k, v in {'a': 1}.items() %}{{ k }}{{ v }}{% endfor %}|{% for k, k in {'a': 1}|dictsort %}{{ k }}{% endfor %}|{% for k, v in [] %}{% else %}{{ k is defined }}{% endfor %})",
              //---------
              R"(a=1;b=2('a', 1);|a1|1|False)")
{
}
// clang-format on

TEST(ForLoopErrorTest, PairUnpackCountError)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{% for a, b, c in {'a': 1}|dictsort %}{% endfor %}"));
    auto result = tpl.RenderAsString(jinja2::ValuesMap{});
    ASSERT_FALSE(result);
    EXPECT_NE(result.error().ToString().find("not enough values to unpack (expected 3, got 2)"), std::string::npos);
}

MULTISTR_TEST(ForLoopTest, DISABLED_MapUnpackLoop,
  R"(
{% for a, b in ({'0'='1', '1'='2', '2'='3'}).items() %}
a[{{a}}] = image[{{b}}];
{% endfor %}
)",
//---------
  R"(

a[0] = image[1];

a[1] = image[2];

a[2] = image[3];

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
)")
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
)")
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
)")
{
    params = {
        {"its", Reflect(std::vector<int64_t>{0, 1, 2} ) }
    };
}

struct RangeForLoopTesstTag {};
using RangeForLoopTest = InputOutputPairTest<RangeForLoopTesstTag>;

TEST_P(RangeForLoopTest, IntegersRangeLoop)
{
    const auto& testParam = GetParam();
    std::string source = "{% for i in " + testParam.tpl + " %}{{i}},{% endfor %}";

    PerformBothTests(source, testParam.result, {});
}

INSTANTIATE_TEST_SUITE_P(RangeParamResolving, RangeForLoopTest, ::testing::Values(
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

INSTANTIATE_TEST_SUITE_P(SequencesLoopTest, RangeForLoopTest, ::testing::Values(
        InputOutputPair{"[1, 2, 3]", "1,2,3,"},
        InputOutputPair{"[1, 2, 3] + [4, 5, 6]", "1,2,3,4,5,6,"},
        InputOutputPair{"[1, 2] * 3", "1,2,1,2,1,2,"},
        InputOutputPair{"'123456'", "1,2,3,4,5,6,"},
        InputOutputPair{"'123' + '456'", "1,2,3,4,5,6,"},
        InputOutputPair{"{'0'='1'}", "0,"}
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
)")
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
)")
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
)")
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
)")
{
}

// clang-format off
MULTISTR_TEST(ForLoopTest, LoopVariableWithIf,
R"(
{% for i in its if i is even%}
{{i}} length={{loop.length}}, index={{loop.index}}, index0={{loop.index0}}, first={{loop.first}}, last={{loop.last}}, previtem={{loop.previtem}}, nextitem={{loop.nextitem}};
{% endfor %}
)",
//-----------
R"(

0 length=3, index=1, index0=0, first=True, last=False, previtem=, nextitem=2;

2 length=3, index=2, index0=1, first=False, last=False, previtem=0, nextitem=4;

4 length=3, index=3, index0=2, first=False, last=True, previtem=2, nextitem=;
)"
)
{
    params = {
        {"its", ValuesList{0, 1, 2, 3, 4} }
    };
}
// clang-format on

// clang-format off
MULTISTR_TEST(ForLoopTest, LoopVariable,
R"(
{% for i in its %}
length={{loop.length}}, index={{loop.index}}, index0={{loop.index0}}, first={{loop.first}}, last={{loop.last}}, previtem={{loop.previtem}}, nextitem={{loop.nextitem}};
{% endfor %}
)",
//--------------
R"(

length=3, index=1, index0=0, first=True, last=False, previtem=, nextitem=1;

length=3, index=2, index0=1, first=False, last=False, previtem=0, nextitem=2;

length=3, index=3, index0=2, first=False, last=True, previtem=1, nextitem=;
)"
)
{
    params = {
        {"its", ValuesList{0, 1, 2} }
    };
}
// clang-format on

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
              "\n\nroot1(1-0) -> child1_1(2-1) -> child1_2(2-1) -> child1_3(2-1) -> root2(1-0) -> child2_1(2-1) -> child2_2(2-1) -> child2_3(2-1) -> root3(1-0) -> child3_1(2-1) -> child3_2(2-1) -> child3_3(2-1) -> ")
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
<empty>)")
{
    params = {
        { "input", jinja2::MakeGenericList([cur = 10]() mutable -> std::optional<Value> {
              if (cur > 90)
                  return std::optional<Value>();

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
<empty>)";

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
>10<>20<>30<>40<>50<>60<>70<>80<>90<)";

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
>10<>20<>30<>40<>50<>60<>70<>80<>90<)";

    PerformBothTests(source, expectedResult, params);
}

// A loop entered again reuses the frame, enumerator and scope of its last run
// (docs/tasks/0133): nothing of one run may show in the next
TEST_F(ForLoopTestSingle, ReenteredLoopStartsClean)
{
    ValuesMap params = {
        { "rows", ValuesList{ ValuesList{ 1, 2, 3 }, ValuesList{ 4 }, ValuesList{}, ValuesList{ 5, 6 } } },
        { "oddRows", ValuesList{ ValuesList{ 1, 2, 3 }, ValuesList{ 4, 6 }, ValuesList{ 5 } } },
        { "dicts", ValuesList{ ValuesMap{ { "a", 1 }, { "b", 2 } }, ValuesMap{ { "c", 3 } } } },
        { "c", 5 },
    };
    const std::pair<std::string, std::string> cases[] = {
        // Lists of other lengths, and an empty one, through the reused enumerator
        { "{% for row in rows %}[{% for c in row %}{{ loop.index }}/{{ loop.length }}:{{ c }}{% if not loop.last %},{% endif %}{% else %}E{% endfor %}]{% endfor %}",
          "[1/3:1,2/3:2,3/3:3][1/1:4][E][1/2:5,2/2:6]" },
        // A loop kept past its run keeps its state; the next run gets another frame
        { "{% set ns = namespace() %}{% for i in [1, 2] %}{% for j in [10, 20, 30] %}{% if i == 1 %}{% set ns.x = loop %}{% endif %}{{ loop.index }}{% endfor %};{% endfor %}{{ ns.x.length }}{{ ns.x.index }}{{ ns.x.previtem }}",
          "123;123;3320" },
        // The names another loop left in a reused frame are not seen
        { "{% for i in [1] %}{% for c in [7] %}{% endfor %}{% endfor %}{% for i in [1] %}{% for d in [8] %}{{ c }}{% endfor %}{% endfor %}", "5" },
        { "{% set c = 'outer' %}{% for r in [[1], [2]] %}{% for c in r %}{{ c }}{% endfor %}{{ c }};{% endfor %}{{ c }}", "1outer;2outer;outer" },
        // Unpacking into the scope kept between runs
        { "{% for r in [[[1, 2, 3]], [[4, 5, 6], [7, 8, 9]]] %}{% for a, b, c in r %}{{ a }}{{ b }}{{ c }}{% endfor %};{% endfor %}", "123;456789;" },
        { "{% for r in dicts %}{% for k, v in r | dictsort %}{{ k }}={{ v }} {% endfor %};{% endfor %}", "a=1 b=2 ;c=3 ;" },
        // loop.changed() starts over in each run
        { "{% for r in [[1, 1], [1, 2]] %}{% for c in r %}{{ loop.changed(c) }} {% endfor %};{% endfor %}", "True False ;True True ;" },
        // A filtered loop, which makes its own enumerator
        { "{% for r in oddRows %}{% for c in r if c is odd %}{{ c }}{% else %}-{% endfor %};{% endfor %}", "13;-;5;" },
        // A recursive loop runs in more than one frame of the same loop at once
        { "{% for n in [[1, [2, [3]]], [4]] recursive %}<{% if n is sequence %}{{ loop(n) }}{% else %}{{ n }}{{ loop.depth }}{% endif %}>{% endfor %}",
          "<<12><<23><<34>>>><<42>>" },
    };
    for (const auto& [source, expected] : cases)
    {
        SCOPED_TRACE(source);
        PerformBothTests(source, expected, params);
    }
}

// A loop left by an error is not reused half-run: the next render starts clean
TEST_F(ForLoopTestSingle, LoopLeftByErrorIsNotReused)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% for r in rows %}{% for c in r %}{{ c }}{{ 1 // c }}{% endfor %};{% endfor %}").has_value());
    ValuesMap failing = {
        { "rows", ValuesList{ ValuesList{ 1, 2 }, ValuesList{ 0 } } }
    };
    // Python raises ZeroDivisionError; Jinja2C++ renders what it can or reports an error
    (void)tpl.RenderAsString(failing);
    ValuesMap params = {
        { "rows", ValuesList{ ValuesList{ 1, 2 }, ValuesList{ 3 } } }
    };
    auto result = tpl.RenderAsString(params);
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    EXPECT_EQ("1120;30;", result.value());
}
