#include <iostream>
#include <string>

#include <jinja2cpp/template.h>
#include <jinja2cpp/binding/boost_json.h>

#include "../test_tools.h"
#include <gtest/gtest.h>

using BoostJsonTest = BasicTemplateRenderer;

MULTISTR_TEST(BoostJsonTest, BasicReflection, R"({{ json.message }})", R"(Hello World from Parser!)")
{
    boost::json::value values = {
        {"message", "Hello World from Parser!"}
    };

    params["json"] = jinja2::Reflect(std::move(values));
}

MULTISTR_TEST(BoostJsonTest, BasicTypesReflection, R"(
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
    boost::json::value values = {
        {"bool", true},
        {"small_int", 100500},
        {"big_int", 100500100500100LL},
        {"double", 100.5},
        {"string", "Hello World!"},
    };

    params["json"] = jinja2::Reflect(std::move(values));
}

MULTISTR_TEST(BoostJsonTest, BasicValuesReflection, R"(
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
    boost::json::value values = {
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

    auto boolVal = values.at("bool");
    auto smallIntVal = values.at("small_int");
    auto bigIntVal = values.at("big_int");
    auto doubleVal = values.at("double");
    auto stringVal = values.at("string");
    auto arrayVal = values.at("array");
    auto objectVal = values.at("object");
    boost::json::value emptyVal;

    params["bool_val"] = jinja2::Reflect(std::move(boolVal));
    params["small_int_val"] = jinja2::Reflect(std::move(smallIntVal));
    params["big_int_val"] = jinja2::Reflect(std::move(bigIntVal));
    params["double_val"] = jinja2::Reflect(std::move(doubleVal));
    params["string_val"] = jinja2::Reflect(std::move(stringVal));
    params["array_val"] = jinja2::Reflect(std::move(arrayVal));
    params["object_val"] = jinja2::Reflect(std::move(objectVal));
    params["empty_val"] = jinja2::Reflect(std::move(emptyVal));
}

MULTISTR_TEST(BoostJsonTest, SubobjectReflection,
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
    boost::json::value values = {
        {"object", {
            {"message", "Hello World from Parser!"},
            {"message2", "Hello World from Parser-123!"}
        }}
    };

    params["json"] = jinja2::Reflect(std::move(values));
}

MULTISTR_TEST(BoostJsonTest, ArrayReflection,
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
    boost::json::value values = {
        {"array", {9, 8, 7, 6, 5, 4, 3, 2, 1}}
    };

    params["json"] = jinja2::Reflect(std::move(values));
}

MULTISTR_TEST(BoostJsonTest, ParsedTypesReflection, R"(
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
    boost::json::value values = boost::json::parse(R"(
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

