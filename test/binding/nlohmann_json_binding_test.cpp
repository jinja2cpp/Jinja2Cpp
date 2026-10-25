#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <string>

#include "../test_tools.h"
#include <gtest/gtest.h>

#include <jinja2cpp/template.h>
#include <jinja2cpp/binding/nlohmann_json.h>

#include "conformance.h"
#include "toy_json.h"


using NlohmannJsonTest = BasicTemplateRenderer;

MULTISTR_TEST(NlohmannJsonTest, BasicReflection, R"({{ json.message }})", R"(Hello World from Parser!)")
{
    nlohmann::json values = {
        {"message", "Hello World from Parser!"}
    };

    params["json"] = jinja2::Reflect(std::move(values));
}

MULTISTR_TEST(NlohmannJsonTest, BasicTypesReflection, R"(
{{ json.bool | pprint }}
{{ json.small_int | pprint }}
{{ json.big_int | pprint }}
{{ json.double | pprint }}
{{ json.string | pprint }}
)",
              R"(
True
100500
100500100500100
100.5
'Hello World!')")
{
    nlohmann::json values = {
        {"bool", true},
        {"small_int", 100500},
        {"big_int", 100500100500100LL},
        {"double", 100.5},
        {"string", "Hello World!"},
    };

    params["json"] = jinja2::Reflect(std::move(values));
}

MULTISTR_TEST(NlohmannJsonTest, BasicValuesReflection, R"(
{{ bool_val | pprint }}
{{ small_int_val | pprint }}
{{ big_int_val | pprint }}
{{ double_val | pprint }}
{{ string_val | pprint }}
{{ array_val | pprint }}
{{ object_val | pprint }}
{{ empty_val | pprint }}
)",
              R"(
True
100500
100500100500100
100.5
'Hello World!'
[1, 2, 3, 4]
{'message': 'Hello World from Parser!',
 'message2': 'Hello World from Parser-123!'}
None)")
{
    nlohmann::json values = {
        {"bool", true},
        {"small_int", 100500},
        {"big_int", 100500100500100LL},
        {"double", 100.5},
        {"string", "Hello World!"},
        {"array", {1, 2, 3, 4}},
        {"object", {
            {"message", "Hello World from Parser!"},
            {"message2", "Hello World from Parser-123!"}
        }}};

    auto boolVal = values["bool"];
    auto smallIntVal = values["small_int"];
    auto bigIntVal = values["big_int"];
    auto doubleVal = values["double"];
    auto stringVal = values["string"];
    auto arrayVal = values["array"];
    auto objectVal = values["object"];
    nlohmann::json emptyVal;

    params["bool_val"] = jinja2::Reflect(std::move(boolVal));
    params["small_int_val"] = jinja2::Reflect(std::move(smallIntVal));
    params["big_int_val"] = jinja2::Reflect(std::move(bigIntVal));
    params["double_val"] = jinja2::Reflect(std::move(doubleVal));
    params["string_val"] = jinja2::Reflect(std::move(stringVal));
    params["array_val"] = jinja2::Reflect(std::move(arrayVal));
    params["object_val"] = jinja2::Reflect(std::move(objectVal));
    params["empty_val"] = jinja2::Reflect(std::move(emptyVal));
}

MULTISTR_TEST(NlohmannJsonTest, SubobjectReflection,
              R"(
{{ json.object.message }}
{{ json.object.message3 }}
{{ json.object | list | join(', ') }}
)",
              R"(
Hello World from Parser!

message, message2)")
{
    nlohmann::json values = {
        {"object", {
            {"message", "Hello World from Parser!"},
            {"message2", "Hello World from Parser-123!"}
        }}
    };

    params["json"] = jinja2::Reflect(std::move(values));
}

MULTISTR_TEST(NlohmannJsonTest, ArrayReflection,
              R"(
{{ json.array | sort | pprint }}
{{ json.array | length }}
{{ json.array | first }}
{{ json.array | last }}
{{ json.array[8] }}-{{ json.array[6] }}-{{ json.array[4] }}
)",
              R"(
[1, 2, 3, 4, 5, 6, 7, 8, 9]
9
9
1
1-3-5)")
{
    nlohmann::json values = {
        {"array", {9, 8, 7, 6, 5, 4, 3, 2, 1}}
    };

    params["json"] = jinja2::Reflect(std::move(values));
}

MULTISTR_TEST(NlohmannJsonTest, ParsedTypesReflection, R"(
{{ json.bool | pprint }}
{{ json.small_int | pprint }}
{{ json.big_int | pprint }}
{{ json.double | pprint }}
{{ json.string | pprint }}
{{ json.object.message | pprint }}
{{ json.array | sort | pprint }}
)",
              R"(
