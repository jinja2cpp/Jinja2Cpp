#include <jinja2cpp/binding/nlohmann_json.h>
#include <jinja2cpp/json_adapter.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>

#include "conformance.h"

#include <gtest/gtest.h>

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>

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

// Release empties a document without allocating, and Copy builds an equal one, for both nlohmann types (task 0487)
template<typename T>
class NlohmannReleaseTest : public ::testing::Test
{
};
using NlohmannJsonTypes = ::testing::Types<nlohmann::json, nlohmann::ordered_json>;
struct NlohmannJsonTypeNames
{
    template<typename T>
    static std::string GetName(int /*index*/)
    {
        return std::is_same_v<T, nlohmann::json> ? "json" : "ordered_json";
    }
};
TYPED_TEST_SUITE(NlohmannReleaseTest, NlohmannJsonTypes, NlohmannJsonTypeNames);

TYPED_TEST(NlohmannReleaseTest, ReleaseEmptiesAndCopyKeepsTheOrder)
{
    using Json = TypeParam;
    using Traits = jinja2::JsonTraits<Json>;
    static_assert(noexcept(Traits::Release(std::declval<Json&>())));
    const auto doc = Json::parse(R"({"z": [1, [2, [], {}], {"y": "text", "x": [null, true, 2.5]}], "a": {}, "m": "s", "b": [[[[]]]]})");
    Json copy;
    Traits::Copy(doc, copy);
    EXPECT_EQ(doc, copy);
    EXPECT_EQ(doc.dump(), copy.dump()); // the same member order, which ordered_json keeps from the text

    Traits::Release(copy);
    EXPECT_TRUE(copy.is_object());
    EXPECT_TRUE(copy.empty());

    Json scalar = "text";
    Traits::Release(scalar);
    EXPECT_EQ("text", scalar);
}

// Past the 64 ancestors the drain keeps, it starts again from the root: a chain much deeper than that, and a wide
// level below it, still end empty
TYPED_TEST(NlohmannReleaseTest, ReleaseEmptiesADeepDocument)
{
    using Json = TypeParam;
    Json doc = Json::array();
    Json* cur = &doc;
    for (int level = 0; level < 1000; ++level)
    {
        if (level % 2 == 0)
        {
            cur->push_back(level);
            cur->push_back(Json::object());
            cur = &(*cur)[1]["k"];
            *cur = Json::array();
        }
        else
        {
            cur->push_back(Json::array());
            cur = &(*cur)[0];
        }
    }
    for (int item = 0; item < 1000; ++item)
    {
        cur->push_back(Json::array({ item, "s" }));
    }
    jinja2::JsonTraits<Json>::Release(doc);
    EXPECT_TRUE(doc.is_array());
    EXPECT_TRUE(doc.empty());
}

// Copy appends the members in order (no lookup per key) and hands a deep document to nlohmann's own copy past
// its depth cap; Release bounds its work on a chain far deeper than its ancestor ring and leaves the rest to the
// destructor
TYPED_TEST(NlohmannReleaseTest, WideAndDeepDocuments)
{
    using Json = TypeParam;
    using Traits = jinja2::JsonTraits<Json>;
    Json wide = Json::object();
    for (int key = 3000; key > 0; --key)
    {
        wide["k" + std::to_string(key)] = Json::array({ key });
    }
    Json wideCopy;
    Traits::Copy(wide, wideCopy);
    EXPECT_EQ(wide.dump(), wideCopy.dump());
    Traits::Release(wideCopy);
    EXPECT_TRUE(wideCopy.empty());

    Json deep = Json::array();
    Json* cur = &deep;
    for (int level = 0; level < 100000; ++level)
    {
        cur->push_back(Json::array());
        cur = &cur->back();
    }
    cur->push_back("leaf");
    Json deepCopy;
    {
        Json shallow = Json::array();
        Json* at = &shallow;
        for (int level = 0; level < 400; ++level)
        {
            at->push_back(Json::object({ { "k", Json::array() } }));
            at = &at->back()["k"];
        }
        Traits::Copy(shallow, deepCopy);
        EXPECT_EQ(shallow, deepCopy);
        Traits::Release(deepCopy);
        Traits::Release(shallow);
    }
    Traits::Release(deep); // bounded: returns with part of the chain left
    EXPECT_TRUE(deep.is_array());
}

// The view of a std::shared_ptr empties the document only when it held the last reference to it
TEST(NlohmannJsonOwnershipTest, SharedDocumentIsEmptiedOnlyByTheLastReference)
{
    const auto* const text = R"({"a": [1, 2]})";
    auto doc = std::make_shared<nlohmann::json>(nlohmann::json::parse(text));
    std::optional<jinja2::Value> view = jinja2::Reflect(doc);
    view.reset();
    EXPECT_EQ(nlohmann::json::parse(text), *doc);

    std::weak_ptr<nlohmann::json> weak = doc;
    view = jinja2::Reflect(std::move(doc));
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{{ j.a|join(',') }}"));
    EXPECT_EQ("1,2", tpl.RenderAsString(jinja2::ValuesMap{ { "j", *view } }).value());
    view.reset();
    EXPECT_TRUE(weak.expired());
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
