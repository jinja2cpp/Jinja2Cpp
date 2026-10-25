#include "../src/json_reader.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include "gtest/gtest.h"

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

// The reader of {% meta %} JSON (docs/tasks/0230, step D6)

using jinja2::JsonReadLimits;
using jinja2::ReadJsonObject;
using jinja2::ReadJsonValue;
using jinja2::ValuesList;
using jinja2::ValuesMap;

namespace
{
struct SuiteCase
{
    const char* name;
    const char* bytes;
    std::size_t size;
};

const SuiteCase SuiteCases[] = {
// NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
#include "json_test_suite_cases.inc"
};

// The implementation-defined cases the reader rejects: every string that is not well-formed
// UTF-8 or holds a lone surrogate (a UTF-8 string cannot hold one), UTF-16 text, a BOM, and
// nesting past JsonReadLimits::maxDepth. It accepts the numbers: an integer past int64_t
// becomes a double, an exponent out of range inf or 0.
bool RejectsImplementationDefined(std::string_view name)
{
    return name.rfind("i_number_", 0) != 0;
}

TEST(JsonReaderTest, JsonTestSuite)
{
    const JsonReadLimits limits;
    std::size_t checked = 0;
    for (const auto& suiteCase : SuiteCases)
    {
        const std::string_view name = suiteCase.name;
        const auto result = ReadJsonValue(std::string_view(suiteCase.bytes, suiteCase.size), limits);
        switch (name[0])
        {
        case 'y':
            EXPECT_TRUE(result.has_value()) << name << ": " << result.error();
            break;
        case 'n':
            EXPECT_FALSE(result.has_value()) << name;
            break;
        default:
            EXPECT_EQ(!result.has_value(), RejectsImplementationDefined(name)) << name << ": " << (result ? std::string("accepted") : result.error());
            break;
        }
        ++checked;
    }
    EXPECT_EQ(checked, 316U);
}

// The two suite files the table leaves out for their size
TEST(JsonReaderTest, DeepNestingIsAnError)
{
    const JsonReadLimits limits;
    const auto arrays = ReadJsonValue(std::string(100000, '['), limits);
    ASSERT_FALSE(arrays.has_value());
    EXPECT_EQ(arrays.error(), "Nesting too deep: line 1 column 129 (char 128)");

    std::string openObjects;
    for (int i = 0; i != 50000; ++i)
    {
        openObjects += "[{\"\":";
    }
    EXPECT_FALSE(ReadJsonValue(openObjects, limits).has_value());

    std::string nested(128, '[');
    nested += std::string(128, ']');
    EXPECT_TRUE(ReadJsonValue(nested, limits).has_value());
    JsonReadLimits shallow;
    shallow.maxDepth = 2;
    EXPECT_TRUE(ReadJsonValue("[[1]]", shallow).has_value());
    EXPECT_FALSE(ReadJsonValue("[[[1]]]", shallow).has_value());
}

TEST(JsonReaderTest, BuildsValues)
{
    const auto result = ReadJsonObject(R"( {"s": "a\"\\\/\b\f\n\r\té😀", "i": -42, "big": 9223372036854775808, "d": 1.5e2,
        "t": true, "f": false, "n": null, "l": [1, [], {}], "o": {"k": "v"}} )",
                                       JsonReadLimits());
    ASSERT_TRUE(result.has_value()) << result.error();
    const ValuesMap& map = result.value();
    ASSERT_EQ(map.size(), 9U);
    EXPECT_EQ(map.at("s").As<std::string>(), "a\"\\/\b\f\n\r\t\xc3\xa9\xf0\x9f\x98\x80");
    EXPECT_EQ(map.at("i").As<int64_t>(), -42);
    EXPECT_DOUBLE_EQ(map.at("big").As<double>(), 9223372036854775808.0);
    EXPECT_DOUBLE_EQ(map.at("d").As<double>(), 150.0);
    EXPECT_TRUE(map.at("t").As<bool>());
    EXPECT_FALSE(map.at("f").As<bool>());
    EXPECT_TRUE(map.at("n").IsNone());
    const auto& list = map.at("l").As<ValuesList>();
    ASSERT_EQ(list.size(), 3U);
    EXPECT_EQ(list[0].As<int64_t>(), 1);
    EXPECT_TRUE(list[1].As<ValuesList>().empty());
    EXPECT_TRUE(list[2].As<ValuesMap>().empty());
    EXPECT_EQ(map.at("o").As<ValuesMap>().at("k").As<std::string>(), "v");

    // Python: a repeated key keeps its first position and its last value
    const auto repeated = ReadJsonObject(R"({"a": 1, "b": 2, "a": 3})", JsonReadLimits());
    ASSERT_TRUE(repeated.has_value());
    EXPECT_EQ(repeated.value().begin()->first, "a");
    EXPECT_EQ(repeated.value().at("a").As<int64_t>(), 3);

    // An exponent out of range gives inf, as Python's json does
    const auto huge = ReadJsonValue("1e400", JsonReadLimits());
    ASSERT_TRUE(huge.has_value());
    EXPECT_TRUE(std::isinf(huge.value().As<double>()));
}

