#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/markup.h>
#include <jinja2cpp/generic_list_iterator.h> // IWYU pragma: keep (GenericList::iterator is only declared in generic_list.h)
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

using namespace jinja2;

std::string UrlProcessorGlobal(const std::string& urlLink, const std::string& labelName, int limitWord, const std::string& targetStr)
{
    std::string result = urlLink + "?";
    for (int n = 0; n < limitWord; ++n)
        result += labelName;
    result += "#" + targetStr;
    return result;
}

class UserCallableTest : public BasicTemplateRenderer
{
public:
    std::string UrlProcessor(const std::string& urlLink, const std::string& labelName, int limitWord, const std::string& targetStr) // NOLINT(readability-convert-member-functions-to-static): tests the member-function overload
    {
        return UrlProcessorGlobal(urlLink, labelName, limitWord, targetStr);
    }
    [[nodiscard]] std::string UrlProcessorConst(const std::string& urlLink, const std::string& labelName, int limitWord, const std::string& targetStr) const // NOLINT(readability-convert-member-functions-to-static): tests the member-function overload
    {
        return UrlProcessorGlobal(urlLink, labelName, limitWord, targetStr);
    }
};

MULTISTR_TEST(UserCallableTest, SimpleUserCallable,
              R"(
{{ test() }}
{{ test() }}
{{ test_wide() }}
{{ test_wide() }}
)",
              //------------
              R"(
Hello World!
Hello World!
Hello World!
Hello World!)")
{
    jinja2::UserCallable uc;
    uc.callable = [](auto&)->jinja2::Value {return "Hello World!";};
    jinja2::UserCallable ucWide;
    ucWide.callable = [](auto&)->jinja2::Value {return std::wstring(L"Hello World!"); };
    params["test"] = std::move(uc);
    params["test_wide"] = std::move(ucWide);
}

// Python compares functions by identity (docs/tasks/0145); a copy of a UserCallable is the same function
MULTISTR_TEST(UserCallableTest, CallableEqualsItself, R"({% set a = f %}{{ f == f }} {{ f != f }} {{ f == g }} {{ f != g }} {{ f == copy }} {{ a == f }} {{ f in [g, f] }} {{ f is sameas f }} {{ f is sameas g }} {{ [f, g, f, copy]|unique|list|length }} {{ f == 'f' }})",
              //------------
              R"(True False False True True True True True False 2 False)")
{
    jinja2::UserCallable f;
    f.callable = [](auto&) -> jinja2::Value { return "f"; };
    jinja2::UserCallable g;
    g.callable = [](auto&) -> jinja2::Value { return "f"; };
    params["copy"] = f;
    params["f"] = std::move(f);
    params["g"] = std::move(g);
}

MULTISTR_TEST(UserCallableTest, SimpleUserCallableWithParams1,
              R"(
{{ test('Hello', 'World!') }}
{{ test(str2='World!', str1='Hello') }}
)",
              //-------------
              R"(
Hello World!
Hello World!)")
{
    jinja2::UserCallable uc;
    uc.callable = [](auto& args) -> jinja2::Value {
        auto str1 = args["str1"];
        auto str2 = args["str2"];

        if (str1.IsString())
        {
            return str1.AsString() + " " + str2.AsString();
        }

        return str1.AsWString() + L" " + str2.AsWString();
    };
    uc.argsInfo = {{"str1", true}, {"str2", true}};
    params["test"] = std::move(uc);
}

MULTISTR_TEST(UserCallableTest, SimpleUserCallableWithParams2,
              R"(
{{ test('Hello', 'World!') }}
{{ test(str2='World!', str1='Hello') }}
{{ test(str2='World!') }}
{{ test('Hello') }}
{{ test_w('Hello', 'World!') }}
{{ test_w(str2='World!', str1='Hello') }}
{{ test_w(str2='World!') }}
{{ test_w('Hello') }}
{{ test2(['H', 'e', 'l', 'l', 'o']) }}
{{ test3("https://google.com", "label1", 3, "someTarget") }}
{{ test4("https://google.com", "label1", 3, "someTarget") }}
{{ test5("https://google.com", "label1", 3, "someTarget") }}
{{ test6("https://google.com", "label1", 3, "someTarget") }}
)",
              //-------------
              R"(
Hello World!
Hello World!
 World!
Hello default
Hello World!
Hello World!
 World!
Hello default
Hello
https://google.com?label1label1label1#someTarget
https://google.com?label1label1label1#someTarget
https://google.com?label1label1label1#someTarget
https://google.com?label1label1label1#someTarget)")
{
    params["test"] = MakeCallable(
                [](const std::string& str1, const std::string& str2) {
                    return str1 + " " + str2;
                },
                ArgInfo{"str1"}, ArgInfo{"str2", false, "default"}
    );
    params["test_w"] = MakeCallable(
                [](const std::wstring& str1, const std::wstring& str2) {
                    return str1 + L" " + str2;
                },
                ArgInfo{ "str1" }, ArgInfo{ "str2", false, "default" }
    );
    params["test2"] = MakeCallable(
        [](const GenericList& list) {
            std::ostringstream os;

            for (const auto& v : list)
                os << ToString(v);

            return os.str();
        },
        ArgInfo{"list"}
    );
    params["test3"] =
      MakeCallable([](auto& urlLink, auto& labelName, auto limitWord, auto& targetStr) { return UrlProcessorGlobal(urlLink, labelName, limitWord, targetStr); },
                   jinja2::ArgInfoT<const std::string&>{ "urlLink" },
                   jinja2::ArgInfoT<const std::string&>{ "labelName" },
                   jinja2::ArgInfoT<int>{ "limitWord", false, 0 },
                   jinja2::ArgInfoT<const std::string&>{ "targetStr", false, "" });
    params["test4"] = MakeCallable(UrlProcessorGlobal,
                                   jinja2::ArgInfo{ "urlLink" },
                                   jinja2::ArgInfo{ "labelName" },
                                   jinja2::ArgInfo{ "limitWord", false, 0 },
                                   jinja2::ArgInfo{ "targetStr", false, "" });
    params["test5"] = MakeCallable(&UserCallableTest::UrlProcessor,
                                   const_cast<UserCallableTest*>(&test),
                                   jinja2::ArgInfo{ "urlLink" },
                                   jinja2::ArgInfo{ "labelName" },
                                   jinja2::ArgInfo{ "limitWord", false, 0 },
                                   jinja2::ArgInfo{ "targetStr", false, "" });
    params["test6"] = MakeCallable(&UserCallableTest::UrlProcessorConst,
                                   &test,
                                   jinja2::ArgInfo{ "urlLink" },
                                   jinja2::ArgInfo{ "labelName" },
                                   jinja2::ArgInfo{ "limitWord", false, 0 },
                                   jinja2::ArgInfo{ "targetStr", false, "" });
}

TEST(UserCallableTestSingle, ReflectedCallable)
{
    std::string source = R"(
{% set callable = reflected.basicCallable %}{{ callable() }}
{{ reflected.basicCallable() }}
{% set inner = reflected.getInnerStruct() %}{{ inner.strValue }}
{% set innerValue = reflected.getInnerStructValue() %}{{ innerValue.strValue }}
{{ innerReflected.strValue }}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    TestStruct reflected;
    auto innerReflected = std::make_shared<TestInnerStruct>();
    innerReflected->strValue = "!!Hello World!!";
    reflected.intValue = 100500;
    reflected.innerStruct = std::make_shared<TestInnerStruct>();
    reflected.innerStruct->strValue = "Hello World!";
    {
        jinja2::ValuesMap params;
        params["reflected"] = jinja2::Reflect(reflected);
        params["innerReflected"] = jinja2::Reflect(innerReflected);

        std::string result = tpl.RenderAsString(params).value();
        std::cout << result << std::endl;
        std::string expectedResult = R"(
100500
100500
Hello World!
Hello World!
!!Hello World!!)";
        EXPECT_EQ(expectedResult, result);
    }
    EXPECT_EQ(1L, innerReflected.use_count());
    EXPECT_EQ(1L, reflected.innerStruct.use_count());
}

// Function pointers take the typed path (ArgInfoT); a view parameter given the other width keeps
// the converted text alive for the call (docs/tasks/0296)
std::string StringViewPtrFn(std::string_view val)
{
    return std::string(val);
}

std::wstring WStringViewPtrFn(std::wstring_view val)
{
    return std::wstring(val);
}

std::string_view ReturnStringViewPtrFn(std::string_view val)
{
    return val;
}

std::wstring_view ReturnWStringViewPtrFn(std::wstring_view val)
{
    return val;
}

