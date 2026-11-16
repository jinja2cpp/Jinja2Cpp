#include "test_tools.h"

#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/value.h>

#include <nlohmann/json.hpp>

#include <cstdint>
#include <iostream>
#include <ostream>
#include <string>

using namespace jinja2;

TEST(MetadataTest, NoMetadata_EmtpyData)
{
    constexpr auto source = "Hello World!";

    Template tpl;
    auto parse_result = tpl.Load(source);
    EXPECT_FALSE(!parse_result);
    EXPECT_EQ(0, tpl.GetMetadata().value().GetSize());
    EXPECT_TRUE(tpl.GetMetadataRaw().value().metadata.empty());
    auto renderResult = tpl.RenderAsString({});
    EXPECT_FALSE(!renderResult);
    EXPECT_EQ("Hello World!", renderResult.value());
}

TEST(MetadataTest, Metadata_EmptyTag)
{
    constexpr auto source = R"({% meta %}{% endmeta %}Hello World!)";

    Template tpl;
    auto parse_result = tpl.Load(source);
    EXPECT_FALSE(!parse_result);
    auto metadataValue = tpl.GetMetadata();
    EXPECT_FALSE(!metadataValue);
    EXPECT_EQ(0, metadataValue.value().GetSize());
    EXPECT_TRUE(tpl.GetMetadataRaw().value().metadata.empty());
    std::cout << tpl.GetMetadataRaw().value().metadata << std::endl;
    auto renderResult = tpl.RenderAsString({});
    EXPECT_FALSE(!renderResult);
    EXPECT_EQ("Hello World!", renderResult.value());
}

TEST(MetadataTest, Metadata_TagWithSpaces)
{
    constexpr auto source = R"({% meta %}
{% endmeta %}Hello World!)";

    Template tpl;
    auto parse_result = tpl.Load(source);
    EXPECT_FALSE(!parse_result);
    auto metadataValue = tpl.GetMetadata();
    EXPECT_FALSE(!metadataValue);
    EXPECT_EQ(0, metadataValue.value().GetSize());
    EXPECT_TRUE(tpl.GetMetadataRaw().value().metadata.empty());
    auto renderResult = tpl.RenderAsString({});
    EXPECT_FALSE(!renderResult);
    EXPECT_EQ("Hello World!", renderResult.value());
}

TEST(MetadataTest, Metadata_Invalid)
{
    constexpr auto source = R"(
{% meta %}INVALID JSON
{% endmeta %}Hello World!)";

    Template tpl;
    auto parse_result = tpl.Load(source);
    EXPECT_FALSE(!parse_result);
    auto metadataValue = tpl.GetMetadata();
    EXPECT_TRUE(!metadataValue);
    auto error = metadataValue.error().ToString();
    std::cout << error << std::endl;
    EXPECT_THAT(error, testing::ContainsRegex("Error occurred during template metadata parsing. Error:"));
}

TEST(MetadataTest, Metadata_JsonData_Narrow)
{
    std::string json = R"({
    "stringValue": "Hello!",
    "subobject": {
        "intValue": 10,
        "array": [1, 2, 3, 4, 5]
    }
})";

    auto source = "{% meta %}" + json + "{% endmeta %}Hello World!";

    Template tpl;
    auto parse_result = tpl.Load(source);
    EXPECT_FALSE(!parse_result);
    auto metadataValue = tpl.GetMetadata();
    EXPECT_FALSE(!metadataValue);
    EXPECT_EQ(2, metadataValue.value().GetSize());

    auto& metadata = metadataValue.value();
    EXPECT_TRUE(metadata.HasValue("stringValue"));
    EXPECT_TRUE(metadata.HasValue("subobject"));
    EXPECT_EQ("Hello!", ToString(metadata["stringValue"]));
    auto subobjectVal = metadata["subobject"];
    const auto& subobject = subobjectVal.As<GenericMap>();
    EXPECT_EQ(10, subobject["intValue"].As<int64_t>());
    EXPECT_EQ(5, subobject["array"].As<GenericList>().GetSize().value());

    auto metadataRaw = tpl.GetMetadataRaw().value();
    EXPECT_FALSE(metadataRaw.metadata.empty());
    EXPECT_EQ("json", metadataRaw.metadataType);
    EXPECT_EQ(std::string_view(json.data(), json.size()), metadataRaw.metadata);
    std::cout << metadataRaw.metadata << std::endl;
    auto renderResult = tpl.RenderAsString({});
    EXPECT_FALSE(!renderResult);
    EXPECT_EQ("Hello World!", renderResult.value());
}

