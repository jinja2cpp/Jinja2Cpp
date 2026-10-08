#include "../src/internal_value.h"
#include "../src/robin_hood.h"
#include "../src/value_visitors.h"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>

using namespace jinja2;

// try_emplace_transparent, the local addition to the vendored robin_hood.h that scopes insert
// names through with the hash the parse tree stored (0118 P5b-3b)

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
using CollidingMap = robin_hood::unordered_map<std::string, int, CollidingHash, NameEqual>;
} // namespace

TEST(NameMapTest, NewNameIsInsertedUnderItsString)
{
    InternalValueMap map;
    const auto [it, isAdded] = map.try_emplace_transparent(Name("a_long_variable_name"));
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
    const auto [it, isAdded] = map.try_emplace_transparent(Name("x"));
    EXPECT_FALSE(isAdded);
    EXPECT_EQ(5, ConvertToInt(it->second));
    EXPECT_EQ(1U, map.size());
}

TEST(NameMapTest, CollidingNamesStayApart)
{
    CollidingMap map;
    map.try_emplace_transparent(Name("first")).first->second = 1;
    const auto [second, isSecondAdded] = map.try_emplace_transparent(Name("second"));
    ASSERT_TRUE(isSecondAdded);
    second->second = 2;
    const auto [again, isAgainAdded] = map.try_emplace_transparent(Name("first"));
    EXPECT_FALSE(isAgainAdded);
    EXPECT_EQ(1, again->second);
    EXPECT_EQ(2, map.find(std::string("second"))->second);
    EXPECT_EQ(2U, map.size());
    // Many more, so that the table grows and rehashes the colliding keys
    for (int idx = 0; idx != 100; ++idx)
    {
        const auto name = "name" + std::to_string(idx);
        EXPECT_TRUE(map.try_emplace_transparent(Name(name)).second) << name;
    }
    EXPECT_EQ(1, map.find(std::string("first"))->second);
    EXPECT_EQ(102U, map.size());
}
