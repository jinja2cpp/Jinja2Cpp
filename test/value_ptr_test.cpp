// Local fixes in the vendored polymorphic_cxx14.h (docs/tasks/0089).

// A user's own copy of jbcoe/value_types, included first: before docs/tasks/0076 the vendored
// copy shared its include guards and its global namespace xyz, so it was silently skipped
// and the library's headers compiled against the user's types.
#define XYZ_INDIRECT_H
#define XYZ_POLYMORPHIC_H_
#define XYZ_IN_PLACE_DEFINED
#define XYZ_IN_PLACE_TYPE_DEFINED
namespace xyz
{
struct in_place_t
{
    int userField;
};
template<class T>
struct in_place_type_t
{
    int userField;
};
} // namespace xyz

#include "jinja2cpp/polymorphic_value/polymorphic_cxx14.h"
#include "jinja2cpp/value_ptr.h"

#include <gtest/gtest.h>

#include <initializer_list>
#include <numeric>
#include <type_traits>
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
    jinja2::types::polymorphic<Shape> shape(jinja2::types::in_place_type_t<Rect>{}, { 2, 3 }, 2);
    EXPECT_EQ(12, shape->Area());
}

TEST(ValuePtrTest, CopyAndMoveKeepDerivedValue)
{
    jinja2::types::polymorphic<Shape> shape(jinja2::types::in_place_type_t<Rect>{}, { 4, 5 }, 1);
    jinja2::types::polymorphic<Shape> copy(shape);
    EXPECT_EQ(20, copy->Area());
    jinja2::types::polymorphic<Shape> moved(std::move(copy));
    EXPECT_EQ(20, moved->Area());
    EXPECT_EQ(20, shape->Area());

    std::vector<jinja2::types::polymorphic<Shape>> shapes(3, shape);
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

TEST(ValuePtrTest, UserCopyOfValueTypesStaysSeparate)
{
    const xyz::in_place_t userTag{ 5 };
    EXPECT_EQ(5, userTag.userField);
    EXPECT_FALSE((std::is_same_v<xyz::in_place_t, jinja2::detail::in_place_t>));
}
