#include "big_int_toy_backend.h"

#include <jinja2cpp/big_int.h>
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/prompt.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <atomic>
#include <cstddef>
#include <exception>
#include <memory>
#include <optional>
#include <string>
#include <thread>
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

// Settings snapshots (docs/tasks/0516): SetSettings and ModifySettings take effect for the templates made after them,
// the templates made before keep theirs, and edits through the mutable GetSettings still count
namespace
{
constexpr const char* TrimSource = "{% if true %}\nx{% endif %}";

std::string RenderTrimSource(TemplateEnv& env)
{
    Template tpl(&env);
    auto loaded = tpl.Load(TrimSource);
    EXPECT_TRUE(loaded) << loaded.error().ToString();
    return tpl.RenderAsString({}).value();
}

Settings Trimming(bool trim)
{
    Settings settings;
    settings.trimBlocks = trim;
    return settings;
}
} // namespace

TEST(TemplateEnvTest, SetSettingsAppliesToTheTemplatesMadeAfterIt)
{
    TemplateEnv env;
    Template before(&env);
    ASSERT_TRUE(before.Load(TrimSource));
    EXPECT_EQ("\nx", RenderTrimSource(env));
    env.SetSettings(Trimming(true));
    EXPECT_TRUE(std::as_const(env).GetSettings().trimBlocks);
    EXPECT_EQ("x", RenderTrimSource(env));
    EXPECT_EQ("x", RenderTrimSource(env));
    EXPECT_EQ("\nx", before.RenderAsString({}).value());
    env.SetSettings(Trimming(false));
    EXPECT_EQ("\nx", RenderTrimSource(env));
}

TEST(TemplateEnvTest, ModifySettingsEditsACopyAndKeepsTheSettingsWhenItThrows)
{
    TemplateEnv env;
    EXPECT_EQ("\nx", RenderTrimSource(env));
    env.ModifySettings([&env](Settings& settings) {
        // The environment is readable from the callback, and still has the old settings
        EXPECT_FALSE(std::as_const(env).GetSettings().trimBlocks);
        settings.trimBlocks = true;
    });
    EXPECT_TRUE(std::as_const(env).GetSettings().trimBlocks);
    EXPECT_EQ("x", RenderTrimSource(env));

    struct Failure
    {
    };
    EXPECT_THROW(env.ModifySettings([](Settings& settings) {
        settings.trimBlocks = false;
        throw Failure();
    }),
                 Failure);
    EXPECT_TRUE(std::as_const(env).GetSettings().trimBlocks);
    EXPECT_EQ("x", RenderTrimSource(env));
}

TEST(TemplateEnvTest, EditsThroughTheMutableGetSettingsApplyToTheNextTemplate)
{
    TemplateEnv env;
    env.SetSettings(Trimming(false));
    EXPECT_EQ("\nx", RenderTrimSource(env));
    Settings& settings = env.GetSettings();
    settings.trimBlocks = true;
    EXPECT_EQ("x", RenderTrimSource(env));
    // An edit after the reference was handed out, without asking for it again
    settings.trimBlocks = false;
    EXPECT_EQ("\nx", RenderTrimSource(env));
    env.ModifySettings([](Settings& changed) { changed.trimBlocks = true; });
    EXPECT_EQ("x", RenderTrimSource(env));
    settings.trimBlocks = false;
    EXPECT_EQ("\nx", RenderTrimSource(env));
}

