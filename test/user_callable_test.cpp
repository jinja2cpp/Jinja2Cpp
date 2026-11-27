#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/markup.h>
#include <jinja2cpp/generic_list_iterator.h> // IWYU pragma: keep (GenericList::iterator is only declared in generic_list.h)
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
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
                            // The tuple the call was handed comes back as itself, as in Python (docs/tasks/0212)
                            InputOutputPair{"VarArgsFn(10.123, (1, 2, 3), arg=1)", "[10.123, (1, 2, 3)]"}
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
                            // ... and so does the range (docs/tasks/0212)
                            InputOutputPair{"GListFn(range(10))", "range(0, 10)"},
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
                            InputOutputPair{"('A', 'B', 'C') | map('surround', before='> ', after=' <') | list | pprint", "['> A <', '> B <', '> C <']"},
                            InputOutputPair{"'str1' if 'str1' is tester('str1') else 'str2'", "str1"},
                            InputOutputPair{"'str1' if 'str3' is tester(pattern='str1') else 'str2'", "str2"},
                            InputOutputPair{"('str1', 'A', 'str1', 'B', 'str1', 'C') | select('tester', 'str1') | map('surround', before='> ', after=' <') | list | pprint", "['> str1 <', '> str1 <', '> str1 <']"}
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

// A filter or test name may be dotted, as Jinja2 reads `x|ns.f` (docs/tasks/0465)
TEST(EnvCallablesTest, DottedNames)
{
    TemplateEnv env;
    env.AddFilter("my.tag", MakeCallable([](const std::string& s, const std::string& end) { return "<" + s + end + ">"; }, ArgInfo{ "s" }, ArgInfo{ "end", false, "" }));
    env.AddTest("my.short", MakeCallable([](const std::string& s) { return s.size() < 3; }, ArgInfo{ "s" }));
    env.AddFilter("tag", MakeCallable([](const std::string& s) { return "[" + s + "]"; }, ArgInfo{ "s" }));

    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{{ 'a'|my.tag }} {{ 'a'|my . tag('!')|upper }} {{ 'ab' is my.short }} {{ 'abc' is not my.short }} {{ 'my.tag' is filter }}").has_value());
    EXPECT_EQ("<a> <A!> True True True", tpl.RenderAsString({}).value());

    // `x|tag.upper` is the filter `tag.upper`, not `(x|tag).upper`; unknown, it fails only when it runs
    Template unknown(&env);
    auto loaded = unknown.Load("{{ 'a'|tag.upper }}");
    ASSERT_FALSE(loaded.has_value());
    EXPECT_EQ(ErrorCode::UnknownFilter, loaded.error().GetCode());
    EXPECT_EQ("noname.j2tpl:1:8: error: No filter named 'tag.upper'.\n{{ 'a'|tag.upper }}\n    ---^-------", loaded.error().ToString());
    Template notRun(&env);
    ASSERT_TRUE(notRun.Load("{% if false %}{{ 'a'|tag.upper() }}{{ 1 is tag.odd }}{% endif %}ok").has_value());
    EXPECT_EQ("ok", notRun.RenderAsString({}).value());

    // A dot ends the name only before a name
    Template broken(&env);
    loaded = broken.Load("{{ 'a'|my.'tag' }}");
    ASSERT_FALSE(loaded.has_value());
    EXPECT_EQ(ErrorCode::ExpectedIdentifier, loaded.error().GetCode());

    TemplateW wide(&env);
    ASSERT_TRUE(wide.Load(L"{{ 'ab' is my.short }}").has_value());
    EXPECT_EQ(L"True", wide.RenderAsString({}).value());
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
                                ArgInfo{ "a" }, ArgInfo{ "b" }, ArgInfo{ "c" }, ArgInfo{ "d" }, ArgInfo{ "e" });
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
    },
                                     ArgInfo{ "v" });
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

// A user callable is a dict key by the jinja2::UserCallable it was converted from, however it was
// read: each read from a host container converts it anew (docs/tasks/0212)
TEST(UserCallableTestSingle, UserCallableAsDictKey)
{
    auto f = MakeCallable([](const Value& x) { return x; }, ArgInfo{ "x" });
    auto g = MakeCallable([](const Value& x) { return x; }, ArgInfo{ "x" });
    ValuesMap params{ { "f", f }, { "fs", ValuesList{ f, g } }, { "fm", ValuesMap{ { "f", f } } } };
    struct Row
    {
        const char* source;
        const char* expected;
    };
    const Row rows[] = {
        { "{{ {f: 1}[f] }}|{{ f in {f: 1} }}|{{ {f: 1, f: 2}|length }}", "1|True|1" },
        { "{{ {fs[0]: 1}[fs[0]] }}|{{ {fm.f: 1}[fm.f] }}|{{ {f: 1}[fm.f] }}|{{ {fs[0]: 1}[fs|first] }}", "1|1|1|1" },
        { "{{ {fs[0]: 1, fs[0]: 2}|length }}|{{ {fs[0]: 1, fs[1]: 2}|length }}|{{ fs[1] in {f: 1} }}", "1|2|False" },
    };
    for (const auto& row : rows)
    {
        Template tpl;
        ASSERT_TRUE(tpl.Load(row.source)) << row.source;
        auto result = tpl.RenderAsString(params);
        ASSERT_TRUE(result) << row.source << ": " << result.error().ToString();
        EXPECT_EQ(row.expected, result.value()) << row.source;
    }
}

