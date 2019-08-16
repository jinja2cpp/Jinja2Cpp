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

MULTISTR_TEST(NlohmannJsonTest, SubobjectReflection, R"({{ json.object.message }})", R"(Hello World from Parser!)")
{
    nlohmann::json values = {
        {"object", {{"message", "Hello World from Parser!"}}}
    };

    params["json"] = jinja2::Reflect(std::move(values));
}

MULTISTR_TEST(NlohmannJsonTest, ArrayReflection, R"({{ json.array | sort | pprint }})", R"([1, 2, 3, 4, 5, 6, 7, 8, 9])")
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