std::string ConstStringViewPtrFn(const std::string_view& val)
{
    return std::string(val);
}

std::wstring ConstWStringViewPtrFn(const std::wstring_view& val)
{
    return std::wstring(val);
}

struct UserCallableParamConvertTestTag;
using UserCallableParamConvertTest = InputOutputPairTest<UserCallableParamConvertTestTag>;

TEST_P(UserCallableParamConvertTest, Test)
{
    const auto& testParam = GetParam();
    std::string source = "{{" + testParam.tpl + " | pprint }}";

    jinja2::ValuesMap params = PrepareTestData();

    params["BoolFn"] = MakeCallable([](bool val) {return val;}, ArgInfo{"val"});
    params["IntFn"] = MakeCallable([](int val) {return val;}, ArgInfo{"val"});
    params["Int64Fn"] = MakeCallable([](int64_t val) {return val;}, ArgInfo{"val"});
    params["DoubleFn"] = MakeCallable([](double val) {return val;}, ArgInfo{"val"});
    params["StringFn"] = MakeCallable([](const std::string& val) {return val;}, ArgInfo{"val"});
    params["WStringFn"] = MakeCallable([](const std::wstring& val) {return val;}, ArgInfo{"val"});
    params["StringViewFn"] = MakeCallable([](const std::string_view& val) { return std::string(val.begin(), val.end()); }, ArgInfo{ "val" });
    params["WStringViewFn"] = MakeCallable([](const std::wstring_view& val) { return std::wstring(val.begin(), val.end()); }, ArgInfo{ "val" });
    params["GListFn"] = MakeCallable([](const GenericList& val)
    {
        return val;
    }, ArgInfo{"val"});
    params["GMapFn"] = MakeCallable([](const GenericMap& val) {return val;}, ArgInfo{"val"});
    params["VarArgsFn"] = MakeCallable([](const ValuesList& val) {
        return val;
    }, ArgInfo{"*args"});
    params["VarKwArgsFn"] = MakeCallable([](const ValuesMap& val) {
        return val;
    }, ArgInfo{"**kwargs"});
    params["ContextArgFn"] =
      MakeCallable([](const GenericMap& val, const std::string& varName) { return val.GetValueByName(varName); }, ArgInfo{ "*context" }, ArgInfo{ "name" });
    params["ContextMidFn"] = MakeCallable(
        [](const std::string& prefix, const GenericMap& ctx, const std::string& varName) { return ValuesList{ prefix, ctx.GetValueByName(varName) }; },
        ArgInfo{ "prefix" },
        ArgInfo{ "*context" },
        ArgInfo{ "name" });
    params["ContextPairFn"] = MakeCallable([](const GenericMap&, const std::string& a, const std::string& b) { return ValuesList{ a, b }; },
                                           ArgInfo{ "*context" },
                                           ArgInfo{ "a" },
                                           ArgInfo{ "b" });
    params["StringViewPtrFn"] = MakeCallable(&StringViewPtrFn, ArgInfo{ "val" });
    params["WStringViewPtrFn"] = MakeCallable(&WStringViewPtrFn, ArgInfo{ "val" });
    params["ConstStringViewPtrFn"] = MakeCallable(&ConstStringViewPtrFn, ArgInfo{ "val" });
    params["ConstWStringViewPtrFn"] = MakeCallable(&ConstWStringViewPtrFn, ArgInfo{ "val" });
    params["ReturnStringViewPtrFn"] = MakeCallable(&ReturnStringViewPtrFn, ArgInfo{ "val" });
    params["ReturnWStringViewPtrFn"] = MakeCallable(&ReturnWStringViewPtrFn, ArgInfo{ "val" });
    params["ReturnStringViewFn"] = MakeCallable([](std::string_view val) { return val; }, ArgInfo{ "val" });
    params["ReturnWStringViewFn"] = MakeCallable([](std::wstring_view val) { return val; }, ArgInfo{ "val" });
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved): the row checks that an rvalue view binds
    params["RvalueStringViewFn"] = MakeCallable([](std::string_view&& val) { return std::string(val); }, ArgInfo{ "val" });
    params["longString"] = std::string(40, 'n');
    params["longWString"] = std::wstring(40, L'w');
    params["markupValue"] = Markup("<b>");
    params["wmarkupValue"] = WMarkup(L"<w>");
    params["test_value"] = 100500;

    PerformBothTests(source, testParam.result, params);
}

struct UserCallableFilterTestTag;
using UserCallableFilterTest = InputOutputPairTest<UserCallableFilterTestTag>;

TEST_P(UserCallableFilterTest, Test)
{
    const auto& testParam = GetParam();
    std::string source = "{{ " + testParam.tpl + " }}";

    jinja2::ValuesMap params = PrepareTestData();

    params["surround"] = MakeCallable([](const std::string& val, const std::string& before, const std::string& after) { return before + val + after; },
                                      ArgInfoT<const std::string&>{ "val" },
                                      ArgInfoT<const std::string&>{ "before", false, ">>> " },
                                      ArgInfoT<const std::string&>{ "after", false, " <<<" });
    params["joiner"] = MakeCallable([](const ValuesList& items, const std::string& delim) {
        std::ostringstream os;
        bool isFirst = true;
        for (const auto& v : items)
        {
            if (isFirst)
            {
                isFirst = false;
            }
            else
            {
                os << delim;
            }
            os << ToString(v);
        }
        return os.str();
    },
                                    ArgInfo{ "*args" }, ArgInfo{ "delim" });
    params["tester"] = MakeCallable([](const std::string& testValue, const std::string& pattern) {
        return testValue == pattern;
        }, ArgInfo{"testVal"}, ArgInfo{"pattern"});

    // Values used as filters and tests need Settings::userCallablesAsFilters (docs/tasks/0193)
    TemplateEnv env;
    env.GetSettings().userCallablesAsFilters = true;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load(source).has_value()) << source;
    EXPECT_EQ(testParam.result, tpl.RenderAsString(params).value()) << source;
    TemplateW tplW(&env);
    ASSERT_TRUE(tplW.Load(ConvertString<std::wstring>(source)).has_value()) << source;
    EXPECT_EQ(ConvertString<std::wstring>(testParam.result), tplW.RenderAsString(params).value()) << source;

    // Without it they are unknown: a load error, or a render error where Jinja2 defers the check
    Template plain;
    auto loaded = plain.Load(source);
    EXPECT_TRUE(!loaded || !plain.RenderAsString(params)) << source;
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(BoolParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"BoolFn()",                   "False"},
                            InputOutputPair{"BoolFn(true)", "True"}
                            ));
// clang-format on

INSTANTIATE_TEST_SUITE_P(IntParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"IntFn()",                   "0"},
                            InputOutputPair{"IntFn(10)", "10"},
                            InputOutputPair{"IntFn(10.123)", "10"}
                            ));

INSTANTIATE_TEST_SUITE_P(Int64ParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"Int64Fn()",                   "0"},
                            InputOutputPair{"Int64Fn(10)", "10"},
                            InputOutputPair{"Int64Fn(10.123)", "10"}
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(DoubleParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"DoubleFn()",                   "0.0"},
                            InputOutputPair{"DoubleFn(10)", "10.0"},
                            InputOutputPair{"DoubleFn(10.123)", "10.123"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(VarArgsParamsConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"VarArgsFn()",                   "[]"},
                            InputOutputPair{"VarArgsFn(10, 'abc', false)", "[10, 'abc', False]"},
                            InputOutputPair{"VarArgsFn(10.123, (1, 2, 3), arg=1)", "[10.123, [1, 2, 3]]"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(VarKwArgsParamsConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"VarKwArgsFn()",                   "{}"},
                            InputOutputPair{"VarKwArgsFn(arg1=10, arg2='abc', arg3=false) | dictsort",
                                            "[('arg1', 10), ('arg2', 'abc'), ('arg3', False)]"},
                            InputOutputPair{"VarKwArgsFn(1, arg1=10.123, arg2=(1, 2, 3)) | dictsort",
                                            "[('arg1', 10.123), ('arg2', [1, 2, 3])]"}
                            ));
// clang-format on

// **kwargs keep the call order (docs/tasks/0043)
INSTANTIATE_TEST_SUITE_P(VarKwArgsOrder, UserCallableParamConvertTest, ::testing::Values(InputOutputPair{ "VarKwArgsFn(z=1, a=2, m=3) | list", "['z', 'a', 'm']" }));

