#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"
#include "test_tools.h"

using namespace jinja2;

using ExpressionsMultiStrTest = BasicTemplateRenderer;

// clang-format off
MULTISTR_TEST(ExpressionsMultiStrTest, BinaryMathOperations,
R"(
{{ 1 + 10 }}
{{ 1 - 10}}
{{ 0.1 + 1 }}
{{ 0.1 - 10.5 }}
{{ 1 * 10 }}
{{ 7 / 3}}
{{ 7 // 3 }}
{{ 7 % intValue }}
{{ 11 % 7 }}
{{ 3 ** 4 }}
{{ 10 ** -2 }}
{{ 10/10 + 2*5 }}
{{ 10 - 2 - 4 }}
{{ 200 / 2 / 2 }}
{{ ([1, 2] + [3, 4]) | pprint }}
{{ 'Hello' + " " + 'World ' + stringValue }}
{{ 'Hello' + " " + 'World ' + wstringValue }}
{{ stringValue + ' ' + wstringValue }}
{{ wstringValue + ' ' + stringValue }}
{{ wstringValue + ' ' + wstringValue }}
{{ stringValue + ' ' + stringValue }}
{{ 'Hello' ~ " " ~ 123 ~ ' ' ~ 1.234 ~ " " ~ true ~ " " ~ intValue ~ " " ~ false ~ ' ' ~ 'World ' ~ stringValue  ~ ' ' ~ wstringValue}}
{{ 'abc' * 0 }}
{{ 'abc' * 1 }}
{{ '123' * intValue }}
{{ ([1, 2, 3] * intValue) | pprint }}
{{ stringValue * intValue }}
{{ wstringValue * intValue }}
)",
//-----------
R"(
11
-9
1.1
-10.4
10
2.3333333333333335
2
1
4
81
0.01
11.0
4
50.0
[1, 2, 3, 4]
Hello World rain
Hello World rain
rain rain
rain rain
rain rain
rain rain
Hello 123 1.234 True 3 False World rain rain

abc
123123123
[1, 2, 3, 1, 2, 3, 1, 2, 3]
rainrainrain
rainrainrain)")
{
    params = {
        {"intValue", 3},
        {"doubleValue", 12.123F},
        {"stringValue", "rain"},
        {"wstringValue", std::wstring(L"rain")},
        {"boolFalseValue", false},
        {"boolTrueValue", true},
    };
}
// clang-format on

// clang-format off
MULTISTR_TEST(ExpressionsMultiStrTest, IfExpression,
R"(
{{ intValue if intValue is eq(3) }}
{{ stringValue if intValue < 3 else doubleValue }}
{{ wstringValue if intValue == 3 else doubleValue }}
)",
//-----------
R"(
3
12.123000144958496
rain)")
{
    params = {
        {"intValue", 3},
        {"doubleValue", 12.123F},
        {"stringValue", "rain"},
        {"wstringValue", std::wstring(L"rain")},
        {"boolFalseValue", false},
        {"boolTrueValue", true},
    };
}
// clang-format on

MULTISTR_TEST(ExpressionsMultiStrTest, EmptyDict,
              R"(
{% set d = {} %}
{{ d.asdf|default(42) }}
)",
              //-----------
              R"(

42)")
{
}

TEST(ExpressionTest, DoStatement)
{
    std::string source = R"(
{{ data.strValue }}{% do setData('Inner Value') %}
{{ data.strValue }}
)";

    TemplateEnv env;
    env.GetSettings().extensions.doStatement = true;

    TestInnerStruct innerStruct;
    innerStruct.strValue = "Outer Value";

    ValuesMap params = {
        {"data", Reflect(&innerStruct)},
        {"setData", MakeCallable(
            [&innerStruct](const std::string& val) -> Value {
                innerStruct.strValue = val;
                return "String not to be shown";
            },
            ArgInfo{"val"})
            },
    };

    Template tpl(&env);

    ASSERT_TRUE(tpl.Load(source));
    std::string result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Outer Value
Inner Value)";

    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}

