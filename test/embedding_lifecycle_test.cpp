#include "embedding_lifecycle_test.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <exception>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>

using namespace jinja2;
using namespace embedding_lifecycle;

// ---------------------------------------------------------------------------------------------------------------------
// A. Cache and reload

namespace
{
template<typename CharT>
void CheckCacheHit()
{
    CountingFs fs;
    fs.Set("a.j2", "A");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    auto first = LoadFrom(env, "a.j2", Tag<CharT>{});
    auto second = LoadFrom(env, "a.j2", Tag<CharT>{});
    ASSERT_TRUE(first && second);
    EXPECT_TRUE(*first == *second);
    EXPECT_EQ(1, fs.TotalOpens("a.j2"));
    EXPECT_EQ(Str<CharT>("A"), Text(first->RenderAsString({})));
}

template<typename CharT>
void CheckReloadOnNewDate()
{
    CountingFs fs(true);
    fs.Set("a.j2", "v1");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    auto held = LoadFrom(env, "a.j2", Tag<CharT>{});
    ASSERT_TRUE(held);
    EXPECT_EQ(Str<CharT>("v1"), Text(held->RenderAsString({})));
    // An unchanged date is a hit
    auto hit = LoadFrom(env, "a.j2", Tag<CharT>{});
    ASSERT_TRUE(hit);
    EXPECT_TRUE(*hit == *held);
    EXPECT_EQ(1, fs.TotalOpens("a.j2"));

    fs.Set("a.j2", "v2");
    fs.Touch("a.j2");
    auto fresh = LoadFrom(env, "a.j2", Tag<CharT>{});
    ASSERT_TRUE(fresh);
    EXPECT_FALSE(*fresh == *held);
    EXPECT_EQ(2, fs.TotalOpens("a.j2"));
    EXPECT_EQ(Str<CharT>("v2"), Text(fresh->RenderAsString({})));
    // The template the host held keeps its text
    EXPECT_EQ(Str<CharT>("v1"), Text(held->RenderAsString({})));
    auto again = LoadFrom(env, "a.j2", Tag<CharT>{});
    ASSERT_TRUE(again);
    EXPECT_TRUE(*again == *fresh);
    EXPECT_EQ(2, fs.TotalOpens("a.j2"));
}

template<typename CharT>
void CheckNoReloadWithoutDate()
{
    // A handler that gives no date (MemoryFileSystem) is never reloaded
    CountingFs fs(false);
    fs.Set("a.j2", "v1");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    auto first = LoadFrom(env, "a.j2", Tag<CharT>{});
    ASSERT_TRUE(first);
    fs.Set("a.j2", "v2");
    fs.Touch("a.j2");
    auto second = LoadFrom(env, "a.j2", Tag<CharT>{});
    ASSERT_TRUE(second);
    EXPECT_TRUE(*first == *second);
    EXPECT_EQ(1, fs.TotalOpens("a.j2"));
    EXPECT_EQ(Str<CharT>("v1"), Text(second->RenderAsString({})));
}

template<typename CharT>
void CheckNoDateChecksWithoutAutoReload()
{
    CountingFs fs(true);
    fs.Set("a.j2", "v1");
    TemplateEnv env;
    env.GetSettings().autoReload = false;
    env.AddFilesystemHandler("", fs);
    auto first = LoadFrom(env, "a.j2", Tag<CharT>{});
    ASSERT_TRUE(first);
    const int checks = fs.DateChecks();
    fs.Set("a.j2", "v2");
    fs.Touch("a.j2");
    auto second = LoadFrom(env, "a.j2", Tag<CharT>{});
    ASSERT_TRUE(second);
    EXPECT_TRUE(*first == *second);
    EXPECT_EQ(checks, fs.DateChecks());
    EXPECT_EQ(1, fs.TotalOpens("a.j2"));
    EXPECT_EQ(Str<CharT>("v1"), Text(second->RenderAsString({})));
}
} // namespace