// `*context` takes no argument of the call, wherever it is declared (docs/tasks/0183)
// clang-format off
INSTANTIATE_TEST_SUITE_P(GlobalContextAccess, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"ContextArgFn(name='test_value')", "100500"},
                            InputOutputPair{"ContextArgFn('test_value')", "100500"},
                            InputOutputPair{"ContextMidFn('x', 'test_value')", "['x', 100500]"},
                            InputOutputPair{"ContextMidFn('x', name='test_value')", "['x', 100500]"},
                            InputOutputPair{"ContextMidFn(name='test_value', prefix='x')", "['x', 100500]"},
                            InputOutputPair{"ContextPairFn('1', '2')", "['1', '2']"},
                            InputOutputPair{"ContextPairFn('1')", "['1', '']"},
                            InputOutputPair{"ContextPairFn('1', b='2')", "['1', '2']"},
                            InputOutputPair{"ContextPairFn('1', '2', '3')", "['1', '2']"},
                            InputOutputPair{"StringFn(val='Hello World')", "'Hello World'"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(StringParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"StringFn()",                   "''"},
                            InputOutputPair{"StringFn('Hello World')", "'Hello World'"},
                            InputOutputPair{"StringFn(stringValue)", "'rain'"},
                            InputOutputPair{"StringFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"StringFn('<i>'|safe)", "'<i>'"}
                            ));

INSTANTIATE_TEST_SUITE_P(WStringParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"WStringFn()",                   "''"},
                            InputOutputPair{"WStringFn('Hello World')", "'Hello World'"},
                            InputOutputPair{"WStringFn(stringValue)", "'rain'"},
                            InputOutputPair{"WStringFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"WStringFn('<i>'|safe)", "'<i>'"}
                            ));

INSTANTIATE_TEST_SUITE_P(StringViewParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"StringViewFn()",                   "''"},
                            InputOutputPair{"StringViewFn('Hello World')", "'Hello World'"},
                            InputOutputPair{"StringViewFn(stringValue)", "'rain'"},
                            InputOutputPair{"StringViewFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"StringViewFn('<i>'|safe)", "'<i>'"}
                            ));

INSTANTIATE_TEST_SUITE_P(StringViewPointerParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"StringViewPtrFn('Hello World')", "'Hello World'"},
                            // An omitted argument reaches a function pointer as it reaches a lambda: default-constructed (docs/tasks/0320)
                            InputOutputPair{"StringViewPtrFn()", "''"},
                            InputOutputPair{"WStringViewPtrFn()", "''"},
                            InputOutputPair{"StringViewPtrFn(stringValue)", "'rain'"},
                            InputOutputPair{"StringViewPtrFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"StringViewPtrFn(longString)", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn'"},
                            InputOutputPair{"StringViewPtrFn(longWString)", "'wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww'"},
                            InputOutputPair{"StringViewPtrFn('<i>'|safe)", "'<i>'"},
                            InputOutputPair{"StringViewPtrFn(markupValue)", "'<b>'"},
                            InputOutputPair{"StringViewPtrFn(wmarkupValue)", "'<w>'"},
                            InputOutputPair{"WStringViewPtrFn('Hello World')", "'Hello World'"},
                            InputOutputPair{"WStringViewPtrFn(stringValue)", "'rain'"},
                            InputOutputPair{"WStringViewPtrFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"WStringViewPtrFn(longString)", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn'"},
                            InputOutputPair{"WStringViewPtrFn(longWString)", "'wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww'"},
                            InputOutputPair{"WStringViewPtrFn('<i>'|safe)", "'<i>'"},
                            InputOutputPair{"WStringViewPtrFn(markupValue)", "'<b>'"},
                            InputOutputPair{"WStringViewPtrFn(wmarkupValue)", "'<w>'"},
                            InputOutputPair{"ConstStringViewPtrFn('Hello World')", "'Hello World'"},
                            InputOutputPair{"ConstStringViewPtrFn(stringValue)", "'rain'"},
                            InputOutputPair{"ConstStringViewPtrFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"ConstStringViewPtrFn(longString)", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn'"},
                            InputOutputPair{"ConstStringViewPtrFn(longWString)", "'wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww'"},
                            InputOutputPair{"ConstStringViewPtrFn('<i>'|safe)", "'<i>'"},
                            InputOutputPair{"ConstStringViewPtrFn(markupValue)", "'<b>'"},
                            InputOutputPair{"ConstStringViewPtrFn(wmarkupValue)", "'<w>'"},
                            InputOutputPair{"ConstWStringViewPtrFn('Hello World')", "'Hello World'"},
                            InputOutputPair{"ConstWStringViewPtrFn(stringValue)", "'rain'"},
                            InputOutputPair{"ConstWStringViewPtrFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"ConstWStringViewPtrFn(longString)", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn'"},
                            InputOutputPair{"ConstWStringViewPtrFn(longWString)", "'wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww'"},
                            InputOutputPair{"ConstWStringViewPtrFn('<i>'|safe)", "'<i>'"},
                            InputOutputPair{"ConstWStringViewPtrFn(markupValue)", "'<b>'"},
                            InputOutputPair{"ConstWStringViewPtrFn(wmarkupValue)", "'<w>'"}
                            ));

// A returned view is copied: it may view an argument, which dies with the call; a lambda takes a
// view by value (docs/tasks/0296)
INSTANTIATE_TEST_SUITE_P(StringViewResultConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"ReturnStringViewPtrFn(longString)", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn'"},
                            InputOutputPair{"ReturnStringViewPtrFn(longWString)", "'wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww'"},
                            InputOutputPair{"ReturnStringViewPtrFn(longString ~ 'x')", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnx'"},
                            InputOutputPair{"ReturnStringViewPtrFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"ReturnWStringViewPtrFn(longString)", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn'"},
                            InputOutputPair{"ReturnWStringViewPtrFn(longWString)", "'wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww'"},
                            InputOutputPair{"ReturnWStringViewPtrFn(longString ~ 'x')", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnx'"},
                            InputOutputPair{"ReturnWStringViewPtrFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"ReturnStringViewFn(longString)", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn'"},
                            InputOutputPair{"ReturnStringViewFn(longWString)", "'wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww'"},
                            InputOutputPair{"ReturnStringViewFn(longString ~ 'x')", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnx'"},
                            InputOutputPair{"ReturnStringViewFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"ReturnWStringViewFn(longString)", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn'"},
                            InputOutputPair{"ReturnWStringViewFn(longWString)", "'wwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwwww'"},
                            InputOutputPair{"ReturnWStringViewFn(longString ~ 'x')", "'nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnx'"},
                            InputOutputPair{"ReturnWStringViewFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"RvalueStringViewFn(stringValue)", "'rain'"},
                            InputOutputPair{"RvalueStringViewFn(wstringValue)", "'  hello world '"}
                            ));

INSTANTIATE_TEST_SUITE_P(WStringViewParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"WStringViewFn()",                   "''"},
                            InputOutputPair{"WStringViewFn('Hello World')", "'Hello World'"},
                            InputOutputPair{"WStringViewFn(stringValue)", "'rain'"},
                            InputOutputPair{"WStringViewFn(wstringValue)", "'  hello world '"},
                            InputOutputPair{"WStringViewFn('<i>'|safe)", "'<i>'"}
                            ));

// clang-format off
INSTANTIATE_TEST_SUITE_P(ListParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"GListFn()",                   "[]"},
                            InputOutputPair{"GListFn([1, 2, 3, 4])",       "[1, 2, 3, 4]"},
                            InputOutputPair{"GListFn(intList)",            "[9, 0, 8, 1, 7, 2, 6, 3, 5, 4]"},
                            InputOutputPair{"GListFn(reflectedIntVector)", "[9, 0, 8, 1, 7, 2, 6, 3, 5, 4]"},
                            InputOutputPair{"GListFn(range(10))", "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"},
                            InputOutputPair{"GListFn([1, 2, 3, 4] | sort)","[1, 2, 3, 4]"},
                            InputOutputPair{"GListFn(intList | sort)",     "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"},
                            InputOutputPair{"GListFn(reflectedVal.tmpStructList)",
                                            "[{'strValue': 'Hello World!'},\n"
                                            " {'strValue': 'Hello World!'},\n"
                                            " {'strValue': 'Hello World!'},\n"
                                            " {'strValue': 'Hello World!'},\n"
                                            " {'strValue': 'Hello World!'},\n"
                                            " {'strValue': 'Hello World!'},\n"
                                            " {'strValue': 'Hello World!'},\n"
                                            " {'strValue': 'Hello World!'},\n"
                                            " {'strValue': 'Hello World!'},\n"
                                            " {'strValue': 'Hello World!'}]"},
                            InputOutputPair{"GListFn(reflectedIntVector | sort)", "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"},
                            InputOutputPair{"GListFn(range(10) | sort)", "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"},
                            InputOutputPair{"GListFn({'name': 'itemName', 'val': 'itemValue'} | list) | sort", "['name', 'val']"},
                            InputOutputPair{"GListFn({'name': 'itemName', 'val': 'itemValue'} | list | sort)", "['name', 'val']"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(MapParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"GMapFn()",                   "{}"},
                            InputOutputPair{"GMapFn({'key': 10})",         "{'key': 10}"},
                            InputOutputPair{"GMapFn(simpleMapValue) | dictsort",
                                            "[('boolValue', True),\n"
                                            " ('dblVal', 100.5),\n"
                                            " ('intVal', 10),\n"
                                            " ('stringVal', 'string100.5')]"},
                            InputOutputPair{"GMapFn(reflectedVal) | dictsort",
                                            "[('basicCallable', <callable>),\n"
                                            " ('boolValue', False),\n"
                                            " ('dblValue', 0.0),\n"
                                            " ('getInnerStruct', <callable>),\n"
                                            " ('getInnerStructValue', <callable>),\n"
                                            " ('innerStruct', {'strValue': 'Hello World!'}),\n"
                                            " ('innerStructList',\n"
                                            "  [{'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'}]),\n"
                                            " ('intEvenValue', 0),\n"
                                            " ('intValue', 0),\n"
                                            " ('strValue', 'test string 0'),\n"
                                            " ('strViewValue', 'test string 0'),\n"
                                            " ('tmpStructList',\n"
                                            "  [{'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'},\n"
                                            "   {'strValue': 'Hello World!'}]),\n"
                                            " ('wstrValue', 'test string 0'),\n"
                                            " ('wstrViewValue', 'test string 0')]" },
                    InputOutputPair{"GMapFn(reflectedVal.innerStruct) | dictsort", "[('strValue', 'Hello World!')]"}
                            ));