TEST(ExpressionTest, MutatingMethodsKeepCallerData)
{
    // l.append() changes the list for the rest of the render, never the caller's ValuesMap
    std::string source = R"({% do l.append(4) %}{% do d.update({'c': 3}) %}{% do n.k.append(2) %}{{ l }}|{{ d|length }}|{{ n.k }})";

    TemplateEnv env;
    env.GetSettings().extensions.doStatement = true;

    ValuesMap params = {
        { "l", ValuesList{ 1, 2, 3 } },
        { "d", ValuesMap{ { "a", 1 }, { "b", 2 } } },
        { "n", ValuesMap{ { "k", ValuesList{ 1 } } } },
    };

    Template tpl(&env);
    ASSERT_TRUE(tpl.Load(source));
    for (int pass = 0; pass != 2; ++pass)
        EXPECT_EQ("[1, 2, 3, 4]|3|[1, 2]", tpl.RenderAsString(params).value());
    EXPECT_EQ(3U, params["l"].asList().size());
    EXPECT_EQ(2U, params["d"].asMap().size());
}

TEST(ExpressionTest, MethodsOnReflectedValues)
{
    // A reflected struct's fields come before dict methods; its string fields have str methods
    std::string source = R"({{ data.strValue.upper() }}|{{ data.strValue.split()|length }}|{{ data.get('strValue') }})";

    TestInnerStruct innerStruct;
    innerStruct.strValue = "Outer Value";
    ValuesMap params = { { "data", Reflect(&innerStruct) } };

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
    EXPECT_EQ("OUTER VALUE|2|Outer Value", tpl.RenderAsString(params).value());
}

TEST(ExpressionTest, SelfContainingListIsRefused)
{
    TemplateEnv env;
    env.GetSettings().extensions.doStatement = true;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set x = [] %}{% do x.append(x) %}{{ x }}"));
    EXPECT_FALSE(tpl.RenderAsString(ValuesMap{}).has_value());
}

// clang-format off
TEST(ExpressionsTest, PipeOperatorPrecedenceTest)
{
    const std::string source = R"(>> {{ 2 < '6' | int }} <<
  >> {{ -30 | abs < str | int }} <<)";

    Template tpl;
    ASSERT_TRUE(tpl.Load(source));
   
    const ValuesMap params = {{"str", "20"}};

    const auto result = tpl.RenderAsString(params).value();
    std::cout << result << std::endl;
    const std::string expectedResult = R"(>> True <<
  >> False <<)";

    EXPECT_STREQ(expectedResult.c_str(), result.c_str());
}
// clang-format on

struct LogicalExprTestTag;
using LogicalExprTest = InputOutputPairTest<LogicalExprTestTag>;