// The messages and positions Python's json.loads (3.12, the pinned oracle) reports for the same text
TEST(JsonReaderTest, ErrorsReadAsPythons)
{
    const std::pair<const char*, const char*> cases[] = {
        // NOLINT(cppcoreguidelines-avoid-c-arrays, modernize-avoid-c-arrays)
        { R"({"a" 1})", "Expecting ':' delimiter: line 1 column 6 (char 5)" },
        { R"({"a":1,})", "Expecting property name enclosed in double quotes: line 1 column 8 (char 7)" },
        { R"({"a":[1,]})", "Expecting value: line 1 column 9 (char 8)" },
        { "{\"\xc3\xa9\": x}", "Expecting value: line 1 column 7 (char 6)" },
        { "\xef\xbb\xbf{}", "Unexpected UTF-8 BOM (decode using utf-8-sig): line 1 column 1 (char 0)" },
        { R"({"a":tru})", "Expecting value: line 1 column 6 (char 5)" },
        { R"({"a":"abc)", "Unterminated string starting at: line 1 column 6 (char 5)" },
        { R"({"a":1} x)", "Extra data: line 1 column 9 (char 8)" },
        { R"({"a":"\x"})", "Invalid \\escape: line 1 column 7 (char 6)" },
        { R"({"a":"\u12"})", "Invalid \\uXXXX escape: line 1 column 8 (char 7)" },
        { "{\n \"a\": [1 2]}", "Expecting ',' delimiter: line 2 column 10 (char 11)" },
        { "{\"a\":\"b\tc\"}", "Invalid control character at: line 1 column 8 (char 7)" },
        { R"({"a":01})", "Expecting ',' delimiter: line 1 column 7 (char 6)" },
        { R"({"a":1.})", "Expecting ',' delimiter: line 1 column 7 (char 6)" },
        { R"({"a":1e})", "Expecting ',' delimiter: line 1 column 7 (char 6)" },
        { R"({"a":-})", "Expecting value: line 1 column 6 (char 5)" },
        { R"({1:2})", "Expecting property name enclosed in double quotes: line 1 column 2 (char 1)" },
        { "", "Expecting value: line 1 column 1 (char 0)" },
        // Not Python's: the reader's own rules
        { "[1]", "Expecting an object: line 1 column 1 (char 0)" },
        { R"({"a":"\ud800"})", "Unpaired surrogate: line 1 column 7 (char 6)" },
        { "{\"a\":\"\xff\"}", "Invalid UTF-8 at: line 1 column 7 (char 6)" },
    };
    for (const auto& [text, message] : cases)
    {
        const auto result = ReadJsonObject(text, JsonReadLimits());
        ASSERT_FALSE(result.has_value()) << text;
        EXPECT_EQ(result.error(), message) << text;
    }
}

TEST(JsonReaderTest, SequenceLimit)
{
    JsonReadLimits limits;
    limits.maxSequenceSize = 3;
    EXPECT_TRUE(ReadJsonObject(R"({"a": [1, 2, 3], "b": "xyz", "c": 0})", limits).has_value());
    EXPECT_EQ(ReadJsonObject(R"({"a": [1, 2, 3, 4]})", limits).error(), "Too many items: line 1 column 17 (char 16)");
    EXPECT_EQ(ReadJsonObject(R"({"a": 1, "b": 2, "c": 3, "d": 4})", limits).error(), "Too many items: line 1 column 32 (char 31)");
    EXPECT_EQ(ReadJsonObject(R"({"a": "abcd"})", limits).error(), "String too long: line 1 column 7 (char 6)");
    EXPECT_EQ(ReadJsonObject(R"({"a": 1.25})", limits).error(), "Number too long: line 1 column 7 (char 6)");
    EXPECT_TRUE(ReadJsonObject(R"({"a": 1.5})", limits).has_value());
}

TEST(JsonReaderTest, TemplateMetadata)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load(R"({% meta %}{"title": "Report", "n": 3}{% endmeta %}x)").has_value());
    const auto metadata = tpl.GetMetadata();
    ASSERT_TRUE(metadata.has_value()) << metadata.error().ToString();
    EXPECT_EQ(metadata.value().GetSize(), 2U);

    // A top level that is not an object used to throw std::bad_variant_access out of GetMetadata
    jinja2::Template list;
    ASSERT_TRUE(list.Load("{% meta %}[1, 2]{% endmeta %}x").has_value());
    const auto notObject = list.GetMetadata();
    ASSERT_FALSE(notObject.has_value());
    EXPECT_EQ(notObject.error().GetCode(), jinja2::ErrorCode::MetadataParseError);

    // Settings::Limits::maxSequenceSize bounds what a block builds
    jinja2::TemplateEnv env;
    jinja2::Settings settings;
    settings.limits.maxSequenceSize = 2;
    env.SetSettings(settings);
    jinja2::Template limited(&env);
    ASSERT_TRUE(limited.Load(R"({% meta %}{"l": [1, 2, 3]}{% endmeta %}x)").has_value());
    const auto tooMany = limited.GetMetadata();
    ASSERT_FALSE(tooMany.has_value());
    EXPECT_NE(tooMany.error().ToString().find("Too many items"), std::string::npos) << tooMany.error().ToString();

    jinja2::TemplateW wide;
    ASSERT_TRUE(wide.Load(L"{% meta %}{\"t\": \"é\"}{% endmeta %}x").has_value());
    const auto wideMetadata = wide.GetMetadata();
    ASSERT_TRUE(wideMetadata.has_value());
    EXPECT_TRUE(wideMetadata.value().HasValue("t"));
}
} // namespace
