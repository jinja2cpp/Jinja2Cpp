#include <jinja2cpp/binding/nlohmann_json.h>
#include <jinja2cpp/json_adapter.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>

#include "conformance.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string_view>

// The rendering rows live in the conformance suite (conformance.h), which runs them on both nlohmann types; this
// file keeps what is specific to the binding: its traits, its pointer overloads of Reflect (docs/tasks/0311)

// Reflect(const std::shared_ptr<J>&) shares the document: it outlives the caller's pointer (task 0311)
TEST(NlohmannJsonOwnershipTest, ConstSharedPtrLvalueKeepsTheDocumentAlive)
{
    auto doc = std::make_shared<nlohmann::json>(nlohmann::json::parse(R"({"a": [1, 2, {"b": "kept"}]})"));
    std::weak_ptr<nlohmann::json> weak = doc;
    const std::shared_ptr<nlohmann::json>& constRef = doc;
    std::optional<jinja2::Value> view = jinja2::Reflect(constRef);
    doc.reset();
    ASSERT_FALSE(weak.expired());

    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{{ j.a[2].b }}|{{ j.a|length }}"));
    {
        const jinja2::ValuesMap params{ { "j", *view } };
        EXPECT_EQ("kept|3", tpl.RenderAsString(params).value());
    }

    view.reset();
    EXPECT_TRUE(weak.expired());
}

TEST(NlohmannJsonOwnershipTest, NullPointerIsAnEmptyValue)
{
    EXPECT_TRUE(jinja2::Reflect(static_cast<const nlohmann::json*>(nullptr)).IsNone());
    EXPECT_TRUE(jinja2::Reflect(static_cast<const nlohmann::ordered_json*>(nullptr)).IsNone());
    EXPECT_TRUE(jinja2::Reflect(std::shared_ptr<nlohmann::json>()).IsNone());
}

// The nlohmann traits are viewable and copyable
static_assert(jinja2::IsJsonViewable<nlohmann::json> && jinja2::IsJsonCopyable<nlohmann::json>);
static_assert(jinja2::IsJsonViewable<nlohmann::ordered_json> && jinja2::IsJsonCopyable<nlohmann::ordered_json>);

namespace
{
struct NlohmannSorted
{
    using Json = nlohmann::json;
    static Json Parse(std::string_view text) { return Json::parse(text.begin(), text.end()); }
    static constexpr bool kSortedKeys = true;
    static constexpr bool kViewable = true;
    static constexpr bool kBigIntText = false;
    static constexpr bool kViewDuplicateKeys = false; // the parser keeps the last value
};

struct NlohmannOrdered
{
    using Json = nlohmann::ordered_json;
    static Json Parse(std::string_view text) { return Json::parse(text.begin(), text.end()); }
    static constexpr bool kSortedKeys = false;
    static constexpr bool kViewable = true;
    static constexpr bool kBigIntText = false;
    static constexpr bool kViewDuplicateKeys = false;
};

} // namespace

using NlohmannBindings = ::testing::Types<NlohmannSorted, NlohmannOrdered>;
INSTANTIATE_TYPED_TEST_SUITE_P(Nlohmann, JsonConformance, NlohmannBindings, JsonBindingNames);
