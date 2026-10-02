#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <vector>

#include "gtest/gtest.h"

#include "jinja2cpp/reflected_value.h"
#include "jinja2cpp/template.h"
#include "jinja2cpp/template_env.h"
#include "test_tools.h"

using namespace jinja2;

static_assert(std::is_same_v<Template, BasicTemplate<char>>);
static_assert(std::is_same_v<TemplateW, BasicTemplate<wchar_t>>);
static_assert(std::is_same_v<ResultW<int>, Result<int, wchar_t>>);
static_assert(std::is_same_v<Result<int>, Result<int, char>>);
static_assert(std::is_same_v<decltype(std::declval<const Template&>().RenderAsString(ValuesMap{})), Result<std::string>>);
static_assert(std::is_same_v<decltype(std::declval<const TemplateW&>().RenderAsString(ValuesMap{})), ResultW<std::wstring>>);

TEST(TemplateApiTest, LoadAcceptsEveryStringForm)
{
    const char* literal = "{{ x }}";
    std::string str = "{{ x }}";
    std::string_view view = str;

    Template fromLiteral;
    ASSERT_TRUE(!!fromLiteral.Load(literal));
    EXPECT_EQ("1", fromLiteral.RenderAsString({ { "x", 1 } }).value());

    Template fromString;
    ASSERT_TRUE(!!fromString.Load(str, "named.j2tpl"));
    EXPECT_EQ("2", fromString.RenderAsString({ { "x", 2 } }).value());

    // The view need not be null-terminated
    Template fromView;
    ASSERT_TRUE(!!fromView.Load(view.substr(0, 7)));
    EXPECT_EQ("3", fromView.RenderAsString({ { "x", 3 } }).value());

    TemplateW wide;
    ASSERT_TRUE(!!wide.Load(std::wstring_view(L"{{ x }}!!", 7)));
    EXPECT_EQ(L"4", wide.RenderAsString({ { "x", 4 } }).value());

    std::istringstream stream("{{ x }}");
    Template fromStream;
    ASSERT_TRUE(!!fromStream.Load(stream));
    EXPECT_EQ("5", fromStream.RenderAsString({ { "x", 5 } }).value());
}

TEST(TemplateApiTest, RenderIsConst)
{
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{{ x }}"));
    const Template& ctpl = tpl;

    std::ostringstream os;
    ASSERT_TRUE(!!ctpl.Render(os, { { "x", "a" } }));
    EXPECT_EQ("a", os.str());
    EXPECT_EQ("b", ctpl.RenderAsString({ { "x", "b" } }).value());
    EXPECT_EQ("", ctpl.RenderAsString({}).value());
    EXPECT_TRUE(!!ctpl.GetMetadata());
    EXPECT_TRUE(!!ctpl.GetMetadataRaw());
}

TEST(TemplateApiTest, RenderWithGenericMapContext)
{
    TestStruct data;
    data.intValue = 10;
    data.strValue = "hello";
    auto reflected = Reflect(data);
    const auto& context = reflected.get<GenericMap>();

    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{{ intValue }} {{ strValue }} {{ missing is undefined }}"));
    EXPECT_EQ("10 hello True", tpl.RenderAsString(context).value());

    std::ostringstream os;
    ASSERT_TRUE(!!tpl.Render(os, context));
    EXPECT_EQ("10 hello True", os.str());

    TemplateW wtpl;
    ASSERT_TRUE(!!wtpl.Load(L"{{ intValue }} {{ strValue }}"));
    EXPECT_EQ(L"10 hello", wtpl.RenderAsString(context).value());
}

TEST(TemplateApiTest, GenericMapContextSeesEnvironmentGlobals)
{
    TemplateEnv env;
    env.AddGlobal("greeting", "hi");
    env.AddGlobal("intValue", 1);

    TestStruct data;
    data.intValue = 7;
    auto reflected = Reflect(data);

    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{{ greeting }} {{ intValue }}"));
    EXPECT_EQ("hi 7", tpl.RenderAsString(reflected.get<GenericMap>()).value());
}

TEST(TemplateApiTest, EqualityComparesTheSharedTemplate)
{
    Template a;
    ASSERT_TRUE(!!a.Load("x"));
    Template b = a;
    Template c;
    ASSERT_TRUE(!!c.Load("x"));
    EXPECT_TRUE(a == b);
    EXPECT_FALSE(a != b);
    EXPECT_TRUE(a != c);
}

// Several threads render one template (and the templates it includes and extends) at
// once. Run under -DJINJA2CPP_WITH_SANITIZERS=thread, which CI does, to check for races.
TEST(TemplateApiTest, ConcurrentRenderOfOneTemplate)
{
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("base.j2tpl", "<{% block body %}{% endblock %}>");
    fs->AddFile("item.j2tpl", "[{{ item | upper }}]");
    TemplateEnv env;
    env.AddFilesystemHandler(std::string(), fs);
    env.AddGlobal("sep", ",");

    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load(R"({% meta %}{"kind": "test"}{% endmeta %}{% extends "base.j2tpl" %}{% block body %})"
                           R"({% macro m(v) %}{{ v * 2 }}{% endmacro %})"
                           R"({% for item in items %}{% include "item.j2tpl" %}{{ m(loop.index) }}{{ sep if not loop.last }}{% endfor %})"
                           R"({% set ns = namespace(total=0) %}{% for i in items %}{% set ns.total = ns.total + 1 %}{% endfor %}={{ ns.total }})"
                           R"({% endblock %})"));
    const Template& shared = tpl;

    constexpr int threadCount = 4;
    constexpr int iterations = 50;
    std::vector<std::string> failures(threadCount);
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (int t = 0; t < threadCount; ++t)
    {
        threads.emplace_back([&shared, &failures, t] {
            ValuesList items;
            for (int n = 0; n <= t; ++n)
                items.emplace_back("t" + std::to_string(n));
            std::string expected = "<";
            for (int n = 0; n <= t; ++n)
                expected += "[T" + std::to_string(n) + "]" + std::to_string((n + 1) * 2) + (n == t ? "" : ",");
            expected += "=" + std::to_string(t + 1) + ">";

            for (int i = 0; i < iterations; ++i)
            {
                auto result = shared.RenderAsString({ { "items", items } });
                if (!result)
                    failures[t] = result.error().ToString();
                else if (result.value() != expected)
                    failures[t] = result.value() + " != " + expected;
                auto metadata = shared.GetMetadata();
                if (!metadata || !metadata.value().HasValue("kind"))
                    failures[t] = "metadata";
                if (!failures[t].empty())
                    return;
            }
        });
    }
    for (auto& th : threads)
        th.join();

    for (int t = 0; t < threadCount; ++t)
        EXPECT_EQ("", failures[t]) << "thread " << t;
}
