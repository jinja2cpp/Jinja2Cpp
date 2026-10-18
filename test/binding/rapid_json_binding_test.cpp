#include <iostream>
#include <string>

#include "../test_tools.h"
#include <gtest/gtest.h>

#include <jinja2cpp/template.h>
#include <jinja2cpp/binding/rapid_json.h>


class RapidJsonTest : public BasicTemplateRenderer
{
public:
    const auto& GetJson() const {return m_json;}
    const auto& GetEmptyVal() const {return m_emptyVal;}
protected:
    void SetUp() override
    {
        const char *json = R"(
{
    "message": "Hello World from Parser!",
    "big_int": 100500100500100,
    "bool_true": true,
    "bool_false": false,
    "double": 100.5,
    "small_int": 100500,
    "string": "Hello World!",
    "object": {"message": "Hello World from Parser!", "message2": "Hello World from Parser-123!"},
    "array": [9, 8, 7, 6, 5, 4, 3, 2, 1]
}
)";
        m_json.Parse(json);
    }
protected:
    rapidjson::Document m_json;
    rapidjson::Value m_emptyVal;
};

MULTISTR_TEST(RapidJsonTest, BasicReflection, R"({{ json.message }})", R"(Hello World from Parser!)")
{
    params["json"] = jinja2::Reflect(test.GetJson());
}

MULTISTR_TEST(RapidJsonTest, BasicTypesReflection, R"(
{{ json.bool_true | pprint }}
{{ json.bool_false | pprint }}
{{ json.small_int | pprint }}
{{ json.big_int | pprint }}
{{ json.double | pprint }}
{{ json.string | pprint }}
)",
              R"(
True
False
100500
100500100500100
100.5
'Hello World!')")
{
    params["json"] = jinja2::Reflect(test.GetJson());
}

MULTISTR_TEST(RapidJsonTest, BasicValuesReflection, R"(
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
[9, 8, 7, 6, 5, 4, 3, 2, 1]
{'message': 'Hello World from Parser!', 'message2': 'Hello World from Parser-123!'}
None)")
{
    auto& values = test.GetJson();

    auto& boolVal = values["bool_true"];
    auto& smallIntVal = values["small_int"];
    auto& bigIntVal = values["big_int"];
    auto& doubleVal = values["double"];
    auto& stringVal = values["string"];
    auto& arrayVal = values["array"];
    auto& objectVal = values["object"];
    auto& emptyVal = test.GetEmptyVal();

    params["bool_val"] = jinja2::Reflect(boolVal);
    params["small_int_val"] = jinja2::Reflect(smallIntVal);
    params["big_int_val"] = jinja2::Reflect(bigIntVal);
    params["double_val"] = jinja2::Reflect(doubleVal);
    params["string_val"] = jinja2::Reflect(stringVal);
    params["array_val"] = jinja2::Reflect(arrayVal);
    params["object_val"] = jinja2::Reflect(objectVal);
    params["empty_val"] = jinja2::Reflect(emptyVal);
}

MULTISTR_TEST(RapidJsonTest, SubobjectReflection,
              R"(
{{ json.object.message }}
{{ json.object.message3 }}
{{ json.object | list | join(', ') }}
)",
              R"(
Hello World from Parser!

message, message2)")
{
    params["json"] = jinja2::Reflect(test.GetJson());
}

MULTISTR_TEST(RapidJsonTest, ArrayReflection,
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
    params["json"] = jinja2::Reflect(test.GetJson());
}

// A JSON null is present and is None, not undefined (task 0047)
MULTISTR_TEST(RapidJsonTest, NullMemberIsNone, R"({{ json.x is defined }}{{ json.x is none }}[{{ json.x }}]{{ json.x|string }}{{ json.y is defined }})", R"(TrueTrue[None]NoneFalse)")
{
    static rapidjson::Document doc;
    doc.Parse(R"({"x": null})");
    params["json"] = jinja2::Reflect(doc);
}

// A JSON unsigned number above INT64_MAX fails the render instead of wrapping (task 0172)
TEST(RapidJsonUnsignedTest, UnsignedAboveInt64MaxFails)
{
    rapidjson::Document doc;
    doc.Parse(R"({"max": 9223372036854775807, "big": 18446744073709551615})");
    jinja2::Template tpl;
    ASSERT_TRUE(tpl.Load("{{ json.max }}|{{ json.big }}"));
    jinja2::ValuesMap params{ { "json", jinja2::Reflect(doc) } };
    const auto result = tpl.RenderAsString(params);
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(jinja2::ErrorCode::UnexpectedException, result.error().GetCode());
    ASSERT_TRUE(tpl.Load("{{ json.max }}"));
    EXPECT_EQ("9223372036854775807", tpl.RenderAsString(params).value());
}
