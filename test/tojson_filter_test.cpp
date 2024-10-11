#include "jinja2cpp/template.h"
#include "test_tools.h"

#include <nlohmann/json.hpp>

using namespace jinja2;

SUBSTITUTION_TEST_P(JsonFilterSubstitutionTest)

INSTANTIATE_TEST_SUITE_P(ToJson,
                        JsonFilterSubstitutionTest,
                        ::testing::Values(InputOutputPair{ "(1, 2, 3) | tojson", "[1,2,3]" },
                                          InputOutputPair{ "(1, 2, 3) | tojson(indent = 1)", "[\n 1,\n 2,\n 3\n]" },
                                          InputOutputPair{ "'\"ba&r\\'' | tojson", "\"\\\"ba\\u0026r\\u0027\"" },
                                          InputOutputPair{ "'<bar>' | tojson", "\"\\u003cbar\\u003e\"" }));

struct ToJson : ::testing::Test
{
    ValuesMap GetObjectParam() const
    {
        const ValuesMap object{ { "intValue", 3 },
                                { "doubleValue", 12.123f },
                                { "stringValue", "rain" },
                                { "wstringValue", std::wstring(L"rain") },
                                { "boolFalseValue", false },
                                { "boolTrueValue", true },
                                { "listValue", ValuesList{ 1, 2, 3 } },
                                { "map", ValuesMap{ { "str1", 1 } } } };

        return ValuesMap{ { "obj", object } };
    }

    ValuesMap GetKeyValuePairParam() const
    {
        const ValuesMap pair{ { "foo", "bar" } };
        return ValuesMap{ { "obj", pair } };
    }

    template<typename TemplateType, typename StringType>
    void PerformJsonTest(const StringType& source, const StringType& expectedResult, const jinja2::ValuesMap& params)
    {
        TemplateType tpl;
        ASSERT_TRUE(tpl.Load(source));
        const auto result = tpl.RenderAsString(params).value();

        const auto expectedJson = nlohmann::json::parse(expectedResult);
        const auto resultJson = nlohmann::json::parse(result);

        EXPECT_EQ(expectedJson, resultJson);
    }

    void PerformBothJsonTests(const std::string& source, const std::string& expectedResult, const jinja2::ValuesMap& params)
    {
        PerformJsonTest<Template>(source, expectedResult, params);
        PerformJsonTest<TemplateW>(ConvertString<std::wstring>(source), ConvertString<std::wstring>(expectedResult), params);
    }
};

TEST_F(ToJson, SerializeKeyValuePair)
{
    constexpr auto source = "{{obj | tojson}}";
    const auto expectedResult = "{\"foo\":\"bar\"}";

    PerformBothJsonTests(source, expectedResult, GetKeyValuePairParam());
}

namespace nlohmann
{
void PrintTo(const nlohmann::json& json, std::ostream* os)
{
    *os << json.dump();
}
} // namespace nlohmann

TEST_F(ToJson, SerializeObject)
{
    const auto source = "{{obj | tojson}}";
    const auto expectedResult = "{\"map\":{\"str1\":1},\"listValue\":[1,2,3],\"boolFalseValue\":false,\"boolTrueValue\":true,\"wstringValue\":\"rain\","
                                "\"stringValue\":\"rain\",\"doubleValue\":12.123000144958496,\"intValue\":3}";

    PerformBothJsonTests(source, expectedResult, GetObjectParam());
}

struct ToJsonIndentationTest : SubstitutionTestBase
{
    ValuesMap getObjectParam() const
    {
        const ValuesMap object{ { "map", ValuesMap{ { "array", ValuesList{ 1, 2, 3 } } } } };
        return ValuesMap{ { "obj", object } };
    }
};

TEST_F(ToJsonIndentationTest, SerializeObjectWithoutIndent)
{
    const auto source = "{{obj | tojson}}";
    const auto expectedResult = "{\"map\":{\"array\":[1,2,3]}}";

    PerformBothTests(source, expectedResult, getObjectParam());
}

TEST_F(ToJsonIndentationTest, SerializeObjectWithIndent)
{
    const auto source = "{{obj | tojson(indent=4)}}";
    const auto expectedResult =
R"({
    "map": {
        "array": [
            1,
            2,
            3
        ]
    }
})";

    PerformBothTests(source, expectedResult, getObjectParam());
}