TEST_P(LogicalExprTest, Test)
{
    const auto& testParam = GetParam();
    std::string source = "{{ 'true' if " + testParam.tpl + " else 'false' }}";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    std::string result = tpl.RenderAsString(PrepareTestData()).value();
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

GTEST_ALLOW_UNINSTANTIATED_PARAMETERIZED_TEST(LogicalExprTest);

SUBSTITUTION_TEST_P(ExpressionSubstitutionTest)

// clang-format off
INSTANTIATE_TEST_SUITE_P(ConstantSubstitutionTest, ExpressionSubstitutionTest, ::testing::Values(
                            InputOutputPair{"'str1'",            "str1"},
                            InputOutputPair{"\"str1\"",          "str1"},
                            InputOutputPair{"100500",            "100500"},
                            InputOutputPair{"'100.555'",         "100.555"},
                            InputOutputPair{"true",              "True"},
                            InputOutputPair{"false",             "False"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(LogicalExpressionTest, ExpressionSubstitutionTest, ::testing::Values(
                            InputOutputPair{"true",            "True"},
                            InputOutputPair{"1 == 1",            "True"},
                            InputOutputPair{"1 != 1",            "False"},
                            InputOutputPair{"2 > 1",             "True"},
                            InputOutputPair{"1 > 1",            "False"},
                            InputOutputPair{"2 >= 1",             "True"},
                            InputOutputPair{"1 >= 1",            "True"},
                            InputOutputPair{"1 < 2",             "True"},
                            InputOutputPair{"1 < 1",            "False"},
                            InputOutputPair{"1 <= 2",             "True"},
                            InputOutputPair{"1 <= 1",            "True"},
                            InputOutputPair{"1 == 2 or 2 == 2",  "True"},
                            InputOutputPair{"2 == 2 or 1 == 2",  "True"},
                            InputOutputPair{"1 == 2 or 3 == 2",  "False"},
                            InputOutputPair{"1 == 2 and 2 == 2",  "False"},
                            InputOutputPair{"2 == 2 and 1 == 2",  "False"},
                            InputOutputPair{"1 == 2 and 3 == 2",  "False"},
                            InputOutputPair{"1 == 1 and 2 == 2",  "True"},
                            InputOutputPair{"not (1 == 2) and 2 == 2",  "True"},
                            InputOutputPair{"not false",         "True"},
                            InputOutputPair{"true and true and true",         "True"},
                            InputOutputPair{"false",             "False"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(BasicValueSubstitutionTest, ExpressionSubstitutionTest, ::testing::Values(
                            InputOutputPair{"intValue",       "3"},
                            InputOutputPair{"doubleValue",    "12.123000144958496"},
                            InputOutputPair{"stringValue",    "rain"},
                            InputOutputPair{"boolTrueValue",  "True"},
                            InputOutputPair{"boolFalseValue", "False"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(IndexSubscriptionTest, ExpressionSubstitutionTest, ::testing::Values(
                            InputOutputPair{"intValue[0]",               ""},
                            InputOutputPair{"doubleValue[0]",            ""},
                            InputOutputPair{"stringValue[0]",            "r"},
                            InputOutputPair{"stringValue[100]",          ""},
                            InputOutputPair{"boolTrueValue[0]",          ""},
                            InputOutputPair{"boolFalseValue[0]",         ""},
                            InputOutputPair{"intList[-1]",               "4"},
                            InputOutputPair{"intList[10]",               ""},
                            InputOutputPair{"intList[0]",                "9"},
                            InputOutputPair{"intList[9]",                "4"},
                            InputOutputPair{"intList[5]",                "2"},
                            InputOutputPair{"mapValue['intVal']",        "10"},
                            InputOutputPair{"mapValue['dblVal']",        "100.5"},
                            InputOutputPair{"mapValue['stringVal']",     "string100.5"},
                            InputOutputPair{"mapValue['boolValue']",     "True"},
                            InputOutputPair{"mapValue['intVAl']",        ""},
                            InputOutputPair{"mapValue[0]",               ""},
                            InputOutputPair{"(mapValue | dictsort | first)['key']", "boolValue"},
                            InputOutputPair{"(mapValue | dictsort | first)['value']", "True"},
                            InputOutputPair{ "reflectedStringVector[0]", "9" },
                            InputOutputPair{ "reflectedStringViewVector[0]", "9" },
                            InputOutputPair{"reflectedVal['intValue']",  "0"},
                            InputOutputPair{"reflectedVal['dblValue']",  "0.0"},
                            InputOutputPair{"reflectedVal['boolValue']", "False"},
                            InputOutputPair{"reflectedVal['strValue']",  "test string 0"},
                            InputOutputPair{"reflectedVal['StrValue']",  ""}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(DotSubscriptionTest, ExpressionSubstitutionTest, ::testing::Values(InputOutputPair{ "mapValue.intVal", "10" },
                                          InputOutputPair{ "mapValue.dblVal", "100.5" },
                                          InputOutputPair{ "mapValue.stringVal", "string100.5" },
                                          InputOutputPair{ "mapValue.boolValue", "True" },
                                          InputOutputPair{ "mapValue.intVAl", "" },
                                          InputOutputPair{ "reflectedVal.intValue", "0" },
                                          InputOutputPair{ "reflectedVal.dblValue", "0.0" },
                                          InputOutputPair{ "reflectedVal.boolValue", "False" },
                                          InputOutputPair{ "reflectedVal.strValue", "test string 0" },
                                          InputOutputPair{ "reflectedVal.wstrValue", "test string 0" },
                                          InputOutputPair{ "reflectedVal.strViewValue", "test string 0" },
                                          InputOutputPair{ "reflectedVal.wstrViewValue", "test string 0" },
                                          InputOutputPair{ "reflectedVal.StrValue", "" }));
// clang-format on


INSTANTIATE_TEST_SUITE_P(ComplexSubscriptionTest, ExpressionSubstitutionTest, ::testing::Values(
                            InputOutputPair{"mapValue.reflectedList[1]['intValue']",    "1"},
                            InputOutputPair{"mapValue['reflectedList'][1]['intValue']",    "1"},
                            InputOutputPair{"mapValue.reflectedList[1].intValue",    "1"},
                            InputOutputPair{"{'fieldName'='field', 'fieldValue'=10}.fieldName",    "field"},
                            InputOutputPair{"{'fieldName'='field', 'fieldValue'=10}['fieldValue']",    "10"},
                            InputOutputPair{R"( ([
                                                    {'fieldName'='field1', 'fieldValue'=10},
                                                    {'fieldName'='field2', 'fieldValue'=11},
                                                    {'fieldName'='field3', 'fieldValue'=12}
                                                 ] | last).fieldValue)",    "12"},
                            InputOutputPair{"reflectedList[1].intValue",    "1"},
                            InputOutputPair{"(reflectedList[1]).intValue",    "1"},
                            InputOutputPair{"(reflectedList | first).intValue",    "0"},
                            InputOutputPair{"reflectedList[1].strValue[0]",    "t"},
                            InputOutputPair{"(reflectedList[1]).strValue[0]",    "t"},
                            InputOutputPair{"(reflectedList | first).strValue[0]",    "t"},
                            InputOutputPair{"reflectedVal.strValue[0]",        "t"},
                            InputOutputPair{"reflectedVal.innerStruct.strValue", "Hello World!"},
                            InputOutputPair{"reflectedVal.innerStructList[5].strValue", "Hello World!"},
                            InputOutputPair{"reflectedVal.tmpStructList[5].strValue", "Hello World!"}
                            ));

namespace
{
// A map whose "self" key returns the map itself
struct SelfMap : jinja2::IMapItemAccessor
{
    [[nodiscard]] size_t GetSize() const override { return 1; }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return name == "self"; }
    [[nodiscard]] Value GetValueByName(const std::string&) const override
    {
        return GenericMap([this] { return this; });
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override { return { "self" }; }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override { return this == &other; }
};

// The list [1, <itself>]
struct SelfList : jinja2::IListItemAccessor
    , jinja2::IIndexBasedAccessor
{
    struct Enumerator : jinja2::IListEnumerator
    {
        explicit Enumerator(const SelfList* list)
            : m_list(list)
        {
        }
        void Reset() override { m_idx = -1; }
        bool MoveNext() override { return ++m_idx < 2; }
        [[nodiscard]] Value GetCurrent() const override { return m_list->GetItemByIndex(m_idx); }
        [[nodiscard]] jinja2::ListEnumeratorPtr Clone() const override { return MakeEnumerator<Enumerator>(*this); }
        jinja2::ListEnumeratorPtr Move() override { return MakeEnumerator<Enumerator>(*this); }
        [[nodiscard]] bool IsEqual(const IComparable& other) const override
        {
            const auto* val = dynamic_cast<const Enumerator*>(&other);
            return val != nullptr && val->m_list == m_list && val->m_idx == m_idx;
        }

        const SelfList* m_list;
        int64_t m_idx = -1;
    };

    [[nodiscard]] std::optional<size_t> GetSize() const override { return 2; }
    [[nodiscard]] const IIndexBasedAccessor* GetIndexer() const override { return this; }
    [[nodiscard]] std::optional<jinja2::ListEnumeratorPtr> CreateEnumerator() const override { return MakeEnumerator<Enumerator>(this); }
    [[nodiscard]] Value GetItemByIndex(int64_t idx) const override
    {
        if (idx == 0)
            return 1;
        return GenericList([this] { return this; });
    }
    [[nodiscard]] bool IsEqual(const IComparable& other) const override { return this == &other; }
};

std::string RenderNarrow(const std::string& source, const ValuesMap& params)
{
    Template tpl;
    EXPECT_TRUE(tpl.Load(source));
    return tpl.RenderAsString(params).value();
}

std::wstring RenderWide(const std::wstring& source, const ValuesMap& params)
{
    TemplateW tpl;
    EXPECT_TRUE(tpl.Load(source));
    return tpl.RenderAsString(params).value();
}
} // namespace

TEST(ValueReprTest, CyclicMapPrintsEllipsis)
{
    SelfMap cycle;
    ValuesMap params{ { "x", GenericMap([&cycle] { return &cycle; }) } };

    EXPECT_EQ("{'self': {...}}", RenderNarrow("{{ x }}", params));
    EXPECT_EQ(L"{'self': {...}}", RenderWide(L"{{ x }}", params));
    EXPECT_EQ("[{'self': {...}}, {'self': {...}}]", RenderNarrow("{{ [x, x] }}", params));
}

TEST(ValueReprTest, CyclicListPrintsEllipsis)
{
    SelfList cycle;
    ValuesMap params{ { "x", GenericList([&cycle] { return &cycle; }) } };

    EXPECT_EQ("[1, [...]]", RenderNarrow("{{ x }}", params));
    EXPECT_EQ(L"[1, [...]]", RenderWide(L"{{ x }}", params));
}

TEST(ValueReprTest, NonPrintableCharactersAreEscaped)
{
    // U+0085 (NEL), U+00A0 (NBSP), U+2028 (line separator), U+FEFF (BOM), U+E000 (private use)
    // are escaped as Python's repr() does; printable non-ASCII characters stay as they are
    ValuesMap params{ { "v", ValuesList{ std::string("a\xc2\x85"
                                                     "b\xc2\xa0"
                                                     "c\xe2\x80\xa8"
                                                     "d\xef\xbb\xbf\xee\x80\x80"),
                                         std::string("\xc3\xa9\xf0\x9f\x98\x80\xf0\x9f\xab\xa8") } } };
    EXPECT_EQ("['a\\x85b\\xa0c\\u2028d\\ufeff\\ue000', '\xc3\xa9\xf0\x9f\x98\x80\xf0\x9f\xab\xa8']", RenderNarrow("{{ v }}", params));

    // wide literals use \x escapes: universal character names below U+00A0 are ill-formed
    // before C++23, and raw UTF-8 depends on the compiler's source charset
    // U+1FAE8 is printable since Unicode 15, the database of the Python 3.12 oracle
    const uint32_t emojiCode = 0x1fae8;
    std::wstring emoji;
    if (sizeof(wchar_t) == 2)
        emoji = { static_cast<wchar_t>(0xd83e), static_cast<wchar_t>(0xdee8) };
    else
        emoji.push_back(static_cast<wchar_t>(emojiCode));
    ValuesMap wideParams{ { "v", ValuesList{ std::wstring(L"a\x85"
                                                          L"b\xa0"
                                                          L"c\x2028"
                                                          L"d\xfeff\xe000"),
                                             L"\xe9" + emoji } } };
    EXPECT_EQ(L"['a\\x85b\\xa0c\\u2028d\\ufeff\\ue000', '\xe9" + emoji + L"']", RenderWide(L"{{ v }}", wideParams));
}

TEST(ExpressionsTest, LongNumberLiteralsDoNotOverflow)
{
    // number literals longer than the lexer's old 35-character buffer overflowed the stack
    std::string digits(200, '1');
    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ 1." + digits + " }}"));
    EXPECT_EQ("1.1111111111111112", tpl.RenderAsString({}).value());

    TemplateW wtpl;
    ASSERT_TRUE(wtpl.Load(L"{{ 1." + std::wstring(200, L'1') + L" }}"));
    EXPECT_EQ(L"1.1111111111111112", wtpl.RenderAsString({}).value());
}
