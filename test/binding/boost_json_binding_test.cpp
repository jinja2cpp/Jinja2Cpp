#include <jinja2cpp/binding/boost_json.h>
#include <jinja2cpp/json_adapter.h>

#include "conformance.h"

#include <gtest/gtest.h>

#include <string_view>

// The rendering rows live in the conformance suite (conformance.h); this file keeps the binding's traits
// (docs/tasks/0311)

namespace
{
// Boost.JSON keeps the document order, and its parser keeps the last value of a repeated key
struct BoostJson
{
    using Json = boost::json::value;
    // The suite's deep rows nest past Boost's default parser depth (32)
    static Json Parse(std::string_view text)
    {
        boost::json::parse_options options;
        options.max_depth = 4096;
        return boost::json::parse(boost::json::string_view(text.data(), text.size()), {}, options);
    }
    static constexpr bool kSortedKeys = false;
    static constexpr bool kViewable = true;
    static constexpr bool kBigIntText = false;
    static constexpr bool kViewDuplicateKeys = false;
};
} // namespace

static_assert(jinja2::IsJsonViewable<boost::json::value> && jinja2::IsJsonCopyable<boost::json::value>);
INSTANTIATE_TYPED_TEST_SUITE_P(Boost, JsonConformance, ::testing::Types<BoostJson>, JsonBindingNames);
