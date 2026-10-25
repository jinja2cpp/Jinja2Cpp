// The JsonTraits contract (jinja2cpp/json_adapter.h) on two toy JSON libraries, built in every
// configuration whatever JINJA2CPP_TEST_BINDINGS lists (docs/tasks/0311): a view with handle nodes and
// std::string results, and forward-only traits that only ToValue accepts
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/json_adapter.h>
#include <jinja2cpp/value.h>

#include "conformance.h"
#include "toy_json.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

// The toy traits show the two shapes of traits; a type without traits is neither
static_assert(jinja2::IsJsonViewable<ToyViewDoc> && jinja2::IsJsonCopyable<ToyViewDoc>);
static_assert(!jinja2::IsJsonViewable<ToyStreamDoc> && jinja2::IsJsonCopyable<ToyStreamDoc>);
static_assert(!jinja2::IsJsonViewable<int> && !jinja2::IsJsonCopyable<int>);

namespace
{
// Qt-style traits: handle nodes, std::string results; the toy parser keeps a repeated key twice, as simdjson's DOM does
struct ToyView
{
    using Json = ToyViewDoc;
    static Json Parse(std::string_view text) { return Json(toyjson::Parser(text, false).Parse()); }
    static constexpr bool kSortedKeys = false;
    static constexpr bool kViewable = true;
    static constexpr bool kBigIntText = false;
    static constexpr bool kViewDuplicateKeys = true;
};

// Forward-only traits: only ToValue works
struct ToyStream
{
    using Json = ToyStreamDoc;
    static Json Parse(std::string_view text) { return Json(toyjson::Parser(text, true).Parse()); }
    static constexpr bool kSortedKeys = false;
    static constexpr bool kViewable = false;
    static constexpr bool kBigIntText = true;
};
} // namespace

using ToyBindings = ::testing::Types<ToyView, ToyStream>;
INSTANTIATE_TYPED_TEST_SUITE_P(Toy, JsonConformance, ToyBindings, JsonBindingNames);

namespace
{
// ExtraParams of an error are strings
std::string Param(const jinja2::ErrorInfo& err, std::size_t idx)
{
    std::string_view text;
    return idx < err.GetExtraParams().size() && err.GetExtraParams()[idx].AsString(text) ? std::string(text) : std::string("<missing>");
}

template<typename Doc>
jinja2::ErrorInfo CopyError(Doc&& doc, const jinja2::JsonCopyLimits& limits = {})
{
    auto result = jinja2::ToValue(std::forward<Doc>(doc), limits);
    EXPECT_FALSE(result.has_value());
    return result.has_value() ? jinja2::ErrorInfo() : result.error();
}
} // namespace

// A failure of the traits mid-document is a DataReadError: the traits' message, then the JSON path of the value
TEST(ToyStreamJsonTest, InjectedFailureIsADataReadError)
{
    const std::string text = R"({"a":[1,2,3],"b":4})";
    ToyStreamDoc doc = ToyStream::Parse(text);
    doc.failAtValue = 3; // the root, "a", a[0], then a[1]

    const jinja2::ErrorInfo err = CopyError(std::move(doc));
    EXPECT_EQ(jinja2::ErrorCode::DataReadError, err.GetCode());
    ASSERT_EQ(2u, err.GetExtraParams().size());
    EXPECT_EQ("injected failure", Param(err, 0));
    EXPECT_EQ("$.a[1]", Param(err, 1));
    EXPECT_EQ("JSON $.a[1] (char " + std::to_string(text.find('2')) + ")", err.GetLocationDescr());
}

TEST(ToyStreamJsonTest, FailureAtTheRootAndAtEveryValue)
{
    const std::string text = R"({"a":[1,{"b":null}],"c":"s"})";
    // Whatever value fails, ToValue reports it and does not crash or leak a partial result
    for (std::int64_t at = 0; at < 6; ++at)
    {
        SCOPED_TRACE(at);
        ToyStreamDoc doc = ToyStream::Parse(text);
        doc.failAtValue = at;
        auto result = jinja2::ToValue(std::move(doc));
        ASSERT_FALSE(result.has_value());
        EXPECT_EQ(jinja2::ErrorCode::DataReadError, result.error().GetCode());
        EXPECT_EQ(0u, result.error().GetLocationDescr().find("JSON $"));
        EXPECT_NE(std::string::npos, result.error().GetLocationDescr().find("(char "));
    }
    ToyStreamDoc root = ToyStream::Parse(text);
    root.failAtValue = 0;
    const jinja2::ErrorInfo err = CopyError(std::move(root));
    EXPECT_EQ("$", Param(err, 1));
    EXPECT_EQ("JSON $ (char 0)", err.GetLocationDescr());
}

TEST(ToyStreamJsonTest, FailingStringAndFailingObject)
{
    const std::string text = R"({"s":["ok","BAD"]})";
    ToyStreamDoc doc = ToyStream::Parse(text);
    doc.failOnString = "BAD";
    const jinja2::ErrorInfo stringErr = CopyError(std::move(doc));
    EXPECT_EQ(jinja2::ErrorCode::DataReadError, stringErr.GetCode());
    EXPECT_EQ("unreadable string", Param(stringErr, 0));
    EXPECT_EQ("$.s[1]", Param(stringErr, 1));
    EXPECT_EQ("JSON $.s[1] (char " + std::to_string(text.find("\"BAD")) + ")", stringErr.GetLocationDescr());

    ToyStreamDoc object = ToyStream::Parse(R"({"a":1,"b":2})");
    object.failInObject = true;
    const jinja2::ErrorInfo objectErr = CopyError(std::move(object));
    EXPECT_EQ(jinja2::ErrorCode::DataReadError, objectErr.GetCode());
    EXPECT_EQ("object cut short", Param(objectErr, 0));
    EXPECT_EQ("$", Param(objectErr, 1));
}

