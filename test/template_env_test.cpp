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

// Template::RenderTo with RenderOptions (docs/tasks/0357 B3): a render's own globals and poll
namespace
{
std::shared_ptr<const ValuesMap> GlobalsOf(ValuesMap globals)
{
    return std::make_shared<const ValuesMap>(std::move(globals));
}

struct PollStop
{
};
} // namespace

TEST(RenderOptionsTest, GlobalsReplaceTheEnvironmentsForTheRenderAndItsImports)
{
    TemplateEnv env;
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("lib.j2tpl", "{% macro who() %}{{ greeting }}{% endmacro %}");
    fs->AddFile("part.j2tpl", "<{{ greeting }}>");
    env.AddFilesystemHandler(std::string(), fs);
    env.AddGlobal("greeting", "env");
    env.AddGlobal("only_env", "kept?");
    auto tpl = env.FromString("{% import 'lib.j2tpl' as lib %}{{ greeting }} {{ lib.who() }} {% include 'part.j2tpl' %} {{ only_env is defined }} {{ shadowed }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();

    RenderOptions options;
    options.globals = GlobalsOf({ { "greeting", "mine" }, { "shadowed", "global" } });
    std::string out = "prefix:";
    auto result = tpl->RenderTo(out, { { "shadowed", "param" } }, options);
    ASSERT_TRUE(result) << result.error().ToString();
    // The params stay over the globals, as in an ordinary render
    EXPECT_EQ("prefix:mine mine <mine> False param", out);

    // The environment's globals are untouched: the next ordinary render sees them
    EXPECT_EQ("env env <env> True ", tpl->RenderAsString({}).value());
    // No globals in the options: the environment's
    out.clear();
    ASSERT_TRUE(tpl->RenderTo(out, {}, RenderOptions{}));
    EXPECT_EQ("env env <env> True ", out);
}

TEST(RenderOptionsTest, GlobalsWorkForWideTemplatesAndSinks)
{
    TemplateEnv env;
    auto tpl = env.FromString(L"{{ name }}{{ n + 1 }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    RenderOptions options;
    options.globals = GlobalsOf({ { "name", "w" }, { "n", 41 } });
    std::wstring out;
    ASSERT_TRUE(tpl->RenderTo(out, {}, options));
    EXPECT_EQ(L"w42", out);

    std::wstring collected;
    ContainerSink<std::wstring, wchar_t> sink(collected);
    ASSERT_TRUE(tpl->RenderTo(sink, {}, options));
    EXPECT_EQ(L"w42", collected);
}

TEST(RenderOptionsTest, AGlobalACallableKeepsOutlivesTheOptions)
{
    // A callable may keep a value of the globals after the render: the options share their map
    TemplateEnv env;
    Value kept;
    const UserCallable keep{ [&kept](const UserCallableParams& params) {
                                kept = params["v"];
                                return Value();
                            },
                             { ArgInfo{ "v" } } };
    auto tpl = env.FromString("{{ keep(items) }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    {
        RenderOptions options;
        options.globals = GlobalsOf({ { "items", ValuesList{ 1, 2, 3 } } });
        std::string out;
        auto result = tpl->RenderTo(out, { { "keep", keep } }, options);
        ASSERT_TRUE(result) << result.error().ToString();
    }
    ASSERT_TRUE(kept.IsList());
    auto join = env.FromString("{{ v|join(',') }}");
    ASSERT_TRUE(join) << join.error().ToString();
    EXPECT_EQ("1,2,3", join->RenderAsString({ { "v", kept } }).value());
}

TEST(RenderOptionsTest, PollRunsDuringALongRenderAndMayStopIt)
{
    TemplateEnv env;
    auto tpl = env.FromString("{% for i in range(20000) %}{{ i }}{% endfor %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();

    int polls = 0;
    RenderOptions options;
    options.poll = [&polls] { ++polls; };
    std::string out;
    ASSERT_TRUE(tpl->RenderTo(out, {}, options));
    // About one poll every 1024 steps, without any limit set
    EXPECT_GT(polls, 5);

    // No poll: none, and an ordinary render is not polled by an earlier one's
    polls = 0;
    out.clear();
    ASSERT_TRUE(tpl->RenderTo(out, {}, RenderOptions{}));
    EXPECT_EQ(0, polls);

    // A throwing poll stops the render with its exception, and the string keeps what it had
    options.poll = [&polls] {
        if (++polls == 3)
        {
            throw PollStop{};
        }
    };
    polls = 0;
    out = "kept";
    auto result = tpl->RenderTo(out, {}, options);
    ASSERT_FALSE(result);
    EXPECT_EQ(3, polls);
    EXPECT_EQ("kept", out);
    ASSERT_TRUE(result.error().GetException());
    EXPECT_THROW(std::rethrow_exception(result.error().GetException()), PollStop);
}

TEST(RenderOptionsTest, PollKeepsTheTemplatesLimits)
{
    TemplateEnv env;
    env.GetSettings().limits.maxRenderSteps = 5000;
    auto tpl = env.FromString("{% for i in range(20000) %}{{ i }}{% endfor %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    int polls = 0;
    RenderOptions options;
    options.poll = [&polls] { ++polls; };
    std::string out;
    auto result = tpl->RenderTo(out, {}, options);
    ASSERT_FALSE(result);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, result.error().GetCode()) << result.error().ToString();
    EXPECT_GT(polls, 0);
}

TEST(RenderOptionsTest, ARenderACallableStartsIsPolledToo)
{
    TemplateEnv env;
    auto inner = env.FromString("{% for i in range(5000) %}.{% endfor %}");
    ASSERT_TRUE(inner) << inner.error().ToString();
    int innerPolls = -1;
    int polls = 0;
    env.AddGlobal("nested", UserCallable{ [&](const UserCallableParams&) {
                                             const int before = polls;
                                             auto out = inner->RenderAsString({});
                                             innerPolls = polls - before;
                                             return Value(out ? out->size() : 0);
                                         },
                                          {} });
    auto tpl = env.FromString("{{ nested() }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    RenderOptions options;
    options.poll = [&polls] { ++polls; };
    std::string out;
    ASSERT_TRUE(tpl->RenderTo(out, {}, options));
    EXPECT_EQ("5000", out);
    EXPECT_GT(innerPolls, 0);
}
