#include "../src/intrusive_ptr.h"
#include "../src/small_vector.h"
#include "../src/span.h"

#include "gtest/gtest.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

// The library's own replacements for Boost utility types (docs/tasks/0230)

using jinja2::IntrusivePtr;
using jinja2::Span;

namespace
{
static_assert(std::is_trivially_copyable_v<Span<int>>);
static_assert(sizeof(Span<int>) == 2 * sizeof(void*));
static_assert(std::is_convertible_v<Span<int>, Span<const int>>);
static_assert(!std::is_convertible_v<Span<const int>, Span<int>>);
static_assert(std::is_convertible_v<std::vector<int>&, Span<int>>);
static_assert(std::is_convertible_v<const std::vector<int>&, Span<const int>>);
static_assert(!std::is_convertible_v<const std::vector<int>&, Span<int>>);
// A span of mutable elements does not view a temporary
static_assert(!std::is_constructible_v<Span<int>, std::vector<int>&&>);
static_assert(std::is_constructible_v<Span<const int>, std::vector<int>&&>);
// Elements convert by qualification only: no derived-to-base slicing
struct Base
{
};
struct Derived : Base
{
    int extra = 0;
};
static_assert(!std::is_constructible_v<Span<Base>, std::vector<Derived>&>);
static_assert(!std::is_constructible_v<Span<Base>, Span<Derived>>);

TEST(SpanTest, ViewsContainers)
{
    std::vector<int> vec{ 1, 2, 3, 4, 5 };
    const Span<int> span = vec;
    EXPECT_EQ(span.data(), vec.data());
    EXPECT_EQ(span.size(), 5U);
    EXPECT_EQ(span.size_bytes(), 5 * sizeof(int));
    EXPECT_FALSE(span.empty());
    EXPECT_EQ(span.front(), 1);
    EXPECT_EQ(span.back(), 5);
    span[2] = 30;
    EXPECT_EQ(vec[2], 30);

    std::array<int, 3> arr{ 7, 8, 9 };
    const Span<const int> fromArray = arr;
    EXPECT_EQ(fromArray.size(), 3U);
    EXPECT_EQ(fromArray[1], 8);

    int carr[] = { 4, 5 }; // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
    const Span fromCArray = carr;
    static_assert(std::is_same_v<decltype(fromCArray), const Span<int>>);
    EXPECT_EQ(fromCArray.size(), 2U);

    const std::string str = "abc";
    const Span fromString = str;
    static_assert(std::is_same_v<decltype(fromString), const Span<const char>>);
    EXPECT_EQ(fromString.size(), 3U);

    const Span<int> empty;
    EXPECT_TRUE(empty.empty());
    EXPECT_EQ(empty.data(), nullptr);
    EXPECT_EQ(empty.begin(), empty.end());
}

TEST(SpanTest, Subviews)
{
    std::vector<int> vec{ 1, 2, 3, 4, 5 };
    const Span<int> span(vec.data(), vec.size());
    EXPECT_EQ(std::vector<int>(span.first(2).begin(), span.first(2).end()), (std::vector<int>{ 1, 2 }));
    EXPECT_EQ(std::vector<int>(span.last(2).begin(), span.last(2).end()), (std::vector<int>{ 4, 5 }));
    EXPECT_EQ(std::vector<int>(span.subspan(1, 3).begin(), span.subspan(1, 3).end()), (std::vector<int>{ 2, 3, 4 }));
    EXPECT_EQ(std::vector<int>(span.subspan(3).begin(), span.subspan(3).end()), (std::vector<int>{ 4, 5 }));
    EXPECT_TRUE(span.subspan(5).empty());
    EXPECT_TRUE(span.first(0).empty());
    EXPECT_EQ(std::vector<int>(span.rbegin(), span.rend()), (std::vector<int>{ 5, 4, 3, 2, 1 }));
    const Span<int> range(vec.data() + 1, vec.data() + 3);
    EXPECT_EQ(range.size(), 2U);
    EXPECT_EQ(range[0], 2);
}

struct Counted
{
    virtual ~Counted() = default;
    mutable int refs = 0;
    int* destroyed = nullptr;