// clang-format on

// clang-format off
INSTANTIATE_TEST_SUITE_P(UserDefinedFilter, UserCallableFilterTest, ::testing::Values(
                            InputOutputPair{"'Hello World' | surround",                   ">>> Hello World <<<"},
                            InputOutputPair{"'Hello World' | surround(before='### ')",    "### Hello World <<<"},
                            InputOutputPair{"'Hello World' | surround(after=' ###', before='### ')",    "### Hello World ###"},
                            InputOutputPair{"'Hello World' | surround('### ', ' ###')",    "### Hello World ###"},
                            InputOutputPair{"'Hello World' | joiner('1', '2', '3', delim=', ')", "Hello World, 1, 2, 3"},
                            InputOutputPair{"'Hello World' | joiner(delim=', ')", "Hello World"},
                            InputOutputPair{"('A', 'B', 'C') | map('surround', before='> ', after=' <') | pprint", "['> A <', '> B <', '> C <']"},
                            InputOutputPair{"'str1' if 'str1' is tester('str1') else 'str2'", "str1"},
                            InputOutputPair{"'str1' if 'str3' is tester(pattern='str1') else 'str2'", "str2"},
                            InputOutputPair{"('str1', 'A', 'str1', 'B', 'str1', 'C') | select('tester', 'str1') | map('surround', before='> ', after=' <') | pprint", "['> str1 <', '> str1 <', '> str1 <']"}
                            ));
// clang-format on


// Filters and tests added to the environment (Jinja2 env.filters, env.tests) and Settings::finalize
TEST(EnvCallablesTest, FiltersAndTestsBindWhenTheTemplateIsLoaded)
{
    TemplateEnv env;
    env.AddFilter("tag", MakeCallable([](const std::string& s) { return "<" + s + ">"; }, ArgInfo{ "s" }));
    env.AddTest("short", MakeCallable([](const std::string& s) { return s.size() < 3; }, ArgInfo{ "s" }));
    // A global callable with the same name is not used as the filter
    env.AddGlobal("tag", MakeCallable([](const std::string& s) { return "global " + s; }, ArgInfo{ "s" }));

    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ 'a'|tag }} {{ 'ab' is short }} {{ 'abc' is short }} {{ 'tag' is filter }} {{ 'short' is test }}").has_value());
    EXPECT_EQ("<a> True False True True", tpl.RenderAsString({}).value());

    env.RemoveFilter("tag");
    env.RemoveTest("short");
    EXPECT_FALSE(env.FindFilter("tag"));
    EXPECT_FALSE(env.FindTest("short"));
    // Loaded templates keep the filters and tests they were loaded with, as in Jinja2; `is filter`
    // and `is test` look the name up when they run
    EXPECT_EQ("<a> True False False False", tpl.RenderAsString({}).value());

    // The global is no filter (docs/tasks/0193)...
    Template other(&env);
    auto loaded = other.Load("{{ 'a'|tag }}");
    ASSERT_FALSE(loaded.has_value());
    EXPECT_EQ(ErrorCode::UnknownFilter, loaded.error().GetCode());
    EXPECT_EQ("noname.j2tpl:1:8: error: No filter named 'tag'.\n{{ 'a'|tag }}\n    ---^-------", loaded.error().ToString());

    // ... unless Settings::userCallablesAsFilters looks such names up when they run
    env.GetSettings().userCallablesAsFilters = true;
    Template legacy(&env);
    ASSERT_TRUE(legacy.Load("{{ 'a'|tag }}").has_value());
    EXPECT_EQ("global a", legacy.RenderAsString({}).value());
}

TEST(EnvCallablesTest, WideTemplate)
{
    TemplateEnv env;
    env.AddFilter("tag", MakeCallable([](const std::wstring& s) { return L"<" + s + L">"; }, ArgInfo{ "s" }));
    TemplateW tpl(&env);
    ASSERT_TRUE(tpl.Load(L"{{ 'a'|tag }}").has_value());
    EXPECT_EQ(L"<a>", tpl.RenderAsString({}).value());
}

TEST(EnvCallablesTest, Finalize)
{
    Settings settings;
    EXPECT_EQ(Settings(), settings);
    settings.finalize = UserCallable([](const UserCallableParams& p) { return p["v"].IsNone() ? Value(std::string("-")) : p["v"]; }, { ArgInfo{ "v" } });
    EXPECT_NE(Settings(), settings);
    EXPECT_EQ(settings, Settings(settings));

    TemplateEnv env;
    env.SetSettings(settings);
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ none }}{{ 1 }}{{ missing }}").has_value());
    // An undefined value is not None: finalize gets its text, an empty string (docs/tasks/0293)
    EXPECT_EQ("-1", tpl.RenderAsString({}).value());
}

// What a callable throws reaches the caller as it was thrown, under UserCallableException (docs/tasks/0254)
TEST(EnvCallablesTest, ThrownExceptionIsKept)
{
    struct HostFailure
    {
        int code;
    };
    TemplateEnv env;
    env.AddFilter("fail", MakeCallable([](const std::string&) -> Value { throw HostFailure{ 7 }; }, ArgInfo{ "s" }));
    env.AddGlobal("raise", MakeCallable([]() -> Value { throw std::out_of_range("raised"); }));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ 'x' | fail }}").has_value());
    auto result = tpl.RenderAsString({});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::UserCallableException, result.error().GetCode());
    try
    {
        std::rethrow_exception(result.error().GetException());
    }
    catch (const HostFailure& failure)
    {
        EXPECT_EQ(7, failure.code);
    }

    ASSERT_TRUE(tpl.Load("{{ raise() }}").has_value());
    result = tpl.RenderAsString({});
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(ErrorCode::UserCallableException, result.error().GetCode());
    EXPECT_EQ("raised", result.error().GetExtraParams().at(0).AsString());
    EXPECT_THROW(std::rethrow_exception(result.error().GetException()), std::out_of_range);
}

TEST(EnvCallablesTest, KeptFilteredLoop)
{
    // A callable that keeps `loop` past a filtered loop still reads its length afterwards
    auto kept = std::make_shared<GenericMap>();
    TemplateEnv env;
    env.AddGlobal("keep", MakeCallable([kept](const GenericMap& v) { *kept = v; return std::string(); }, ArgInfo{ "v" }));
    env.AddGlobal("kept_length", MakeCallable([kept]() { return kept->GetValueByName("length"); }));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% for i in [1, 2, 3, 4, 5] if i is odd %}{{ keep(loop) if loop.first }}{{ i }}{% endfor %}|{{ kept_length() }}").has_value());
    EXPECT_EQ("135|3", tpl.RenderAsString({}).value());
}