// What a host's call is handed comes back as the template's own object while the call runs
// (docs/tasks/0212 P2): the expected outputs are Python Jinja2's, with `ident` returning its
// argument, `wrap` returning it in a list and `callWith` calling its first argument with its second
TEST(UserCallableTestSingle, HandedObjectsComeBackAsThemselves)
{
    ValuesMap params;
    params["ident"] = UserCallable([](const UserCallableParams& p) { return p["v"]; }, { ArgInfo{ "v" } });
    params["wrap"] = UserCallable([](const UserCallableParams& p) { return Value(ValuesList{ p["v"] }); }, { ArgInfo{ "v" } });
    params["call0"] = UserCallable([](const UserCallableParams& p) { return p["f"].GetIf<UserCallable>()->callable(UserCallableParams{}); }, { ArgInfo{ "f" } });
    params["callWith"] = UserCallable(
        [](const UserCallableParams& p) {
            UserCallableParams args;
            args.extraPosArgs = Value(ValuesList{ p["v"] });
            args.extraKwArgs = Value(ValuesMap{});
            return p["f"].GetIf<UserCallable>()->callable(args);
        },
        { ArgInfo{ "f" }, ArgInfo{ "v" } });
    params["ctxl"] = ValuesList{ 1 };
    struct Row
    {
        const char* source;
        const char* expected; // nullptr: the render fails
    };
    const Row rows[] = {
        { "{% set n = namespace(a=1) %}{{ ident(n) is sameas n }}|{{ ident(n) == n }}|{{ {ident(n): 1}[n] }}", "True|True|1" },
        { "{% set c = cycler(1, 2) %}{{ ident(c) == c }}|{{ ident(c) is sameas c }}|{{ ident(c).next() }}{{ c.next() }}", "True|True|12" },
        { "{% set n = namespace(a=1) %}{% set m = ident(n) %}{% set m.a = 2 %}{{ n.a }}|{{ ident(n) }}", "2|<Namespace {'a': 2}>" },
        { "{% set l = [1] %}{% do ident(l).append(2) %}{{ l }}|{{ ident(l) is sameas l }}", "[1, 2]|True" },
        { "{% set d = {'a': 1} %}{% do ident(d).update(b=2) %}{{ d }}|{{ ident(d) is sameas d }}", "{'a': 1, 'b': 2}|True" },
        { "{% macro m() %}hi{% endmacro %}{{ ident(m)() }}|{{ ident(m) is sameas m }}", "hi|True" },
        { "{{ ident(range)(2)|list }}", "[0, 1]" },
        { "{% set j = joiner('/') %}{{ ident(j)() }}{{ j() }}", "/" },
        { "{% for i in [1] %}{{ ident(loop).index }}{% endfor %}", "1" },
        { "{% set n = namespace() %}{{ wrap(n)[0] is sameas n }}|{{ ident([n])[0] is sameas n }}", "True|True" },
        { "{{ ident(ctxl) is sameas ctxl }}|{% do ident(ctxl).append(7) %}{{ ctxl }}", "True|[1, 7]" },
        // Python: TypeError, Object of type Namespace is not JSON serializable
        { "{% set n = namespace(a=1) %}{{ ident(n)|tojson }}", nullptr },
        // A call back gets what the call was handed as itself
        { "{% set n = namespace() %}{% macro m(x) %}{{ x is sameas n }}{% endmacro %}{{ callWith(m, n) }}", "True" },
        { "{% macro m() %}hi{% endmacro %}{{ call0(ident(m)) }}", "hi" },
    };
    for (const auto& row : rows)
    {
        const std::string source = row.source;
        TemplateEnv env;
        env.GetSettings().extensions.doStatement = true;
        Template tpl(&env);
        ASSERT_TRUE(tpl.Load(source)) << source;
        auto result = tpl.RenderAsString(params);
        TemplateW tplW(&env);
        ASSERT_TRUE(tplW.Load(ConvertString<std::wstring>(source))) << source;
        auto resultW = tplW.RenderAsString(params);
        if (!row.expected)
        {
            EXPECT_FALSE(result) << source;
            EXPECT_FALSE(resultW) << source;
            continue;
        }
        ASSERT_TRUE(result) << source << ": " << result.error().ToString();
        EXPECT_EQ(row.expected, result.value()) << source;
        ASSERT_TRUE(resultW) << source << ": " << ConvertString<std::string>(resultW.error().ToString());
        EXPECT_EQ(ConvertString<std::wstring>(std::string(row.expected)), resultW.value()) << source;
    }
}

// An object the host keeps past the call comes back in a later call as a view of what it was
// exported as, never as the template's object (docs/tasks/0212 P2): a later render that writes to
// it writes to its own copy, so renders never share a mutable object
TEST(UserCallableTestSingle, KeptObjectsComeBackAsViews)
{
    Value kept;
    ValuesMap params;
    params["keep"] = UserCallable(
        [&kept](const UserCallableParams& p) {
            kept = p["v"];
            return Value();
        },
        { ArgInfo{ "v" } });
    params["give"] = UserCallable([&kept](const UserCallableParams&) { return kept; }, {});
    TemplateEnv env;
    env.GetSettings().extensions.doStatement = true;
    const auto render = [&](const std::string& source) {
        Template tpl(&env);
        EXPECT_TRUE(tpl.Load(source)) << source;
        auto result = tpl.RenderAsString(params);
        EXPECT_TRUE(result) << source << ": " << result.error().ToString();
        return result ? result.value() : std::string();
    };

    // Python gives True for `give() is sameas d`: the call that returns the dict is not the one it was handed to
    EXPECT_EQ("1|True|False", render("{% set d = {'a': 1} %}{% do keep(d) %}{{ give().a }}|{{ give() is sameas give() }}|{{ give() is sameas d }}"));
    // The next render's write to the kept dict reaches its own copy only
    EXPECT_EQ("{'a': 1, 'b': 2}|{'a': 1, 'b': 2}", render("{% set x = give() %}{% do x.update(b=2) %}{{ x }}|{{ x }}"));
    const auto* map = kept.GetIf<GenericMap>();
    ASSERT_NE(nullptr, map);
    EXPECT_TRUE(map->GetValueByName("b").IsNone());
    EXPECT_EQ(1, map->GetValueByName("a").As<int64_t>());
    EXPECT_EQ("{'a': 1}", render("{{ give() }}"));
}

// Two renders on two threads swap their dicts through a host callable: each writes only to its own
// copy of the other's dict, never to the dict itself (docs/tasks/0212 P2; the TSan job checks it)
TEST(UserCallableTestSingle, SwappedObjectsStayInTheirRender)
{
    std::mutex mutex;
    Value kept;
    ValuesMap params;
    params["swap"] = UserCallable(
        [&](const UserCallableParams& p) {
            const std::scoped_lock lock(mutex);
            Value previous = std::move(kept);
            kept = p["v"];
            return previous;
        },
        { ArgInfo{ "v" } });
    TemplateEnv env;
    env.GetSettings().extensions.doStatement = true;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set d = {'n': 0} %}{% set x = swap(d) %}{% if x %}{% do x.update(n=1) %}{{ x.n }}{% endif %}|{{ d.n }}"));
    const auto run = [&] {
        for (int i = 0; i < 200; ++i)
        {
            auto result = tpl.RenderAsString(params);
            ASSERT_TRUE(result) << result.error().ToString();
            ASSERT_TRUE(result.value() == "1|0" || result.value() == "|0") << result.value();
        }
    };
    std::thread first(run);
    std::thread second(run);
    first.join();
    second.join();
    const std::scoped_lock lock(mutex);
    kept = Value();
}

