#include "gtest/gtest.h"

#include "../src/helpers.h"
#include "../src/expression_evaluator.h"
#include "../src/internal_value.h"
#include "../src/lookup_result.h"
#include "../src/render_context.h"
#include "../src/value_visitors.h"

#include <jinja2cpp/string_helpers.h>

#include <clocale>
#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <string_view>
#include <thread>

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

TEST(Helpers, CompileEscapesPythonSequences)
{
    EXPECT_EQ("\a\b\f\v", CompileEscapes(std::string{ R"(\a\b\f\v)" }));
    EXPECT_EQ("\\'\"", CompileEscapes(std::string{ R"(\\\'\")" }));
    EXPECT_EQ("AB\xC3\xA9\xE2\x82\xAC\xF0\x9F\x98\x80", CompileEscapes(std::string{ R"(\x41\102\xe9\u20ac\U0001F600)" }));
    EXPECT_EQ(std::string("\0"
                          "7",
                          2),
              CompileEscapes(std::string{ R"(\0\067)" }));
    EXPECT_EQ("\xC4\x80", CompileEscapes(std::string{ R"(\400)" }));
    EXPECT_EQ("ab", CompileEscapes(std::string{ "a\\\nb" }));
    EXPECT_EQ(R"(\q\8)", CompileEscapes(std::string{ R"(\q\8)" }));
    EXPECT_EQ("a\\", CompileEscapes(std::string{ "a\\" }));
    EXPECT_EQ(L"\u00e9\x20ac", CompileEscapes(std::wstring{ LR"(\xe9\u20ac)" }));
    // The wide literal is a surrogate pair where wchar_t is 16 bits wide (MSVC) and one code unit elsewhere
    EXPECT_EQ(std::wstring{ L"\U0001F600" }, CompileEscapes(std::wstring{ LR"(\U0001F600)" }));
}

