// jinja2::DestroyDeep and jinja2::DeepCopy: host values nested deeper than the stack allows
// (docs/tasks/0449)

#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace
{
using jinja2::Value;
using jinja2::ValuesList;
using jinja2::ValuesMap;

// Deep enough to overflow an 8 MB stack through Value's recursive destructor, under ASan too
constexpr std::size_t VeryDeep = 200000;

// A value `depth` levels deep around the leaf 42, alternating lists and maps when `mixed`, built
// without recursion
Value Nested(std::size_t depth, bool mixed)
{
    Value value = 42;
    for (std::size_t level = 0; level < depth; ++level)
    {
        if (mixed && level % 2 == 1)
        {
            ValuesMap map;
            map.emplace("next", std::move(value));
            map.emplace("level", static_cast<int64_t>(level));
            value = Value(std::move(map));
        }
        else
        {
            ValuesList list;
            list.emplace_back(static_cast<int64_t>(level));
            list.push_back(std::move(value));
            value = Value(std::move(list));
        }
    }
    return value;
}

// The nesting depth of a value built by Nested, walked without recursion; -1 where it is not
// the shape Nested builds
std::ptrdiff_t DepthOf(const Value& value, bool mixed)
{
    const Value* current = &value;
    std::ptrdiff_t depth = 0;
    while (!current->IsInt() || current->AsInt() != 42)
    {
        if (const auto* list = current->GetIf<ValuesList>())
        {
            if (list->size() != 2 || !(*list)[0].IsInt())
            {
                return -1;
            }
            current = &(*list)[1];
        }
        else if (const auto* map = current->GetIf<ValuesMap>())
        {
            if (!mixed || map->size() != 2 || map->begin()->first != "next")
            {
                return -1;
            }
            current = &map->at("next");
        }
        else
        {
            return -1;
        }
        ++depth;
    }
    return depth;
}
} // namespace

TEST(ValueDeepTest, DestroyDeepReleasesAVeryDeepList)
{
    Value value = Nested(VeryDeep, false);
    jinja2::DestroyDeep(std::move(value));
    EXPECT_TRUE(value.IsNone()); // NOLINT(bugprone-use-after-move): DestroyDeep leaves it none
}

TEST(ValueDeepTest, DestroyDeepReleasesAVeryDeepMixedValue)
{
    Value value = Nested(VeryDeep, true);
    jinja2::DestroyDeep(std::move(value));
    EXPECT_TRUE(value.IsNone()); // NOLINT(bugprone-use-after-move)
}

TEST(ValueDeepTest, DestroyDeepTakesScalarsEmptyAndMovedFromValues)
{
    Value text = std::string("text");
    jinja2::DestroyDeep(std::move(text));
    EXPECT_TRUE(text.IsNone()); // NOLINT(bugprone-use-after-move)

    Value list = ValuesList{};
    jinja2::DestroyDeep(std::move(list));
    EXPECT_TRUE(list.IsNone()); // NOLINT(bugprone-use-after-move)

    Value source = ValuesList{ 1, 2 };
    Value taken = std::move(source);
    jinja2::DestroyDeep(std::move(source)); // NOLINT(bugprone-use-after-move): a moved-from wrapper
    EXPECT_EQ(taken, Value(ValuesList{ 1, 2 }));
}

TEST(ValueDeepTest, DeepCopyCopiesAVeryDeepValueWithoutALimit)
{
    for (const bool mixed : { false, true })
    {
        Value value = Nested(VeryDeep, mixed);
        auto copy = jinja2::DeepCopy(value, std::numeric_limits<std::size_t>::max());
        ASSERT_TRUE(copy.has_value());
        EXPECT_EQ(DepthOf(copy.value(), mixed), static_cast<std::ptrdiff_t>(VeryDeep));
        jinja2::DestroyDeep(std::move(copy).value());
        // The source is untouched
        EXPECT_EQ(DepthOf(value, mixed), static_cast<std::ptrdiff_t>(VeryDeep));
        jinja2::DestroyDeep(std::move(value));
    }
}

