#include "../src/internal_value.h"
#include "../src/template_dict.h"
#include "../src/value_visitors.h"

#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <variant>
#include <vector>

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

// Dict keys follow Python's equality (docs/tasks/0036): the token is the same for keys that are
// one key
TEST(TemplateDictTest, KeyTokensFollowPythonEquality)
{
    const auto token = [](const InternalValue& key) { return MakeDictKey(key); };
    const auto tuple = [](InternalValueList items) { return InternalValue(ListAdapter::CreateAdapter(std::move(items)).MarkAsTuple()); };
    const InternalValue one(int64_t{ 1 });

    ASSERT_TRUE(token(one));
    EXPECT_EQ(token(one), token(InternalValue(1.0)));
    EXPECT_EQ(token(one), token(InternalValue(true)));
    EXPECT_NE(token(one), token(InternalValue(1.5)));
    EXPECT_NE(token(one), token(InternalValue(int64_t{ 2 })));
    EXPECT_EQ(token(InternalValue(int64_t{ 0 })), token(InternalValue(-0.0)));
    EXPECT_EQ(token(InternalValue(false)), token(InternalValue(0.0)));
    // A float beyond int64 is still a float
    EXPECT_NE(token(InternalValue(9223372036854775808.0)), token(InternalValue(std::numeric_limits<int64_t>::max())));

    EXPECT_EQ(token(tuple({ one, InternalValue(2.0) })), token(tuple({ InternalValue(1.0), InternalValue(int64_t{ 2 }) })));
    EXPECT_NE(token(tuple({ one, InternalValue(int64_t{ 2 }) })), token(tuple({ InternalValue(int64_t{ 2 }), one })));
    // (1, 2) is not the nested ((1), 2) nor the longer (1, 2, 3)
    EXPECT_NE(token(tuple({ one, InternalValue(int64_t{ 2 }) })), token(tuple({ one, InternalValue(int64_t{ 2 }), one })));
    EXPECT_EQ(token(tuple({ tuple({ one }), InternalValue(std::string("a")) })), token(tuple({ tuple({ InternalValue(1.0) }), InternalValue(std::wstring(L"a")) })));

    EXPECT_EQ(token(InternalValue(std::string("a"))), token(InternalValue(std::wstring(L"a"))));
    EXPECT_EQ(std::optional<std::string>("a"), token(InternalValue(std::string("a"))));
    // A string that spells the token of the integer 1 is another key
    const auto oneToken = token(one);
    ASSERT_TRUE(oneToken.has_value());
    EXPECT_NE(oneToken, token(InternalValue(oneToken.value_or(std::string()))));
    EXPECT_NE(token(InternalValue(NoneValue{})), token(InternalValue(std::string("N"))));

    const auto range = [](int64_t start, int64_t stop, int64_t step) { return InternalValue(ListAdapter::CreateRange(start, stop, step)); };
    // Equal ranges are equal keys: range(0, 3, 2) and range(0, 4, 2) are both [0, 2]
    EXPECT_EQ(token(range(0, 3, 2)), token(range(0, 4, 2)));
    EXPECT_EQ(token(range(5, 5, 1)), token(range(0, 0, 3)));
    EXPECT_NE(token(range(0, 3, 1)), token(range(0, 4, 2)));
}

TEST(TemplateDictTest, UnhashableKeys)
{
    const char* type = nullptr;
    const InternalValue list(ListAdapter::CreateAdapter(InternalValueList{ InternalValue(int64_t{ 1 }) }));
    EXPECT_FALSE(MakeDictKey(list, &type));
    EXPECT_STREQ("list", type);

    const InternalValue tupleWithList(ListAdapter::CreateAdapter(InternalValueList{ InternalValue(int64_t{ 1 }), list }).MarkAsTuple());
    type = nullptr;
    EXPECT_FALSE(MakeDictKey(tupleWithList, &type));
    EXPECT_STREQ("list", type);

    const InternalValue dict(CreateMapAdapter(TemplateDict()));
    type = nullptr;
    EXPECT_FALSE(MakeDictKey(dict, &type));
    EXPECT_STREQ("dict", type);

    TemplateDict target;
    EXPECT_THROW(target.Set(list, InternalValue(int64_t{ 1 })), std::runtime_error);
    EXPECT_TRUE(target.empty());
    EXPECT_EQ(target.end(), target.Find(list));
}

