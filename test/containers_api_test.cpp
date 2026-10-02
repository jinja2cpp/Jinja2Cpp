// 2.0 API of the extension interfaces (docs/tasks/0075): containers, reflection, errors, user callables.
// This file and forloop_test.cpp both include make_generic_list.h, which checks that the header links
// from two translation units (0069).
#include "test_tools.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>

#include <gtest/gtest.h>

#include <list>
#include <map>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

using namespace jinja2;

namespace
{
// A map accessor written the way the docs show it, without IsEqual: IComparable supplies identity.
struct PlainMapAccessor : IMapItemAccessor
{
    std::map<std::string, int64_t> items{ { "a", 1 }, { "b", 2 }, { "c", 3 } };

    [[nodiscard]] size_t GetSize() const override { return items.size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return items.count(name) != 0; }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        auto p = items.find(name);
        return p == items.end() ? Value() : Value(p->second);
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> keys;
        for (const auto& item : items)
            keys.push_back(item.first);
        return keys;
    }
};

struct PlainFilesystemHandler : IFilesystemHandler
{
    [[nodiscard]] CharFileStreamPtr OpenStream(const std::string&) const override
    {
        return CharFileStreamPtr(nullptr, [](std::istream*) {});
    }
    [[nodiscard]] WCharFileStreamPtr OpenWStream(const std::string&) const override
    {
        return WCharFileStreamPtr(nullptr, [](std::wistream*) {});
    }
    [[nodiscard]] std::optional<std::chrono::system_clock::time_point> GetLastModificationDate(const std::string&) const override { return {}; }
};

struct Celsius
{
    double degrees;
};

struct LegacyReflected
{
    int value;
};

struct LegacyDelegating
{
    std::string text;
};

std::string Render(const std::string& source, const ValuesMap& params)
{
    Template tpl;
    auto load = tpl.Load(source);
    if (!load)
        return "load error: " + load.error().ToString();
    auto result = tpl.RenderAsString(params);
    if (!result)
        return "render error: " + result.error().ToString();
    return result.value();
}
} // namespace

namespace jinja2
{
// The 2.0 extension point
template<>
struct Reflector<Celsius>
{
    static Value Create(const Celsius& val) { return Value(std::to_string(static_cast<int>(val.degrees)) + "C"); }
    static Value CreateFromPtr(const Celsius* val) { return Create(*val); }
};

namespace detail
{
// A 1.x-style specialisation keeps working
template<>
struct Reflector<LegacyReflected>
{
    static Value Create(const LegacyReflected& val) { return Value(static_cast<int64_t>(val.value * 10)); }
    static Value CreateFromPtr(const LegacyReflected* val) { return Create(*val); }
};

// 1.x code could delegate to the library's reflectors, and override one for a type the library also covers
template<>
struct Reflector<LegacyDelegating>
{
    static Value Create(const LegacyDelegating& val) { return Reflector<std::string>::Create(val.text + "!"); }
    static Value CreateFromPtr(const LegacyDelegating* val) { return Create(*val); }
};

template<>
struct Reflector<char16_t>
{
    static Value Create(char16_t val) { return Value(std::string(1, static_cast<char>(val))); }
    static Value CreateFromPtr(const char16_t* val) { return Create(*val); }
};
} // namespace detail
} // namespace jinja2

TEST(ContainersApiTest, ErrorCodeValuesAreStable)
{
    static_assert(static_cast<int>(ErrorCode::Unspecified) == 0);
    static_assert(static_cast<int>(ErrorCode::UnexpectedException) == 1);
    static_assert(static_cast<int>(ErrorCode::TemplateNotFound) == 6);
    static_assert(static_cast<int>(ErrorCode::UndefinedError) == 11);
    static_assert(static_cast<int>(ErrorCode::ExpectedStringLiteral) == 1001);
    static_assert(static_cast<int>(ErrorCode::ExpectedEndOfStatement) == 1008);
    static_assert(static_cast<int>(ErrorCode::UnexpectedMetaEnd) == 1022);
    static_assert(std::is_same_v<ErrorInfo, BasicErrorInfo<char>>);
    static_assert(std::is_same_v<ErrorInfoW, BasicErrorInfo<wchar_t>>);
}

TEST(ContainersApiTest, IComparableDefaultsToIdentity)
{
    PlainMapAccessor acc1;
    PlainMapAccessor acc2;
    GenericMap map1([&acc1]() { return &acc1; });
    GenericMap sameAsMap1([&acc1]() { return &acc1; });
    GenericMap map2([&acc2]() { return &acc2; });
    EXPECT_TRUE(map1 == sameAsMap1);
    EXPECT_FALSE(map1 == map2);

    PlainFilesystemHandler fs1;
    PlainFilesystemHandler fs2;
    EXPECT_TRUE(fs1.IsEqual(fs1));
    EXPECT_FALSE(fs1.IsEqual(fs2));
}