TEST(ValueDeepTest, DeepCopyRefusesPastMaxDepth)
{
    Value value = Nested(VeryDeep, true);
    EXPECT_FALSE(jinja2::DeepCopy(value).has_value());
    jinja2::DestroyDeep(std::move(value));

    for (const bool mixed : { false, true })
    {
        Value shallow = Nested(jinja2::ValueMaxSafeDepth, mixed);
        auto copy = jinja2::DeepCopy(shallow);
        ASSERT_TRUE(copy.has_value());
        EXPECT_EQ(DepthOf(copy.value(), mixed), static_cast<std::ptrdiff_t>(jinja2::ValueMaxSafeDepth));
        EXPECT_FALSE(jinja2::DeepCopy(shallow, jinja2::ValueMaxSafeDepth - 1).has_value());
    }
}

TEST(ValueDeepTest, DeepCopyCountsListAndMapLevels)
{
    const Value scalar = 1;
    EXPECT_EQ(jinja2::DeepCopy(scalar, 0), std::optional<Value>(scalar));
    const Value empty = ValuesList{};
    EXPECT_FALSE(jinja2::DeepCopy(empty, 0).has_value());
    EXPECT_EQ(jinja2::DeepCopy(empty, 1), std::optional<Value>(empty));
    const Value nested = ValuesList{ ValuesMap{ { "a", ValuesList{} } } };
    EXPECT_FALSE(jinja2::DeepCopy(nested, 2).has_value());
    EXPECT_EQ(jinja2::DeepCopy(nested, 3), std::optional<Value>(nested));
}

TEST(ValueDeepTest, DeepCopyKeepsTheOrderOfAnIndexedMapAndMovedFromItems)
{
    // Enough keys for OrderedMap to index them, inserted out of order, each holding a list
    ValuesMap map;
    std::string expected;
    for (int idx = 0; idx < 40; ++idx)
    {
        const std::string key = std::to_string((idx * 17) % 40);
        expected += key + ",";
        map.emplace(key, ValuesList{ idx, ValuesList{ key } });
    }
    ValuesList list{ 1, ValuesList{ 2 } };
    Value taken = std::move(list[1]);
    map.emplace("moved", std::move(list));
    expected += "moved,";
    const Value value = std::move(map);

    const auto result = jinja2::DeepCopy(value);
    ASSERT_TRUE(result.has_value());
    const Value& copy = result.value();
    EXPECT_EQ(copy, value);
    std::string keys;
    for (const auto& [key, item] : copy.AsMap())
    {
        keys += key + ",";
    }
    EXPECT_EQ(keys, expected);
    EXPECT_EQ(taken, Value(ValuesList{ 2 }));
}

TEST(ValueDeepTest, DeepCopyMatchesTheCopyConstructor)
{
    static constexpr std::string_view Text = "view";
    const Value value = ValuesMap{
        { "z", ValuesList{ 1, 2.5, "s", ValuesMap{ { "inner", ValuesList{ true, Value() } } } } },
        { "a", Text },
        { "m", ValuesMap{ { "y", 1 }, { "b", ValuesList{} }, { "k", ValuesMap{} } } },
        { "w", std::wstring(L"wide") },
    };
    const auto result = jinja2::DeepCopy(value);
    ASSERT_TRUE(result.has_value());
    const Value& copy = result.value();
    EXPECT_EQ(copy, value);

    // The order of the keys is kept at every level
    const auto& map = copy.AsMap();
    std::string keys;
    for (const auto& [key, item] : map)
    {
        keys += key;
    }
    EXPECT_EQ(keys, "zamw");
    std::string inner;
    for (const auto& [key, item] : map.at("m").AsMap())
    {
        inner += key;
    }
    EXPECT_EQ(inner, "ybk");

    // A string view stays a view of the same text, as with the copy constructor
    const auto* view = map.at("a").GetIf<std::string_view>();
    ASSERT_NE(view, nullptr);
    EXPECT_EQ(view->data(), Text.data());

    // The copy owns its containers
    EXPECT_NE(&copy.AsMap().at("z").AsList(), &value.AsMap().at("z").AsList());
}
