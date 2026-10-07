#include "gtest/gtest.h"
#include "test_tools.h"

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <atomic>
#include <cstddef>
#include <memory>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

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

// The globals are converted once per change and kept between renders (docs/tasks/0139)
TEST(TemplateApiTest, GlobalsChangedBetweenRenders)
{
    TemplateEnv env;
    env.AddGlobal("g", "a");
    TemplateEnv other;
    other.AddGlobal("g", "other");

    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{{ g }}"));
    Template otherTpl(&other);
    ASSERT_TRUE(!!otherTpl.Load("{{ g }}"));
    EXPECT_EQ("a", tpl.RenderAsString({}).value());
    EXPECT_EQ("other", otherTpl.RenderAsString({}).value());
    EXPECT_EQ("a", tpl.RenderAsString({}).value());
    env.AddGlobal("g", "b");
    EXPECT_EQ("b", tpl.RenderAsString({}).value());
    env.RemoveGlobal("g");
    EXPECT_EQ("", tpl.RenderAsString({}).value());
    EXPECT_EQ("other", otherTpl.RenderAsString({}).value());
}

// A global the template changes in place is changed for that render only
TEST(TemplateApiTest, GlobalChangedInPlaceForOneRender)
{
    TemplateEnv env;
    env.AddGlobal("l", ValuesList{ 1 });
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{{ l.append(2) }}{{ l }}|{% include 'x' ignore missing %}{{ l }}"));
    EXPECT_EQ("None[1, 2]|[1, 2]", tpl.RenderAsString({}).value());
    EXPECT_EQ("None[1, 2]|[1, 2]", tpl.RenderAsString({}).value());
}

// A global changed in place in an include, a block or an imported macro is seen by the rest of the render
TEST(TemplateApiTest, GlobalChangedInPlaceSeenEverywhere)
{
    TemplateEnv env;
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("inc", "{{ gl.append(3) }}");
    fs->AddFile("lib", "{% macro m() %}{{ gl.append(4) }}{% endmacro %}");
    env.AddFilesystemHandler(std::string(), fs);
    env.AddGlobal("gl", ValuesList{ 1, 2 });

    auto render = [&env](const std::string& source) {
        Template tpl(&env);
        EXPECT_TRUE(!!tpl.Load(source));
        return tpl.RenderAsString({}).value();
    };
    for (int i = 0; i < 2; ++i)
    {
        EXPECT_EQ("[1, 2]None[1, 2, 3]None[1, 2, 3, 3]", render("{% for i in [1, 2] %}{{ gl }}{% include 'inc' %}{% endfor %}{{ gl }}"));
        EXPECT_EQ("[1, 2]None[1, 2, 4]None", render("{% import 'lib' as l %}{% for i in [1, 2] %}{{ gl }}{{ l.m() }}{% endfor %}"));
        EXPECT_EQ("[1, 2]NONE[1, 2, 1]NONE[1, 2, 1, 2]", render("{% for i in [1, 2] %}{{ gl }}{% filter upper %}{{ gl.append(i) }}{% endfilter %}{% endfor %}{{ gl }}"));
    }
}

