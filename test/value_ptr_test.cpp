// Local fixes in the vendored polymorphic_cxx14.h (docs/tasks/0089).
#include "jinja2cpp/polymorphic_value/polymorphic_cxx14.h"
#include "jinja2cpp/value_ptr.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <numeric>
#include <utility>
#include <vector>

namespace
{
struct Shape
{
    Shape() = default;
    Shape(const Shape&) = default;
    Shape& operator=(const Shape&) = default;
    virtual ~Shape() = default;
    [[nodiscard]] virtual int Area() const = 0;
};

struct Rect : Shape
{
    Rect(std::initializer_list<int> sides, int scale)
        : m_area(std::accumulate(sides.begin(), sides.end(), 1, [](int a, int b) { return a * b; }) * scale)
    {
    }
    [[nodiscard]] int Area() const override { return m_area; }
    int m_area;
};
} // namespace

// value_ptr.h put `using namespace xyz;` into jinja2::types, so user code that pulled in
// jinja2::types also saw xyz::detail and xyz::unreachable (docs/tasks/0069).
namespace detail
{
struct UserProbe
{
    int value = 7;
};
} // namespace detail

// The initializer-list in_place constructor built a T instead of the requested U, which does not
// even compile for an abstract T.
TEST(ValuePtrTest, InPlaceInitializerListBuildsDerivedType)
{
    xyz::polymorphic<Shape> shape(xyz::in_place_type_t<Rect>{}, { 2, 3 }, 2);
    EXPECT_EQ(12, shape->Area());
}

TEST(ValuePtrTest, CopyAndMoveKeepDerivedValue)
{
    xyz::polymorphic<Shape> shape(xyz::in_place_type_t<Rect>{}, { 4, 5 }, 1);
    xyz::polymorphic<Shape> copy(shape);
    EXPECT_EQ(20, copy->Area());
    xyz::polymorphic<Shape> moved(std::move(copy));
    EXPECT_EQ(20, moved->Area());
    EXPECT_EQ(20, shape->Area());

    std::vector<xyz::polymorphic<Shape>> shapes(3, shape);
    EXPECT_EQ(20, shapes[2]->Area());
}

TEST(ValuePtrTest, TypesNamespaceDoesNotLeakVendoredNames)
{
    using namespace jinja2::types;
    // Ambiguous with xyz::detail if jinja2::types re-exported the vendored namespace
    const detail::UserProbe probe;
    EXPECT_EQ(7, probe.value);

    const ValuePtr<Rect> rect = MakeValuePtr<Rect>(std::initializer_list<int>{ 3, 3 }, 1);
    EXPECT_EQ(9, rect->Area());
    const ValuePtr<Shape> inPlace(in_place_type_t<Rect>{}, { 2, 2 }, 1);
    EXPECT_EQ(4, inPlace->Area());
}
