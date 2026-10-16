#include "gtest/gtest.h"

#include "../src/helpers.h"
#include "../src/expression_evaluator.h"
#include "../src/internal_value.h"
#include "../src/lookup_result.h"
#include "../src/render_context.h"
#include "../src/value_visitors.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <cstddef>
#include <cstdint>
#include <new>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

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

// Narrow strings are UTF-8 and wide ones UTF-16/UTF-32 whatever the C locale; the test
// binary never calls setlocale, so it runs in the "C" locale (docs/tasks/0035)
TEST(Helpers, ConvertStringUnicodeWithoutLocale)
{
    const std::string utf8("\x41\xC3\xA9\xE2\x98\x83\0\xF0\x9F\x98\x80\xC3\xA9", 13);
    const std::wstring wide(L"A\u00E9\u2603\0\U0001F600\u00E9", sizeof(wchar_t) == 2 ? 7 : 6);
    EXPECT_EQ(utf8, ConvertString<std::string>(wide));
    EXPECT_EQ(wide, ConvertString<std::wstring>(utf8));
    EXPECT_EQ(utf8, ToString(ToWString(utf8)));
}

// Each ill-formed UTF-8 sequence becomes one U+FFFD per maximal subpart, as in Python's
// bytes.decode('utf-8', 'replace'); the rest of the string converts
TEST(Helpers, ConvertStringReplacesMalformedUtf8)
{
    const std::pair<std::string, std::wstring> cases[] = {
        { "a\x80-", L"a\uFFFD-" },                           // stray continuation byte
        { "a\xC3", L"a\uFFFD" },                             // truncated at the end
        { "\xE2\x98x", L"\uFFFDx" },                         // truncated three-byte sequence: one U+FFFD
        { "\xC0\xAF", L"\uFFFD\uFFFD" },                     // overlong lead
        { "\xE0\x80\x80", L"\uFFFD\uFFFD\uFFFD" },           // overlong three-byte form
        { "\xED\xA0\x80", L"\uFFFD\uFFFD\uFFFD" },           // encoded surrogate
        { "\xF4\x90\x80\x80", L"\uFFFD\uFFFD\uFFFD\uFFFD" }, // above U+10FFFF
        { "\xF5\xFF", L"\uFFFD\uFFFD" },                     // bytes that never occur
        { "\xF0\x9F\x98\xC3\xA9", L"\uFFFD\u00E9" },
    };
    for (const auto& [narrow, expected] : cases)
    {
        EXPECT_EQ(expected, ConvertString<std::wstring>(narrow)) << narrow;
    }
}

// A lone surrogate, or a wchar_t value outside Unicode, becomes U+FFFD in UTF-8
TEST(Helpers, ConvertStringReplacesMalformedWide)
{
    std::wstring wide{ L'a', static_cast<wchar_t>(0xD800), L'-', static_cast<wchar_t>(0xDC00) };
    if constexpr (sizeof(wchar_t) == 4)
    {
        for (const uint32_t unit : { 0x110000u, 0xFFFFFFFFu })
        {
            wide.push_back(static_cast<wchar_t>(unit));
        }
        EXPECT_EQ("a\xEF\xBF\xBD-\xEF\xBF\xBD\xEF\xBF\xBD\xEF\xBF\xBD", ConvertString<std::string>(wide));
    }
    else
    {
        EXPECT_EQ("a\xEF\xBF\xBD-\xEF\xBF\xBD", ConvertString<std::string>(wide));
    }
}

