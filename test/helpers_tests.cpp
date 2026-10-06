#include "gtest/gtest.h"

#include "../src/helpers.h"
#include "../src/expression_evaluator.h"
#include "../src/internal_value.h"
#include "../src/render_context.h"
#include "../src/value_visitors.h"

#include <jinja2cpp/string_helpers.h>

#include <clocale>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <string_view>

using namespace jinja2;

TEST(Helpers, CompileEscapes)
{
    EXPECT_STREQ("\n", CompileEscapes(std::string{"\\n"}).c_str());
    EXPECT_STREQ("\t", CompileEscapes(std::string{"\\t"}).c_str());
    EXPECT_STREQ("\r", CompileEscapes(std::string{"\\r"}).c_str());
    EXPECT_STREQ("\r\n\t", CompileEscapes(std::string{R"(\r\n\t)"}).c_str());
    EXPECT_STREQ(
        "aa\rbb\ncc\tdd",
        CompileEscapes(std::string{R"(aa\rbb\ncc\tdd)"}).c_str());
    EXPECT_STREQ("", CompileEscapes(std::string{""}).c_str());
    EXPECT_STREQ(
        "aa bb cc dd",
        CompileEscapes(std::string{"aa bb cc dd"}).c_str());
}

TEST(Helpers, ConvertStringKeepsEmbeddedNul)
{
    using namespace std::string_literals;
    EXPECT_EQ("ab\0cd\0"s, ConvertString<std::string>(L"ab\0cd\0"s));
    EXPECT_EQ(L"\0ab\0\0cd"s, ConvertString<std::wstring>("\0ab\0\0cd"s));
    EXPECT_EQ(""s, ConvertString<std::string>(std::wstring()));
}

TEST(Helpers, ConvertStringReadsOnlyTheView)
{
    constexpr std::wstring_view wide(L"abcdef", 3);
    constexpr std::string_view narrow("abcdef", 3);
    EXPECT_EQ("abc", ConvertString<std::string>(wide));
    EXPECT_EQ(L"abc", ConvertString<std::wstring>(narrow));
}

// The conversion follows the C locale; with a UTF-8 one, multibyte output must not be
// cut to the length of the wide source
TEST(Helpers, ConvertStringMultibyte)
{
    const std::string saved = std::setlocale(LC_CTYPE, nullptr);
    if (!std::setlocale(LC_CTYPE, "C.UTF-8") && !std::setlocale(LC_CTYPE, "en_US.UTF-8") && !std::setlocale(LC_CTYPE, ".UTF-8"))
    {
        GTEST_SKIP() << "no UTF-8 locale available";
    }
    const std::string utf8("\xC3\xA9\xE2\x98\x83\0\xC3\xA9", 8);
    const std::wstring wide(L"\u00E9\u2603\0\u00E9", 4);
    const auto narrowed = ConvertString<std::string>(wide);
    const auto widened = ConvertString<std::wstring>(utf8);
    std::setlocale(LC_CTYPE, saved.c_str());
    EXPECT_EQ(utf8, narrowed);
    EXPECT_EQ(wide, widened);
}

// Scope lookups compare names in words up to 16 bytes (docs/tasks/0100)
TEST(Helpers, NameEqualComparesEveryByte)
{
    for (size_t size = 0; size <= 33; ++size)
    {
        std::string name(size, 'a');
        for (size_t n = 0; n < size; ++n)
        {
            name[n] = static_cast<char>('a' + (n % 26));
        }
        EXPECT_TRUE(jinja2::NameEqual::Equal(name, std::string(name))) << size;
        EXPECT_FALSE(jinja2::NameEqual::Equal(name, name + "x")) << size;
        for (size_t n = 0; n < size; ++n)
        {
            auto other = name;
            other[n] = '_';
            EXPECT_FALSE(jinja2::NameEqual::Equal(name, other)) << size << " " << n;
        }
    }
}

// A name expression made during a render and freed may be followed by another at the same
// address while the lookup epoch is still current: the second must not get the first one's
// cached slot (docs/tasks/0100 idea 7)
TEST(Helpers, LookupCacheForgetsFreedKeys)
{
    InternalValueMap ext = { { "a", InternalValue(int64_t{ 1 }) }, { "b", InternalValue(int64_t{ 2 }) } };
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    auto& cache = LookupCache::ForThisThread();
    context.SetLookupCache(&cache);

    int key = 0;
    const auto slot = LookupCache::NewSlot();
    const auto* a = context.FindValueCached(&key, slot, HashedName{ "a", HashedName::Hash("a") });
    ASSERT_TRUE(a);
    EXPECT_EQ(1, ConvertToInt(*a));
    cache.Forget(&key, slot);
    const auto* b = context.FindValueCached(&key, slot, HashedName{ "b", HashedName::Hash("b") });
    ASSERT_TRUE(b);
    EXPECT_EQ(2, ConvertToInt(*b));
}

// Name expressions take cache entries in turn, so the names of one template do not share an
// entry wherever the heap puts them (docs/tasks/0139)
TEST(Helpers, LookupCacheSlotsTakenInTurn)
{
    const auto first = LookupCache::NewSlot();
    const auto second = LookupCache::NewSlot();
    EXPECT_NE(first, second);
}

// The expression itself forgets its entry when destroyed. Its vtable is not exported from a
// shared library, so this part runs against the static one only
#ifndef JINJA2CPP_LINK_AS_SHARED
TEST(Helpers, LookupCacheForgetsFreedExpressions)
{
    InternalValueMap ext = { { "a", InternalValue(int64_t{ 1 }) }, { "b", InternalValue(int64_t{ 2 }) } };
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    context.SetLookupCache(&LookupCache::ForThisThread());

    alignas(ValueRefExpression) unsigned char storage[sizeof(ValueRefExpression)];
    auto* first = new (storage) ValueRefExpression("a");
    EXPECT_EQ(1, ConvertToInt(first->Evaluate(context)));
    first->~ValueRefExpression();
    auto* second = new (storage) ValueRefExpression("b");
    EXPECT_EQ(2, ConvertToInt(second->Evaluate(context)));
    second->~ValueRefExpression();
}
#endif
