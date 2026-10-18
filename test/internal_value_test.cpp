#include "../src/internal_value.h"
#include "../src/value_visitors.h"

#include "gtest/gtest.h"

#include <jinja2cpp/markup.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>

using namespace jinja2;

// The 32-byte value layout (docs/tasks/0140 P2): what it keeps in place, what it shares, and
// the rules a copy, a move and Markup follow across the representations of a string

TEST(InternalValueLayoutTest, RenderedStringsAroundTheInPlaceLimit)
{
    // 15 characters stay in the value, 16 go to a shared object: both print, concatenate,
    // slice and compare the same
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% set a = 'x' * 15 %}{% set b = a ~ 'y' %}{{ a|length }} {{ b|length }} {{ b[14:] }} {{ (a ~ 'y') == b }} {{ [a, b]|join('|')|length }}")
                    .has_value());
    EXPECT_EQ("15 16 xy True 32", tpl.RenderAsString({}).value());

    TemplateW wtpl;
    ASSERT_TRUE(wtpl.Load(L"{% set a = 'xyz' %}{% set b = a ~ 'w' %}{{ a }} {{ b }} {{ b|upper }} {{ b[1:] }}").has_value());
    EXPECT_EQ(L"xyz xyzw XYZW yzw", wtpl.RenderAsString({}).value());
}

TEST(InternalValueLayoutTest, MarkupOfHostStringsShortAndLongStrings)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% autoescape true %}{{ s|safe }} {{ l|safe }} {{ ('<' ~ 'b>')|safe }} {{ s }}{% endautoescape %}").has_value());
    ValuesMap params{ { "s", "<i>" }, { "l", "<i>a longer host string</i>" } };
    EXPECT_EQ("<i> <i>a longer host string</i> <b> &lt;i&gt;", tpl.RenderAsString(params).value());
}

// A user callable may keep its arguments: a long string built in the render reaches it as a
// string of its own, not a view of the render's value (docs/tasks/0151)
TEST(InternalValueLayoutTest, CallableKeepsBuiltStringArguments)
{
    Value kept;
    UserCallable keep;
    keep.callable = [&kept](auto& args) -> Value {
        kept = args["s"];
        return {};
    };
    keep.argsInfo = { { "s", true } };
    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ keep('a' * 20 ~ 'b') }}").has_value());
    ASSERT_TRUE(tpl.RenderAsString({ { "keep", keep } }).has_value());
    ASSERT_TRUE(kept.IsString());
    EXPECT_EQ(std::string(20, 'a') + "b", kept.AsString());
}

// Internal functions are not exported from a shared library
#ifndef JINJA2CPP_LINK_AS_SHARED
namespace
{
std::string Text(const InternalValue& value)
{
    auto text = AsStringView<char>(value);
    return text ? std::string(*text) : std::string("<not a string>");
}

InternalValue Item(const InternalValue& list, int64_t idx)
{
    auto items = AsList(list);
    return items ? items->GetValueByIndex(idx) : InternalValue();
}

std::string Taken(const std::optional<TargetString>& taken)
{
    const auto* text = taken ? std::get_if<std::string>(&*taken) : nullptr;
    return text ? *text : std::string("<not taken>");
}
} // namespace

TEST(InternalValueLayoutTest, SizeAndCopyProtocol)
{
    static_assert(sizeof(InternalValue) == 32);
    static_assert(std::is_trivially_copyable_v<detail::ValueStorage>);
    static_assert(std::is_nothrow_move_constructible_v<InternalValue>);

    InternalValue str(std::string(40, 'a'));
    InternalValue copy = str;
    EXPECT_EQ(std::string(40, 'a'), Text(copy));
    InternalValue moved = std::move(str);
    EXPECT_TRUE(str.IsUndefined()); // NOLINT(bugprone-use-after-move,clang-analyzer-cplusplus.Move): moved from values are undefined
    EXPECT_EQ(std::string(40, 'a'), Text(moved));

    // Assigning to a value an item of the list only that value owns
    InternalValue list = ListAdapter::CreateAdapter(InternalValueList{ InternalValue(std::string(20, 'b')), InternalValue(int64_t{ 1 }) });
    list = Item(list, 0);
    EXPECT_EQ(std::string(20, 'b'), Text(list));

    InternalValue& self = list;
    list = self;
    EXPECT_EQ(std::string(20, 'b'), Text(list));
}