TEST(EnvCallablesTest, KeptLoopAfterError)
{
    // A loop kept by a callable when its body fails can no longer fetch: its enumerator ran
    // over the statement's range, which the error unwound
    auto kept = std::make_shared<GenericMap>();
    TemplateEnv env;
    env.AddGlobal("keep", MakeCallable([kept](const GenericMap& v) { *kept = v; return std::string(); }, ArgInfo{ "v" }));
    env.AddGlobal("raise", MakeCallable([]() -> Value { throw std::out_of_range("raised"); }));
    for (const auto* source : { "{% for x in range(4) %}{{ keep(loop) }}{% set t = loop|first %}{{ raise() }}{% endfor %}",
                                "{% for x in range(8) if x > 1 %}{{ keep(loop) }}{% set t = loop|first %}{{ raise() }}{% endfor %}" })
    {
        Template tpl(&env);
        ASSERT_TRUE(tpl.Load(source).has_value()) << source;
        EXPECT_FALSE(tpl.RenderAsString({}).has_value()) << source;
        EXPECT_TRUE(kept->GetValueByName("last").IsBool()) << source;
        EXPECT_NO_THROW(static_cast<void>(kept->GetValueByName("nextitem"))) << source;
        EXPECT_NO_THROW(static_cast<void>(kept->GetValueByName("length"))) << source;
    }
}

// A reflected record for OwnedUserDataTest: its nested items are reflected from pointers
// into the record (docs/tasks/0125)
struct OwnedRec
{
    std::string name;
    std::vector<std::string> words;
};

// Its fields are invalid (default-constructed) containers, lent as they are
struct OwnedEmpty
{
};

namespace jinja2
{
template<>
struct TypeReflection<OwnedRec> : TypeReflected<OwnedRec>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            { "name", [](const OwnedRec& obj) { return Reflect(obj.name); } },
            { "words", [](const OwnedRec& obj) { return Reflect(obj.words); } },
        };
        return accessors;
    }
};
template<>
struct TypeReflection<OwnedEmpty> : TypeReflected<OwnedEmpty>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            { "lst", [](const OwnedEmpty&) { return Value(GenericList()); } },
            { "mp", [](const OwnedEmpty&) { return Value(GenericMap()); } },
        };
        return accessors;
    }
};
} // namespace jinja2

// Items of a list or dict a callable returns outlive the call's result: they used to be
// views into it (heap-use-after-free under ASan). Strings are longer than SSO; X and Y in
// the expected output stand for them.
class OwnedUserDataTest : public ::testing::TestWithParam<InputOutputPair>
{
};

TEST_P(OwnedUserDataTest, Test)
{
    const auto& testParam = GetParam();
    const std::string x(40, 'x');
    const std::string y(40, 'y');
    TemplateEnv env;
    env.AddGlobal("words", MakeCallable([x, y]() { return Value(ValuesList{ y, x }); }));
    env.AddGlobal("obj", MakeCallable([x, y]() { return Value(ValuesMap{ { "w", ValuesList{ y, x } } }); }));
    env.AddGlobal("m", MakeCallable([x, y]() { return Value(ValuesMap{ { "b", y }, { "a", x } }); }));
    env.AddGlobal("m1", MakeCallable([x]() { return Value(ValuesMap{ { "a", x } }); }));
    env.AddGlobal("rnested", MakeCallable([x, y]() { return Reflect(std::vector<std::vector<std::string>>{ { y, x }, { x } }); }));
    env.AddGlobal("rrec", MakeCallable([x, y]() { return Reflect(OwnedRec{ x, { y, x } }); }));
    env.AddGlobal("rempty", MakeCallable([]() { return Reflect(OwnedEmpty{}); }));
    env.AddGlobal("rrecs", MakeCallable([x, y]() { return Reflect(std::vector<OwnedRec>{ { y, { x } }, { x, { y, x } } }); }));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load(testParam.tpl).has_value());
    std::string expected;
    for (char ch : std::string(testParam.result))
    {
        if (ch == 'X')
        {
            expected += x;
        }
        else if (ch == 'Y')
        {
            expected += y;
        }
        else
        {
            expected += ch;
        }
    }
    EXPECT_EQ(expected, tpl.RenderAsString({}).value());
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(Escapes, OwnedUserDataTest, ::testing::Values(
                            InputOutputPair{ R"({% set ns = namespace(v="") %}{% for w in words() %}{% set ns.v = w %}{% endfor %}{{ ns.v }})", "X" },
                            InputOutputPair{ "{% set s = words() | sort %}{{ s | join(',') }}", "X,Y" },
                            InputOutputPair{ "{% set s = words() | select %}{{ s | join(',') }}", "Y,X" },
                            InputOutputPair{ "{% set s = words() | reverse %}{{ s | join(',') }}", "X,Y" },
                            InputOutputPair{ "{% set s = words() | list %}{{ s | join(',') }}", "Y,X" },
                            InputOutputPair{ R"({% set ns = namespace(v="") %}{% for w in obj().w %}{% set ns.v = w %}{% endfor %}{{ ns.v }})", "X" },
                            InputOutputPair{ "{% set s = obj().w | sort %}{{ s | join(',') }}", "X,Y" },
                            InputOutputPair{ R"({% set ns = namespace(v="") %}{% for k, v in m() | dictsort %}{% set ns.v = v %}{% endfor %}{{ ns.v }})", "Y" },
                            InputOutputPair{ R"({% set ns = namespace(v="") %}{% for k, v in m1().items() %}{% set ns.v = v %}{% endfor %}{{ ns.v }})", "X" },
                            InputOutputPair{ "{% set v = m().a %}{{ v }}", "X" },
                            InputOutputPair{ "{% set r = rnested() | sort %}{{ r }}", "[['X'], ['Y', 'X']]" },
                            InputOutputPair{ R"({% set ns = namespace(v=none) %}{% for it in rnested() %}{% set ns.v = it %}{% endfor %}{{ ns.v }})", "['X']" },
                            InputOutputPair{ "{% set r = rrec().copy() %}{{ r.words | join(',') }}", "Y,X" },
                            InputOutputPair{ "{% set r = rrec().items() | list | sort %}{{ r[1][1] | join(',') }}", "Y,X" },
                            InputOutputPair{ "{% set r = rrecs()[1:] %}{{ r[0].words | join(',') }}", "Y,X" },
                            InputOutputPair{ "{% set r = rrec().copy() %}{{ r | tojson }}", R"({"name": "X", "words": ["Y", "X"]})" },
                            InputOutputPair{ "{% set r = rrecs()[1:] %}{{ r | tojson }}", R"([{"name": "X", "words": ["Y", "X"]}])" },
                            InputOutputPair{ "{% set w = rrecs()[1].words %}{{ w | join(',') }}", "Y,X" },
                            InputOutputPair{ R"({% set ns = namespace(v=none) %}{% for r in rrecs() %}{% set ns.v = r.words %}{% endfor %}{{ ns.v | join(',') }})", "Y,X" },
                            InputOutputPair{ "{% set e = rempty() %}{{ e.lst | length }}|{{ e.mp | length }}|{{ e.lst | list }}", "0|0|[]" },
                            InputOutputPair{ "{% set r = rrecs() | sort(attribute='name') %}{{ r | map(attribute='words') | map('join') | join('|') }}", "YX|X" }));
// clang-format on

Markup MarkupIdentity(const Markup& v)
{
    return v;
}

WMarkup WMarkupIdentity(const WMarkup& v)
{
    return v;
}

// Markup from C++ (docs/tasks/0180): the expected outputs are Python Jinja2's, with markupsafe.Markup
// values for x, heap, l[0], m.k and the callable results
struct MarkupRow
{
    std::string tpl;
    std::string escaped; // autoescape on
    std::string plain;   // autoescape off
    friend std::ostream& operator<<(std::ostream& os, const MarkupRow& row) { return os << row.tpl; }
};

class MarkupTest : public ::testing::TestWithParam<MarkupRow>
{
protected:
    static ValuesMap Params()
    {
        ValuesMap params;
        params["x"] = Markup("<b>");
        params["wx"] = WMarkup(L"<b>");
        params["s"] = "<i>";
        params["heap"] = Markup("<b>" + std::string("x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&"));
        params["l"] = ValuesList{ Markup("<u>") };
        params["m"] = ValuesMap{ { "k", Markup("<k>") } };
        params["ret"] = UserCallable([](const UserCallableParams&) { return Value(Markup("<r>")); }, {});
        params["retList"] = UserCallable([](const UserCallableParams&) { return Value(ValuesList{ Markup("<a>"), "<b>" }); }, {});
        params["ident"] = UserCallable([](const UserCallableParams& p) { return p["v"]; }, { ArgInfo{ "v" } });
        params["kind"] = UserCallable([](const UserCallableParams& p) { return Value(p["v"].IsMarkup() || p["v"].IsWMarkup() ? "markup" : "string"); }, { ArgInfo{ "v" } });
        params["typed"] = MakeCallable([](const std::string& v) { return v; }, ArgInfo{ "v" });
        params["ws"] = std::wstring(L"<i>");
        params["typedMarkup"] = MakeCallable(&MarkupIdentity, ArgInfo{ "v" });
        params["typedWMarkup"] = MakeCallable(&WMarkupIdentity, ArgInfo{ "v" });
        params["lambdaMarkup"] = MakeCallable([](const Markup& v) { return v; }, ArgInfo{ "v" });
        return params;
    }

