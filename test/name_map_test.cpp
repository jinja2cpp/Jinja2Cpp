#include "../src/hash_map.h"
#include "../src/internal_value.h"
#include "../src/value_visitors.h"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

using namespace jinja2;

// The transparent try_emplace that scopes insert names through with the hash the parse tree
// stored (0118 P5b-3b), and the value stability the lookup cache relies on (0158, 0231)

namespace
{
HashedName Name(std::string_view name)
{
    return { name, HashedName::Hash(name) };
}

// Every name hashes alike, so that each insert and lookup walks past the other keys
struct CollidingHash
{
    using is_transparent = void;
    std::size_t operator()(const std::string& /*name*/) const noexcept { return 42; }
    std::size_t operator()(const HashedName& /*name*/) const noexcept { return 42; }
};
using CollidingMap = detail::StableMap<std::string, int, CollidingHash, NameEqual>;
} // namespace

TEST(NameMapTest, NewNameIsInsertedUnderItsString)
{
    InternalValueMap map;
    const auto [it, isAdded] = map.try_emplace(Name("a_long_variable_name"));
    ASSERT_TRUE(isAdded);
    EXPECT_EQ("a_long_variable_name", it->first);
    it->second = InternalValue(int64_t{ 1 });
    // Found again by the string, which hashes as the stored hash does
    const auto found = map.find(std::string("a_long_variable_name"));
    ASSERT_NE(map.end(), found);
    EXPECT_EQ(1, ConvertToInt(found->second));
}

TEST(NameMapTest, ExistingNameIsFoundNotInserted)
{
    InternalValueMap map;
    map[std::string("x")] = InternalValue(int64_t{ 5 });
    const auto [it, isAdded] = map.try_emplace(Name("x"));
    EXPECT_FALSE(isAdded);
    EXPECT_EQ(5, ConvertToInt(it->second));
    EXPECT_EQ(1U, map.size());
}

TEST(NameMapTest, CollidingNamesStayApart)
{
    CollidingMap map;
    map.try_emplace(Name("first")).first->second = 1;
    const auto [second, isSecondAdded] = map.try_emplace(Name("second"));
    ASSERT_TRUE(isSecondAdded);
    second->second = 2;
    const auto [again, isAgainAdded] = map.try_emplace(Name("first"));
    EXPECT_FALSE(isAgainAdded);
    EXPECT_EQ(1, again->second);
    EXPECT_EQ(2, map.find(std::string("second"))->second);
    EXPECT_EQ(2U, map.size());
    // Many more, so that the table grows and rehashes the colliding keys
    for (int idx = 0; idx != 100; ++idx)
    {
        const auto name = "name" + std::to_string(idx);
        EXPECT_TRUE(map.try_emplace(Name(name)).second) << name;
    }
    EXPECT_EQ(1, map.find(std::string("first"))->second);
    EXPECT_EQ(102U, map.size());
}

TEST(NameMapTest, ValuesStayInPlaceWhileNamesAreAdded)
{
    InternalValueMap map;
    auto& first = map.try_emplace(Name("first")).first->second;
    first = InternalValue(int64_t{ 7 });
    const auto* where = &first;
    // Past several segments and rehashes
    for (int idx = 0; idx != 1000; ++idx)
    {
        map.try_emplace(Name("name" + std::to_string(idx)));
    }
    EXPECT_EQ(where, &map.find(std::string("first"))->second);
    EXPECT_EQ(7, ConvertToInt(*where));
}

TEST(NameMapTest, EraseMovesOnlyTheLastValue)
{
    InternalValueMap map;
    for (int idx = 0; idx != 10; ++idx)
    {
        map.try_emplace(Name("name" + std::to_string(idx))).first->second = InternalValue(int64_t{ idx });
    }
    const auto* kept = &map.find(std::string("name3"))->second;
    EXPECT_EQ("name9", map.values().back().first);
    map.erase(std::string("name5"));
    // name9 took name5's place; the others did not move
    EXPECT_EQ(kept, &map.find(std::string("name3"))->second);
    EXPECT_EQ(9, ConvertToInt(map.find(std::string("name9"))->second));
    EXPECT_EQ(map.end(), map.find(std::string("name5")));
    EXPECT_EQ(9U, map.size());
}
