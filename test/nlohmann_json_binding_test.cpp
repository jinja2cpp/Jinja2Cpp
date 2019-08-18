#include <iostream>
#include <string>

#include "gtest/gtest.h"

#include <jinja2cpp/template.h>
#include <jinja2cpp/binding/nlohmann_json.h>
#include "test_tools.h"

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
true
100500
100500100500100
100.5
'Hello World!'
)")
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
true
100500
100500100500100
100.5
'Hello World!'
[1, 2, 3, 4]
{'message': 'Hello World from Parser!', 'message2': 'Hello World from Parser-123!'}
none
)")
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

message, message2
)")
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
1-3-5
)")
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
true
100500
100500100500100
100.5
'Hello World!'
'Hello World from Parser!'
[1, 2, 3, 4, 5, 6, 7, 8, 9]
)")
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