    template<typename TemplateT, typename CharT>
    static void Check(const std::string& source, const std::string& expected, bool autoescape)
    {
        TemplateEnv env;
        env.GetSettings().autoescape = autoescape;
        TemplateT tpl(&env);
        ASSERT_TRUE(tpl.Load(ConvertString<std::basic_string<CharT>>(source)).has_value()) << source;
        auto result = tpl.RenderAsString(Params());
        ASSERT_TRUE(result.has_value()) << source;
        EXPECT_EQ(ConvertString<std::basic_string<CharT>>(expected), result.value()) << source << " autoescape=" << autoescape;
    }
};

TEST_P(MarkupTest, Renders)
{
    const auto& row = GetParam();
    Check<Template, char>(row.tpl, row.escaped, true);
    Check<Template, char>(row.tpl, row.plain, false);
    Check<TemplateW, wchar_t>(row.tpl, row.escaped, true);
    Check<TemplateW, wchar_t>(row.tpl, row.plain, false);
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(Markup, MarkupTest, ::testing::Values(
    MarkupRow{ "{{ x }}", "<b>", "<b>" },
    MarkupRow{ "{{ wx }}", "<b>", "<b>" },
    MarkupRow{ "{{ s }}", "&lt;i&gt;", "<i>" },
    MarkupRow{ "{{ heap }}", "<b>x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&", "<b>x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&x&" },
    MarkupRow{ "{{ x is escaped }} {{ wx is escaped }} {{ s is escaped }} {{ heap is escaped }}", "True True False True", "True True False True" },
    MarkupRow{ "{{ x|e }}{{ s|e }}", "<b>&lt;i&gt;", "<b>&lt;i&gt;" },
    MarkupRow{ "{{ (s|e) is escaped }}", "True", "True" },
    MarkupRow{ "{{ x ~ s }}", "<b>&lt;i&gt;", "<b><i>" },
    MarkupRow{ "{{ s ~ x }}", "&lt;i&gt;<b>", "<i><b>" },
    MarkupRow{ "{{ x == '<b>' }}", "True", "True" },
    MarkupRow{ "{{ l[0] }}{{ m.k }}", "<u><k>", "<u><k>" },
    MarkupRow{ "{% for i in l %}{{ i }}{% endfor %}", "<u>", "<u>" },
    MarkupRow{ "{{ [x, s]|join }}", "<b>&lt;i&gt;", "<b><i>" },
    MarkupRow{ "{{ [x] }}", "[Markup(&#39;&lt;b&gt;&#39;)]", "[Markup('<b>')]" },
    MarkupRow{ "{{ [x, s] }}", "[Markup(&#39;&lt;b&gt;&#39;), &#39;&lt;i&gt;&#39;]", "[Markup('<b>'), '<i>']" },
    MarkupRow{ "{{ ret() }}", "<r>", "<r>" },
    MarkupRow{ "{{ retList()|join }}", "<a>&lt;b&gt;", "<a><b>" },
    MarkupRow{ "{{ ident(x) }}|{{ ident(s) }}", "<b>|&lt;i&gt;", "<b>|<i>" },
    MarkupRow{ "{{ kind(x) }} {{ kind(s) }} {{ kind(s|e) }} {{ kind(heap) }} {{ kind(s|safe) }} {{ kind(l[0]) }}", "markup string markup markup markup markup",
               "markup string markup markup markup markup" },
    MarkupRow{ "{{ x|tojson }}", "\"\\u003cb\\u003e\"", "\"\\u003cb\\u003e\"" },
    MarkupRow{ "{{ x|upper }}", "<B>", "<B>" },
    MarkupRow{ "{{ x|length }}", "3", "3" },
    MarkupRow{ "{{ x|replace('b','i') }}", "<i>", "<i>" },
    MarkupRow{ "{{ x|string }}", "<b>", "<b>" },
    // A typed std::string parameter receives the text of a Markup, escaped again when it comes back as a plain string
    MarkupRow{ "{{ typed(x) }}", "&lt;b&gt;", "<b>" },
    // A typed Markup parameter given text of the other width converts it: a Markup stays one, a string is escaped (docs/tasks/0296)
    MarkupRow{ "{{ typedMarkup(x) }}|{{ typedMarkup(wx) }}|{{ typedMarkup(s) }}|{{ typedMarkup(ws) }}", "<b>|<b>|&lt;i&gt;|&lt;i&gt;", "<b>|<b>|&lt;i&gt;|&lt;i&gt;" },
    MarkupRow{ "{{ typedWMarkup(x) }}|{{ typedWMarkup(wx) }}|{{ typedWMarkup(s) }}|{{ typedWMarkup(ws) }}", "<b>|<b>|&lt;i&gt;|&lt;i&gt;", "<b>|<b>|&lt;i&gt;|&lt;i&gt;" },
    // ... and so does a lambda's (docs/tasks/0320)
    MarkupRow{ "{{ lambdaMarkup(x) }}|{{ lambdaMarkup(wx) }}|{{ lambdaMarkup(s) }}|{{ lambdaMarkup(ws) }}", "<b>|<b>|&lt;i&gt;|&lt;i&gt;", "<b>|<b>|&lt;i&gt;|&lt;i&gt;" }
));
// clang-format on

TEST(MarkupValueTest, RoundTripsThroughTheContext)
{
    // A callable's Markup argument and result keep the flag, short and long
    UserCallable echo([](const UserCallableParams& p) { return p["v"]; }, { ArgInfo{ "v" } });
    ValuesMap params{ { "echo", echo }, { "short", Markup("<s>") }, { "long", Markup(std::string(100, '<')) } };
    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ echo(short) is escaped }} {{ echo(long) is escaped }} {{ echo(short|string) is escaped }} {{ echo('<') is escaped }}").has_value());
    EXPECT_EQ("True True True False", tpl.RenderAsString(params).value());
}

// A call that gives one parameter both by position and by keyword fails, as Python's TypeError
// "got multiple values for argument" (docs/tasks/0258); macros already reject it with
// Python's own message, "takes no keyword argument"
TEST(UserCallableTestSingle, PositionalAndKeywordForOneParameter)
{
    jinja2::ValuesMap params;
    params["f"] = MakeCallable([](const std::string& a, const std::string& b, const std::string& c) { return a + b + c; }, ArgInfo{ "a" }, ArgInfo{ "b" }, ArgInfo{ "c" });
    params["v"] = MakeCallable([](const std::string& a, const ValuesList& rest) { return a + std::to_string(rest.size()); }, ArgInfo{ "a" }, ArgInfo{ "*args" });
    params["k"] = MakeCallable([](const ValuesList& rest, const std::string& sep) { return std::to_string(rest.size()) + sep; }, ArgInfo{ "*args" }, ArgInfo{ "sep" });
    struct Row
    {
        const char* source;
        const char* expected; // nullptr: the render fails naming the argument
        const char* argument;
    };
    const Row rows[] = {
        { "{{ f('2', '3', a='1') }}", nullptr, "a" },
        { "{{ f('1', '2', b='x') }}", nullptr, "b" },
        { "{{ v('1', '2', '3', a='x') }}", nullptr, "a" },
        { "{{ f('1', '2', c='3') }}", "123", nullptr },
        { "{{ f('1', b='2', c='3') }}", "123", nullptr },
        { "{{ f(c='3', b='2', a='1') }}", "123", nullptr },
        { "{{ v('1', '2', '3') }}", "12", nullptr },
        // A parameter after *args takes keywords only
        { "{{ k('1', '2', sep='x') }}", "2x", nullptr },
        { "{% macro m(a, b, c) %}{{ a }}{{ b }}{{ c }}{% endmacro %}{{ m('2', '3', a='1') }}", nullptr, nullptr },
    };
    for (const auto& row : rows)
    {
        for (const bool wide : { false, true })
        {
            const std::string source = row.source;
            std::string error;
            std::string output;
            if (wide)
            {
                TemplateW tpl;
                ASSERT_TRUE(tpl.Load(std::wstring(source.begin(), source.end()))) << source;
                auto result = tpl.RenderAsString(params);
                if (result)
                {
                    output = ConvertString<std::string>(result.value());
                }
                else
                {
                    error = ConvertString<std::string>(result.error().ToString());
                }
            }
            else
            {
                Template tpl;
                ASSERT_TRUE(tpl.Load(source)) << source;
                auto result = tpl.RenderAsString(params);
                if (result)
                {
                    output = result.value();
                }
                else
                {
                    error = result.error().ToString();
                }
            }
            if (row.expected)
            {
                EXPECT_EQ(row.expected, output) << source << " " << error;
                continue;
            }
            EXPECT_FALSE(error.empty()) << source << " rendered " << output;
            const std::string message = row.argument ? std::string("got multiple values for argument '") + row.argument + "'" : "takes no keyword argument 'a'";
            EXPECT_NE(std::string::npos, error.find(message)) << source << ": " << error;
        }
    }
}