    // NOLINTNEXTLINE(readability-identifier-naming)
    friend void intrusive_ptr_add_ref(const Counted* obj) noexcept { ++obj->refs; }
    // NOLINTNEXTLINE(readability-identifier-naming)
    friend void intrusive_ptr_release(const Counted* obj) noexcept
    {
        if (--obj->refs == 0)
        {
            ++*obj->destroyed;
            delete obj; // NOLINT(cppcoreguidelines-owning-memory)
        }
    }
};

struct CountedChild : Counted
{
};

IntrusivePtr<Counted> MakeCounted(int& destroyed)
{
    auto* obj = new CountedChild; // NOLINT(cppcoreguidelines-owning-memory): owned by the pointer
    obj->destroyed = &destroyed;
    return IntrusivePtr<Counted>(obj);
}

static_assert(sizeof(IntrusivePtr<Counted>) == sizeof(void*));
static_assert(std::is_nothrow_move_constructible_v<IntrusivePtr<Counted>>);
static_assert(std::is_convertible_v<IntrusivePtr<CountedChild>, IntrusivePtr<const Counted>>);
static_assert(!std::is_convertible_v<IntrusivePtr<Counted>, IntrusivePtr<CountedChild>>);
static_assert(!std::is_convertible_v<Counted*, IntrusivePtr<Counted>>);

TEST(IntrusivePtrTest, CountsReferences)
{
    int destroyed = 0;
    {
        auto ptr = MakeCounted(destroyed);
        EXPECT_EQ(ptr->refs, 1);
        {
            const auto copy = ptr; // NOLINT(performance-unnecessary-copy-initialization): counts the copy
            EXPECT_EQ(ptr->refs, 2);
            EXPECT_EQ(copy, ptr);
            const IntrusivePtr<const Counted> toConst = ptr;
            EXPECT_EQ(ptr->refs, 3);
            EXPECT_EQ(toConst.get(), ptr.get());
        }
        EXPECT_EQ(ptr->refs, 1);

        auto moved = std::move(ptr);
        EXPECT_EQ(ptr, nullptr); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move): a moved-from pointer is null
        EXPECT_EQ(moved->refs, 1);

        IntrusivePtr<Counted> other;
        EXPECT_FALSE(other);
        other = moved;
        EXPECT_EQ(moved->refs, 2);
        other = other; // NOLINT(misc-redundant-expression): self-assignment keeps the object
        EXPECT_EQ(moved->refs, 2);
        other = nullptr;
        EXPECT_EQ(moved->refs, 1);
        EXPECT_EQ(destroyed, 0);

        Counted* raw = moved.detach();
        EXPECT_FALSE(moved);
        EXPECT_EQ(raw->refs, 1);
        other.reset(raw, false);
        EXPECT_EQ(other->refs, 1);
        swap(other, moved);
        EXPECT_FALSE(other);
        EXPECT_TRUE(moved);
    }
    EXPECT_EQ(destroyed, 1);

    auto ptr = MakeCounted(destroyed);
    ptr.reset();
    EXPECT_EQ(destroyed, 2);
}
using jinja2::SmallVector;
using jinja2::SmallVectorBase;
using jinja2::SmallVectorDoubling;

static_assert(sizeof(SmallVectorBase<int>) == 3 * sizeof(void*));
static_assert(sizeof(SmallVector<void*, 2>) == 5 * sizeof(void*));
static_assert(std::is_nothrow_move_constructible_v<SmallVector<std::string, 2>>);

bool IsInside(const void* ptr, const void* object, std::size_t size)
{
    const auto addr = reinterpret_cast<std::uintptr_t>(ptr);     // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    const auto first = reinterpret_cast<std::uintptr_t>(object); // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
    return addr >= first && addr < first + size;
}

