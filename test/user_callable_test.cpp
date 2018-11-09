#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include "jinja2cpp/template.h"
#include "jinja2cpp/user_callable.h"
#include "test_tools.h"

using namespace jinja2;

TEST(UserCallableTest, SimpleUserCallable)
{
    std::string source = R"(
{{ test() }}
{{ test() }}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    jinja2::UserCallable uc;
    uc.callable = [](auto&)->jinja2::Value {return "Hello World!";};
    jinja2::ValuesMap params;
    params["test"] = std::move(uc);

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Hello World!
Hello World!
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(UserCallableTest, SimpleUserCallableWithParams1)
{
    std::string source = R"(
{{ test('Hello', 'World!') }}
{{ test(str2='World!', str1='Hello') }}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    jinja2::UserCallable uc;
    uc.callable = [](auto& params)->jinja2::Value {
        return params["str1"].asString() + " " + params["str2"].asString();
    };
    uc.argsInfo = {{"str1", true}, {"str2", true}};
    jinja2::ValuesMap params;
    params["test"] = std::move(uc);

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Hello World!
Hello World!
)";
    EXPECT_EQ(expectedResult, result);
}

TEST(UserCallableTest, SimpleUserCallableWithParams2)
{
    std::string source = R"(
{{ test('Hello', 'World!') }}
{{ test(str2='World!', str1='Hello') }}
{{ test(str2='World!') }}
{{ test('Hello') }}
)";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    jinja2::ValuesMap params;
    params["test"] = MakeCallable(
                [](const std::string& str1, const std::string& str2) {
                    return str1 + " " + str2;
                },
                ArgInfo{"str1"}, ArgInfo{"str2", false, "default"}
    );

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = R"(
Hello World!
Hello World!
 World!
Hello default
)";
    EXPECT_EQ(expectedResult, result);
}

struct UserCallableParamConvertTestTag;
using UserCallableParamConvertTest = InputOutputPairTest<UserCallableParamConvertTestTag>;

TEST_P(UserCallableParamConvertTest, Test)
{
    auto& testParam = GetParam();
    std::string source = "{{" + testParam.tpl + " | pprint }}";

    Template tpl;
    auto parseRes = tpl.Load(source);
    EXPECT_TRUE(parseRes.has_value());
    if (!parseRes)
    {
        std::cout << parseRes.error() << std::endl;
        return;
    }

    jinja2::ValuesMap params = PrepareTestData();

    params["BoolFn"] = MakeCallable([](bool val) {return val;}, ArgInfo{"val"});
    params["IntFn"] = MakeCallable([](int val) {return val;}, ArgInfo{"val"});
    params["Int64Fn"] = MakeCallable([](int64_t val) {return val;}, ArgInfo{"val"});
    params["DoubleFn"] = MakeCallable([](double val) {return val;}, ArgInfo{"val"});
    params["StringFn"] = MakeCallable([](const std::string& val) {return val;}, ArgInfo{"val"});
    params["WStringFn"] = MakeCallable([](const std::wstring& val) {return val;}, ArgInfo{"val"});
    params["GListFn"] = MakeCallable([](const GenericList& val) {return val;}, ArgInfo{"val"});
    params["GMapFn"] = MakeCallable([](const GenericMap& val) {return val;}, ArgInfo{"val"});

    std::string result = tpl.RenderAsString(params);
    std::cout << result << std::endl;
    std::string expectedResult = testParam.result;
    EXPECT_EQ(expectedResult, result);
}

INSTANTIATE_TEST_CASE_P(BoolParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"BoolFn()",                   "false"},
                            InputOutputPair{"BoolFn(true)", "true"}
                            ));

INSTANTIATE_TEST_CASE_P(IntParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"IntFn()",                   "0"},
                            InputOutputPair{"IntFn(10)", "10"},
                            InputOutputPair{"IntFn(10.123)", "10"}
                            ));

INSTANTIATE_TEST_CASE_P(Int64ParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"Int64Fn()",                   "0"},
                            InputOutputPair{"Int64Fn(10)", "10"},
                            InputOutputPair{"Int64Fn(10.123)", "10"}
                            ));

INSTANTIATE_TEST_CASE_P(DoubleParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"DoubleFn()",                   "0"},
                            InputOutputPair{"DoubleFn(10)", "10"},
                            InputOutputPair{"DoubleFn(10.123)", "10.123"}
                            ));

INSTANTIATE_TEST_CASE_P(StringParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"StringFn()",                   "''"},
                            InputOutputPair{"StringFn('Hello World')", "'Hello World'"},
                            InputOutputPair{"StringFn(stringValue)", "'rain'"}
                            ));

INSTANTIATE_TEST_CASE_P(ListParamConvert, UserCallableParamConvertTest, ::testing::Values(
                            InputOutputPair{"GListFn()",                   "[]"},
                            InputOutputPair{"GListFn([1, 2, 3, 4])",       "[1, 2, 3, 4]"},
                            InputOutputPair{"GListFn(intList)",            "[9, 0, 8, 1, 7, 2, 6, 3, 5, 4]"},
                            InputOutputPair{"GListFn(reflectedIntVector)", "[9, 0, 8, 1, 7, 2, 6, 3, 5, 4]"},
                            InputOutputPair{"GListFn(range(10))", "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"},
                            InputOutputPair{"GListFn([1, 2, 3, 4] | sort)","[1, 2, 3, 4]"},
                            InputOutputPair{"GListFn(intList | sort)",     "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"},
                            InputOutputPair{"GListFn(reflectedIntVector | sort)", "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"},
                            InputOutputPair{"GListFn(range(10) | sort)", "[0, 1, 2, 3, 4, 5, 6, 7, 8, 9]"},
                            InputOutputPair{"GListFn({'name'='itemName', 'val'='itemValue'} | list) | sort", "['name', 'val']"},
                            InputOutputPair{"GListFn({'name'='itemName', 'val'='itemValue'} | list | sort)", "['name', 'val']"}
                            ));
