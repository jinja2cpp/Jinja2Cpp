// StableSortValues (src/stable_sort_values.h): a stable sort that stays inside its range whatever
// the comparator answers. Host objects bring their own ordering (task 0381), and libstdc++'s
// std::stable_sort writes below the range when a comparator is inconsistent.
#include "../src/stable_sort_values.h"

#include <jinja2cpp/template.h>

#include <gtest/gtest.h>

#include <algorithm>
#include <cstddef>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
using Item = std::pair<int, std::string>;

std::vector<Item> MakeItems(std::size_t count, std::mt19937& rng)
{
    std::uniform_int_distribution<int> keys(0, 7);
    std::vector<Item> items;
    items.reserve(count);
    for (std::size_t idx = 0; idx < count; ++idx)
    {
        items.emplace_back(keys(rng), "item" + std::to_string(idx));
    }
    return items;
}

std::vector<std::string> SortedNames(std::vector<Item> items)
{
    std::vector<std::string> names;
    names.reserve(items.size());
    for (auto& item : items)
    {
        names.push_back(std::move(item.second));
    }
    std::sort(names.begin(), names.end());
    return names;
}
} // namespace

TEST(StableSortValuesTest, MatchesStdStableSortWithTies)
{
    std::mt19937 rng(381); // NOLINT(bugprone-random-generator-seed): a fixed sample, the same in every run
    const auto byKey = [](const Item& left, const Item& right) { return left.first < right.first; };
    for (std::size_t count : std::vector<std::size_t>{ 0, 1, 2, 3, 15, 16, 17, 31, 32, 33, 47, 64, 100, 257, 1000 })
    {
        auto items = MakeItems(count, rng);
        auto expected = items;
        std::stable_sort(expected.begin(), expected.end(), byKey);
        jinja2::StableSortValues(items.begin(), items.end(), byKey);
        EXPECT_EQ(expected, items) << count;

        // A reversed order keeps ties in their original order, as sort(reverse=true) does
        auto reversed = expected;
        std::stable_sort(reversed.begin(), reversed.end(), [](const Item& left, const Item& right) { return left.first > right.first; });
        jinja2::StableSortValues(items.begin(), items.end(), [](const Item& left, const Item& right) { return left.first > right.first; });
        EXPECT_EQ(reversed, items) << count;
    }
}

// Every answer is wrong for some pair; the sort must still keep each item exactly once and stay
// inside the range (ASan and JINJA2CPP_DEBUG_CONTAINERS check the accesses)
TEST(StableSortValuesTest, InconsistentComparatorsStayInsideTheRange)
{
    std::mt19937 rng(1); // NOLINT(bugprone-random-generator-seed): a fixed sample, the same in every run
    for (std::size_t count : std::vector<std::size_t>{ 2, 5, 16, 17, 40, 100, 1000 })
    {
        const auto items = MakeItems(count, rng);
        const auto names = SortedNames(items);

        auto always = items;
        jinja2::StableSortValues(always.begin(), always.end(), [](const Item&, const Item&) { return true; });
        EXPECT_EQ(names, SortedNames(always)) << count;

        auto alternating = items;
        bool answer = false;
        jinja2::StableSortValues(alternating.begin(), alternating.end(), [&answer](const Item&, const Item&) { return answer = !answer; });
        EXPECT_EQ(names, SortedNames(alternating)) << count;

        // Mostly "less": std::stable_sort's unguarded insertion overflows its buffer on this
        std::bernoulli_distribution coin(0.9);
        for (int trial = 0; trial < 20; ++trial)
        {
            auto random = items;
            jinja2::StableSortValues(random.begin(), random.end(), [&rng, &coin](const Item&, const Item&) { return coin(rng); });
            EXPECT_EQ(names, SortedNames(random)) << count;
        }
    }
}

TEST(StableSortValuesTest, ThrowingComparatorLeavesAPermutation)
{
    std::mt19937 rng(2); // NOLINT(bugprone-random-generator-seed): a fixed sample, the same in every run
    for (std::size_t count : std::vector<std::size_t>{ 3, 16, 17, 40, 200 })
    {
        const auto items = MakeItems(count, rng);
        const auto names = SortedNames(items);
        // Throw at every comparison count in turn, covering the insertion runs and the merges
        for (int limit = 0; limit < static_cast<int>(count) * 8; limit += 3)
        {
            auto sorted = items;
            int calls = 0;
            const auto comp = [&calls, limit](const Item& left, const Item& right) {
                if (calls++ == limit)
                {
                    throw std::runtime_error("unorderable");
                }
                return left.first < right.first;
            };
            try
            {
                jinja2::StableSortValues(sorted.begin(), sorted.end(), comp);
            }
            catch (const std::runtime_error&)
            {
            }
            EXPECT_EQ(names, SortedNames(sorted)) << count << ' ' << limit;
        }
    }
}

// Ties across more than one insertion run: the merge keeps the original order, also reversed
TEST(StableSortValuesTest, SortFilterIsStableAcrossRuns)
{
    std::string list = "[";
    for (int idx = 0; idx < 40; ++idx)
    {
        list += (idx ? "," : "") + std::string("(") + std::to_string(idx * 7 % 5) + ",'" + static_cast<char>('a' + idx % 26) + std::to_string(idx) + "')";
    }
    list += "]";
    const auto render = [&list](const std::string& args) {
        jinja2::Template tpl;
        EXPECT_TRUE(tpl.Load("{{ " + list + "|sort(" + args + ")|map(attribute='1')|join(',') }}"));
        return tpl.RenderAsString({}).value();
    };
    // Python Jinja2 3.1.6
    EXPECT_EQ("a0,f5,k10,p15,u20,z25,e30,j35,d3,i8,n13,s18,x23,c28,h33,m38,b1,g6,l11,q16,v21,a26,f31,k36,"
              "e4,j9,o14,t19,y24,d29,i34,n39,c2,h7,m12,r17,w22,b27,g32,l37",
              render("attribute='0'"));
    EXPECT_EQ("c2,h7,m12,r17,w22,b27,g32,l37,e4,j9,o14,t19,y24,d29,i34,n39,b1,g6,l11,q16,v21,a26,f31,k36,"
              "d3,i8,n13,s18,x23,c28,h33,m38,a0,f5,k10,p15,u20,z25,e30,j35",
              render("attribute='0', reverse=true"));
}