TEST(TemplateDictTest, FirstKeyAndPositionKeptLastValueWins)
{
    TemplateDict dict;
    dict.Set(InternalValue(int64_t{ 1 }), InternalValue(std::string("a")));
    dict.Set(InternalValue(std::string("x")), InternalValue(std::string("b")));
    dict.Set(InternalValue(1.0), InternalValue(std::string("c")));
    dict.Set(InternalValue(true), InternalValue(std::string("d")));

    ASSERT_EQ(2U, dict.size());
    EXPECT_TRUE(dict.HasKeyValues());
    auto first = dict.begin();
    EXPECT_EQ(ValueKind::Int, dict.KeyOf(first->first).Kind());
    EXPECT_EQ("d", AsString(first->second));
    EXPECT_EQ(ValueKind::String, dict.KeyOf(std::next(first)->first).Kind());

    auto found = dict.Find(InternalValue(1.0));
    ASSERT_NE(dict.end(), found);
    EXPECT_EQ("d", AsString(found->second));
    dict.Erase(found);
    EXPECT_FALSE(dict.HasKeyValues());
    EXPECT_EQ(1U, dict.size());

    // A copy has keys of its own
    dict.Set(InternalValue(2.5), InternalValue(int64_t{ 1 }));
    TemplateDict copy(dict);
    copy.Set(InternalValue(2.5), InternalValue(int64_t{ 2 }));
    copy.Set(InternalValue(3.5), InternalValue(int64_t{ 3 }));
    EXPECT_EQ(2U, dict.size());
    EXPECT_EQ(3U, copy.size());
    const auto key = dict.KeyOf(dict.Find(InternalValue(2.5))->first);
    ASSERT_NE(nullptr, GetIf<double>(&key));
    EXPECT_EQ(2.5, *GetIf<double>(&key));
    EXPECT_FALSE(dict == copy);
}

TEST(TemplateDictTest, StringKeysStartingWithTheMarkAreEscaped)
{
    TemplateDict dict;
    const std::string odd = std::string(1, DictKeyMark) + "n12345678";
    dict.SetString(odd, InternalValue(int64_t{ 7 }));
    dict.Set(InternalValue(int64_t{ 1 }), InternalValue(int64_t{ 8 }));

    ASSERT_EQ(2U, dict.size());
    ASSERT_NE(dict.end(), dict.FindString(odd));
    EXPECT_EQ(7, ConvertToInt(dict.FindString(odd)->second));
    ASSERT_NE(dict.end(), dict.Find(InternalValue(odd)));
    // Read back as the string it was
    EXPECT_EQ(odd, AsString(dict.KeyOf(dict.begin()->first)));
    EXPECT_EQ(ValueKind::Int, dict.KeyOf(std::next(dict.begin())->first).Kind());
}

TEST(TemplateDictTest, PublicMapSpellsKeys)
{
    TemplateDict dict;
    dict.Set(InternalValue(int64_t{ 1 }), InternalValue(std::string("x")));
    dict.SetString("b", InternalValue(int64_t{ 2 }));
    dict.Set(InternalValue(ListAdapter::CreateAdapter(InternalValueList{ InternalValue(int64_t{ 1 }), InternalValue(int64_t{ 2 }) }).MarkAsTuple()), InternalValue(int64_t{ 3 }));
    dict.Set(InternalValue(std::string("1")), InternalValue(std::string("y")));

    const auto map = CreateMapAdapter(std::move(dict));
    const auto value = IntValue2Value(InternalValue(map));
    const auto generic = value.ToGenericMap();
    ASSERT_NE(nullptr, generic.GetAccessor());
    // Later value wins at the position of the first spelling
    EXPECT_EQ((std::vector<std::string>{ "1", "b", "(1, 2)" }), generic.GetKeys());
    EXPECT_EQ("y", generic.GetValueByName("1").ToString());
    EXPECT_EQ(3U, generic.GetSize());
    // A copy: later changes of the template's dict do not reach it
    map.GetMutableItems()->SetString("c", InternalValue(int64_t{ 4 }));
    EXPECT_EQ(3U, generic.GetSize());

    // A dict of string keys stays a live view
    TemplateDict strings;
    strings.SetString("a", InternalValue(int64_t{ 1 }));
    const auto stringMap = CreateMapAdapter(std::move(strings));
    const auto stringGeneric = IntValue2Value(InternalValue(stringMap)).ToGenericMap();
    EXPECT_EQ(1U, stringGeneric.GetSize());
    stringMap.GetMutableItems()->SetString("b", InternalValue(int64_t{ 2 }));
    EXPECT_EQ(2U, stringGeneric.GetSize());
}

TEST(TemplateDictTest, DictsCompareAsPythonDicts)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{{ {1: 'a', (1, 2): 'b'} == {(1.0, 2): 'b', True: 'a'} }}|{{ {1: 'a'} == {'1': 'a'} }}|{{ 1 in {True: 0} }}").has_value());
    EXPECT_EQ("True|False|True", tpl.RenderAsString({}).value());
}
#endif