TEST(TemplateEnvTest, TemplatesMadeWhileTheSettingsChangeSeeTheOldOrTheNewOnes)
{
    TemplateEnv env;
    std::atomic<bool> done{ false };
    std::atomic<int> odd{ 0 };
    std::thread loader([&] {
        for (int idx = 0; idx < 200 || !done; ++idx)
        {
            Template tpl(&env);
            if (!tpl.Load(TrimSource))
            {
                ++odd;
                continue;
            }
            const auto text = tpl.RenderAsString({});
            if (!text || (*text != "x" && *text != "\nx"))
            {
                ++odd;
            }
        }
    });
    for (int idx = 0; idx != 200; ++idx)
    {
        if (idx % 2 != 0)
        {
            env.SetSettings(Trimming(true));
        }
        else
        {
            env.ModifySettings([](Settings& settings) { settings.trimBlocks = false; });
        }
    }
    done = true;
    loader.join();
    EXPECT_EQ(0, odd.load());
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

TEST(RenderOptionsTest, AGlobalPastTheBigIntCapFailsTheRenderAsAParamDoes)
{
    TemplateEnv env;
    BigInt huge;
    ASSERT_TRUE(BigInt::TryParse(std::string(400000, '7'), huge));
    auto tpl = env.FromString("{{ n }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    std::string out;
    auto asParam = tpl->RenderTo(out, { { "n", huge } }, RenderOptions{});
    ASSERT_FALSE(asParam);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, asParam.error().GetCode()) << asParam.error().ToString();

    RenderOptions options;
    options.globals = GlobalsOf({ { "n", huge } });
    // Threw out of RenderTo before the fix: gtest reports an escaping exception as a failure
    auto asGlobal = tpl->RenderTo(out, {}, options);
    ASSERT_FALSE(asGlobal);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, asGlobal.error().GetCode()) << asGlobal.error().ToString();
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

TEST(RenderOptionsTest, ModulePollRunsDuringItsRenderAndItsMacroCalls)
{
    // Template::MakeModule with RenderOptions (docs/tasks/0440): the poll runs during the top level's render and,
    // kept by the module, during each call of its macros
    TemplateEnv env;
    auto tpl = env.FromString("{% for i in range(20000) %}{% endfor %}{% macro m(n) %}{% for i in range(n) %}{{ i }}{% endfor %}{% endmacro %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    int polls = 0;
    bool stop = false;
    RenderOptions options;
    options.poll = [&polls, &stop] {
        ++polls;
        if (stop)
        {
            throw PollStop{};
        }
    };
    auto module = tpl->MakeModule({}, options);
    ASSERT_TRUE(module) << module.error().ToString();
    EXPECT_GT(polls, 5);

    const auto& macro = module->exports["m"].As<UserCallable>();
    UserCallableParams params;
    params.extraPosArgs = ValuesList{ 20000 };
    polls = 0;
    EXPECT_NO_THROW(macro.callable(params));
    EXPECT_GT(polls, 5);

    // A throwing poll stops the call, which fails with its exception
    stop = true;
    polls = 0;
    try
    {
        macro.callable(params);
        FAIL() << "the call was not stopped";
    }
    catch (const BadResultAccess<char>& failure)
    {
        EXPECT_EQ(1, polls);
        ASSERT_TRUE(failure.error().GetException());
        EXPECT_THROW(std::rethrow_exception(failure.error().GetException()), PollStop);
    }
    // The module works again once the poll lets it
    stop = false;
    params.extraPosArgs = ValuesList{ 3 };
    EXPECT_EQ("012", macro.callable(params).AsString());

    // Without options nothing polls the module or its calls
    auto plain = tpl->MakeModule();
    ASSERT_TRUE(plain) << plain.error().ToString();
    polls = 0;
    stop = true;
    params.extraPosArgs = ValuesList{ 20000 };
    EXPECT_NO_THROW(plain->exports["m"].As<UserCallable>().callable(params));
    EXPECT_EQ(0, polls);
}

TEST(RenderOptionsTest, WideModulePollStopsAMacroCall)
{
    TemplateEnv env;
    TemplateW tpl(&env);
    ASSERT_TRUE(tpl.Load(L"{% macro m(n) %}{% for i in range(n) %}{{ i }}{% endfor %}{% endmacro %}").has_value());
    int polls = 0;
    RenderOptions options;
    options.poll = [&polls] {
        if (++polls == 2)
        {
            throw PollStop{};
        }
    };
    auto module = tpl.MakeModule({}, options);
    ASSERT_TRUE(module) << module.error().ToString();
    UserCallableParams params;
    params.extraPosArgs = ValuesList{ 20000 };
    try
    {
        module->exports["m"].As<UserCallable>().callable(params);
        FAIL() << "the call was not stopped";
    }
    catch (const BadResultAccess<wchar_t>& failure)
    {
        ASSERT_TRUE(failure.error().GetException());
        EXPECT_THROW(std::rethrow_exception(failure.error().GetException()), PollStop);
    }
}

TEST(RenderOptionsTest, ModuleGlobalsReplaceTheEnvironments)
{
    TemplateEnv env;
    env.AddGlobal("site", std::string("env"));
    env.AddGlobal("only_env", 1);
    auto tpl = env.FromString("{% set top = site %}{% macro m() %}{{ site }} {{ only_env is defined }} {{ arg }}{% endmacro %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    RenderOptions options;
    options.globals = std::make_shared<const ValuesMap>(ValuesMap{ { "site", std::string("host") }, { "arg", std::string("global") } });
    // The params are on top of the globals
    auto module = tpl->MakeModule({ { "arg", std::string("param") } }, options);
    ASSERT_TRUE(module) << module.error().ToString();
    EXPECT_EQ("host", module->exports["top"].AsString());
    options.globals.reset();
    EXPECT_EQ("host False param", module->exports["m"].As<UserCallable>().callable(UserCallableParams{}).AsString());

    // Null globals keep the environment's
    auto envModule = tpl->MakeModule({}, RenderOptions{});
    ASSERT_TRUE(envModule) << envModule.error().ToString();
    EXPECT_EQ("env", envModule->exports["top"].AsString());
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

TEST(RenderOptionsTest, PollReachesTheItemsALoopFilterSkips)
{
    // A template loaded without limits: the items the loop's filter drops render nothing, and are polled all the same
    TemplateEnv env;
    auto tpl = env.FromString("{% for i in range(20000) if false %}{{ i }}{% endfor %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    int polls = 0;
    RenderOptions options;
    options.poll = [&polls] {
        if (++polls == 3)
        {
            throw PollStop{};
        }
    };
    std::string out;
    auto result = tpl->RenderTo(out, {}, options);
    ASSERT_FALSE(result);
    EXPECT_EQ(3, polls);
    ASSERT_TRUE(result.error().GetException());
    EXPECT_THROW(std::rethrow_exception(result.error().GetException()), PollStop);
}

TEST(RenderOptionsTest, PollCountsStepsWithACustomBigIntBackendAndNoLimits)
{
    // A big-integer backend alone installs the template's limits scope, with no step limit to count against
    TemplateEnv env;
    ASSERT_TRUE(env.SetBigIntBackend(big_int_toy::Backend()));
    auto tpl = env.FromString("{% for i in range(20000) %}{{ i }}{% endfor %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    int polls = 0;
    RenderOptions options;
    options.poll = [&polls] { ++polls; };
    std::string out;
    auto result = tpl->RenderTo(out, {}, options);
    ASSERT_TRUE(result) << result.error().ToString();
    EXPECT_GT(polls, 5);
}

class RenderOptionsNestedTest : public ::testing::TestWithParam<bool>
{
};

TEST_P(RenderOptionsNestedTest, ARenderACallableStartsIsPolledToo)
{
    TemplateEnv env;
    // With a custom big-integer backend, the inner template installs a limits scope that sets no limit
    if (GetParam())
    {
        ASSERT_TRUE(env.SetBigIntBackend(big_int_toy::Backend()));
    }
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

INSTANTIATE_TEST_SUITE_P(BigIntBackend, RenderOptionsNestedTest, ::testing::Bool());

// Filter policies (docs/tasks/0435): Settings::filterPolicies and RenderOptions::filterPolicies
namespace
{
std::shared_ptr<const FilterPolicies> PoliciesOf(FilterPolicies policies)
{
    return std::make_shared<const FilterPolicies>(std::move(policies));
}

// What a Python `json.dumps_function` would see: the value and, only when given, the indent
UserCallable RecordingDumps()
{
    auto dumps = [](const UserCallableParams& params) -> Value {
        const Value indent = params["indent"];
        const Value obj = params["obj"];
        // A wide template passes its strings wide; these are ASCII
        std::string text = "?";
        if (const auto* narrow = obj.GetIf<std::string_view>())
        {
            text = std::string(*narrow);
        }
        else if (const auto* wide = obj.GetIf<std::wstring_view>())
        {
            text.clear();
            for (const wchar_t ch : *wide)
            {
                text.push_back(static_cast<char>(ch));
            }
        }
        else if (const auto* owned = obj.GetIf<std::string>())
        {
            text = *owned;
        }
        else if (const auto* wideOwned = obj.GetIf<std::wstring>())
        {
            text.clear();
            for (const wchar_t ch : *wideOwned)
            {
                text.push_back(static_cast<char>(ch));
            }
        }
        const std::string given = indent.IsString() ? "indent=" + indent.AsString() : "indent given";
        return text + "/" + (indent.IsNone() ? "no indent" : given);
    };
    return UserCallable(dumps, { ArgInfo{ "obj", true }, ArgInfo{ "indent", false, Value() } });
}
} // namespace

TEST(RenderOptionsTest, FilterPoliciesReplaceTheTemplatesForTheRenderItsIncludesAndImports)
{
    TemplateEnv env;
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("lib.j2tpl", "{% macro cut() %}{{ 'abcdefghijk'|truncate(10) }}{% endmacro %}");
    fs->AddFile("part.j2tpl", "<{{ 'abcdefghijk'|truncate(10) }}>");
    env.AddFilesystemHandler(std::string(), fs);
    auto tpl = env.FromString("{{ 'abcdefghijk'|truncate(10) }} {% import 'lib.j2tpl' as lib %}{{ lib.cut() }} {% include 'part.j2tpl' %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    // Jinja2's leeway of 5 keeps a string of 11
    EXPECT_EQ("abcdefghijk abcdefghijk <abcdefghijk>", tpl->RenderAsString({}).value());

    FilterPolicies strict;
    strict.truncateLeeway = 0;
    RenderOptions options;
    options.filterPolicies = PoliciesOf(strict);
    std::string out;
    ASSERT_TRUE(tpl->RenderTo(out, {}, options));
    EXPECT_EQ("abcdefg... abcdefg... <abcdefg...>", out);
    // Only for that render
    EXPECT_EQ("abcdefghijk abcdefghijk <abcdefghijk>", tpl->RenderAsString({}).value());
    // The argument still wins
    auto argument = env.FromString("{{ 'abcdefghijk'|truncate(10, leeway=1) }}");
    ASSERT_TRUE(argument);
    out.clear();
    ASSERT_TRUE(argument->RenderTo(out, {}, options));
    EXPECT_EQ("abcdefghijk", out);
}

TEST(RenderOptionsTest, NoFilterPoliciesKeepTheTemplatesOwn)
{
    TemplateEnv env;
    env.GetSettings().filterPolicies.truncateLeeway = 0;
    env.GetSettings().filterPolicies.urlizeRel = "";
    auto tpl = env.FromString("{{ 'abcdefghijk'|truncate(10) }} {{ 'http://example.com'|urlize }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    const std::string own = "abcdefg... <a href=\"http://example.com\">http://example.com</a>";
    EXPECT_EQ(own, tpl->RenderAsString({}).value());
    std::string out;
    ASSERT_TRUE(tpl->RenderTo(out, {}, RenderOptions{}));
    EXPECT_EQ(own, out);

    // A template made with other settings keeps them
    FilterPolicies defaults;
    RenderOptions options;
    options.filterPolicies = PoliciesOf(defaults);
    out.clear();
    ASSERT_TRUE(tpl->RenderTo(out, {}, options));
    EXPECT_EQ("abcdefghijk <a href=\"http://example.com\" rel=\"noopener\">http://example.com</a>", out);
}

// The macros of a module keep the policies of MakeModule, or take a call's own (docs/tasks/0557)
TEST(RenderOptionsTest, ModuleMacrosUseTheModulesOrTheCallsFilterPolicies)
{
    TemplateEnv env;
    auto fs = std::make_shared<MemoryFileSystem>();
    fs->AddFile("lib.j2tpl", "{% macro cut() %}{{ 'abcdefghijk'|truncate(10) }}{% endmacro %}");
    env.AddFilesystemHandler(std::string(), fs);
    auto tpl = env.FromString("{{ 'abcdefghijk'|truncate(10) }}{% import 'lib.j2tpl' as lib %}"
                              "{% macro cut() %}{{ 'abcdefghijk'|truncate(10) }}/{{ lib.cut() }}{% endmacro %}{% set d = {'f': cut} %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    FilterPolicies strict;
    strict.truncateLeeway = 0;
    const auto strictPolicies = PoliciesOf(strict);

    // Without options the template's own, for the call too
    auto plain = tpl->MakeModule({}).value();
    EXPECT_EQ("abcdefghijk", plain.body);
    const auto& plainCut = plain.exports["cut"].As<UserCallable>();
    EXPECT_EQ("abcdefghijk/abcdefghijk", plainCut.callable(UserCallableParams{}).AsString());
    ModuleCallOptions callOptions;
    callOptions.filterPolicies = strictPolicies;
    // A call's own, for that call only, also in the module the macro imported
    EXPECT_EQ("abcdefg.../abcdefg...", CallModuleMacro(plainCut, UserCallableParams{}, callOptions).AsString());
    EXPECT_EQ("abcdefghijk/abcdefghijk", plainCut.callable(UserCallableParams{}).AsString());
    // A macro inside an exported dict is one too
    const auto& inDict = plain.exports["d"].As<ValuesMap>().at("f").As<UserCallable>();
    EXPECT_EQ("abcdefg.../abcdefg...", CallModuleMacro(inDict, UserCallableParams{}, callOptions).AsString());
    // Null keeps the module's
    EXPECT_EQ("abcdefghijk/abcdefghijk", CallModuleMacro(plainCut, UserCallableParams{}, ModuleCallOptions{}).AsString());

    // The module's own, for its top level and its calls
    RenderOptions options;
    options.filterPolicies = strictPolicies;
    auto strictModule = tpl->MakeModule({}, options).value();
    EXPECT_EQ("abcdefg...", strictModule.body);
    const auto& strictCut = strictModule.exports["cut"].As<UserCallable>();
    EXPECT_EQ("abcdefg.../abcdefg...", strictCut.callable(UserCallableParams{}).AsString());
    callOptions.filterPolicies = PoliciesOf(FilterPolicies{});
    EXPECT_EQ("abcdefghijk/abcdefghijk", CallModuleMacro(strictCut, UserCallableParams{}, callOptions).AsString());
    EXPECT_EQ("abcdefg.../abcdefg...", strictCut.callable(UserCallableParams{}).AsString());

    // Another callable is called as it is
    const UserCallable other([](const UserCallableParams&) -> Value { return std::string("other"); }, {});
    EXPECT_EQ("other", CallModuleMacro(other, UserCallableParams{}, callOptions).AsString());
}

TEST(RenderOptionsTest, WideModuleMacrosTakeTheCallsFilterPolicies)
{
    TemplateEnv env;
    TemplateW tpl(&env);
    ASSERT_TRUE(tpl.Load(L"{% macro cut() %}{{ 'abcdefghijk'|truncate(10) }}{% endmacro %}"));
    auto module = tpl.MakeModule({}).value();
    const auto& cut = module.exports["cut"].As<UserCallable>();
    FilterPolicies strict;
    strict.truncateLeeway = 0;
    ModuleCallOptions callOptions;
    callOptions.filterPolicies = PoliciesOf(strict);
    EXPECT_EQ(L"abcdefg...", CallModuleMacro(cut, UserCallableParams{}, callOptions).AsWString());
    EXPECT_EQ(L"abcdefghijk", cut.callable(UserCallableParams{}).AsWString());
}

TEST(RenderOptionsTest, UrlizeAndTojsonReadThePolicies)
{
    TemplateEnv env;
    auto tpl = env.FromString("{{ 'http://example.com ssh:a'|urlize }}|{{ {'b': 1, 'a': 'é'}|tojson }}|{{ [1]|tojson(1) }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    FilterPolicies policies;
    policies.urlizeRel = "x y";
    policies.urlizeTarget = "_blank";
    policies.urlizeExtraSchemes = { "ssh:" };
    policies.jsonSortKeys = false;
    policies.jsonEnsureAscii = false;
    policies.jsonSeparators = std::make_pair(";", "=");
    policies.jsonIndent = "\t";
    RenderOptions options;
    options.filterPolicies = PoliciesOf(policies);
    std::string out;
    ASSERT_TRUE(tpl->RenderTo(out, {}, options));
    EXPECT_EQ("<a href=\"http://example.com\" rel=\"x y\" target=\"_blank\">http://example.com</a> <a href=\"ssh:a\" rel=\"x y\" target=\"_blank\">ssh:a</a>"
              "|{\n\t\"b\"=1;\n\t\"a\"=\"é\"\n}|[\n 1\n]",
              out);
}

TEST(RenderOptionsTest, AnInvalidPolicySchemeAndANegativeLeewayFailTheFilter)
{
    TemplateEnv env;
    auto urlize = env.FromString("{{ 'ssh:a'|urlize }}");
    auto truncate = env.FromString("{{ 'abc'|truncate(10) }}");
    ASSERT_TRUE(urlize);
    ASSERT_TRUE(truncate);
    FilterPolicies policies;
    policies.urlizeExtraSchemes = { "ssh" };
    RenderOptions options;
    options.filterPolicies = PoliciesOf(policies);
    std::string out;
    EXPECT_FALSE(urlize->RenderTo(out, {}, options));
    // An explicit empty list is not None: the policy's schemes do not apply
    auto empty = env.FromString("{{ 'ssh:a'|urlize(extra_schemes=[]) }}");
    ASSERT_TRUE(empty);
    out.clear();
    ASSERT_TRUE(empty->RenderTo(out, {}, options));
    EXPECT_EQ("ssh:a", out);

    policies = FilterPolicies{};
    policies.truncateLeeway = -1;
    options.filterPolicies = PoliciesOf(policies);
    out.clear();
    EXPECT_FALSE(truncate->RenderTo(out, {}, options));
}

TEST(RenderOptionsTest, JsonDumpsGetsTheValueAndTheIndentOnlyWhenGiven)
{
    TemplateEnv env;
    auto tpl = env.FromString("{{ 'x'|tojson }};{{ 'y'|tojson(2) }};{{ 'z'|tojson(none) }};{{ 'w'|tojson('ab') }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    FilterPolicies policies;
    policies.jsonDumps = RecordingDumps();
    // The native options do not apply to it
    policies.jsonIndent = "  ";
    RenderOptions options;
    options.filterPolicies = PoliciesOf(policies);
    std::string out;
    ASSERT_TRUE(tpl->RenderTo(out, {}, options));
    EXPECT_EQ("x/no indent;y/indent given;z/no indent;w/indent=ab", out);
}

TEST(RenderOptionsTest, JsonDumpsResultMustBeAStringAndIsHtmlSafeMarkup)
{
    TemplateEnv env;
    env.GetSettings().autoescape = true;
    auto tpl = env.FromString("{{ v|tojson }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    FilterPolicies policies;
    policies.jsonDumps = UserCallable([](const UserCallableParams&) -> Value { return std::string("<\"a\" & 'b'>"); }, {});
    RenderOptions options;
    options.filterPolicies = PoliciesOf(policies);
    std::string out;
    ASSERT_TRUE(tpl->RenderTo(out, { { "v", 1 } }, options)) << out;
    // htmlsafe_json_dumps escapes <, >, & and '; the result is Markup, so autoescape leaves the quotes
    EXPECT_EQ("\\u003c\"a\" \\u0026 \\u0027b\\u0027\\u003e", out);

    policies.jsonDumps = UserCallable([](const UserCallableParams&) -> Value { return 42; }, {});
    options.filterPolicies = PoliciesOf(policies);
    out.clear();
    auto failed = tpl->RenderTo(out, { { "v", 1 } }, options);
    EXPECT_FALSE(failed);
}

TEST(RenderOptionsTest, JsonDumpsResultIsCheckedAgainstTheSequenceCap)
{
    TemplateEnv env;
    env.GetSettings().limits.maxSequenceSize = 10;
    auto tpl = env.FromString("{{ 1|tojson }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    FilterPolicies policies;
    // Two characters that escape to twelve
    policies.jsonDumps = UserCallable([](const UserCallableParams&) -> Value { return std::string("<<"); }, {});
    RenderOptions options;
    options.filterPolicies = PoliciesOf(policies);
    std::string out;
    auto failed = tpl->RenderTo(out, {}, options);
    ASSERT_FALSE(failed);
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, failed.error().GetCode());

    policies.jsonDumps = UserCallable([](const UserCallableParams&) -> Value { return std::string("<"); }, {});
    options.filterPolicies = PoliciesOf(policies);
    out.clear();
    ASSERT_TRUE(tpl->RenderTo(out, {}, options));
    EXPECT_EQ("\\u003c", out);
}

TEST(RenderOptionsTest, JsonDumpsTakesAnIndentTheBuiltInWriterRejects)
{
    TemplateEnv env;
    auto tpl = env.FromString("{{ 1|tojson(indent=[]) }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    FilterPolicies policies;
    policies.jsonDumps = UserCallable([](const UserCallableParams& params) -> Value { return std::string(params["indent"].IsList() ? "list" : "other"); }, { ArgInfo{ "obj", true }, ArgInfo{ "indent", false, Value() } });
    RenderOptions options;
    options.filterPolicies = PoliciesOf(policies);
    std::string out;
    ASSERT_TRUE(tpl->RenderTo(out, {}, options));
    EXPECT_EQ("list", out);
    // Without a dumps function json.dumps rejects it
    out.clear();
    EXPECT_FALSE(tpl->RenderTo(out, {}));
}

TEST(RenderOptionsTest, JsonDumpsResultInPromptAutoescapeMode)
{
    TemplateEnv env;
    env.GetSettings().autoescape = true;
    env.GetSettings().autoescapeMode = AutoescapeMode::Prompt;
    env.GetSettings().promptEscaper = PromptEscaper({ { "<s>" }, {} });
    auto tpl = env.FromString("{{ v|tojson }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    FilterPolicies policies;
    policies.jsonDumps = UserCallable([](const UserCallableParams&) -> Value { return std::string("<s>[1]"); }, {});
    RenderOptions options;
    options.filterPolicies = PoliciesOf(policies);
    std::string out;
    ASSERT_TRUE(tpl->RenderTo(out, { { "v", 1 } }, options)) << out;
    EXPECT_EQ("\\u003cs\\u003e[1]", out);
}

TEST(RenderOptionsTest, FilterPoliciesWorkForWideTemplates)
{
    TemplateEnv env;
    auto tpl = env.FromString(L"{{ 'abcdefghijk'|truncate(10) }}|{{ 'http://example.com'|urlize }}|{{ v|tojson }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    FilterPolicies policies;
    policies.truncateLeeway = 0;
    policies.urlizeRel = "";
    policies.urlizeTarget = "_blank";
    policies.jsonDumps = RecordingDumps();
    RenderOptions options;
    options.filterPolicies = PoliciesOf(policies);
    std::wstring out;
    auto wideResult = tpl->RenderTo(out, { { "v", std::wstring(L"s") } }, options);
    ASSERT_TRUE(wideResult) << wideResult.error().ToString();
    EXPECT_EQ(L"abcdefg...|<a href=\"http://example.com\" target=\"_blank\">http://example.com</a>|s/no indent", out);
}

TEST(TemplateEnvTest, SettingsEqualityCoversTheFilterPoliciesAndTheTransDefault)
{
    Settings a;
    Settings b;
    b.extensions.i18nTrimmed = true;
    EXPECT_TRUE(a != b);
    b = a;
    b.filterPolicies.urlizeRel = "";
    EXPECT_TRUE(a != b);
    b = a;
    b.filterPolicies.urlizeTarget = "_blank";
    EXPECT_TRUE(a != b);
    b = a;
    b.filterPolicies.urlizeExtraSchemes = { "ssh:" };
    EXPECT_TRUE(a != b);
    b = a;
    b.filterPolicies.truncateLeeway = 0;
    EXPECT_TRUE(a != b);
    b = a;
    b.filterPolicies.jsonSortKeys = false;
    EXPECT_TRUE(a != b);
    b = a;
    b.filterPolicies.jsonEnsureAscii = false;
    EXPECT_TRUE(a != b);
    b = a;
    b.filterPolicies.jsonIndent = "";
    EXPECT_TRUE(a != b);
    b = a;
    b.filterPolicies.jsonSeparators = std::make_pair(",", ":");
    EXPECT_TRUE(a != b);
    b = a;
    b.filterPolicies.jsonDumps = RecordingDumps();
    EXPECT_TRUE(a != b);
    a.filterPolicies.jsonDumps = b.filterPolicies.jsonDumps;
    EXPECT_TRUE(a == b);
}