TEST(ToyStreamJsonTest, NoFailureIsACopy)
{
    ToyStreamDoc doc = ToyStream::Parse(R"({"a":[1,2.5,"x",null,true],"b":{}})");
    auto result = jinja2::ToValue(std::move(doc));
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(2u, result->ToGenericMap().GetSize());
}

// Limit errors carry the byte offset of the value the traits report
TEST(ToyStreamJsonTest, LimitErrorsShowTheOffset)
{
    const std::string text = R"({"a":[1,2,3,4]})";
    const jinja2::ErrorInfo seq = CopyError(ToyStream::Parse(text), jinja2::JsonCopyLimits{ 128, 3 });
    EXPECT_EQ(jinja2::ErrorCode::ResourceLimitExceeded, seq.GetCode());
    EXPECT_EQ("maxSequenceSize", Param(seq, 1));
    EXPECT_EQ("$.a", Param(seq, 2));
    EXPECT_EQ("JSON $.a (char " + std::to_string(text.find('[')) + ")", seq.GetLocationDescr());

    const jinja2::ErrorInfo depth = CopyError(ToyStream::Parse(std::string(129, '[') + std::string(129, ']')));
    EXPECT_EQ(jinja2::ErrorCode::ResourceLimitExceeded, depth.GetCode());
    EXPECT_EQ("maxDepth", Param(depth, 1));
    EXPECT_NE(std::string::npos, depth.GetLocationDescr().find("(char 128)")) << depth.GetLocationDescr();

    const jinja2::ErrorInfo str = CopyError(ToyStream::Parse(R"(["ab", "abcd"])"), jinja2::JsonCopyLimits{ 128, 3 });
    EXPECT_EQ("$[1]", Param(str, 2));
    EXPECT_NE(std::string::npos, str.GetLocationDescr().find("(char 7)")) << str.GetLocationDescr();
}

// Keys that are not identifiers are quoted in the path, with quotes, backslashes and control characters escaped
TEST(ToyStreamJsonTest, PathQuotesOddKeys)
{
    const jinja2::JsonCopyLimits cap3{ 128, 3 };
    EXPECT_EQ(R"($["a b"][0])", Param(CopyError(ToyStream::Parse(R"({"a b":[[1,2,3,4]]})"), cap3), 2));
    EXPECT_EQ(R"($[""])", Param(CopyError(ToyStream::Parse(R"({"":[1,2,3,4]})"), cap3), 2));
    EXPECT_EQ(R"($["q\"\\"])", Param(CopyError(ToyStream::Parse(R"({"q\"\\":[1,2,3,4]})"), cap3), 2));
    EXPECT_EQ(R"($["a\u000ab"])", Param(CopyError(ToyStream::Parse(R"({"a\nb":[1,2,3,4]})"), cap3), 2));
    EXPECT_EQ(R"($["1x"]._y2)", Param(CopyError(ToyStream::Parse(R"({"1x":{"_y2":[1,2,3,4]}})"), cap3), 2));
}

// A traits error reads as "Data read error:" in the text of the error
TEST(ToyStreamJsonTest, ErrorTextNamesTheKind)
{
    ToyStreamDoc doc = ToyStream::Parse(R"({"a":[1,2]})");
    doc.failAtValue = 2;
    const jinja2::ErrorInfo err = CopyError(std::move(doc));
    EXPECT_NE(std::string::npos, err.ToString().find("Data read error:")) << err.ToString();
    EXPECT_NE(std::string::npos, err.ToString().find("injected failure")) << err.ToString();
    std::ostringstream stream;
    stream << err;
    EXPECT_NE(std::string::npos, stream.str().find("Data read error:")) << stream.str();
}

// BigInt text is exact where a library would round to a double
TEST(ToyStreamJsonTest, BigIntTextIsExact)
{
    auto result = jinja2::ToValue(ToyStream::Parse(R"({"h":123456789012345678901234567890,"n":-5,"u":18446744073709551615})"));
    ASSERT_TRUE(result.has_value());
    const jinja2::GenericMap map = result->ToGenericMap();
    ASSERT_TRUE(map.GetValueByName("h").IsBigInt());
    EXPECT_EQ("123456789012345678901234567890", map.GetValueByName("h").AsBigInt().ToString());
    EXPECT_TRUE(map.GetValueByName("n").IsInt());
    ASSERT_TRUE(map.GetValueByName("u").IsBigInt());

    // Without AsBigIntText the same document rounds to a double in the library's parser (the view toy)
    auto rounded = jinja2::ToValue(ToyView::Parse(R"({"h":123456789012345678901234567890})"));
    ASSERT_TRUE(rounded.has_value());
    EXPECT_TRUE(rounded->ToGenericMap().GetValueByName("h").IsDouble());
}

// The view traits of the toy library: the repeated key stays in the view, not in the copy
TEST(ToyViewJsonTest, RepeatedKeyStaysInTheViewOnly)
{
    const std::string text = R"({"a":1,"b":2,"a":3})";
    EXPECT_EQ(3u, jinja2::Reflect(ToyView::Parse(text)).ToGenericMap().GetSize());
    auto copy = jinja2::ToValue(ToyView::Parse(text));
    ASSERT_TRUE(copy.has_value());
    EXPECT_EQ(2u, copy->ToGenericMap().GetSize());
    EXPECT_EQ((std::vector<std::string>{ "a", "b" }), copy->ToGenericMap().GetKeys());
}