// A call handed many containers finds each one in log time, not by a scan per item (docs/tasks/0212)
TEST(UserCallableTestSingle, ManyHandedContainersComeBack)
{
    ValuesMap params;
    params["va"] = UserCallable([](const UserCallableParams& p) { return p.extraPosArgs; }, { ArgInfo{ "*args" } });
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% set big = range(20000)|batch(1)|list %}{% set r = va(*big) %}{{ r|length }}|{{ r[0] is sameas big[0] }}|{{ r[19999] is sameas big[19999] }}"));
    auto result = tpl.RenderAsString(params);
    ASSERT_TRUE(result) << result.error().ToString();
    EXPECT_EQ("20000|True|True", result.value());
}

// A list of handed containers the host returns counts against maxSequenceSize when it becomes a
// template list (docs/tasks/0212)
TEST(UserCallableTestSingle, RestoredListObeysTheSequenceLimit)
{
    ValuesMap params;
    params["many"] = UserCallable(
        [](const UserCallableParams& p) {
            const auto count = p["n"].As<int64_t>();
            return Value(ValuesList(static_cast<size_t>(count), p["v"]));
        },
        { ArgInfo{ "v" }, ArgInfo{ "n" } });
    Settings settings;
    settings.limits.maxSequenceSize = 16;
    TemplateEnv env;
    env.SetSettings(settings);
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set n = namespace() %}{{ many(n, 16)|length }}|{{ many(n, 16)[15] is sameas n }}"));
    auto result = tpl.RenderAsString(params);
    ASSERT_TRUE(result) << result.error().ToString();
    EXPECT_EQ("16|True", result.value());
    ASSERT_TRUE(tpl.Load("{% set n = namespace() %}{{ many(n, 17)|length }}"));
    result = tpl.RenderAsString(params);
    ASSERT_FALSE(result);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode());
}

// Call-site information for a callable that asks for the context (task 0357 B4)

namespace
{
struct SiteProbe
{
    std::vector<std::string> keys;
    size_t size = 0;
    std::string templateName = "unset";
    bool autoescape = false;
    bool called = false;
};

UserCallable MakeSiteProbe(const std::shared_ptr<SiteProbe>& probe, bool askForContext)
{
    std::vector<ArgInfo> args;
    if (askForContext)
    {
        args.emplace_back("*context");
    }
    return UserCallable(
        [probe](const UserCallableParams& params) {
            probe->called = true;
            probe->templateName = std::string(params.site.templateName);
            probe->autoescape = params.site.autoescape;
            if (const auto* map = params.context.GetIf<GenericMap>())
            {
                probe->keys = map->GetKeys();
                probe->size = map->GetSize();
            }
            return Value(std::string());
        },
        args);
}
} // namespace

// The context map of an ended call is used again by the next call on the thread only when the host
// kept no copy of it (docs/tasks/0519): a copy the host kept stays expired, and each call reads its own
TEST(CallSiteTest, KeptContextStaysExpiredWhenTheNextCallRuns)
{
    std::vector<Value> kept;
    std::string expiredReads;
    TemplateEnv env;
    env.AddGlobal("probe", UserCallable(
                               [&kept, &expiredReads](const UserCallableParams& params) {
                                   for (const auto& old : kept)
                                   {
                                       try
                                       {
                                           (void)old.GetIf<GenericMap>()->GetAccessor()->GetValueByName("x");
                                           expiredReads += "live,";
                                       }
                                       catch (const ErrorInfo& error)
                                       {
                                           expiredReads += error.GetCode() == ErrorCode::TemplateExpired ? "expired," : "other,";
                                       }
                                   }
                                   const Value keepArg = params["keep"];
                                   if (const auto* keep = keepArg.GetIf<bool>(); keep && *keep)
                                   {
                                       kept.push_back(params.context);
                                   }
                                   return params.context.GetIf<GenericMap>()->GetAccessor()->GetValueByName("x");
                               },
                               { ArgInfo{ "keep" } }));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% for k in [false, true, false, true, false] %}{% set x = loop.index %}{{ probe(k) }}{% endfor %}").has_value());
    const auto result = tpl.RenderAsString({});
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    EXPECT_EQ("12345", result.value());
    EXPECT_EQ("expired,expired,expired,expired,", expiredReads);
    ASSERT_EQ(2u, kept.size());
    EXPECT_NE(kept[0].GetIf<GenericMap>()->GetAccessor(), kept[1].GetIf<GenericMap>()->GetAccessor());
}

TEST(CallSiteTest, ContextKeysListVisibleNames)
{
    auto probe = std::make_shared<SiteProbe>();
    TemplateEnv env;
    env.AddGlobal("probe", MakeSiteProbe(probe, true));
    env.AddGlobal("site_global", Value(1));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set top = 1 %}{% macro m(arg) %}{% set inner = 2 %}{{ probe() }}{% endmacro %}{{ m(3) }}").has_value());
    ASSERT_TRUE(tpl.RenderAsString({ { "param", 5 } }).has_value());
    ASSERT_TRUE(probe->called);
    const auto& keys = probe->keys;
    for (const char* name : { "top", "arg", "inner", "param", "site_global", "probe", "m" })
    {
        EXPECT_EQ(1, std::count(keys.begin(), keys.end(), name)) << name;
    }
    EXPECT_TRUE(std::is_sorted(keys.begin(), keys.end()));
    EXPECT_TRUE(std::adjacent_find(keys.begin(), keys.end()) == keys.end());
    EXPECT_EQ(keys.size(), probe->size);
    EXPECT_EQ(0, std::count(keys.begin(), keys.end(), "nothing_here"));
}