TEST(ContainersApiTest, UserMapAccessorWithoutIsEqualRenders)
{
    ValuesMap params{ { "m", GenericMap([acc = PlainMapAccessor()]() { return &acc; }) } };
    EXPECT_EQ("1 3|a=1,b=2,c=3,", Render("{{ m.a }} {{ m['c'] }}|{% for k, v in m | dictsort %}{{ k }}={{ v }},{% endfor %}", params));
}

TEST(ContainersApiTest, GenericMapIteration)
{
    GenericMap map([acc = PlainMapAccessor()]() { return &acc; });
    static_assert(std::is_same_v<GenericMap::iterator::value_type, std::pair<std::string, Value>>);

    std::string out;
    for (const auto& [key, value] : map)
        out += key + "=" + std::to_string(value.get<int64_t>()) + ";";
    EXPECT_EQ("a=1;b=2;c=3;", out);

    auto it = map.cbegin();
    EXPECT_EQ("a", it->first);
    auto prev = it++;
    EXPECT_EQ("a", prev->first);
    EXPECT_EQ("b", it->first);
    EXPECT_TRUE(prev != it);
    EXPECT_EQ(3, std::distance(map.begin(), map.end()));
}

TEST(ContainersApiTest, GenericMapOfReflectedStruct)
{
    TestStruct obj;
    obj.intValue = 7;
    auto value = Reflect(obj);
    auto map = value.get<GenericMap>();

    size_t count = 0;
    for (const auto& item : map)
    {
        EXPECT_TRUE(map.HasValue(item.first));
        ++count;
    }
    EXPECT_EQ(map.GetSize(), count);

    // An unknown field is an empty value, as IMapItemAccessor documents, not an exception
    EXPECT_TRUE(map.GetValueByName("noSuchField").isEmpty());
    EXPECT_EQ(7, map.GetValueByName("intValue").get<int64_t>());
}

TEST(ContainersApiTest, DefaultConstructedGenericMap)
{
    GenericMap map;
    EXPECT_EQ(nullptr, map.GetAccessor());
    EXPECT_TRUE(map.begin() == map.end());
    EXPECT_EQ(0U, map.GetSize());
    EXPECT_FALSE(map == GenericMap());
}

TEST(ContainersApiTest, GenericListIterator)
{
    static_assert(std::is_same_v<GenericList::iterator, detail::GenericListIterator>);
    static_assert(std::is_same_v<std::iterator_traits<GenericList::iterator>::value_type, Value>);

    std::vector<int> items{ 1, 2, 3 };
    GenericList list = MakeGenericList(items.begin(), items.end());
    int64_t sum = 0;
    for (auto it = list.cbegin(); it != list.cend(); ++it)
        sum += it->get<int64_t>();
    EXPECT_EQ(6, sum);
    EXPECT_EQ(nullptr, GenericList().GetAccessor());
}

TEST(ContainersApiTest, MakeGenericListFromNamedIterators)
{
    std::vector<int> vec{ 1, 2, 3 };
    auto vb = vec.begin();
    auto ve = vec.end();
    EXPECT_EQ("1,2,3", Render("{{ l | join(',') }}", { { "l", MakeGenericList(vb, ve) } }));

    std::list<std::string> lst{ "a", "b" };
    const auto lb = lst.cbegin();
    const auto le = lst.cend();
    EXPECT_EQ("a,b", Render("{{ l | join(',') }}", { { "l", MakeGenericList(lb, le) } }));

    std::istringstream input("4 5 6");
    std::istream_iterator<int> ib(input);
    std::istream_iterator<int> ie;
    EXPECT_EQ("4,5,6", Render("{{ l | join(',') }}", { { "l", MakeGenericList(ib, ie) } }));
}

TEST(ContainersApiTest, GeneratedListIsComparedByIdentity)
{
    int calls = 0;
    GenericList list = MakeGenericList([&calls]() -> std::optional<Value> {
        ++calls;
        return std::nullopt;
    });
    GenericList copy = list;
    EXPECT_TRUE(list == list);
    (void)(list == copy);
    EXPECT_EQ(0, calls);
}