template<typename V>
bool IsInlineIn(const V& vec)
{
    return IsInside(vec.data(), &vec, sizeof(vec)); // NOLINT(clang-analyzer-cplusplus.Move): also asked of moved-from vectors
}

TEST(SmallVectorTest, GrowsPastInlineBuffer)
{
    SmallVector<std::string, 2> vec;
    EXPECT_TRUE(vec.empty());
    EXPECT_EQ(vec.capacity(), 2U);
    vec.push_back("a");
    vec.emplace_back("b");
    EXPECT_TRUE(IsInlineIn(vec));
    // The argument names an element that the growth moves
    vec.push_back(vec[0]);
    EXPECT_FALSE(IsInlineIn(vec));
    EXPECT_EQ(vec.capacity(), 3U);
    vec.emplace_back(vec.back());
    EXPECT_EQ(vec.capacity(), 4U);
    EXPECT_EQ(vec, (SmallVector<std::string, 2>{ "a", "b", "a", "a" }));
    vec.pop_back();
    EXPECT_EQ(vec.size(), 3U);
    EXPECT_EQ(vec.front(), "a");
    EXPECT_EQ(vec.back(), "a");
    EXPECT_THROW((void)vec.at(3), std::out_of_range);
}

TEST(SmallVectorTest, GrowthPolicies)
{
    SmallVector<int, 5> slow;
    SmallVector<int, 5, SmallVectorDoubling> doubling;
    std::vector<std::size_t> slowCaps;
    std::vector<std::size_t> doublingCaps;
    for (int i = 0; i != 40; ++i)
    {
        slow.push_back(i);
        doubling.push_back(i);
        if (slowCaps.empty() || slowCaps.back() != slow.capacity())
        {
            slowCaps.push_back(slow.capacity());
        }
        if (doublingCaps.empty() || doublingCaps.back() != doubling.capacity())
        {
            doublingCaps.push_back(doubling.capacity());
        }
    }
    EXPECT_EQ(slowCaps, (std::vector<std::size_t>{ 5, 8, 12, 19, 30, 48 }));
    EXPECT_EQ(doublingCaps, (std::vector<std::size_t>{ 5, 10, 20, 40 }));
}

TEST(SmallVectorTest, InsertEraseResize)
{
    SmallVector<int, 4> vec{ 1, 2, 3 };
    vec.insert(vec.begin() + 1, 10);
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 1, 10, 2, 3 }));
    vec.insert(vec.begin(), vec[3]);
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 3, 1, 10, 2, 3 }));
    const std::vector<int> more{ 7, 8 };
    vec.insert(vec.end() - 1, more.begin(), more.end());
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 3, 1, 10, 2, 7, 8, 3 }));
    vec.insert(vec.begin(), 2, vec[2]);
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 10, 10, 3, 1, 10, 2, 7, 8, 3 }));
    auto* it = vec.erase(vec.begin() + 1, vec.begin() + 4);
    EXPECT_EQ(*it, 10);
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 10, 10, 2, 7, 8, 3 }));
    vec.erase(vec.begin());
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 10, 2, 7, 8, 3 }));
    vec.resize(2);
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 10, 2 }));
    vec.resize(4);
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 10, 2, 0, 0 }));
    vec.resize(6, 5);
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 10, 2, 0, 0, 5, 5 }));
    EXPECT_EQ(std::vector<int>(vec.rbegin(), vec.rend()), (std::vector<int>{ 5, 5, 0, 0, 2, 10 }));
    vec.clear();
    EXPECT_TRUE(vec.empty());
    vec.reserve(20);
    EXPECT_GE(vec.capacity(), 20U);
    vec.assign(3, 9);
    EXPECT_EQ(vec, (SmallVector<int, 4>{ 9, 9, 9 }));
}