TEST(InternalValueLayoutTest, MarkupIsAFlagOfStringsOnly)
{
    const std::string host = "<i>";
    for (auto value : { InternalValue(std::string("short")), InternalValue(std::string(30, 'l')), InternalValue(std::string_view(host)) })
    {
        EXPECT_FALSE(value.IsMarkup());
        value.SetMarkup();
        EXPECT_TRUE(value.IsMarkup());
        const InternalValue copy = value;
        EXPECT_TRUE(copy.IsMarkup());
        EXPECT_TRUE(IsStringValue(copy));
        value.SetMarkup(false);
        EXPECT_FALSE(value.IsMarkup());
    }
    InternalValue number(int64_t{ 5 });
    number.SetMarkup();
    EXPECT_FALSE(number.IsMarkup());
    EXPECT_EQ(5, ConvertToInt(number));
}

TEST(InternalValueLayoutTest, TakeStringCopiesTextNoOtherValueShares)
{
    const std::string text(30, 't');
    InternalValue shared(text);
    const InternalValue other = shared;
    EXPECT_EQ(text, Taken(TakeString(std::move(shared))));
    EXPECT_EQ(text, Text(other));

    EXPECT_EQ(text, Taken(TakeString(InternalValue(text))));
    EXPECT_EQ(text, Taken(TakeString(InternalValue(std::string_view(text)))));
    EXPECT_FALSE(TakeString(InternalValue(int64_t{ 1 })).has_value());
}

// docs/tasks/0151: a string value keeps its characters in its own object, written once
TEST(InternalValueLayoutTest, BuiltStringsAroundTheInPlaceLimit)
{
    for (size_t size : { size_t{ 0 }, size_t{ 1 }, size_t{ 15 }, size_t{ 16 }, size_t{ 600 } })
    {
        const std::string text(size, 'b');
        EXPECT_EQ(text, Text(InternalValue::FromText(std::string_view(text)))) << size;
        EXPECT_EQ(std::string(text).append("-").append(text), Text(InternalValue::FromText<char>({ text, "-", text }))) << size;
        EXPECT_EQ(text, Text(InternalValue::MakeText<char>(size, [](boost::span<char> chars) { std::fill(chars.begin(), chars.end(), 'b'); }))) << size;
        const std::wstring wide(size, L'w');
        const auto wideValue = InternalValue::FromText(std::wstring_view(wide));
        EXPECT_EQ(wide, std::wstring(AsStringView<wchar_t>(wideValue).value_or(L"<not a string>"))) << size;
    }
    // The text is copied, not borrowed
    std::string source(20, 's');
    auto value = InternalValue::FromText(std::string_view(source));
    source.assign(20, 'x');
    EXPECT_EQ(std::string(20, 's'), Text(value));
}

// A string a value owns is visited as a view of its object; a Value made of it gets a copy,
// so it outlives the value. Host text stays a view of the host's string
TEST(InternalValueLayoutTest, ValuesOfOwnedStringsCopyTheText)
{
    for (size_t size : { size_t{ 3 }, size_t{ 30 } })
    {
        Value owned = IntValue2Value(InternalValue(std::string(size, 'o')));
        ASSERT_TRUE(owned.IsString()) << size;
        EXPECT_EQ(std::string(size, 'o'), owned.AsString());
        Value wide = IntValue2Value(InternalValue(std::wstring(size, L'o')));
        ASSERT_TRUE(wide.IsWString()) << size;
        EXPECT_EQ(std::wstring(size, L'o'), wide.AsWString());
    }
    const std::string host(30, 'h');
    EXPECT_TRUE(std::holds_alternative<std::string_view>(IntValue2Value(InternalValue(std::string_view(host))).GetData()));
}