TEST(ContainersApiTest, ReflectEveryArithmeticType)
{
    auto asInt = [](const Value& v) { return std::get<int64_t>(v.data()); };
    EXPECT_EQ(5, asInt(Reflect(5LL)));
    EXPECT_EQ(5, asInt(Reflect(5ULL)));
    EXPECT_EQ(5, asInt(Reflect(5L)));
    EXPECT_EQ(5, asInt(Reflect(5UL)));
    EXPECT_EQ(5, asInt(Reflect(static_cast<short>(5))));
    EXPECT_EQ(5, asInt(Reflect(static_cast<unsigned short>(5))));
    EXPECT_EQ(5, asInt(Reflect(static_cast<size_t>(5))));
    EXPECT_EQ(65, asInt(Reflect('A')));
    EXPECT_EQ(65, asInt(Reflect(U'A')));
    const long long named = 6;
    EXPECT_EQ(6, asInt(Reflect(named)));
    EXPECT_EQ(6, asInt(Reflect(&named)));

    EXPECT_EQ(1.5, std::get<double>(Reflect(1.5F).data()));
    EXPECT_EQ(1.5, std::get<double>(Reflect(1.5L).data()));
    EXPECT_TRUE(std::get<bool>(Reflect(true).data()));
    const bool constTrue = true;
    EXPECT_TRUE(std::get<bool>(Reflect(std::move(constTrue)).data()));

    auto shared = std::make_shared<std::vector<int>>(std::vector<int>{ 7, 8 });
    EXPECT_EQ("7,8", Render("{{ l | join(',') }}", { { "l", Reflect(shared) } }));

    std::vector<unsigned long long> vec{ 1, 2 };
    EXPECT_EQ("1,2", Render("{{ l | join(',') }}", { { "l", Reflect(vec) } }));
}

TEST(ContainersApiTest, PublicAndLegacyReflector)
{
    EXPECT_EQ("21C", Render("{{ t }}", { { "t", Reflect(Celsius{ 21.0 }) } }));
    std::vector<Celsius> temps{ { 1.0 }, { 2.0 } };
    EXPECT_EQ("1C,2C", Render("{{ t | join(',') }}", { { "t", Reflect(temps) } }));

    LegacyReflected legacy{ 4 };
    EXPECT_EQ("40", Render("{{ v }}", { { "v", Reflect(legacy) } }));
    EXPECT_EQ("40", Render("{{ v }}", { { "v", Reflect(&legacy) } }));
    EXPECT_EQ("hi!", Render("{{ v }}", { { "v", Reflect(LegacyDelegating{ "hi" }) } }));
    EXPECT_EQ("x", Render("{{ v }}", { { "v", Reflect(u'x') } }));
}

TEST(ContainersApiTest, ArgInfoConstants)
{
    EXPECT_STREQ("*args", ArgInfo::VarArgs);
    EXPECT_STREQ("**kwargs", ArgInfo::VarKwArgs);
    EXPECT_STREQ("*context", ArgInfo::Context);

    static_assert(std::is_same_v<UserCallable::Function, std::function<Value(const UserCallableParams&)>>);
    UserCallable::Function fn = [](const UserCallableParams& params) -> Value {
        return static_cast<int64_t>(params.extraPosArgs.asList().size());
    };
    UserCallable count(fn, { ArgInfo{ ArgInfo::VarArgs } });
    EXPECT_EQ("3", Render("{{ count(1, 2, 3) }}", { { "count", count } }));

    auto sumKw = MakeCallable(
        [](const ValuesMap& kwargs) {
            int64_t sum = 0;
            for (const auto& item : kwargs)
                sum += item.second.get<int64_t>();
            return sum;
        },
        ArgInfo{ ArgInfo::VarKwArgs });
    EXPECT_EQ("5", Render("{{ sum(a=2, b=3) }}", { { "sum", sumKw } }));
}

// The 1.x names are deprecated aliases until 3.0: they must keep compiling, with the deprecation warning silenced here.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
TEST(ContainersApiTest, V1NamesStillCompile)
{
    static_assert(std::is_same_v<ErrorInfoTpl<char>, ErrorInfo>);
    static_assert(std::is_same_v<UserCallable::UserCallableFunctionPtr, UserCallable::Function>);
    std::vector<int> items{ 1, 2 };
    GenericList list = MakeGenericList(items.begin(), items.end());
    EXPECT_EQ(2U, list.GetSize().value());
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

TEST(ContainersApiTest, CloneOfUnstartedEnumeratorSeesAllItems)
{
    std::vector<int> vec{ 1, 2, 3 };
    std::list<int> lst{ 1, 2, 3 };
    for (const GenericList& list : { MakeGenericList(vec.begin(), vec.end()), MakeGenericList(lst.begin(), lst.end()) })
    {
        auto enumerator = list.GetAccessor()->CreateEnumerator();
        ASSERT_TRUE(enumerator.has_value());
        auto clone = (*enumerator)->Clone();
        int count = 0;
        while (clone->MoveNext())
            ++count;
        EXPECT_EQ(3, count);

        // A clone taken mid-way continues from the same item, and Reset() rewinds it to the first one
        ASSERT_TRUE((*enumerator)->MoveNext());
        ASSERT_TRUE((*enumerator)->MoveNext());
        auto midClone = (*enumerator)->Clone();
        EXPECT_EQ(2, midClone->GetCurrent().get<int64_t>());
        midClone->Reset();
        ASSERT_TRUE(midClone->MoveNext());
        EXPECT_EQ(1, midClone->GetCurrent().get<int64_t>());
    }
}
