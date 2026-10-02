#include "../src/ordered_map.h"

#include "test_tools.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <string>
#include <vector>

using jinja2::OrderedMap;
using Map = OrderedMap<std::string, int>;

namespace
{
std::vector<std::string> Keys(const Map& map)
{
    std::vector<std::string> result;
    for (const auto& item : map)
        result.push_back(item.first);
    return result;
}

using Keys_t = std::vector<std::string>;

// Every entry must be reachable through the index, and the index must not see stale keys
void ExpectConsistent(const Map& map)
{
    for (const auto& item : map)
    {
        auto p = map.find(item.first);
        ASSERT_NE(map.end(), p) << item.first;
        EXPECT_EQ(&*p, &item) << item.first;
    }
}
} // namespace

TEST(OrderedMapTest, IteratesInInsertionOrder)
{
    Map map{ { "b", 1 }, { "a", 2 } };
    map["C"] = 3;
    map.emplace("z", 4);
    map.insert({ "m", 5 });
    map.try_emplace("y", 6);
    EXPECT_EQ((Keys_t{ "b", "a", "C", "z", "m", "y" }), Keys(map));
    EXPECT_EQ(6u, map.size());
    ExpectConsistent(map);
}

TEST(OrderedMapTest, ExistingKeyKeepsPosition)
{
    Map map{ { "b", 1 }, { "a", 2 }, { "b", 3 } };
    EXPECT_EQ((Keys_t{ "b", "a" }), Keys(map));
    EXPECT_EQ(1, map["b"]); // like std::unordered_map, construction keeps the first value

    map["b"] = 4;
    EXPECT_FALSE(map.emplace("a", 5).second);
    EXPECT_FALSE(map.insert_or_assign("a", 6).second);
    EXPECT_EQ((Keys_t{ "b", "a" }), Keys(map));
    EXPECT_EQ(4, map.at("b"));
    EXPECT_EQ(6, map.at("a"));
}

TEST(OrderedMapTest, EraseKeepsOrderOfOthers)
{
    Map map{ { "a", 1 }, { "b", 2 }, { "c", 3 }, { "d", 4 } };
    EXPECT_EQ(1u, map.erase("b"));
    EXPECT_EQ(0u, map.erase("b"));
    EXPECT_EQ((Keys_t{ "a", "c", "d" }), Keys(map));

    auto next = map.erase(map.find("a"));
    EXPECT_EQ("c", next->first);
    map["b"] = 5; // re-inserted keys go to the end, as in a Python dict
    EXPECT_EQ((Keys_t{ "c", "d", "b" }), Keys(map));
    ExpectConsistent(map);

    map.erase(map.begin(), map.end());
    EXPECT_TRUE(map.empty());
    EXPECT_EQ(map.end(), map.find("c"));
}

TEST(OrderedMapTest, LookupAndAt)
{
    const Map map{ { "a", 1 } };
    EXPECT_EQ(1u, map.count("a"));
    EXPECT_TRUE(map.contains("a"));
    EXPECT_FALSE(map.contains("b"));
    EXPECT_EQ(map.end(), map.find("b"));
    EXPECT_THROW(map.at("b"), std::out_of_range);
    auto range = map.equal_range("a");
    EXPECT_EQ(1, std::distance(range.first, range.second));
}

TEST(OrderedMapTest, CopyRebuildsIndex)
{
    Map src{ { "b", 1 }, { "a", 2 } };
    Map copy(src);
    src.erase("b");
    src["a"] = 10;
    EXPECT_EQ((Keys_t{ "b", "a" }), Keys(copy));
    EXPECT_EQ(2, copy.at("a"));
    ExpectConsistent(copy);

    Map assigned;
    assigned = copy;
    copy.clear();
    EXPECT_EQ((Keys_t{ "b", "a" }), Keys(assigned));
    ExpectConsistent(assigned);

    const Map& self = assigned;
    assigned = self;
    EXPECT_EQ((Keys_t{ "b", "a" }), Keys(assigned));
    ExpectConsistent(assigned);
}

TEST(OrderedMapTest, MoveAndSwapKeepEntries)
{
    Map src{ { "b", 1 }, { "a", 2 } };
    const int* value = &src.at("a");
    Map moved(std::move(src));
    EXPECT_EQ(value, &moved.at("a"));
    EXPECT_EQ((Keys_t{ "b", "a" }), Keys(moved));
    ExpectConsistent(moved);

    Map other{ { "x", 3 } };
    other = std::move(moved);
    EXPECT_EQ(value, &other.at("a"));
    ExpectConsistent(other);

    Map third{ { "y", 4 } };
    swap(other, third);
    EXPECT_EQ((Keys_t{ "y" }), Keys(other));
    EXPECT_EQ((Keys_t{ "b", "a" }), Keys(third));
    ExpectConsistent(other);
    ExpectConsistent(third);
}