// ContextView: Python Jinja2's Context.vars and Context.parent of a call (docs/tasks/0416)
TEST(CallSiteTest, ContextViewsSplitTopLevelNamesFromParams)
{
    std::vector<std::string> seen;
    TemplateEnv env;
    env.AddGlobal("site_global", Value(1));
    env.AddGlobal("probe", UserCallable(
                               [&seen](const UserCallableParams& params) {
                                   const auto vars = ContextView(params.context, ContextNames::TopLevel);
                                   const auto parent = ContextView(params.context, ContextNames::Parent);
                                   std::string line;
                                   for (const auto& name : vars.GetIf<GenericMap>()->GetKeys())
                                   {
                                       line += name + ",";
                                   }
                                   line += "|";
                                   for (const char* name : { "top", "i", "w", "arg", "inner", "param", "site_global", "range", "loop" })
                                   {
                                       const bool inVars = vars.GetIf<GenericMap>()->Find(name).has_value();
                                       const bool inParent = parent.GetIf<GenericMap>()->Find(name).has_value();
                                       line += std::string(name) + (inVars ? "V" : "") + (inParent ? "P" : "") + ",";
                                   }
                                   const auto parentKeys = parent.GetIf<GenericMap>()->GetKeys();
                                   line += std::count(parentKeys.begin(), parentKeys.end(), "param") == 1 ? "listed" : "unlisted";
                                   seen.push_back(std::move(line));
                                   return Value();
                               },
                               { ArgInfo(ArgInfo::Context) }));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set top = 1 %}{{ probe() }}{% for i in [1] %}{{ probe() }}{% endfor %}{% with w = 2 %}{{ probe() }}{% endwith %}"
                         "{% macro m(arg) %}{% set inner = 2 %}{{ probe() }}{% endmacro %}{{ m(3) }}")
                    .has_value());
    const auto result = tpl.RenderAsString({ { "param", 5 } });
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    ASSERT_EQ(4u, seen.size());
    // `i`, `w`, `arg` and `inner` are in neither view where the template binds them; elsewhere a lookup finds nothing
    EXPECT_EQ("top,|topV,i,w,arg,inner,paramP,site_globalP,rangeP,loop,listed", seen[0]);
    EXPECT_EQ(seen[0], seen[1]);
    EXPECT_EQ(seen[0], seen[2]);
    EXPECT_EQ("m,top,|topV,i,w,arg,inner,paramP,site_globalP,rangeP,loop,listed", seen[3]);

    // Not a call's context: no view
    EXPECT_TRUE(ContextView(Value(ValuesMap{}), ContextNames::TopLevel).IsNone());
    EXPECT_TRUE(ContextView(Value(1), ContextNames::Parent).IsNone());
}

TEST(CallSiteTest, ContextViewOfAWideTemplate)
{
    std::string seen;
    TemplateEnv env;
    env.AddGlobal("probe", UserCallable(
                               [&seen](const UserCallableParams& params) {
                                   for (const auto& name : ContextView(params.context, ContextNames::TopLevel).GetIf<GenericMap>()->GetKeys())
                                   {
                                       seen += name + ",";
                                   }
                                   seen += "|";
                                   for (const auto& name : ContextView(params.context, ContextNames::BodyVars).GetIf<GenericMap>()->GetKeys())
                                   {
                                       seen += name + ",";
                                   }
                                   return Value();
                               },
                               { ArgInfo(ArgInfo::Context) }));
    TemplateW tpl(&env);
    ASSERT_TRUE(tpl.Load(L"{% set t = 1 %}{% for i in [1] %}{% set u = 2 %}{{ probe() }}{% endfor %}").has_value());
    const auto result = tpl.RenderAsString({});
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    EXPECT_EQ("t,|u,", seen);
}

TEST(CallSiteTest, ContextViewExpiresWithTheCall)
{
    Value kept;
    Value keptBody;
    TemplateEnv env;
    env.AddGlobal("probe", UserCallable(
                               [&kept, &keptBody](const UserCallableParams& params) {
                                   kept = ContextView(params.context, ContextNames::TopLevel);
                                   keptBody = ContextView(params.context, ContextNames::BodyVars);
                                   return Value();
                               },
                               { ArgInfo(ArgInfo::Context) }));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% set top = 1 %}{% for i in [1] %}{% set b = 2 %}{{ probe() }}{{ probe() }}{% endfor %}").has_value());
    ASSERT_TRUE(tpl.RenderAsString({}).has_value());
    ASSERT_TRUE(kept.GetIf<GenericMap>());
    ASSERT_TRUE(keptBody.GetIf<GenericMap>());
    const std::pair<Value*, std::string> expiredViews[] = { { &kept, "top" }, { &keptBody, "b" } };
    for (const auto& [view, name] : expiredViews)
    {
        try
        {
            (void)view->GetIf<GenericMap>()->Find(name);
            FAIL() << "the view outlived its call";
        }
        catch (const ErrorInfo& error)
        {
            EXPECT_EQ(ErrorCode::TemplateExpired, error.GetCode());
        }
    }
}

// The call's context as Python Jinja2 hands a `pass_context` callable (docs/tasks/0558): `vars` and `parent` of
// `Context.call`, which derives the context when the call has BodyVars. Compared with Jinja2 3.1.6 for every row
struct ContextBodyRow
{
    const char* name;
    const char* source;
    std::vector<std::pair<std::string, std::string>> files;
    // `b` is a render parameter
    bool hasB;
    // `vars|body|parent` of each call, restricted to the names the templates use
    const char* expected;
};

void PrintTo(const ContextBodyRow& row, std::ostream* os)
{
    *os << row.name;
}

class CallSiteContextBodyVarsTest : public ::testing::TestWithParam<ContextBodyRow>
{
};

namespace
{
std::string ShownValue(const Value& value)
{
    if (const auto* number = value.GetIf<int64_t>())
    {
        return std::to_string(*number);
    }
    if (value.IsString())
    {
        return value.AsString();
    }
    if (const auto* wide = value.GetIf<std::wstring>())
    {
        return ConvertString<std::string>(*wide);
    }
    return "?";
}

std::string JoinNames(const std::vector<std::string>& names)
{
    std::string joined;
    for (const auto& name : names)
    {
        joined += (joined.empty() ? "" : ",") + name;
    }
    return joined;
}

// What Context.vars, Context.parent and the derive Python makes of them show for a call
std::string DescribeContextOfCall(const Value& context)
{
    const auto keys = [&](ContextNames part) { return ContextView(context, part).GetIf<GenericMap>()->GetKeys(); };
    auto vars = keys(ContextNames::TopLevel);
    auto parent = keys(ContextNames::Parent);
    const auto bodyView = ContextView(context, ContextNames::BodyVars);
    const auto bodyNames = bodyView.GetIf<GenericMap>()->GetKeys();
    std::string shownBody;
    for (const auto& name : bodyNames)
    {
        shownBody += (shownBody.empty() ? "" : ",") + name + "=" + ShownValue(bodyView.GetIf<GenericMap>()->GetValueByName(name));
    }
    if (!shownBody.empty())
    {
        // Context.call: the callable gets context.derived(body), with `vars` empty and everything else in `parent`
        parent.insert(parent.end(), vars.begin(), vars.end());
        parent.insert(parent.end(), bodyNames.begin(), bodyNames.end());
        vars.clear();
    }
    std::vector<std::string> shownParent;
    for (const char* name : { "_b", "a", "b", "c", "i", "j", "loop", "m", "ns", "x" })
    {
        if (std::find(parent.begin(), parent.end(), name) != parent.end())
        {
            shownParent.emplace_back(name);
        }
    }
    return "V:" + JoinNames(vars) + "|B:" + shownBody + "|P:" + JoinNames(shownParent);
}
} // namespace