TEST(SmallVectorTest, CopyAndMove)
{
    SmallVector<std::string, 2> small{ "x" };
    SmallVector<std::string, 2> big{ "a", "b", "c" };
    const auto* bigData = big.data();

    SmallVector<std::string, 2> copy(big);
    EXPECT_EQ(copy, big);
    copy = small;
    EXPECT_EQ(copy, small);

    // A heap buffer moves over whole and the source gets its inline buffer back
    SmallVector<std::string, 2> moved(std::move(big));
    EXPECT_EQ(moved.data(), bigData);
    EXPECT_TRUE(big.empty()); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move): moved-from state is specified
    EXPECT_TRUE(IsInlineIn(big));
    EXPECT_EQ(big.capacity(), 2U);
    big.push_back("again");
    EXPECT_TRUE(IsInlineIn(big));

    // Inline elements move one by one
    SmallVector<std::string, 2> movedSmall(std::move(small));
    EXPECT_EQ(movedSmall, (SmallVector<std::string, 2>{ "x" }));
    EXPECT_TRUE(IsInlineIn(movedSmall));

    moved = std::move(movedSmall);
    EXPECT_EQ(moved, (SmallVector<std::string, 2>{ "x" }));
    movedSmall = std::move(copy);
    EXPECT_EQ(movedSmall, (SmallVector<std::string, 2>{ "x" }));
}

TEST(SmallVectorTest, ShrinkToFit)
{
    SmallVector<int, 3> vec{ 1, 2, 3, 4, 5 };
    EXPECT_FALSE(IsInlineIn(vec));
    vec.resize(4);
    vec.shrink_to_fit();
    EXPECT_FALSE(IsInlineIn(vec));
    EXPECT_EQ(vec.capacity(), 4U);
    vec.resize(2);
    vec.shrink_to_fit();
    EXPECT_TRUE(IsInlineIn(vec));
    EXPECT_EQ(vec.capacity(), 3U);
    EXPECT_EQ(vec, (SmallVector<int, 3>{ 1, 2 }));
    vec.clear();
    vec.shrink_to_fit();
    EXPECT_TRUE(IsInlineIn(vec));
}

void AppendThrough(SmallVectorBase<int>& vec, int count)
{
    for (int i = 0; i != count; ++i)
    {
        vec.push_back(i);
    }
}

TEST(SmallVectorTest, CapacityErasedBase)
{
    SmallVector<int, 2> two;
    SmallVector<int, 8> eight;
    AppendThrough(two, 5);
    AppendThrough(eight, 5);
    EXPECT_TRUE(IsInlineIn(eight));
    EXPECT_FALSE(IsInlineIn(two));
    EXPECT_TRUE(std::equal(two.begin(), two.end(), eight.begin(), eight.end()));
    SmallVectorBase<int>& base = eight;
    base = std::move(static_cast<SmallVectorBase<int>&>(two));
    EXPECT_EQ(eight.size(), 5U);
    EXPECT_TRUE(two.empty()); // NOLINT(bugprone-use-after-move, clang-analyzer-cplusplus.Move)
    two.push_back(1);
    EXPECT_EQ(two.size(), 1U);
}

// Copies throw on the third one: a failed growth leaves the vector as it was
struct ThrowingCopy
{
    static inline int copies = 0;
    int value = 0;
    explicit ThrowingCopy(int v)
        : value(v)
    {
    }
    ThrowingCopy(const ThrowingCopy& other)
        : value(other.value)
    {
        if (++copies == 3)
        {
            throw std::runtime_error("copy");
        }
    }
    ThrowingCopy& operator=(const ThrowingCopy&) = default;
    ~ThrowingCopy() = default;
};

TEST(SmallVectorTest, StrongGuaranteeOnGrowth)
{
    SmallVector<ThrowingCopy, 2> vec;
    vec.emplace_back(1);
    vec.emplace_back(2);
    ThrowingCopy::copies = 0;
    const ThrowingCopy extra(3);
    EXPECT_THROW(vec.push_back(extra), std::runtime_error);
    ASSERT_EQ(vec.size(), 2U);
    EXPECT_EQ(vec[0].value, 1);
    EXPECT_EQ(vec[1].value, 2);
    EXPECT_TRUE(IsInlineIn(vec));
}

} // namespace
