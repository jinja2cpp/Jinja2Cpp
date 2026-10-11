#include "../src/internal_value.h"
#include "../src/value_visitors.h"

#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

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

TEST(InternalValueLayoutTest, TakeStringMovesOnlyTextNoOtherValueShares)
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