// 3.10 caching 1: a hit returns the cached template and opens the file once; the narrow and wide caches are independent
TEST(EmbeddingLifecycleTest, A1_CacheHitReturnsSameTemplate)
{
    CheckCacheHit<char>();
    CheckCacheHit<wchar_t>();

    CountingFs fs;
    fs.Set("a.j2", "A");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    auto narrow = env.LoadTemplate("a.j2");
    auto wide = env.LoadTemplateW("a.j2");
    ASSERT_TRUE(narrow && wide);
    EXPECT_EQ(1, fs.Opens("a.j2"));
    EXPECT_EQ(1, fs.WideOpens("a.j2"));
    EXPECT_TRUE(*narrow == *env.LoadTemplate("a.j2"));
    EXPECT_TRUE(*wide == *env.LoadTemplateW("a.j2"));
    EXPECT_EQ(1, fs.Opens("a.j2"));
    EXPECT_EQ(1, fs.WideOpens("a.j2"));
}

// 3.10 caching 1: autoReload with a new date makes a new template and the held one keeps its text; a handler without
// a date never reloads; without autoReload a hit asks for no date
TEST(EmbeddingLifecycleTest, A2_AutoReloadMakesANewTemplate)
{
    CheckReloadOnNewDate<char>();
    CheckReloadOnNewDate<wchar_t>();
    CheckNoReloadWithoutDate<char>();
    CheckNoReloadWithoutDate<wchar_t>();
    CheckNoDateChecksWithoutAutoReload<char>();
    CheckNoDateChecksWithoutAutoReload<wchar_t>();
}

namespace
{
// Thread 1 renders `inc` twice around a callable that waits; thread 2 reloads `inc` meanwhile. Returns the first
// render's output and the next render's
std::pair<std::string, std::string> ReloadWhileRenderHoldsInclude(TemplateLookup lookup)
{
    CountingFs fs(true);
    fs.Set("inc.j2", "old");
    TemplateEnv env;
    env.GetSettings().templateLookup = lookup;
    env.AddFilesystemHandler("", fs);
    auto main = env.FromString("{% include 'inc.j2' %}{{ wait() }}{% include 'inc.j2' %}");
    if (!main)
    {
        ADD_FAILURE() << main.error().ToString();
        return {};
    }
    Latch inWait;
    Latch proceed;
    const ValuesMap params{ { "wait", MakeCallable([&]() {
                                  inWait.CountDown();
                                  EXPECT_TRUE(proceed.Wait());
                                  return std::string("|");
                              }) } };
    std::string first;
    std::thread worker([&] { first = Out(main->RenderAsString(params)); });
    EXPECT_TRUE(inWait.Wait());
    fs.Set("inc.j2", "new");
    fs.Touch("inc.j2");
    auto fresh = env.LoadTemplate("inc.j2");
    EXPECT_TRUE(fresh);
    proceed.CountDown();
    worker.join();
    return { first, Out(main->RenderAsString(params)) };
}
} // namespace

// 3.10 caching 1 and 3: a reload while a render holds the old include. OncePerRender: the render ends on the old text,
// the next one sees the new. EveryUse: the next use in the same render sees the new text
TEST(EmbeddingLifecycleTest, A3_ReloadWhileRenderHoldsOldInclude)
{
    const auto once = ReloadWhileRenderHoldsInclude(TemplateLookup::OncePerRender);
    EXPECT_EQ("old|old", once.first);
    EXPECT_EQ("new|new", once.second);
    const auto every = ReloadWhileRenderHoldsInclude(TemplateLookup::EveryUse);
    EXPECT_EQ("old|new", every.first);
    EXPECT_EQ("new|new", every.second);
}

