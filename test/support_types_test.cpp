#include "../src/intrusive_ptr.h"
#include "../src/span.h"

#include "gtest/gtest.h"

#include <array>
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
} // namespace