TEST_P(CallSiteContextBodyVarsTest, MatchesPythonContext)
{
    const auto& row = GetParam();
    std::vector<std::string> seen;
    TemplateEnv env;
    env.GetSettings().userCallablesAsFilters = true;
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("main", row.source);
    for (const auto& [name, text] : row.files)
    {
        fs->AddFile(name, text);
    }
    env.AddFilesystemHandler(std::string(), fs);
    const auto probe = UserCallable(
        [&seen](const UserCallableParams& params) {
            seen.push_back(DescribeContextOfCall(params.context));
            return Value(std::string());
        },
        { ArgInfo(ArgInfo::Context), ArgInfo("*args") });
    env.AddGlobal("p", probe);
    env.AddGlobal("pf", probe);

    ValuesMap params;
    if (row.hasB)
    {
        params["b"] = 9;
    }
    const auto tpl = env.LoadTemplate("main");
    ASSERT_TRUE(tpl.has_value()) << tpl.error().ToString();
    const auto rendered = tpl->RenderAsString(params);
    ASSERT_TRUE(rendered.has_value()) << rendered.error().ToString();
    std::string joined;
    for (const auto& line : seen)
    {
        joined += (joined.empty() ? "" : "; ") + line;
    }
    EXPECT_EQ(row.expected, joined) << row.source;

    // Wide templates take the same path
    if (row.files.empty())
    {
        seen.clear();
        TemplateW wide(&env);
        ASSERT_TRUE(wide.Load(ConvertString<std::wstring>(std::string(row.source))).has_value());
        ASSERT_TRUE(wide.RenderAsString(params).has_value());
        joined.clear();
        for (const auto& line : seen)
        {
            joined += (joined.empty() ? "" : "; ") + line;
        }
        EXPECT_EQ(row.expected, joined) << row.source;
    }
}