TEST(OrderedMapTest, ReferencesSurviveGrowth)
{
    Map map;
    map["first"] = 1;
    int* first = &map["first"];
    for (int n = 0; n < 1000; ++n)
        map[std::to_string(n)] = n;
    EXPECT_EQ(first, &map["first"]);
    EXPECT_EQ("first", map.begin()->first);
    ExpectConsistent(map);
}

// Maps above the linear-search threshold go through the index; check it across erase,
// re-insertion and copies for sizes on both sides of it
TEST(OrderedMapTest, IndexedAndLinearAgree)
{
    for (int size : { 1, 7, 8, 9, 10, 17, 100 })
    {
        Map map;
        Keys_t expected;
        for (int n = size; n > 0; --n)
        {
            map["k" + std::to_string(n)] = n;
            expected.push_back("k" + std::to_string(n));
        }
        ExpectConsistent(map);
        for (int n = 1; n <= size; n += 2)
        {
            EXPECT_EQ(1u, map.erase("k" + std::to_string(n))) << size;
            expected.erase(std::find(expected.begin(), expected.end(), "k" + std::to_string(n)));
        }
        EXPECT_EQ(expected, Keys(map)) << size;
        ExpectConsistent(map);
        for (int n = 1; n <= size; n += 2)
        {
            map["k" + std::to_string(n)] = n;
            expected.push_back("k" + std::to_string(n));
        }
        EXPECT_EQ(expected, Keys(map)) << size;
        ExpectConsistent(map);

        Map copy(map);
        EXPECT_EQ(expected, Keys(copy)) << size;
        EXPECT_TRUE(copy == map);
        ExpectConsistent(copy);
        for (auto& key : expected)
            EXPECT_EQ(1u, copy.erase(key)) << size;
        EXPECT_TRUE(copy.empty());
        EXPECT_EQ(copy.end(), copy.find(expected.front()));
    }
}

TEST(OrderedMapTest, EqualityIgnoresOrder)
{
    Map lhs{ { "a", 1 }, { "b", 2 } };
    Map rhs{ { "b", 2 }, { "a", 1 } };
    EXPECT_TRUE(lhs == rhs);
    rhs["a"] = 3;
    EXPECT_TRUE(lhs != rhs);
    rhs.erase("a");
    EXPECT_TRUE(lhs != rhs);
}

// Rendering: dict literals and kwargs follow source order, pprint and tojson sort keys
// (docs/tasks/0031). The corpus pins the same behaviour against Python where it can; dictsort
// is blocked there by 0019.
class OrderedMappingTest : public BasicTemplateRenderer
{
protected:
    static void PerformBothTests(const std::string& tpl, const std::string& result)
    {
        ExecuteTest<jinja2::Template>(tpl, result, {}, "Narrow version");
        ExecuteTest<jinja2::TemplateW>(jinja2::ConvertString<std::wstring>(tpl), jinja2::ConvertString<std::wstring>(result), {}, "Wide version");
    }
};

TEST_F(OrderedMappingTest, DictLiteralIteratesInSourceOrder)
{
    std::string source = "{% for k in {'b': 1, 'a': 2, 'C': 3} %}{{ k }}{% endfor %}";
    PerformBothTests(source, "baC");
}

TEST_F(OrderedMappingTest, MacroKwargsKeepCallOrder)
{
    std::string source = "{% macro m() %}{% for k in kwargs %}{{ k }}{% endfor %}{% endmacro %}{{ m(z=1, a=2, m=3) }}";
    PerformBothTests(source, "zam");
}

// More than 16 entries: below that libstdc++'s std::sort is an insertion sort and so stable too
TEST_F(OrderedMappingTest, DictsortIsStable)
{
    std::string dict = "{";
    std::string zeros;
    std::string ones;
    for (int n = 23; n >= 0; --n)
    {
        auto key = std::string(n < 10 ? "x0" : "x") + std::to_string(n);
        dict += "'" + key + "': " + std::to_string(n % 2) + (n ? ", " : "}");
        (n % 2 ? ones : zeros) += key + ",";
    }
    std::string loop = "{% for p in d | dictsort(by='value'ARGS) %}{{ p['key'] }},{% endfor %}";
    std::string source = "{% set d = " + dict + " %}" + loop + "|" + loop;
    source.replace(source.find("ARGS"), 4, "");
    source.replace(source.find("ARGS"), 4, ", reverse=true");
    PerformBothTests(source, zeros + ones + "|" + ones + zeros);
}

TEST_F(OrderedMappingTest, PprintAndTojsonSortKeys)
{
    std::string source = "{{ {'b': 1, 'a': 2, 'C': 3} | pprint }}|{{ {'b': 1, 'a': 2, 'C': 3} | tojson }}";
    PerformBothTests(source, "{'C': 3, 'a': 2, 'b': 1}|{\"C\": 3, \"a\": 2, \"b\": 1}");
}