// Values of the other width render with their text, in the "C" locale (docs/tasks/0035)
TEST(Helpers, RendersValuesOfTheOtherWidth)
{
    const std::string expected = "h\xC3\xA9llo w\xC3\xB6rld|11|h\xC3\xA9ll\xE2\x80\xA6|{'\xC3\xA9': '\xD0\xB6'}";
    const std::string tpl = "{{ u }}|{{ u|length }}|{{ u|truncate(5, true, '\xE2\x80\xA6') }}|{{ d }}";

    TemplateW wideTpl;
    ASSERT_TRUE(wideTpl.Load(ConvertString<std::wstring>(tpl)));
    const ValuesMap narrowValues{ { "u", std::string("h\xC3\xA9llo w\xC3\xB6rld") }, { "d", ValuesMap{ { "\xC3\xA9", std::string("\xD0\xB6") } } } };
    const auto wideOut = wideTpl.RenderAsString(narrowValues);
    ASSERT_TRUE(wideOut);
    EXPECT_EQ(expected, ConvertString<std::string>(wideOut.value()));

    Template narrowTpl;
    ASSERT_TRUE(narrowTpl.Load(tpl));
    const ValuesMap wideValues{ { "u", std::wstring(L"h\u00E9llo w\u00F6rld") }, { "d", ValuesMap{ { "\xC3\xA9", std::wstring(L"\u0436") } } } };
    const auto narrowOut = narrowTpl.RenderAsString(wideValues);
    ASSERT_TRUE(narrowOut);
    EXPECT_EQ(expected, narrowOut.value());
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

// A scope a closure records gets a serial of its own, past the first chunk too; clearing the
// scope keeps it, and a scope pushed again at its index has none until stamped, then another
// one (docs/tasks/0165)
TEST(Helpers, ScopeSerialsAreStampedOnceAndSurviveClear)
{
    ScopeStack scopes;
    std::vector<uint32_t> serials;
    for (size_t idx = 0; idx != ScopeStack::ChunkSize + 2; ++idx)
    {
        scopes.Push();
        EXPECT_EQ(0u, scopes.Serial(idx));
        serials.push_back(scopes.Stamp(idx));
        EXPECT_EQ(serials.back(), scopes.Stamp(idx));
    }
    for (size_t idx = 1; idx != serials.size(); ++idx)
    {
        EXPECT_LT(serials[idx - 1], serials[idx]);
    }
    const auto top = scopes.size() - 1;
    scopes.back()["a"] = InternalValue(int64_t{ 1 });
    scopes.back().clear();
    EXPECT_EQ(serials[top], scopes.Serial(top));
    scopes.pop_back();
    scopes.Push();
    EXPECT_EQ(0u, scopes.Serial(top));
    EXPECT_GT(scopes.Stamp(top), serials[top]);
    scopes.pop_back();
    scopes.pop_back();
    scopes.pop_back();
    scopes.Push();
    EXPECT_NE(serials[ScopeStack::ChunkSize - 1], scopes.Stamp(ScopeStack::ChunkSize - 1));
    // A copy is another stack, whose scopes are not stamped
    const ScopeStack copy(scopes);
    EXPECT_EQ(0u, copy.Serial(0));
}

namespace
{
int64_t IntAt(const LookupResult& result)
{
    return result ? ConvertToInt(*result) : -1;
}
} // namespace

// A closure body's scope links below the scopes its caller pushed: lookups, writes and
// children pass over them (docs/tasks/0165)
TEST(Helpers, LinkedViewHidesTheScopesBetween)
{
    InternalValueMap ext = { { "y", InternalValue(int64_t{ 0 }) } };
    const InternalValueMap globals;
    RenderContext context(ext, globals, nullptr);
    context.GetCurrentScope()["x"] = InternalValue(int64_t{ 1 });
    const auto& rootScope = static_cast<const RenderContext&>(context).GetCurrentScope();
    auto callerScope = context.EnterScope();
    callerScope["x"] = InternalValue(int64_t{ 2 });
    callerScope["y"] = InternalValue(int64_t{ 2 });
    auto body = context.EnterScope();
    context.PushFrameView({}, 1);
    body["z"] = InternalValue(int64_t{ 3 });

    EXPECT_EQ(1, IntAt(context.FindValue(std::string("x"))));
    EXPECT_EQ(0, IntAt(context.FindValue(std::string("y"))));
    EXPECT_EQ(3, IntAt(context.FindValue(HashedName{ "z", HashedName::Hash("z") })));

    auto target = context.FindForWrite("x");
    ASSERT_TRUE(target);
    *target = InternalValue(int64_t{ 5 });
    EXPECT_EQ(5, ConvertToInt(rootScope.at("x")));
    auto written = context.FindForWrite("y");
    ASSERT_TRUE(written);
    *written = InternalValue(int64_t{ 7 });
    EXPECT_EQ(2, ConvertToInt(callerScope.Map().at("y")));

    {
        // A child of the body's context sees through the link as well
        RenderContext child(context, context.GetScopesCount());
        EXPECT_EQ(5, IntAt(child.FindValue(std::string("x"))));
        EXPECT_EQ(3, IntAt(child.FindValue(std::string("z"))));
        EXPECT_EQ(7, IntAt(child.FindValue(std::string("y"))));
    }

    context.PopFrameView();
    context.ExitScope();
    EXPECT_EQ(2, IntAt(context.FindValue(std::string("x"))));
    context.ExitScope();
    EXPECT_EQ(5, IntAt(context.FindValue(std::string("x"))));
}

// A link from a child's scope down into the scopes it sees of its parent
TEST(Helpers, LinkedViewReachesIntoTheParent)
{
    const InternalValueMap ext;
    const InternalValueMap globals;
    RenderContext parent(ext, globals, nullptr);
    parent.GetCurrentScope()["x"] = InternalValue(int64_t{ 1 });
    parent.EnterScope()["x"] = InternalValue(int64_t{ 2 });
    {
        RenderContext child(parent, parent.GetScopesCount());
        child.GetCurrentScope()["x"] = InternalValue(int64_t{ 3 });
        EXPECT_EQ(3, IntAt(child.FindValue(std::string("x"))));
        child.EnterScope();
        child.PushFrameView({}, 1);
        EXPECT_EQ(1, IntAt(child.FindValue(std::string("x"))));
        EXPECT_EQ(1, ConvertToInt(*child.FindForWrite("x")));
        child.PopFrameView();
        child.ExitScope();
        EXPECT_EQ(3, IntAt(child.FindValue(std::string("x"))));
    }
    parent.ExitScope();
}
