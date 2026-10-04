#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <memory>
#include <optional>
#include <utility>

using namespace jinja2;

// The environment's state lives behind one pointer, so adding to it does not change the public layout
static_assert(sizeof(TemplateEnv) == sizeof(std::shared_ptr<int>), "TemplateEnv is a pimpl");

TEST(TemplateEnvTest, FromString)
{
    TemplateEnv env;
    env.AddGlobal("who", "World");
    auto tpl = env.FromString("Hello {{ who }}!");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    EXPECT_EQ("Hello World!", tpl->RenderAsString({}).value());
}

TEST(TemplateEnvTest, FromStringWide)
{
    TemplateEnv env;
    env.AddGlobal("who", "World");
    auto tpl = env.FromString(L"Hello {{ who }}!");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    EXPECT_EQ(L"Hello World!", tpl->RenderAsString({}).value());
}

TEST(TemplateEnvTest, FromStringIncludesAndReportsErrorsUnderItsName)
{
    TemplateEnv env;
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("part.j2tpl", "part");
    env.AddFilesystemHandler(std::string(), fs);

    auto tpl = env.FromString("[{% include 'part.j2tpl' %}]");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    EXPECT_EQ("[part]", tpl->RenderAsString({}).value());

    auto bad = env.FromString("{{ x", "inline.j2tpl");
    ASSERT_FALSE(bad);
    EXPECT_EQ("inline.j2tpl", bad.error().GetErrorLocation().fileName);
}

TEST(TemplateEnvTest, TemplateOutlivesEnvironment)
{
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("main.j2tpl", "{{ greeting }}, {% include 'part.j2tpl' %}{{ 'x'|twice }}");
    fs->AddFile("part.j2tpl", "{{ greeting|upper }} ");

    std::optional<Template> tpl;
    {
        TemplateEnv env;
        env.AddFilesystemHandler(std::string(), fs);
        env.AddGlobal("greeting", "hi");
        env.AddFilter("twice", MakeCallable([](const std::string& s) { return s + s; }, ArgInfo{ "s" }));
        auto loaded = env.LoadTemplate("main.j2tpl");
        ASSERT_TRUE(loaded) << loaded.error().ToString();
        tpl = std::move(loaded.value());
    }
    // The include is loaded at render time, through the state the template keeps alive
    auto result = tpl->RenderAsString({});
    ASSERT_TRUE(result) << result.error().ToString();
    EXPECT_EQ("hi, HI xx", result.value());
}

TEST(TemplateEnvTest, CachedTemplatesDoNotKeepEnvironmentAlive)
{
    std::weak_ptr<MemoryFileSystem> weakFs;
    {
        auto fs = std::make_shared<MemoryFileSystem>();
        weakFs = fs;
        fs->AddFile("main.j2tpl", "{% include 'part.j2tpl' %}");
        fs->AddFile("part.j2tpl", "part");
        TemplateEnv env;
        env.AddFilesystemHandler(std::string(), std::move(fs));
        auto tpl = env.LoadTemplate("main.j2tpl");
        ASSERT_TRUE(tpl);
        EXPECT_EQ("part", tpl->RenderAsString({}).value());
    }
    // The environment's state (which owns the handler) is gone with the environment and its templates
    EXPECT_TRUE(weakFs.expired());
}

TEST(TemplateEnvTest, CacheServesLoadedTemplate)
{
    TemplateEnv env;
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("main.j2tpl", "first");
    env.AddFilesystemHandler(std::string(), fs);
    env.GetSettings().autoReload = false;
    auto first = env.LoadTemplate("main.j2tpl");
    ASSERT_TRUE(first);
    fs->AddFile("main.j2tpl", "second");
    auto second = env.LoadTemplate("main.j2tpl");
    ASSERT_TRUE(second);
    EXPECT_EQ("first", second->RenderAsString({}).value());
}

TEST(TemplateEnvTest, ApplyGlobalsSeesConstGlobals)
{
    TemplateEnv env;
    env.AddGlobal("a", 1);
    env.AddGlobal("b", 2);
    size_t count = 0;
    env.ApplyGlobals([&count](const ValuesMap& globals) { count = globals.size(); });
    EXPECT_EQ(2U, count);
}

TEST(TemplateEnvTest, SettingsEquality)
{
    Settings a;
    Settings b;
    EXPECT_TRUE(a == b);
    b.extensions.loopControls = true;
    EXPECT_TRUE(a != b);
    b = a;
    b.defaultMetadataType = "yaml";
    EXPECT_TRUE(a != b);
    b = a;
    b.undefinedPolicy = UndefinedPolicy::Strict;
    EXPECT_TRUE(a != b);
    b = a;
    b.templateLookup = TemplateLookup::EveryUse;
    EXPECT_TRUE(a != b);
    b = a;
    b.finalize = UserCallable([](const UserCallableParams& p) { return p["v"]; }, { ArgInfo{ "v" } });
    EXPECT_TRUE(a != b);
    a.finalize = b.finalize;
    EXPECT_TRUE(a == b);
}