TEST(InternalValueLayoutTest, ParentDataKeepsTheValueItWasTakenFrom)
{
    InternalValue parent = ListAdapter::CreateAdapter(InternalValueList{ InternalValue(std::string(20, 'p')) });
    InternalValue item = Item(parent, 0);
    EXPECT_FALSE(item.ShouldExtendLifetime());
    item.SetParentData(parent);
    EXPECT_TRUE(item.ShouldExtendLifetime());
    EXPECT_EQ(std::string(20, 'p'), Text(item));

    InternalValue same = Item(parent, 0);
    same.SetParentData(parent);
    EXPECT_TRUE(item.IsEqual(same));
    EXPECT_FALSE(item.IsEqual(Item(parent, 0)));

    item.SetParentData(InternalValue());
    EXPECT_FALSE(item.ShouldExtendLifetime());
}

TEST(InternalValueLayoutTest, CopiesOfAGeneratorIterateApart)
{
    auto makeGenerator = [] {
        return InternalValue(ListAdapter::CreateAdapter([count = 0]() mutable -> std::optional<InternalValue> {
            if (count == 3)
            {
                return std::nullopt;
            }
            return InternalValue(int64_t{ count++ });
        }));
    };
    auto first = [](const InternalValue& list) {
        auto items = AsList(list);
        auto enumerator = items ? items->GetEnumerator() : std::nullopt;
        return enumerator && (*enumerator)->MoveNext() ? ConvertToInt((*enumerator)->GetCurrent()) : int64_t{ -1 };
    };

    for (const bool anchored : { false, true })
    {
        auto generator = makeGenerator();
        if (anchored)
        {
            generator.SetParentData(InternalValue(std::string(20, 'g')));
        }
        const InternalValue copy = generator;
        EXPECT_EQ(0, first(generator));
        EXPECT_EQ(0, first(copy));
        EXPECT_EQ(anchored, copy.ShouldExtendLifetime());
    }
}
#endif

namespace
{
// What Value2IntValue makes of a host value: a view of its text, or a copy
InternalValue ToInternal(const Value& val, bool byValue)
{
    return std::visit(visitors::InputValueConvertor(byValue, !byValue), val.GetData());
}
} // namespace

// A Markup of the host keeps its flag through InternalValue and back (docs/tasks/0180)
TEST(InternalValueLayoutTest, HostMarkupRoundTrips)
{
    for (const size_t size : { std::size_t{ 3 }, std::size_t{ 15 }, std::size_t{ 16 }, std::size_t{ 200 } })
    {
        const std::string text(size, '<');
        const Value markup{ Markup(text) };
        const Value wmarkup{ WMarkup(std::wstring(size, L'<')) };

        // Lent as a view of the host text, and converted by value
        for (const auto& internal : { ToInternal(markup, false), ToInternal(Value(markup), true) })
        {
            EXPECT_TRUE(internal.IsMarkup()) << size;
            const Value back = IntValue2Value(internal);
            EXPECT_TRUE(back.IsMarkup()) << size;
            EXPECT_EQ(text, back.AsString());
        }
        EXPECT_TRUE(IntValue2Value(ToInternal(wmarkup, false)).IsWMarkup()) << size;

        // A plain string stays plain (a view of host text stays a view)
        const Value plain{ text };
        EXPECT_FALSE(ToInternal(plain, false).IsMarkup()) << size;
        EXPECT_FALSE(IntValue2Value(ToInternal(plain, false)).IsMarkup()) << size;
        EXPECT_FALSE(IntValue2Value(ToInternal(Value(plain), true)).IsMarkup()) << size;

        // A template-built Markup (safe filter output, owned text) becomes a Markup value
        EXPECT_TRUE(IntValue2Value(InternalValue(std::string(text)).SetMarkup()).IsMarkup()) << size;
        EXPECT_FALSE(IntValue2Value(InternalValue(std::string(text))).IsMarkup()) << size;
        EXPECT_TRUE(IntValue2Value(InternalValue(std::string_view(text)).SetMarkup()).IsMarkup()) << size;
    }
}
