// The 1.x names kept as deprecated forwards until 3.0 (docs/api-2.0.md 5.1, task 0072). Built with
// deprecation warnings off, so the forwards stay compiled and keep their 1.x behaviour.
#define JINJA2CPP_NO_DEPRECATION_WARNINGS

#include "gtest/gtest.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/value.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

using namespace jinja2;

TEST(V1NamesTest, ValuePredicatesAndAccessors)
{
    Value str("text");
    EXPECT_TRUE(str.isString());
    EXPECT_FALSE(str.isWString());
    EXPECT_EQ("text", str.asString());
    str.asString() += "!";
    EXPECT_EQ("text!", std::as_const(str).asString());
    EXPECT_THROW((void)Value(5).asString(), std::bad_variant_access);

    Value wstr(L"w");
    EXPECT_TRUE(wstr.isWString());
    EXPECT_EQ(L"w", wstr.asWString());
    EXPECT_EQ(L"w", std::as_const(wstr).asWString());

    Value list(ValuesList{ 1 });
    EXPECT_TRUE(list.isList());
    EXPECT_TRUE(Value(Reflect(std::vector<int>{ 1 })).isList());
    list.asList().push_back(2);
    EXPECT_EQ(2U, std::as_const(list).asList().size());

    Value map(ValuesMap{ { "k", 1 } });
    EXPECT_TRUE(map.isMap());
    map.asMap()["n"] = 2;
    EXPECT_EQ(2U, std::as_const(map).asMap().size());

    EXPECT_TRUE(Value().isEmpty());
    EXPECT_FALSE(str.isEmpty());
}

TEST(V1NamesTest, GetCopiesAndGetPtrPoints)
{
    Value str("text");
    auto copy = str.get<std::string>();
    static_assert(std::is_same_v<decltype(copy), std::string>);
    copy += "!";
    EXPECT_EQ("text", str.AsString());
    EXPECT_EQ("text", std::as_const(str).get<std::string>());
    EXPECT_EQ(&str.AsString(), str.getPtr<std::string>());
    EXPECT_EQ(nullptr, std::as_const(str).getPtr<int64_t>());
    EXPECT_EQ(&str.GetData(), &str.data());
    EXPECT_EQ(&str.GetData(), &std::as_const(str).data());
}

TEST(V1NamesTest, FreeAsStringReturnsEmptyForNonStrings)
{
    EXPECT_EQ("text", AsString(Value("text")));
    EXPECT_EQ("wide", AsString(Value(L"wide")));
    EXPECT_EQ("", AsString(Value(5)));
    EXPECT_EQ(L"text", AsWString(Value("text")));
    EXPECT_EQ(L"", AsWString(Value(5)));

    EXPECT_EQ("abc", AsString(std::string("abc")));
    EXPECT_EQ("abc", AsString(std::wstring(L"abc")));
    EXPECT_EQ("abc", AsString(std::string_view("abc")));
    EXPECT_EQ("abc", AsString(std::wstring_view(L"abc")));
    EXPECT_EQ(L"abc", AsWString(std::wstring(L"abc")));
    EXPECT_EQ(L"abc", AsWString(std::string("abc")));
    EXPECT_EQ(L"abc", AsWString(std::wstring_view(L"abc")));
    EXPECT_EQ(L"abc", AsWString(std::string_view("abc")));
}

TEST(V1NamesTest, TypeAliases)
{
    static_assert(std::is_same_v<EmptyValue, NoneValue>);
    static_assert(std::is_same_v<UserCallable::UserCallableFunctionPtr, UserCallable::Function>);
    static_assert(std::is_same_v<ErrorInfoTpl<char>, BasicErrorInfo<char>>);
    EXPECT_TRUE(Value(EmptyValue()).IsNone());
}