// f(*args, **kwargs) (docs/tasks/0220): the unpacked items reach a host callable as if written
// out, from template and host values alike, and Python's TypeErrors name what does not fit
TEST(UserCallableTestSingle, UnpackedArguments)
{
    jinja2::ValuesMap params;
    params["f"] = MakeCallable([](const std::string& a, const std::string& b, const std::string& c) { return a + b + c; }, ArgInfo{ "a" }, ArgInfo{ "b" }, ArgInfo{ "c" });
    params["v"] = MakeCallable([](const std::string& a, const ValuesList& rest) { return a + std::to_string(rest.size()); }, ArgInfo{ "a" }, ArgInfo{ "*args" });
    params["hl"] = ValuesList{ "1", "2" };
    params["hm"] = ValuesMap{ { "b", "2" }, { "c", "3" } };
    struct Row
    {
        const char* source;
        const char* expected; // nullptr: the render fails with `message`
        const char* message;
    };
    const Row rows[] = {
        { "{{ f(*['1', '2', '3']) }}", "123", nullptr },
        { "{{ f('1', *['2'], c='3') }}", "123", nullptr },
        { "{{ f(c='3', *hl) }}", "123", nullptr },
        { "{{ f('1', **hm) }}", "123", nullptr },
        { "{{ f(*hl, **{'c': '3'}) }}", "123", nullptr },
        { "{{ v(*'abc') }}", "a2", nullptr },
        { "{{ v(*hl, *[]) }}", nullptr, nullptr },
        { "{{ f(*1) }}", nullptr, "Value after * must be an iterable, not int" },
        { "{{ f(**hl) }}", nullptr, "argument after ** must be a mapping, not list" },
        { "{{ f(b='x', **hm) }}", nullptr, "got multiple values for keyword argument 'b'" },
        { "{{ f(**{1: 2}) }}", nullptr, "keywords must be strings" },
        { "{{ f('1', '2', a='x', *[]) }}", nullptr, "got multiple values for argument 'a'" },
        { "{% macro m() %}{{ caller() }}{% endmacro %}{% call m(**{'caller': 1}) %}x{% endcall %}", nullptr, "got multiple values for keyword argument 'caller'" },
    };
    for (const auto& row : rows)
    {
        for (const bool wide : { false, true })
        {
            const std::string source = row.source;
            std::string loadError;
            std::string error;
            std::string output;
            if (wide)
            {
                TemplateW tpl;
                if (auto loaded = tpl.Load(std::wstring(source.begin(), source.end())); !loaded)
                {
                    loadError = ConvertString<std::string>(loaded.error().ToString());
                }
                else if (auto result = tpl.RenderAsString(params))
                {
                    output = ConvertString<std::string>(result.value());
                }
                else
                {
                    error = ConvertString<std::string>(result.error().ToString());
                }
            }
            else
            {
                Template tpl;
                if (auto loaded = tpl.Load(source); !loaded)
                {
                    loadError = loaded.error().ToString();
                }
                else if (auto result = tpl.RenderAsString(params))
                {
                    output = result.value();
                }
                else
                {
                    error = result.error().ToString();
                }
            }
            if (!row.expected && !row.message)
            {
                // A second *args is a syntax error, as in Jinja2
                EXPECT_FALSE(loadError.empty()) << source;
                continue;
            }
            ASSERT_TRUE(loadError.empty()) << source << ": " << loadError;
            if (row.expected)
            {
                EXPECT_EQ(row.expected, output) << source << " " << error;
                continue;
            }
            EXPECT_NE(std::string::npos, error.find(row.message)) << source << ": " << error;
        }
    }
}

// Spliced items that borrow from a temporary keep it alive: a host callable's reflected struct,
// whose string view reads the struct (docs/tasks/0220)
TEST(UserCallableTestSingle, UnpackedItemsKeepTheirSource)
{
    const std::string text(64, 'x');
    jinja2::ValuesMap params;
    params["make"] = MakeCallable([&text] {
        TestStruct obj;
        obj.strValue = text;
        return jinja2::Reflect(std::move(obj));
    });
    params["makeList"] = MakeCallable([&text] {
        std::vector<TestStruct> items(2);
        items[0].strValue = text;
        items[1].strValue = text;
        return jinja2::Reflect(std::move(items));
    });
    const char* sources[] = {
        "{% macro m() %}{{ kwargs.strViewValue }}{% endmacro %}{{ m(**make()) }}",
        "{% macro m() %}{{ varargs[1].strViewValue }}{% endmacro %}{{ m(*makeList()) }}",
    };
    for (const auto* source : sources)
    {
        Template tpl;
        ASSERT_TRUE(tpl.Load(source).has_value()) << source;
        EXPECT_EQ(text, tpl.RenderAsString(params).value()) << source;
    }
}

// A value a host callable receives stays valid after the call, a list or map that came from the
// environment's globals included: it keeps the globals it was read from when the host replaces
// the global and the next render drops the old snapshot (docs/tasks/0318, 0223 probe 15; ASan)
TEST(UserCallableTestSingle, KeptGlobalsOutliveTheirSnapshot)
{
    const std::string text(40, 'g');
    std::vector<Value> kept;
    TemplateEnv env;
    env.AddGlobal("list", ValuesList{ text, 2, ValuesList{ text + "n" } });
    // One key a map: ValuesMap does not keep the order
    env.AddGlobal("map", ValuesMap{ { "nested", ValuesMap{ { "n", text } } } });
    env.AddGlobal("str", text);
    env.AddGlobal("keep", UserCallable{ [&kept](const UserCallableParams& p) {
                                           kept.push_back(p["v"]);
                                           return Value(std::string());
                                       },
                                        { ArgInfo{ "v" } } });
    Template tpl(&env);
    // The globals themselves, an item of each, and a value the template made that holds them
    ASSERT_TRUE(!!tpl.Load("{{ keep(list) }}{{ keep(map) }}{{ keep(str) }}{{ keep(list[2]) }}{{ keep(map.nested) }}"
                           "{% for l in [list] %}{{ keep(l) }}{% endfor %}{{ keep({'l': list, 'm': map}) }}{{ keep([list[0], map.nested.n]) }}"));
    ASSERT_TRUE(!!tpl.RenderAsString({}));
    ASSERT_EQ(8u, kept.size());

    // Same shape, so that the template renders again
    env.AddGlobal("list", ValuesList{ "x", 3, ValuesList{ "y" } });
    env.AddGlobal("map", ValuesMap{ { "nested", ValuesMap{ { "n", "z" } } } });
    env.AddGlobal("str", std::string("short"));
    ASSERT_TRUE(!!tpl.RenderAsString({}));
    ASSERT_EQ(16u, kept.size());
    kept.resize(8);
    EXPECT_TRUE(ReleaseThreadCaches());

    // Printed by a template of no environment, which reads each one whole
    Template print;
    ASSERT_TRUE(!!print.Load("{{ v }}"));
    auto show = [&print](const Value& v) { return print.RenderAsString({ { "v", v } }).value(); };
    const std::string quoted = "'" + text + "'";
    const std::string nested = "'" + text + "n'";
    const std::string listText = "[" + quoted + ", 2, [" + nested + "]]";
    const std::string mapText = "{'nested': {'n': " + quoted + "}}";
    EXPECT_EQ(listText, show(kept[0]));
    EXPECT_EQ(mapText, show(kept[1]));
    EXPECT_EQ(text, show(kept[2]));
    EXPECT_EQ("[" + nested + "]", show(kept[3]));
    EXPECT_EQ("{'n': " + quoted + "}", show(kept[4]));
    EXPECT_EQ(listText, show(kept[5]));
    const auto made = show(kept[6]);
    EXPECT_EQ("[" + quoted + ", " + quoted + "]", show(kept[7]));
    EXPECT_TRUE(made == "{'l': " + listText + ", 'm': " + mapText + "}" || made == "{'m': " + mapText + ", 'l': " + listText + "}") << made;
}