TEST(MetadataTest, Metadata_JsonData_Wide)
{
    std::wstring json = LR"({
    "stringValue": "Hello!",
    "subobject": {
        "intValue": 10,
        "array": [1, 2, 3, 4, 5]
    }
})";

    auto source = L"{% meta %}" + json + L"{% endmeta %}Hello World!";

    TemplateW tpl;
    auto parse_result = tpl.Load(source);
    EXPECT_FALSE(!parse_result);
    auto metadataValue = tpl.GetMetadata();
    EXPECT_FALSE(!metadataValue);
    EXPECT_EQ(2, metadataValue.value().GetSize());

    auto& metadata = metadataValue.value();
    EXPECT_TRUE(metadata.HasValue("stringValue"));
    EXPECT_TRUE(metadata.HasValue("subobject"));
    EXPECT_EQ("Hello!", ToString(metadata["stringValue"]));
    auto subobjectVal = metadata["subobject"];
    const auto& subobject = subobjectVal.As<GenericMap>();
    EXPECT_EQ(10, subobject["intValue"].As<int64_t>());
    EXPECT_EQ(5, subobject["array"].As<GenericList>().GetSize().value());

    auto metadataRaw = tpl.GetMetadataRaw().value();
    EXPECT_FALSE(metadataRaw.metadata.empty());
    EXPECT_EQ("json", metadataRaw.metadataType);
    EXPECT_EQ(std::wstring_view(json.data(), json.size()), metadataRaw.metadata);
    std::wcout << metadataRaw.metadata << std::endl;
    auto renderResult = tpl.RenderAsString({});
    EXPECT_FALSE(!renderResult);
    EXPECT_EQ(L"Hello World!", renderResult.value());
}

// The type comes from the template's settings and the block from its last successful Load: a
// reload with or without a block, and a failed one, leave what GetMetadataRaw reports consistent
// (docs/tasks/0515: Load no longer copies the type, nor moves an absent block)
TEST(MetadataTest, Metadata_FollowsReloads)
{
    TemplateEnv env;
    env.GetSettings().defaultMetadataType = "yaml";
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{% meta %}{\"a\": 1}{% endmeta %}x", "first.j2"));
    auto raw = tpl.GetMetadataRaw().value();
    EXPECT_EQ("yaml", raw.metadataType);
    EXPECT_EQ("{\"a\": 1}", raw.metadata);
    EXPECT_EQ("first.j2", raw.location.fileName);

    ASSERT_TRUE(!!tpl.Load("no block", "second.j2"));
    raw = tpl.GetMetadataRaw().value();
    EXPECT_EQ("yaml", raw.metadataType);
    EXPECT_TRUE(raw.metadata.empty());
    EXPECT_TRUE(raw.location.fileName.empty());
    EXPECT_EQ(0u, raw.location.line);

    ASSERT_TRUE(!!tpl.Load("{% meta %}{\"b\": 2}{% endmeta %}y", "third.j2"));
    ASSERT_FALSE(!!tpl.Load("{% meta %}{\"c\": 3}{% endmeta %}{{ broken", "fourth.j2"));
    raw = tpl.GetMetadataRaw().value();
    EXPECT_EQ("{\"b\": 2}", raw.metadata);
    EXPECT_EQ("third.j2", raw.location.fileName);
    EXPECT_EQ("y", tpl.RenderAsString({}).value());

    Template json;
    ASSERT_TRUE(!!json.Load("{% meta %}{\"d\": 4}{% endmeta %}"));
    EXPECT_EQ("json", json.GetMetadataRaw().value().metadataType);
    EXPECT_EQ(4, json.GetMetadata().value()["d"].As<int64_t>());
}