// clang-format off
INSTANTIATE_TEST_SUITE_P(CallSiteTest, CallSiteContextBodyVarsTest, ::testing::Values(
    ContextBodyRow{ "loop_set_twice", R"({% for i in [1,2] %}{{ p() }}{% set b = i %}{{ p() }}{% endfor %})", {}, true,
      "V:|B:|P:b; V:|B:b=1|P:b; V:|B:|P:b; V:|B:b=2|P:b" },
    ContextBodyRow{ "set_in_if", R"({% for i in [1] %}{% if true %}{% set b = i %}{% endif %}{{ p() }}{% endfor %})", {}, false,
      "V:|B:b=1|P:b" },
    ContextBodyRow{ "call_in_if", R"({% for i in [1] %}{% set b = i %}{% if true %}{{ p() }}{% endif %}{{ 0 if false else p() }}{{ p() if true else 0 }}{% endfor %})", {}, false,
      "V:|B:b=1|P:b; V:|B:b=1|P:b; V:|B:b=1|P:b" },
    ContextBodyRow{ "call_in_with", R"({% for i in [1] %}{% set b = i %}{% with x = 1 %}{{ p() }}{% endwith %}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "call_in_filter", R"({% for i in [1] %}{% set b = i %}{% filter upper %}{{ p() }}{% endfilter %}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "call_in_autoescape", R"({% for i in [1] %}{% set b = i %}{% autoescape false %}{{ p() }}{% endautoescape %}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "call_in_loop_else", R"({% for i in [1] %}{% set b = i %}{% for j in [] %}{% else %}{{ p() }}{% endfor %}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "call_in_set_block", R"({% for i in [1] %}{% set b = i %}{% set x %}{{ p() }}{% endset %}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "set_the_loop_target", R"({% for i in [1] %}{% set i = 5 %}{{ p() }}{% endfor %})", {}, false,
      "V:|B:i=5|P:i" },
    ContextBodyRow{ "set_underscore", R"({% for i in [1] %}{% set _b = 2 %}{{ p() }}{% endfor %})", {}, false,
      "V:|B:_b=2|P:_b" },
    ContextBodyRow{ "set_tuple", R"({% for i in [1] %}{% set b, c = 2, 3 %}{{ p() }}{% endfor %})", {}, false,
      "V:|B:b=2,c=3|P:b,c" },
    ContextBodyRow{ "set_block", R"({% for i in [1] %}{% set b %}x{% endset %}{{ p() }}{% endfor %})", {}, false,
      "V:|B:b=x|P:b" },
    ContextBodyRow{ "set_namespace_attr", R"({% set ns = namespace() %}{% for i in [1] %}{% set ns.b = 2 %}{{ p() }}{% endfor %})", {}, false,
      "V:ns|B:|P:" },
    ContextBodyRow{ "macro_in_body", R"({% for i in [1] %}{% macro b() %}{% endmacro %}{{ p() }}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "import_in_body", R"({% for i in [1] %}{% import 'lib' as b %}{{ p() }}{% endfor %})", { { "lib", R"({% macro f() %}{% endmacro %})" } }, false,
      "V:|B:|P:" },
    ContextBodyRow{ "block_set", R"({% block x %}{% set b = 1 %}{{ p() }}{% endblock %})", {}, false,
      "V:|B:b=1|P:b" },
    ContextBodyRow{ "loop_in_block", R"({% block x %}{% set b = 1 %}{% for i in [1] %}{{ p() }}{% endfor %}{% endblock %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "loop_in_macro", R"({% macro m() %}{% for i in [1] %}{% set b = i %}{{ p() }}{% endfor %}{% endmacro %}{{ m() }})", {}, false,
      "V:|B:b=1|P:b,m" },
    ContextBodyRow{ "macro_body_set", R"({% macro m() %}{% set b = 1 %}{{ p() }}{% endmacro %}{{ m() }})", {}, false,
      "V:m|B:|P:" },
    ContextBodyRow{ "nested_loops", R"({% for i in [1] %}{% set b = 1 %}{{ p() }}{% for j in [2] %}{% set c = 2 %}{{ p() }}{% endfor %}{% endfor %})", {}, false,
      "V:|B:b=1|P:b; V:|B:c=2|P:c" },
    ContextBodyRow{ "recursive_loop", R"({% for i in [1, 2] recursive %}{{ p() }}{% set b = i %}{{ p() }}{% if i == 1 %}{{ loop([2]) }}{% endif %}{% endfor %})", {}, true,
      "V:|B:|P:b; V:|B:b=1|P:b; V:|B:|P:b; V:|B:b=2|P:b; V:|B:|P:b; V:|B:b=2|P:b" },
    ContextBodyRow{ "inner_iterable", R"({% for i in [1] %}{% set b = i %}{% for j in [p()] %}{% endfor %}{% endfor %})", {}, false,
      "V:|B:b=1|P:b" },
    ContextBodyRow{ "with_value", R"({% for i in [1] %}{% set b = i %}{% with x = p() %}{% endwith %}{% endfor %})", {}, false,
      "V:|B:b=1|P:b" },
    ContextBodyRow{ "loop_filter", R"({% for i in [1] if p() %}{% set b = i %}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "filter_args", R"({% for i in [1] %}{% set b = i %}{% filter replace('x', p()) %}x{% endfilter %}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "macro_default", R"({% for i in [1] %}{% set b = i %}{% macro mm(z=p()) %}{% endmacro %}{{ mm() }}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "filter_with_context", R"({% for i in [1] %}{% set b = i %}{{ 1|pf }}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "unscoped_block_in_loop", R"({% for i in [1] %}{% set b = 2 %}{% block x %}{{ p() }}{% endblock %}{% endfor %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "call_block", R"({% macro m(x=0) %}{{ caller() }}{% endmacro %}{% for i in [1] %}{% set b = i %}{% call m() %}{{ p() }}{% endcall %}{% endfor %})", {}, false,
      "V:m|B:|P:" },
    ContextBodyRow{ "call_block_head", R"({% macro m(x=0) %}{{ caller() }}{% endmacro %}{% for i in [1] %}{% set b = i %}{% call m(p()) %}x{% endcall %}{% endfor %})", {}, false,
      "V:|B:b=1|P:b,m" },
    ContextBodyRow{ "filter_arg_expr", R"({% for i in [1] %}{% set b = i %}{{ 1|replace('1', p()) }}{% endfor %})", {}, false,
      "V:|B:b=1|P:b" },
    ContextBodyRow{ "p_as_arg_of_p", R"({% for i in [1] %}{% set b = i %}{{ p(p()) }}{% endfor %})", {}, false,
      "V:|B:b=1|P:b; V:|B:b=1|P:b" },
    ContextBodyRow{ "many_sets", R"({% for i in [1] %}{% set n00 = 0 %}{% set n01 = 1 %}{% set n02 = 2 %}{% set n03 = 3 %}{% set n04 = 4 %}{% set n05 = 5 %}{% set n06 = 6 %}{% set n07 = 7 %}{% set n08 = 8 %}{% set n09 = 9 %}{% set n10 = 10 %}{% set n11 = 11 %}{% set n12 = 12 %}{% set n13 = 13 %}{% set n14 = 14 %}{% set n15 = 15 %}{% set n16 = 16 %}{% set n17 = 17 %}{% set n18 = 18 %}{% set n19 = 19 %}{% set n03 = 30 %}{{ p() }}{% endfor %})", {}, false,
      "V:|B:n00=0,n01=1,n02=2,n03=30,n04=4,n05=5,n06=6,n07=7,n08=8,n09=9,n10=10,n11=11,n12=12,n13=13,n14=14,n15=15,n16=16,n17=17,n18=18,n19=19|P:" },
    ContextBodyRow{ "top_set", R"({% set a = 1 %}{{ p() }})", {}, false,
      "V:a|B:|P:" },
    ContextBodyRow{ "scoped_top", R"({% set a = 1 %}{% block x scoped %}{{ p() }}{% endblock %})", {}, false,
      "V:|B:|P:a" },
    ContextBodyRow{ "scoped_in_loop", R"({% for i in [1] %}{% block x scoped %}{{ p() }}{% endblock %}{% endfor %})", {}, false,
      "V:|B:|P:i,loop" },
    ContextBodyRow{ "scoped_after_set", R"({% for i in [1] %}{% set b = i %}{% block x scoped %}{{ p() }}{% endblock %}{% endfor %})", {}, false,
      "V:|B:|P:b,i,loop" },
    ContextBodyRow{ "scoped_body_set", R"({% block x scoped %}{% set b = 1 %}{{ p() }}{% endblock %})", {}, false,
      "V:|B:b=1|P:b" },
    ContextBodyRow{ "scoped_nested", R"({% for i in [1] %}{% block x scoped %}{% block y scoped %}{{ p() }}{% endblock %}{% endblock %}{% endfor %})", {}, false,
      "V:|B:|P:i,loop" },
    ContextBodyRow{ "scoped_with", R"({% block x scoped %}{% with b = 4 %}{{ p() }}{% endwith %}{% endblock %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "scoped_macro_defined_in", R"({% block x scoped %}{% macro m() %}{{ p() }}{% endmacro %}{{ m() }}{% endblock %})", {}, false,
      "V:|B:|P:" },
    ContextBodyRow{ "scoped_macro_from_top", R"({% macro m() %}{{ p() }}{% endmacro %}{% block x scoped %}{{ m() }}{% endblock %})", {}, false,
      "V:m|B:|P:" },
    ContextBodyRow{ "scoped_include", R"({% set a = 1 %}{% block x scoped %}{% include 'inc' %}{% endblock %})", { { "inc", R"({{ p() }})" } }, false,
      "V:|B:|P:a" },
    ContextBodyRow{ "scoped_override", R"({% extends 'base' %}{% block x %}{{ p() }}{% endblock %})", { { "base", R"({% for i in [1] %}{% block x scoped %}{% endblock %}{% endfor %})" } }, false,
      "V:|B:|P:i,loop" },
    ContextBodyRow{ "set_loop_top", R"({% set loop = 6 %}{{ p() }})", {}, false,
      "V:loop|B:|P:" },
    ContextBodyRow{ "set_loop_top_in_for", R"({% set loop = 6 %}{% for i in [1] %}{{ p() }}{% endfor %})", {}, false,
      "V:loop|B:|P:" }
), [](const ::testing::TestParamInfo<ContextBodyRow>& param) { return std::string(param.param.name); });
// clang-format on

// DescribeLentCallable: how Python Jinja2 prints a template callable lent to a host's call (docs/tasks/0439)
TEST(CallSiteTest, LentCallablesDescribeThemselvesAsPython)
{
    std::string seen;
    Value kept;
    TemplateEnv env;
    env.AddGlobal("describe", UserCallable(
                                  [&seen, &kept](const UserCallableParams& params) {
                                      const Value fn = params["fn"];
                                      const auto* callable = fn.GetIf<UserCallable>();
                                      const auto info = callable ? DescribeLentCallable(*callable) : std::nullopt;
                                      seen += info ? info->repr + " " + info->typeName + ";" : std::string("none;");
                                      kept = fn;
                                      return Value();
                                  },
                                  { ArgInfo{ "fn" } }));
    env.AddGlobal("host", UserCallable([](const UserCallableParams&) { return Value(); }, {}));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% macro m() %}{% endmacro %}{{ describe(range) }}{{ describe(dict) }}{{ describe(namespace) }}{{ describe(m) }}"
                         "{{ describe(caller) }}{% for x in [1] %}{{ describe(loop.cycle) }}{% endfor %}{{ describe(host) }}{{ describe(1) }}")
                    .has_value());
    const auto result = tpl.RenderAsString({});
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    EXPECT_EQ("<class 'range'> type;<class 'dict'> type;<class 'jinja2.utils.Namespace'> type;<Macro 'm'> Macro;none;"
              "<bound method LoopContext.cycle of <LoopContext 1/1>> method;none;none;",
              seen);
    // Once the call returned, the callable it was lent is not described
    seen.clear();
    ASSERT_TRUE(tpl.Load("{{ describe(range) }}").has_value());
    ASSERT_TRUE(tpl.RenderAsString({}).has_value());
    ASSERT_TRUE(kept.GetIf<UserCallable>());
    EXPECT_FALSE(DescribeLentCallable(*kept.GetIf<UserCallable>()));
}

// The Python object a lent global is in Jinja2: its __module__ and __qualname__ (docs/tasks/0559)
TEST(CallSiteTest, LentGlobalsNameTheirPythonObjects)
{
    std::string seen;
    TemplateEnv env;
    env.AddGlobal("describe", UserCallable(
                                  [&seen](const UserCallableParams& params) {
                                      const Value fn = params["fn"];
                                      const auto* callable = fn.GetIf<UserCallable>();
                                      const auto info = callable ? DescribeLentCallable(*callable) : std::nullopt;
                                      seen += info ? info->pythonModule + ":" + info->pythonQualname + ";" : std::string("none;");
                                      return Value();
                                  },
                                  { ArgInfo{ "fn" } }));
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("{% macro m() %}{% endmacro %}{{ describe(range) }}{{ describe(dict) }}{{ describe(cycler) }}{{ describe(joiner) }}"
                         "{{ describe(namespace) }}{{ describe(lipsum) }}{{ describe(m) }}{% for x in [1] %}{{ describe(loop.cycle) }}{% endfor %}")
                    .has_value());
    const auto result = tpl.RenderAsString({});
    ASSERT_TRUE(result.has_value()) << result.error().ToString();
    EXPECT_EQ("builtins:range;builtins:dict;jinja2.utils:Cycler;jinja2.utils:Joiner;jinja2.utils:Namespace;jinja2.utils:generate_lorem_ipsum;:;:;", seen);
}