// Each item kept alone, so that no other kept value holds the globals for it
TEST(UserCallableTestSingle, KeptGlobalItemsOutliveTheirSnapshot)
{
    const std::string text(40, 'g');
    const std::vector<std::pair<std::string, std::string>> rows = {
        { "{{ keep(list[0]) }}", text },
        { "{{ keep([list[0]]) }}", "['" + text + "']" },
        { "{{ keep(list[1:2]) }}", "['" + text + "k']" },
        { "{% for k, v in map|dictsort %}{{ keep([k, v]) }}{% endfor %}", "['" + text + "k', '" + text + "v']" },
        { "{% for b in list|batch(1) %}{% if loop.first %}{{ keep(b) }}{% endif %}{% endfor %}", "['" + text + "']" },
        { "{{ keep(list|select|first) }}", text },
        { "{{ keep(map|list) }}", "['" + text + "k']" },
    };
    for (const auto& [source, expected] : rows)
    {
        SCOPED_TRACE(source);
        Value kept;
        TemplateEnv env;
        env.AddGlobal("list", ValuesList{ text, text + "k" });
        env.AddGlobal("map", ValuesMap{ { text + "k", text + "v" } });
        env.AddGlobal("keep", UserCallable{ [&kept](const UserCallableParams& p) {
                                               // The first render's value only
                                               if (kept.IsNone())
                                               {
                                                   kept = p["v"];
                                               }
                                               return Value(std::string());
                                           },
                                            { ArgInfo{ "v" } } });
        Template tpl(&env);
        ASSERT_TRUE(!!tpl.Load(source));
        ASSERT_TRUE(!!tpl.RenderAsString({}));
        env.AddGlobal("list", ValuesList{ "x", "y" });
        env.AddGlobal("map", ValuesMap{ { "x", "y" } });
        // Renders again to drop the old snapshot
        ASSERT_TRUE(!!tpl.RenderAsString({}));
        EXPECT_TRUE(ReleaseThreadCaches());

        Template print;
        ASSERT_TRUE(!!print.Load("{{ v }}"));
        EXPECT_EQ(expected, print.RenderAsString({ { "v", kept } }).value());
    }
}

namespace
{
std::string Describe(const Value& v)
{
    if (v.IsNone())
    {
        return "none";
    }
    if (v.IsMarkup() || v.IsWMarkup())
    {
        return "markup";
    }
    if (v.IsString() || v.IsWString())
    {
        return "string";
    }
    if (v.IsInt())
    {
        return "int";
    }
    return "other";
}

std::string JoinTwo(const std::string& a, const std::string& b)
{
    return a + b;
}

std::string JoinFour(const std::string& a, int64_t b, double c, const std::wstring& d)
{
    return a + "|" + std::to_string(b) + "|" + std::to_string(static_cast<int>(c * 10)) + "|" + ConvertString<std::string>(d);
}
} // namespace

// Each argument of a callable with readable parameter types is converted on its own (docs/tasks/0334), a Value parameter
// takes the argument as passed (docs/tasks/0319), and a generic callable still matches up to two arguments jointly
TEST(UserCallableTestSingle, PerArgumentDispatch)
{
    jinja2::ValuesMap params;
    params["f4"] = MakeCallable(
        [](const std::string& a, int64_t b, double c, const std::wstring& d) { return JoinFour(a, b, c, d); }, ArgInfo{ "a" }, ArgInfo{ "b" }, ArgInfo{ "c" }, ArgInfo{ "d", false, "dd" });
    params["f5"] = MakeCallable([](std::string a, std::string_view b, std::string&& c, const Markup& d, bool e) {
                                    a += b;
                                    a += std::string(std::move(c));
                                    return a + d.str() + (e ? "T" : "F");
                                },
                                ArgInfo{ "a" },
                                ArgInfo{ "b" },
                                ArgInfo{ "c" },
                                ArgInfo{ "d" },
                                ArgInfo{ "e" });
    params["fptr4"] = MakeCallable(&JoinFour, ArgInfo{ "a" }, ArgInfo{ "b" }, ArgInfo{ "c" }, ArgInfo{ "d", false, "dd" });
    params["func3"] = MakeCallable(std::function<std::string(const std::string&, const std::string&, const std::string&)>(
                                       [](const std::string& a, const std::string& b, const std::string& c) { return a + b + c; }),
                                   ArgInfo{ "a" },
                                   ArgInfo{ "b" },
                                   ArgInfo{ "c" });
    params["nothrow"] = MakeCallable([](int64_t a) noexcept { return a * 2; }, ArgInfo{ "a" });
    params["cref"] = MakeCallable([](const Value& v) { return Describe(v); }, ArgInfo{ "v" });
    params["byValue"] = MakeCallable([](Value v) {
        const Value kept = std::move(v);
        return Describe(kept);
    }, ArgInfo{ "v" });
    // NOLINTNEXTLINE(cppcoreguidelines-rvalue-reference-param-not-moved): the row checks that an rvalue Value binds
    params["rref"] = MakeCallable([](Value&& v) { return Describe(v); }, ArgInfo{ "v" });
    params["echo"] = MakeCallable([](const Value& v) { return v; }, ArgInfo{ "v" });
    params["valueAndText"] = MakeCallable([](const Value& v, const std::string& s) { return Describe(v) + ":" + s; }, ArgInfo{ "v" }, ArgInfo{ "s" });
    // A generic lambda gets the arguments' promoters, which convert to the types it passes them on as
    params["generic"] = MakeCallable([](const auto& a, const auto& b) -> decltype(JoinTwo(a, b)) { return JoinTwo(a, b); }, ArgInfo{ "a" }, ArgInfo{ "b" });
    params["markup"] = Markup("<b>");
    struct Row
    {
        const char* source;
        const char* expected;
    };
    const Row rows[] = {
        { "{{ f4('x', 2, 1.5, 'y') }}", "x|2|15|y" },
        { "{{ f4('x', 2, 1.5) }}", "x|2|15|dd" },
        { "{{ f4('x', 2.9, 3, 'y') }}", "x|2|30|y" },
        // An argument that cannot be converted: the function is not called
        { "{{ f4('x', 'two', 1.5, 'y') is none }}", "True" },
        { "{{ f4([1], 2, 1.5, 'y') is none }}", "True" },
        { "{{ f4('x', 2, none, 'y') }}", "x|2|0|y" },
        { "{{ fptr4('x', 2, 1.5, 'y') }}|{{ fptr4('x', 2, 1.5) }}|{{ fptr4('x', 'two', 1.5) is none }}", "x|2|15|y|x|2|15|dd|True" },
        { "{{ f5('a', 'b', 'c', 'd', true) }}", "abcdT" },
        { "{{ f5('a', 'b', 'c', markup, 0) }}", "abc<b>F" },
        { "{{ f5() }}", "F" },
        { "{{ func3('a', 'b', 'c') }}", "abc" },
        { "{{ nothrow(21) }}", "42" },
        { "{{ cref(1) }} {{ cref('s') }} {{ cref(markup) }} {{ cref(none) }} {{ cref() }} {{ cref([]) }}", "int string markup none none other" },
        { "{{ byValue(1) }} {{ byValue(markup) }} {{ byValue() }}", "int markup none" },
        { "{{ rref(1) }} {{ rref(markup) }} {{ rref() }}", "int markup none" },
        { "{{ echo(markup) is escaped }} {{ echo([1, 'a']) }} {{ echo(none) is none }} {{ echo() is none }}", "True [1, 'a'] True True" },
        { "{{ valueAndText(markup, 'x') }} {{ valueAndText(1, [1]) is none }}", "markup:x True" },
        { "{{ generic('a', 'b') }} {{ generic(markup, 'b') }} {{ generic('a', 1) is none }}", "ab <b>b True" },
    };
    for (const auto& row : rows)
    {
        const std::string source = row.source;
        Template tpl;
        ASSERT_TRUE(tpl.Load(source)) << source;
        auto result = tpl.RenderAsString(params);
        ASSERT_TRUE(result) << source << ": " << result.error().ToString();
        EXPECT_EQ(row.expected, result.value()) << source;

        TemplateW tplW;
        ASSERT_TRUE(tplW.Load(ConvertString<std::wstring>(source))) << source;
        auto resultW = tplW.RenderAsString(params);
        ASSERT_TRUE(resultW) << source << ": " << ConvertString<std::string>(resultW.error().ToString());
        EXPECT_EQ(ConvertString<std::wstring>(std::string(row.expected)), resultW.value()) << source;
    }
}