// 3.10 caching 2 and 4: with cacheSize 0 every lookup opens the file, and the parses are freed when the render ends.
// A parse holds its environment's state; the sentinel sits in the globals, so it is released only when no parse is left
TEST(EmbeddingLifecycleTest, A4_NoCacheOpensEveryLookupAndFreesParses)
{
    for (const auto lookup : { TemplateLookup::OncePerRender, TemplateLookup::EveryUse })
    {
        Sentinel probe;
        {
            CountingFs fs;
            fs.Set("a.j2", "a");
            TemplateEnv env;
            env.GetSettings().cacheSize = 0;
            env.GetSettings().templateLookup = lookup;
            env.AddFilesystemHandler("", fs);
            env.AddGlobal("probe", MakeCallable([hold = probe.Hold()]() { return std::string("P"); }));
            auto main = env.FromString("{% include 'a.j2' %}{% include 'a.j2' %}{{ probe() }}");
            ASSERT_TRUE(main) << main.error().ToString();
            EXPECT_EQ("aaP", Out(main->RenderAsString({})));
            EXPECT_EQ(lookup == TemplateLookup::EveryUse ? 2 : 1, fs.Opens("a.j2"));

            auto first = env.LoadTemplate("a.j2");
            auto second = env.LoadTemplate("a.j2");
            ASSERT_TRUE(first && second);
            EXPECT_FALSE(*first == *second);
            EXPECT_EQ(lookup == TemplateLookup::EveryUse ? 4 : 3, fs.Opens("a.j2"));
        }
        EXPECT_TRUE(ReleaseThreadCaches());
        EXPECT_TRUE(probe.Expired()) << "a parse outlived the render that made it";
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// B. Destruction order

namespace
{
template<typename CharT>
void CheckTemplateOutlivesEnv()
{
    auto fs = std::make_shared<CountingFs>();
    std::weak_ptr<CountingFs> watch = fs;
    fs->Set("a.j2", "[a{{ g }}]");
    fs->Set("m.j2", "{% set x = 'X' %}{% macro f() %}F{% endmacro %}");
    std::optional<BasicTemplate<CharT>> kept;
    {
        TemplateEnv env;
        env.AddFilesystemHandler("", FilesystemHandlerPtr(fs));
        env.AddGlobal("g", "G");
        auto tpl = FromStringIn(env, "{% include 'a.j2' %}{% import 'm.j2' as m %}{{ m.x }}{{ m.f() }}", Tag<CharT>{});
        ASSERT_TRUE(tpl);
        kept = *tpl;
        EXPECT_EQ(Str<CharT>("[aG]XF"), Text(kept->RenderAsString({})));
        EXPECT_EQ(1, fs->TotalOpens("a.j2"));
    }
    // The host lets go of the handler it gave away: the template's environment state still holds it
    fs.reset();
    ASSERT_FALSE(watch.expired());
    for (int render = 1; render <= 3; ++render)
    {
        EXPECT_EQ(Str<CharT>("[aG]XF"), Text(kept->RenderAsString({})));
        // Uncached: every render opens both files again
        EXPECT_EQ(1 + render, watch.lock()->TotalOpens("a.j2"));
        EXPECT_EQ(1 + render, watch.lock()->TotalOpens("m.j2"));
    }
    kept.reset();
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_TRUE(watch.expired());
}
} // namespace

// 3.10 caching 4 and destruction order: a template outlives its environment and renders include and import uncached;
// the handler the host released lives as long as the template
TEST(EmbeddingLifecycleTest, B1_TemplateOutlivesEnvAndRendersUncached)
{
    CheckTemplateOutlivesEnv<char>();
    CheckTemplateOutlivesEnv<wchar_t>();
}

// 3.10 destruction order: the environment destroyed from a callable mid-render, and from another thread while a render
// waits on a latch inside it; the remaining includes render and the next render works
TEST(EmbeddingLifecycleTest, B2_EnvDestroyedMidRender)
{
    {
        CountingFs fs;
        fs.Set("a.j2", "[a{{ g }}]");
        auto env = std::make_unique<TemplateEnv>();
        env->AddFilesystemHandler("", fs);
        env->AddGlobal("g", "G");
        Template main(env.get());
        ASSERT_TRUE(main.Load("{% include 'a.j2' %}{{ drop() }}{% include 'a.j2' %}{% include 'a.j2' %}"));
        const ValuesMap params{ { "drop", MakeCallable([&env]() {
                                      env.reset();
                                      return std::string("D");
                                  }) } };
        EXPECT_EQ("[aG]D[aG][aG]", Out(main.RenderAsString(params)));
        EXPECT_FALSE(env);
        EXPECT_EQ("[aG]D[aG][aG]", Out(main.RenderAsString(params)));
    }
    {
        CountingFs fs;
        fs.Set("a.j2", "[a{{ g }}]");
        auto env = std::make_unique<TemplateEnv>();
        env->GetSettings().templateLookup = TemplateLookup::EveryUse;
        env->AddFilesystemHandler("", fs);
        env->AddGlobal("g", "G");
        auto main = env->FromString("{% include 'a.j2' %}{{ wait() }}{% include 'a.j2' %}");
        ASSERT_TRUE(main) << main.error().ToString();
        Latch inWait;
        Latch proceed;
        const ValuesMap params{ { "wait", MakeCallable([&]() {
                                      inWait.CountDown();
                                      EXPECT_TRUE(proceed.Wait());
                                      return std::string("W");
                                  }) } };
        std::string first;
        std::thread worker([&] { first = Out(main->RenderAsString(params)); });
        EXPECT_TRUE(inWait.Wait());
        env.reset();
        proceed.CountDown();
        worker.join();
        EXPECT_EQ("[aG]W[aG]", first);
        // The second include of the render had to open the file again
        EXPECT_EQ(2, fs.Opens("a.j2"));
        EXPECT_EQ("[aG]W[aG]", Out(main->RenderAsString(params)));
    }
}

namespace
{
template<typename CharT>
void CheckModuleOutlivesTemplateAndEnv()
{
    Sentinel probe;
    std::optional<BasicTemplateModule<CharT>> module;
    {
        CountingFs fs;
        fs.Set("m.j2", "{% macro hi(n) %}hi {{ n }}{{ tag() }}{% endmacro %}");
        TemplateEnv env;
        env.AddFilesystemHandler("", fs);
        env.AddGlobal("tag", MakeCallable([hold = probe.Hold()]() { return std::string("T"); }));
        auto tpl = LoadFrom(env, "m.j2", Tag<CharT>{});
        ASSERT_TRUE(tpl);
        auto made = tpl->MakeModule();
        ASSERT_TRUE(made);
        module = std::move(*made);
    }
    // The template and the environment object are gone; the module's macros keep the environment's state
    EXPECT_FALSE(probe.Expired());
    BasicTemplate<CharT> user;
    ASSERT_TRUE(user.Load(Str<CharT>("{{ hi('x') }}")));
    EXPECT_EQ(Str<CharT>("hi xT"), Text(user.RenderAsString({ { "hi", module->exports["hi"] } })));
    EXPECT_EQ(Str<CharT>("hi xT"), Text(user.RenderAsString({ { "hi", module->exports["hi"] } })));
    module.reset();
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_TRUE(probe.Expired());
}
} // namespace

// 3.10 destruction order and values 6: a module outlives its template and environment, its macros render, and what it
// holds is released with it
TEST(EmbeddingLifecycleTest, B3_ModuleOutlivesTemplateAndEnv)
{
    CheckModuleOutlivesTemplateAndEnv<char>();
    CheckModuleOutlivesTemplateAndEnv<wchar_t>();
}

// 3.10 destruction order: an ErrorInfo, from an include and from a host exception, outlives the environment, the
// templates and the handler
TEST(EmbeddingLifecycleTest, B4_ErrorInfoOutlivesEverything)
{
    std::optional<ErrorInfo> engineError;
    std::optional<ErrorInfo> hostError;
    std::optional<ErrorInfoW> wideError;
    {
        auto fs = std::make_shared<CountingFs>();
        fs->Set("bad.j2", "line1\n{{ missing.attr }}");
        TemplateEnv env;
        env.AddFilesystemHandler("", FilesystemHandlerPtr(fs));
        env.AddGlobal("boom", MakeCallable([]() -> std::string { throw std::runtime_error("boom!"); }));
        {
            auto tpl = env.FromString("{% include 'bad.j2' %}", "outer.j2");
            ASSERT_TRUE(tpl);
            auto result = tpl->RenderAsString({});
            ASSERT_FALSE(result);
            engineError = result.error();
        }
        {
            auto tpl = env.FromString("x\n{{ boom() }}", "host.j2");
            ASSERT_TRUE(tpl);
            auto result = tpl->RenderAsString({});
            ASSERT_FALSE(result);
            hostError = result.error();
        }
        {
            auto tpl = env.FromString(L"{% include 'bad.j2' %}", "wide.j2");
            ASSERT_TRUE(tpl);
            auto result = tpl->RenderAsString({});
            ASSERT_FALSE(result);
            wideError = result.error();
        }
    }
    EXPECT_TRUE(ReleaseThreadCaches());

    EXPECT_EQ(ErrorCode::UndefinedError, engineError->GetCode());
    EXPECT_EQ("bad.j2", engineError->GetErrorLocation().fileName);
    EXPECT_EQ(2u, engineError->GetErrorLocation().line);
    EXPECT_NE(std::string::npos, engineError->ToString().find("bad.j2"));
    EXPECT_FALSE(engineError->GetLocationDescr().empty());

    EXPECT_EQ(ErrorCode::UserCallableException, hostError->GetCode());
    EXPECT_EQ("host.j2", hostError->GetErrorLocation().fileName);
    EXPECT_EQ(2u, hostError->GetErrorLocation().line);
    ASSERT_TRUE(hostError->GetException());
    EXPECT_NE(std::string::npos, hostError->ToString().find("boom!"));
    try
    {
        std::rethrow_exception(hostError->GetException());
        ADD_FAILURE() << "rethrow_exception returned";
    }
    catch (const std::runtime_error& e)
    {
        EXPECT_STREQ("boom!", e.what());
    }

    EXPECT_EQ(ErrorCode::UndefinedError, wideError->GetCode());
    EXPECT_EQ("bad.j2", wideError->GetErrorLocation().fileName);
    EXPECT_NE(std::wstring::npos, wideError->ToString().find(L"bad.j2"));
    EXPECT_FALSE(wideError->GetLocationDescr().empty());
}

// 3.10 destruction order: a thread keeps the last globals it rendered with, after the environment is gone, until
// ReleaseThreadCaches on that thread or the thread's exit (0275)
TEST(EmbeddingLifecycleTest, B5_ThreadCachesKeepDeadEnvGlobals)
{
    {
        Sentinel probe;
        {
            TemplateEnv env;
            env.AddGlobal("probe", MakeCallable([hold = probe.Hold()]() { return std::string("P"); }));
            Template tpl(&env);
            ASSERT_TRUE(tpl.Load("{{ probe() }}"));
            EXPECT_EQ("P", Out(tpl.RenderAsString({})));
        }
        EXPECT_FALSE(probe.Expired());
        EXPECT_TRUE(ReleaseThreadCaches());
        EXPECT_TRUE(probe.Expired());
    }
    {
        // A pool thread that exits without calling it
        Sentinel probe;
        std::optional<TemplateEnv> env;
        env.emplace();
        env->AddGlobal("probe", MakeCallable([hold = probe.Hold()]() { return std::string("P"); }));
        std::optional<Template> tpl;
        tpl.emplace(&*env);
        ASSERT_TRUE(tpl->Load("{{ probe() }}"));
        Latch rendered;
        Latch go;
        std::string out;
        std::thread worker([&] {
            out = Out(tpl->RenderAsString({}));
            rendered.CountDown();
            EXPECT_TRUE(go.Wait());
        });
        EXPECT_TRUE(rendered.Wait());
        tpl.reset();
        env.reset();
        EXPECT_FALSE(probe.Expired());
        go.CountDown();
        worker.join();
        EXPECT_EQ("P", out);
        EXPECT_TRUE(probe.Expired());
    }
}

// 3.10 destruction order (D5): a module macro stored in the globals holds a template of its environment, so the
// environment's state stays alive until RemoveGlobal removes it, and RemoveGlobal then frees everything. The cycle
// itself cannot be tested to its end: with the environment object gone, nothing is left to remove the global with
TEST(EmbeddingLifecycleTest, B6_GlobalMacroCycleBrokenByRemoveGlobal)
{
    Sentinel probe;
    std::weak_ptr<CountingFs> watchFs;
    {
        auto fs = std::make_shared<CountingFs>();
        watchFs = fs;
        fs->Set("m.j2", "{% macro greet(n) %}hi {{ n }}{% endmacro %}");
        TemplateEnv env;
        env.AddFilesystemHandler("", FilesystemHandlerPtr(fs));
        fs.reset();
        env.AddGlobal("probe", MakeCallable([hold = probe.Hold()]() { return std::string("P"); }));
        {
            auto tpl = env.LoadTemplate("m.j2");
            ASSERT_TRUE(tpl);
            auto module = tpl->MakeModule();
            ASSERT_TRUE(module);
            env.AddGlobal("greet", module->exports["greet"]);
        }
        Template user(&env);
        ASSERT_TRUE(user.Load("{{ greet('x') }}{{ probe() }}"));
        EXPECT_EQ("hi xP", Out(user.RenderAsString({})));
        EXPECT_FALSE(watchFs.expired());
        env.RemoveGlobal("greet");
        EXPECT_NE("hi xP", Out(user.RenderAsString({})));
    }
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_TRUE(watchFs.expired());
    EXPECT_TRUE(probe.Expired());
}