TEST(CallSiteTest, TemplateNameFromEnvironmentAndInline)
{
    auto probe = std::make_shared<SiteProbe>();
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("page.j2", "{{ probe() }}");
    TemplateEnv env;
    env.AddFilesystemHandler(std::string(), fs);
    env.AddGlobal("probe", MakeSiteProbe(probe, true));

    auto named = env.LoadTemplate("page.j2");
    ASSERT_TRUE(named.has_value());
    ASSERT_TRUE(named->RenderAsString({}).has_value());
    EXPECT_EQ("page.j2", probe->templateName);

    probe->templateName = "unset";
    Template inlineTpl(&env);
    ASSERT_TRUE(inlineTpl.Load("{{ probe() }}").has_value());
    ASSERT_TRUE(inlineTpl.RenderAsString({}).has_value());
    EXPECT_EQ("", probe->templateName);
}

TEST(CallSiteTest, AutoescapeState)
{
    auto probe = std::make_shared<SiteProbe>();
    TemplateEnv env;
    env.AddGlobal("probe", MakeSiteProbe(probe, true));
    Template tpl(&env);

    ASSERT_TRUE(tpl.Load("{{ probe() }}").has_value());
    ASSERT_TRUE(tpl.RenderAsString({}).has_value());
    EXPECT_FALSE(probe->autoescape);

    ASSERT_TRUE(tpl.Load("{% autoescape true %}{{ probe() }}{% endautoescape %}").has_value());
    ASSERT_TRUE(tpl.RenderAsString({}).has_value());
    EXPECT_TRUE(probe->autoescape);

    env.GetSettings().autoescape = true;
    Template escaping(&env);
    ASSERT_TRUE(escaping.Load("{{ probe() }}{% autoescape false %}{{ probe() }}{% endautoescape %}").has_value());
    ASSERT_TRUE(escaping.RenderAsString({}).has_value());
    EXPECT_FALSE(probe->autoescape);
    ASSERT_TRUE(escaping.Load("{{ probe() }}").has_value());
    ASSERT_TRUE(escaping.RenderAsString({}).has_value());
    EXPECT_TRUE(probe->autoescape);
}

TEST(CallSiteTest, DefaultWithoutContextArgument)
{
    auto probe = std::make_shared<SiteProbe>();
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("page.j2", "{% autoescape true %}{{ probe() }}{% endautoescape %}");
    TemplateEnv env;
    env.AddFilesystemHandler(std::string(), fs);
    env.AddGlobal("probe", MakeSiteProbe(probe, false));
    auto named = env.LoadTemplate("page.j2");
    ASSERT_TRUE(named.has_value());
    ASSERT_TRUE(named->RenderAsString({}).has_value());
    ASSERT_TRUE(probe->called);
    EXPECT_EQ("", probe->templateName);
    EXPECT_FALSE(probe->autoescape);
}

// A number outside an integer parameter's range fails the call rather than wrap (docs/tasks/0544)
struct RangeOverloads
{
    int64_t operator()(int x) const { return x; }
    std::string operator()(const std::string& s) const { return s; }
};

