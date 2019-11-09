#include "jinja2cpp/template.h"
#include "test_tools.h"

#include <nlohmann/json.hpp>

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
    EXPECT_EQ("noname.j2tpl:2:1: error: Error occurred during template metadata parsing. Error: Invalid value.\n", error);
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
    EXPECT_EQ("Hello!", AsString(metadata["stringValue"]));
    auto subobjectVal = metadata["subobject"];
    const auto& subobject = subobjectVal.get<GenericMap>();
    EXPECT_EQ(10, subobject["intValue"].get<int64_t>());
    EXPECT_EQ(5, subobject["array"].get<GenericList>().GetSize().value());

    auto metadataRaw = tpl.GetMetadataRaw().value();
    EXPECT_FALSE(metadataRaw.metadata.empty());
    EXPECT_EQ("json", metadataRaw.metadataType);
    EXPECT_EQ(json, metadataRaw.metadata);
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
    EXPECT_EQ("Hello!", AsString(metadata["stringValue"]));
    auto subobjectVal = metadata["subobject"];
    const auto& subobject = subobjectVal.get<GenericMap>();
    EXPECT_EQ(10, subobject["intValue"].get<int64_t>());
    EXPECT_EQ(5, subobject["array"].get<GenericList>().GetSize().value());

    auto metadataRaw = tpl.GetMetadataRaw().value();
    EXPECT_FALSE(metadataRaw.metadata.empty());
    EXPECT_EQ("json", metadataRaw.metadataType);
    EXPECT_EQ(json, metadataRaw.metadata);
    std::wcout << metadataRaw.metadata << std::endl;
    auto renderResult = tpl.RenderAsString({});
    EXPECT_FALSE(!renderResult);
    EXPECT_EQ(L"Hello World!", renderResult.value());
}