True
100500
100500100500100
100.5
'Hello World!'
'Hello World from Parser!'
[1, 2, 3, 4, 5, 6, 7, 8, 9])")
{
    nlohmann::json values = nlohmann::json::parse(R"(
{
    "big_int": 100500100500100,
    "bool": true,
    "double": 100.5,
    "small_int": 100500,
    "string": "Hello World!",
    "object": {"message": "Hello World from Parser!"},
    "array": [9, 8, 7, 6, 5, 4, 3, 2, 1]
}
)");;

    params["json"] = jinja2::Reflect(std::move(values));
}


// docs/tasks/0002: iterating a reflected array crashed in GenericListAdapter::Enumerator::MoveNext
MULTISTR_TEST(NlohmannJsonTest, ArrayIteration,
              R"(
{% for x in list %}{{ x }},{% endfor %}
{% for x in json.array %}{{ x }},{% endfor %}
{% for o in json.objects %}{{ o.name }}={% for v in o['values'] %}{{ v }}{% endfor %};{% endfor %}
{% for x in json.array if x > 1 %}{{ loop.index }}:{{ x }},{% endfor %}
{{ list | join('-') }} {{ json.objects | map(attribute='name') | join }}
)",
              R"(
1,2,3,
3,2,1,
a=12;b=;
1:3,2:2,
1-2-3 ab)")
{
    nlohmann::json values = nlohmann::json::parse(R"(
{
    "array": [3, 2, 1],
    "objects": [{"name": "a", "values": [1, 2]}, {"name": "b", "values": []}]
}
)");

    params["list"] = jinja2::Reflect(nlohmann::json::parse("[1, 2, 3]"));
    params["json"] = jinja2::Reflect(std::move(values));
}

// A JSON null is present and is None, not undefined (task 0047)
MULTISTR_TEST(NlohmannJsonTest, NullMemberIsNone, R"({{ json.x is defined }}{{ json.x is none }}[{{ json.x }}]{{ json.x|string }}{{ json.y is defined }})", R"(TrueTrue[None]NoneFalse)")
{
    nlohmann::json values = nlohmann::json::parse(R"({"x": null})");

    params["json"] = jinja2::Reflect(std::move(values));
}

// nlohmann::json sorts object keys; ordered_json keeps the document order, nested objects too (task 0043)
MULTISTR_TEST(NlohmannJsonTest, OrderedJsonKeepsDocumentOrder, R"({{ sorted }}|{{ json }}|{% for k, v in json.inner.items() %}{{ k }}={{ v }},{% endfor %}|{{ json.list[0] | list | join }})", R"({'a': 2, 'b': 1}|{'b': 1, 'a': 2, 'inner': {'z': 1, 'y': 2}, 'list': [{'q': 1, 'p': 2}]}|z=1,y=2,|qp)")
{
    const char* doc = R"({"b": 1, "a": 2, "inner": {"z": 1, "y": 2}, "list": [{"q": 1, "p": 2}]})";
    params["sorted"] = jinja2::Reflect(nlohmann::json::parse(R"({"b": 1, "a": 2})"));
    params["json"] = jinja2::Reflect(nlohmann::ordered_json::parse(doc));
}

// A JSON unsigned number above INT64_MAX is a BigInt instead of wrapping (task 0172, 0269), which templates
// print exactly (task 0270)
TEST(NlohmannJsonUnsignedTest, UnsignedAboveInt64MaxIsBigInt)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{{ json.max }}|{{ json.big }}"));
    jinja2::ValuesMap params{ { "json", jinja2::Reflect(nlohmann::json::parse(R"({"max": 9223372036854775807, "big": 18446744073709551615})")) } };
    const auto big = params["json"].ToGenericMap().GetValueByName("big");
    ASSERT_TRUE(big.IsBigInt());
    EXPECT_EQ("18446744073709551615", big.AsBigInt().ToString());
    EXPECT_EQ("9223372036854775807|18446744073709551615", tpl.RenderAsString(params).value());
}


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

// The nlohmann traits are viewable and copyable; the toy ones show the two other shapes of traits
static_assert(jinja2::IsJsonViewable<nlohmann::json> && jinja2::IsJsonCopyable<nlohmann::json>);
static_assert(jinja2::IsJsonViewable<nlohmann::ordered_json> && jinja2::IsJsonCopyable<nlohmann::ordered_json>);
static_assert(jinja2::IsJsonViewable<ToyViewDoc> && jinja2::IsJsonCopyable<ToyViewDoc>);
static_assert(!jinja2::IsJsonViewable<ToyStreamDoc> && jinja2::IsJsonCopyable<ToyStreamDoc>);
static_assert(!jinja2::IsJsonViewable<int> && !jinja2::IsJsonCopyable<int>);

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

using ConformingBindings = ::testing::Types<NlohmannSorted, NlohmannOrdered, ToyView, ToyStream>;
INSTANTIATE_TYPED_TEST_SUITE_P(Nlohmann, JsonConformance, ConformingBindings, JsonBindingNames);

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