TEST(UserCallableRangeTest, NarrowIntegerParameters)
{
    const ValuesMap params{
        { "i", MakeCallable([](int x) { return int64_t{ x }; }, ArgInfo{ "x" }) },
        { "u", MakeCallable([](unsigned x) { return int64_t{ x }; }, ArgInfo{ "x" }) },
        { "c", MakeCallable([](char x) { return int64_t{ x }; }, ArgInfo{ "x" }) },
        { "sc", MakeCallable([](signed char x) { return int64_t{ x }; }, ArgInfo{ "x" }) },
        { "uc", MakeCallable([](unsigned char x) { return int64_t{ x }; }, ArgInfo{ "x" }) },
        { "i16", MakeCallable([](int16_t x) { return int64_t{ x }; }, ArgInfo{ "x" }) },
        { "u16", MakeCallable([](uint16_t x) { return int64_t{ x }; }, ArgInfo{ "x" }) },
        { "i64", MakeCallable([](int64_t x) { return x; }, ArgInfo{ "x" }) },
        { "u64", MakeCallable([](uint64_t x) { return std::to_string(x); }, ArgInfo{ "x" }) },
        { "wc", MakeCallable([](wchar_t x) { return static_cast<int64_t>(x); }, ArgInfo{ "x" }) },
        { "c16", MakeCallable([](char16_t x) { return int64_t{ x }; }, ArgInfo{ "x" }) },
        { "c32", MakeCallable([](char32_t x) { return int64_t{ x }; }, ArgInfo{ "x" }) },
        { "b", MakeCallable([](bool x) { return x; }, ArgInfo{ "x" }) },
        { "d", MakeCallable([](double x) { return x; }, ArgInfo{ "x" }) },
        // ArgInfoT describes a generic lambda's parameter; an overload set goes through the joint visit
        { "gen", MakeCallable([](auto x) { return int64_t{ x }; }, ArgInfoT<int>{ "x" }) },
        { "ovl", MakeCallable(RangeOverloads{}, ArgInfo{ "x" }) },
    };
    struct Row
    {
        std::string tpl;
        std::string expected; // empty: fails with IntegerOutOfRange
    };
    const std::vector<Row> rows{
        // The task's examples
        { "{{ i(2**40 + 5) }}", "" },
        { "{{ u(-1) }}", "" },
        { "{{ c(300) }}", "" },
        { "{{ i(3.9) }}", "3" },
        // The ends of each range pass, one step past fails
        { "{{ i(2147483647) }} {{ i(-2147483648) }} {{ i(none) }} {{ i() }}", "2147483647 -2147483648 0 0" },
        { "{{ i(2147483648) }}", "" },
        { "{{ i(-2147483649) }}", "" },
        { "{{ u(4294967295) }} {{ u(0) }}", "4294967295 0" },
        { "{{ u(4294967296) }}", "" },
        { "{{ c(65) }} {{ c(0) }}", "65 0" },
        { "{{ sc(-128) }} {{ sc(127) }}", "-128 127" },
        { "{{ sc(128) }}", "" },
        { "{{ sc(-129) }}", "" },
        { "{{ uc(255) }} {{ uc(0) }}", "255 0" },
        { "{{ uc(256) }}", "" },
        { "{{ uc(-1) }}", "" },
        { "{{ i16(-32768) }} {{ i16(32767) }}", "-32768 32767" },
        { "{{ i16(32768) }}", "" },
        { "{{ u16(65535) }}", "65535" },
        { "{{ u16(65536) }}", "" },
        { "{{ i64(9223372036854775807) }} {{ i64(-9223372036854775807 - 1) }}", "9223372036854775807 -9223372036854775808" },
        { "{{ u64(9223372036854775807) }} {{ u64(0) }}", "9223372036854775807 0" },
        { "{{ u64(-1) }}", "" },
        { "{{ wc(65) }} {{ c16(65535) }} {{ c32(1114111) }}", "65 65535 1114111" },
        { "{{ c16(65536) }}", "" },
        { "{{ c16(-1) }}", "" },
        { "{{ c32(-1) }}", "" },
        { "{{ c32(4294967296) }}", "" },
        // bool and double parameters take any number as before
        { "{{ b(2**40) }} {{ b(0) }} {{ d(2**40) }}", "True False 1099511627776.0" },
        // A float in range truncates toward zero; NaN, infinities and floats past the range fail
        { "{{ i(-3.9) }} {{ u(-0.5) }} {{ u(4294967295.9) }} {{ sc(-128.9) }} {{ sc(127.9) }}", "-3 0 4294967295 -128 127" },
        { "{{ i(1e300) }}", "" },
        { "{{ i(-2147483649.0) }}", "" },
        { "{{ u(-1.0) }}", "" },
        { "{{ u(4294967296.0) }}", "" },
        { "{{ i('nan'|float) }}", "" },
        { "{{ i('inf'|float) }}", "" },
        { "{{ i64(-9223372036854775808.0) }}", "-9223372036854775808" },
        { "{{ i64(9223372036854775808.0) }}", "" },
        { "{{ i64(-1e19) }}", "" },
        { "{{ u64(18446744073709549568.0) }}", "18446744073709549568" },
        { "{{ u64(18446744073709551616.0) }}", "" },
        // Described generic lambda and overload set
        { "{{ gen(7) }} {{ ovl(7) }} {{ ovl('s') }}", "7 7 s" },
        { "{{ gen(2**40) }}", "" },
        { "{{ ovl(2**40) }}", "" },
    };
    for (const auto& row : rows)
    {
        SCOPED_TRACE(row.tpl);
        Template narrow;
        ASSERT_TRUE(narrow.Load(row.tpl).has_value());
        TemplateW wide;
        ASSERT_TRUE(wide.Load(ConvertString<std::wstring>(row.tpl)).has_value());
        const auto narrowResult = narrow.RenderAsString(params);
        const auto wideResult = wide.RenderAsString(params);
        if (row.expected.empty())
        {
            ASSERT_FALSE(narrowResult.has_value());
            EXPECT_EQ(ErrorCode::IntegerOutOfRange, narrowResult.error().GetCode());
            ASSERT_FALSE(wideResult.has_value());
            EXPECT_EQ(ErrorCode::IntegerOutOfRange, wideResult.error().GetCode());
        }
        else
        {
            ASSERT_TRUE(narrowResult.has_value()) << narrowResult.error();
            EXPECT_EQ(row.expected, narrowResult.value());
            ASSERT_TRUE(wideResult.has_value());
            EXPECT_EQ(ConvertString<std::wstring>(row.expected), wideResult.value());
        }
    }
}

TEST(UserCallableRangeTest, ErrorNamesTheParameter)
{
    const ValuesMap params{
        { "f", MakeCallable([](int count) { return int64_t{ count }; }, ArgInfo{ "count" }) },
        { "ovl", MakeCallable(RangeOverloads{}, ArgInfo{ "x" }) },
    };
    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ f(2**40 + 5) }}").has_value());
    auto result = tpl.RenderAsString(params);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("callable argument 'count' = 1099511627781 is out of range for its 32-bit signed integer parameter",
              result.error().GetExtraParams().at(0).AsString());
    ASSERT_TRUE(tpl.Load("{{ f(1e300) }}").has_value());
    result = tpl.RenderAsString(params);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("callable argument 'count' = 1e+300 is out of range for its 32-bit signed integer parameter",
              result.error().GetExtraParams().at(0).AsString());
    // Through the joint visit the parameter has no name
    ASSERT_TRUE(tpl.Load("{{ ovl(-(2**40)) }}").has_value());
    result = tpl.RenderAsString(params);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ("callable argument -1099511627776 is out of range for its 32-bit signed integer parameter",
              result.error().GetExtraParams().at(0).AsString());
}
