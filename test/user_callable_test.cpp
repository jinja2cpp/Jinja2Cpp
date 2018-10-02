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

TEST(UserCallableTest, UserCallableParamsReflection)
{
    std::string source = R"(
{{ BoolFn() | pprint }}
{{ BoolFn(true) | pprint }}
{{ IntFn() | pprint }}
{{ IntFn(10) | pprint }}
{{ IntFn(10.123) | pprint }}
{{ Int64Fn() | pprint }}
{{ Int64Fn(10) | pprint }}
{{ Int64Fn(10.123) | pprint }}
{{ DoubleFn() | pprint }}
{{ DoubleFn(10) | pprint }}
{{ DoubleFn(10.123) | pprint }}
{{ StringFn() | pprint }}
{{ StringFn('Hello World') | pprint }}
{{ StringFn(stringValue) | pprint }}
{# { GListFn() | pprint }}
{{ GListFn([1, 2, 3, 4]) | pprint }}
{{ GListFn(intList) | pprint }}
{{ GListFn(reflectedIntVector) | pprint }#}
)";

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
    std::string expectedResult = R"(
false
true
0
10
10
0
10
10
0
10
10.123
''
'Hello World'
'rain'
)";
    EXPECT_EQ(expectedResult, result);
}
