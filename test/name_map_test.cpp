#include "../src/hash_map.h"
#include "../src/internal_value.h"
#include "../src/value_visitors.h"

#include "gtest/gtest.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

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

// The list of a map's segments starts in the tail of its first segment and moves to an array of
// its own once it outgrows that tail (docs/tasks/0506): the values stay where they are, the
// iteration order is the insertion order, and an iterator survives a move or a swap of the map,
// as it did while the list was a std::vector
TEST(NameMapTest, SegmentListOutgrowsTheFirstSegment)
{
    InternalValueMap map;
    std::vector<const InternalValue*> where;
    // Past the tail of the first segment and several doublings of the list after it
    for (int idx = 0; idx != 2000; ++idx)
    {
        auto& value = map.try_emplace(Name("name" + std::to_string(idx))).first->second;
        value = InternalValue(int64_t{ idx });
        where.push_back(&value);
    }
    for (int idx = 0; idx != 2000; ++idx)
    {
        const auto found = map.find(std::string("name") + std::to_string(idx));
        ASSERT_NE(map.end(), found);
        EXPECT_EQ(where[static_cast<size_t>(idx)], &found->second);
        EXPECT_EQ(idx, ConvertToInt(map.values()[static_cast<size_t>(idx)].second));
    }

    const auto first = map.begin();
    InternalValueMap moved(std::move(map));
    EXPECT_EQ("name0", first->first);
    InternalValueMap other;
    other.try_emplace(Name("other"));
    const auto otherFirst = other.begin();
    moved.swap(other);
    EXPECT_EQ("name0", first->first);
    EXPECT_EQ("other", otherFirst->first);
    EXPECT_EQ(2000U, other.size());
    EXPECT_EQ(1U, moved.size());
}

// Emptied and filled again, the map reuses its segments, and a map shrunk to a few values moves
// its list back into its first segment (shrink_to_fit, rehash(0))
TEST(NameMapTest, SegmentListShrinksBackIntoTheFirstSegment)
{
    InternalValueMap map;
    for (int idx = 0; idx != 500; ++idx)
    {
        map.try_emplace(Name("name" + std::to_string(idx)));
    }
    map.clear();
    EXPECT_TRUE(detail::HasStorage(map));
    for (int idx = 0; idx != 3; ++idx)
    {
        map.try_emplace(Name("kept" + std::to_string(idx))).first->second = InternalValue(int64_t{ idx });
    }
    map.rehash(0);
    EXPECT_EQ(3U, map.size());
    for (int idx = 0; idx != 3; ++idx)
    {
        EXPECT_EQ(idx, ConvertToInt(map.find(std::string("kept") + std::to_string(idx))->second));
    }
    // And grows again from there
    for (int idx = 0; idx != 500; ++idx)
    {
        map.try_emplace(Name("again" + std::to_string(idx)));
    }
    EXPECT_EQ(503U, map.size());
    const InternalValueMap copy(map);
    EXPECT_EQ(503U, copy.size());
    EXPECT_NE(copy.end(), copy.find(std::string("again499")));
    map = InternalValueMap();
    EXPECT_FALSE(detail::HasStorage(map));
}

// The name hash is keyed per process: the key stays put while the process runs, and the hash
// is not unordered_dense's unkeyed one, whose groups names can be chosen offline to crowd
// (security review of #891)
TEST(NameMapTest, NameHashIsKeyedPerProcess)
{
    EXPECT_EQ(HashedName::Hash("name"), HashedName::Hash("name"));
    const std::string_view name = "name";
    EXPECT_EQ(HashedName::Hash(name), HashedName::Hash(std::string(name)));
    EXPECT_NE(HashedName::Hash(name), static_cast<std::size_t>(detail::ankerl::unordered_dense::detail::hash_bytes(name.data(), name.size())));
}

// Past 144 bytes hash_bytes chains its lanes, and these two identifiers steer one to zero: the
// unkeyed hash drops the eight bytes they differ in, whatever key is applied to it after. Long
// names are hashed in pieces of 144 bytes chained through the key (security review of #891)
TEST(NameMapTest, LongNamesThatCollideUnkeyedHashApart)
{
    const std::string a = "uVgtkYqDRLAFZmngABCDEFGHIJKLMNOPQRSTUVWXYZabcdefaaaaaaaawQjjBIAJtail_tai"
                          "l_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_";
    const std::string b = "uVgtkYqDRLAFZmngABCDEFGHIJKLMNOPQRSTUVWXYZabcdefbaaaaaaawQjjBIAJtail_tai"
                          "l_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_tail_";
    ASSERT_EQ(detail::ankerl::unordered_dense::detail::hash_bytes(a.data(), a.size()),
              detail::ankerl::unordered_dense::detail::hash_bytes(b.data(), b.size()));
    EXPECT_NE(HashedName::Hash(a), HashedName::Hash(b));
    // A name of exactly one piece and one just past it hash as names of their own
    const std::string piece(144, 'x');
    EXPECT_NE(HashedName::Hash(piece), HashedName::Hash(piece + "x"));
}