TEST(Helpers, CompileEscapesRejectsMalformed)
{
    for (const char* text : { R"(\x)", R"(\x4)", R"(\x4g)", R"(\u123)", R"(\U0001F60)", R"(\U00110000)", R"(\N{BULLET})", R"(ab\N)" })
    {
        bool ok = true;
        CompileEscapes(std::string{ text }, &ok);
        EXPECT_FALSE(ok) << text;
    }
    bool ok = true;
    CompileEscapes(std::string{ R"(\x41\u0042)" }, &ok);
    EXPECT_TRUE(ok);
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

// A lookup that found nothing is false; one that found a value refers to it in place
TEST(Helpers, LookupResult)
{
    const LookupResult none;
    EXPECT_FALSE(none);

    InternalValueMap ext = { { "a", InternalValue(int64_t{ 1 }) } };
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    EXPECT_FALSE(context.FindValue(std::string("b")));
    const auto a = context.FindValue(std::string("a"));
    ASSERT_TRUE(a);
    EXPECT_EQ(1, ConvertToInt(*a));
    EXPECT_TRUE(a.IsSame(context.FindValue(std::string("a"))));
    EXPECT_FALSE(a.IsSame(none));

    auto slot = context.FindForWrite("a");
    ASSERT_TRUE(slot);
    *slot = InternalValue(int64_t{ 2 });
    EXPECT_EQ(2, ConvertToInt(*a));
    EXPECT_FALSE(context.FindForWrite("b"));
}

// An entry is used only under the epoch it was cached in, and each render takes a new one: a
// key freed after a render and reused by another tree is looked up afresh (0118 P5b)
TEST(Helpers, LookupCacheEntriesEndWithTheirEpoch)
{
    InternalValueMap ext = { { "a", InternalValue(int64_t{ 1 }) }, { "b", InternalValue(int64_t{ 2 }) } };
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    auto& cache = LookupCache::ForThisThread();
    context.SetLookupCache(&cache);

    int key = 0;
    const auto slot = LookupCache::NewSlot(HashedName::Hash("a"));
    const auto a = context.FindValueCached(&key, slot, HashedName{ "a", HashedName::Hash("a") });
    ASSERT_TRUE(a);
    EXPECT_EQ(1, ConvertToInt(*a));
    // The next render
    context.SetLookupCache(&cache);
    const auto b = context.FindValueCached(&key, slot, HashedName{ "b", HashedName::Hash("b") });
    ASSERT_TRUE(b);
    EXPECT_EQ(2, ConvertToInt(*b));
}

// Each thread looks names up in a cache of its own, whose epochs no other thread sees: a
// context is used only on the thread whose cache it holds (0118 P5b)
TEST(Helpers, LookupCacheIsPerThread)
{
    const LookupCache* here = &LookupCache::ForThisThread();
    const LookupCache* there = nullptr;
    std::thread other([&there] { there = &LookupCache::ForThisThread(); });
    other.join();
    EXPECT_NE(here, there);
}

// Name expressions take the cache entries of their name's bucket in turn, so the names of one
// template do not share an entry wherever the heap puts them (docs/tasks/0139, 0158)
TEST(Helpers, LookupCacheSlotsTakenInTurn)
{
    const auto hash = HashedName::Hash("a");
    const auto first = LookupCache::NewSlot(hash);
    const auto second = LookupCache::NewSlot(hash);
    EXPECT_NE(first, second);
    EXPECT_EQ(hash % LookupCache::Buckets, first / LookupCache::Ways);
    EXPECT_EQ(hash % LookupCache::Buckets, second / LookupCache::Ways);
}

// A name added to a scope, or a scope cleared, forgets the cached lookups of its names only:
// the other names stay cached (docs/tasks/0158)
TEST(Helpers, LookupCacheForgetsOnlyTheNamesChanged)
{
    const auto bucketOf = [](const char* name) { return HashedName::Hash(name) % LookupCache::Buckets; };
    // A name of another bucket than `a`'s, which would be forgotten with it
    std::string other = "b";
    while (bucketOf(other.c_str()) == bucketOf("a"))
    {
        other += "b";
    }
    InternalValueMap ext = { { "a", InternalValue(int64_t{ 1 }) }, { other, InternalValue(int64_t{ 2 }) } };
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    auto& cache = LookupCache::ForThisThread();
    context.SetLookupCache(&cache);

    const HashedName a{ "a", HashedName::Hash("a") };
    const HashedName b{ other, HashedName::Hash(other) };
    int keyA = 0;
    int keyB = 0;
    const auto slotA = LookupCache::NewSlot(a.hash);
    const auto slotB = LookupCache::NewSlot(b.hash);
    EXPECT_EQ(1, ConvertToInt(*context.FindValueCached(&keyA, slotA, a)));
    EXPECT_EQ(2, ConvertToInt(*context.FindValueCached(&keyB, slotB, b)));

    auto scope = context.EnterScope();
    scope.ForName(a) = InternalValue(int64_t{ 3 });
    EXPECT_EQ(nullptr, cache.KeyAt(slotA));
    EXPECT_EQ(&keyB, cache.KeyAt(slotB));
    EXPECT_EQ(3, ConvertToInt(*context.FindValueCached(&keyA, slotA, a)));
    EXPECT_EQ(2, ConvertToInt(*context.FindValueCached(&keyB, slotB, b)));

    // A value changed in place keeps every entry
    scope.ForName(a) = InternalValue(int64_t{ 4 });
    EXPECT_EQ(&keyA, cache.KeyAt(slotA));
    EXPECT_EQ(4, ConvertToInt(*context.FindValueCached(&keyA, slotA, a)));

    scope.Clear();
    EXPECT_EQ(nullptr, cache.KeyAt(slotA));
    EXPECT_EQ(&keyB, cache.KeyAt(slotB));
    EXPECT_EQ(1, ConvertToInt(*context.FindValueCached(&keyA, slotA, a)));
    context.ExitScope();
}
