#include <iostream>
#include <string>

#include "../test_tools.h"
#include <gtest/gtest.h>

#include <jinja2cpp/template.h>
#include <jinja2cpp/binding/nlohmann_json.h>


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
{'message': 'Hello World from Parser!', 'message2': 'Hello World from Parser-123!'}
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

// A JSON unsigned number above INT64_MAX fails the render instead of wrapping (task 0172)
TEST(NlohmannJsonUnsignedTest, UnsignedAboveInt64MaxFails)
{
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{{ json.max }}|{{ json.big }}"));
    jinja2::ValuesMap params{ { "json", jinja2::Reflect(nlohmann::json::parse(R"({"max": 9223372036854775807, "big": 18446744073709551615})")) } };
    const auto result = tpl.RenderAsString(params);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(jinja2::ErrorCode::UnexpectedException, result.error().GetCode());
    ASSERT_TRUE(tpl.Load("{{ json.max }}"));
    EXPECT_EQ("9223372036854775807", tpl.RenderAsString(params).value());
}