// A global replaced while other threads render: each render sees one state or the other,
// and the values it converted stay alive
TEST(TemplateApiTest, GlobalsChangedWhileRendering)
{
    TemplateEnv env;
    env.AddGlobal("g", std::string(64, 'a'));
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{% for i in range(20) %}{{ g }}{% endfor %}"));

    std::atomic<bool> stop{ false };
    std::atomic<int> bad{ 0 };
    std::vector<std::thread> threads;
    threads.reserve(3);
    for (int t = 0; t < 3; ++t)
    {
        threads.emplace_back([&] {
            while (!stop)
            {
                auto out = tpl.RenderAsString({}).value();
                constexpr size_t size = size_t{ 64 } * 20;
                if (out != std::string(size, 'a') && out != std::string(size, 'b'))
                {
                    ++bad;
                }
            }
        });
    }
    for (int i = 0; i < 2000; ++i)
    {
        env.AddGlobal("g", std::string(64, i % 2 ? 'a' : 'b'));
    }
    stop = true;
    for (auto& thread : threads)
    {
        thread.join();
    }
    EXPECT_EQ(0, bad.load());
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

// A failed reload keeps the previous template: its tree points into its source, which must
// stay alive (task 0134; the source is long enough to be outside the small-string buffer)
TEST(TemplateApiTest, FailedReloadKeepsPreviousTemplate)
{
    const std::string text(200, 'A');
    Template tpl;
    ASSERT_TRUE(!!tpl.Load(text + "{{ x }}", "first.j2tpl"));
    Template copy = tpl;

    auto reload = tpl.Load("{{ broken", "second.j2tpl");
    ASSERT_FALSE(!!reload);
    EXPECT_EQ("second.j2tpl", reload.error().GetErrorLocation().fileName);

    EXPECT_EQ(text + "1", tpl.RenderAsString({ { "x", 1 } }).value());
    EXPECT_EQ(text + "2", copy.RenderAsString({ { "x", 2 } }).value());

    TemplateW wide;
    const std::wstring wideText(200, L'A');
    ASSERT_TRUE(!!wide.Load(wideText + L"{{ x }}"));
    ASSERT_FALSE(!!wide.Load(L"{% if %}"));
    EXPECT_EQ(wideText + L"3", wide.RenderAsString({ { "x", 3 } }).value());

    // A template whose first Load fails stays unloaded
    Template never;
    ASSERT_FALSE(!!never.Load("{{ broken"));
    auto unloaded = never.RenderAsString({});
    ASSERT_FALSE(!!unloaded);
    EXPECT_EQ(ErrorCode::TemplateNotParsed, unloaded.error().GetCode());
}

TEST(TemplateApiTest, MetadataStaysValidAcrossCalls)
{
    Template tpl;
    ASSERT_TRUE(!!tpl.Load(R"({% meta %}{"name": "first", "list": [1, 2]}{% endmeta %}x)"));
    auto first = tpl.GetMetadata();
    auto second = tpl.GetMetadata();
    ASSERT_TRUE(!!first);
    ASSERT_TRUE(!!second);
    // The first map must still be readable after the second call
    EXPECT_EQ("first", AsString(first.value()["name"]));
    EXPECT_EQ(std::optional<size_t>(2), first.value()["list"].get<GenericList>().GetSize());
    EXPECT_EQ("first", AsString(second.value()["name"]));

    ASSERT_TRUE(!!tpl.Load(R"({% meta %}{"name": "second"}{% endmeta %}y)"));
    EXPECT_EQ("second", AsString(tpl.GetMetadata().value()["name"]));
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
                           R"(;{{ items | map('upper') | join }};{{ items | select('equalto', 't0') | join }})"
                           R"({% endblock %})"));
    const Template& shared = tpl;

    static constexpr std::size_t threadCount = 4;
    static constexpr int iterations = 50;
    std::vector<std::string> failures(threadCount);
    std::vector<std::thread> threads;
    threads.reserve(threadCount);
    for (std::size_t t = 0; t < threadCount; ++t)
    {
        threads.emplace_back([&shared, &failures, t] {
            ValuesList items;
            for (std::size_t n = 0; n <= t; ++n)
            {
                items.emplace_back("t" + std::to_string(n));
            }
            std::string expected = "<";
            for (std::size_t n = 0; n <= t; ++n)
            {
                expected += "[T" + std::to_string(n) + "]" + std::to_string((n + 1) * 2) + (n == t ? "" : ",");
            }
            expected += "=" + std::to_string(t + 1) + ";";
            for (std::size_t n = 0; n <= t; ++n)
            {
                expected += "T" + std::to_string(n);
            }
            expected += ";t0>";

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

    for (std::size_t t = 0; t < threadCount; ++t)
    {
        EXPECT_EQ("", failures[t]) << "thread " << t;
    }
}
