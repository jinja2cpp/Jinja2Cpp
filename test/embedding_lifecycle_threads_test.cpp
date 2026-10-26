#include "embedding_lifecycle_test.h"

#include <jinja2cpp/error_info.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace jinja2;
using namespace embedding_lifecycle;

// ---------------------------------------------------------------------------------------------------------------------
// C. Threads

// 3.10 threads 1: four threads render one template with independent contexts; each render calls a host callable that
// renders a second template of the same environment, which includes a third
TEST(EmbeddingLifecycleTest, C1_ConcurrentRendersWithNestedRenders)
{
    CountingFs fs;
    fs.Set("inc.j2", "i{{ n }}");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    auto sub = env.FromString("<{{ n * 2 }}{% include 'inc.j2' %}>");
    ASSERT_TRUE(sub) << sub.error().ToString();
    env.AddGlobal("sub", MakeCallable([&sub](int64_t n) { return Text(sub->RenderAsString({ { "n", n } })); }, ArgInfo{ "n" }));
    auto main = env.FromString("{{ t }}={{ sub(t) }}");
    ASSERT_TRUE(main) << main.error().ToString();

    constexpr int threads = 4;
    constexpr int iterations = 50;
    Latch start;
    Failures failures;
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (int w = 0; w < threads; ++w)
    {
        workers.emplace_back([&, w] {
            EXPECT_TRUE(start.Wait());
            for (int i = 0; i < iterations; ++i)
            {
                const int64_t t = (w * 1000) + i;
                const std::string expected = std::to_string(t) + "=<" + std::to_string(t * 2) + "i" + std::to_string(t) + ">";
                const std::string actual = Out(main->RenderAsString({ { "t", t } }));
                if (actual != expected)
                {
                    failures.Add(std::string("expected ").append(expected).append(", got ").append(actual));
                }
            }
        });
    }
    start.CountDown();
    for (auto& worker : workers)
    {
        worker.join();
    }
    EXPECT_EQ("", failures.Summary());
}

// 3.10 threads 3: LoadTemplate, LoadTemplateW and FromString run together, with renders that include the names they load
TEST(EmbeddingLifecycleTest, C2_ConcurrentLoadsAndRenders)
{
    CountingFs fs(true);
    for (int i = 0; i < 3; ++i)
    {
        fs.Set("f" + std::to_string(i) + ".j2", "F" + std::to_string(i));
    }
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);

    constexpr int threads = 4;
    constexpr int iterations = 50;
    Latch start;
    Failures failures;
    std::vector<std::thread> workers;
    workers.reserve(threads);
    for (int w = 0; w < threads; ++w)
    {
        workers.emplace_back([&, w] {
            EXPECT_TRUE(start.Wait());
            for (int i = 0; i < iterations; ++i)
            {
                const std::string index = std::to_string((w + i) % 3);
                const std::string name = "f" + index + ".j2";
                const auto narrow = env.LoadTemplate(name);
                const auto wide = env.LoadTemplateW(name);
                const auto inline1 = env.FromString("{% include '" + name + "' %}!");
                const auto inline2 = env.FromString(Str<wchar_t>("{% include 'f0.j2' %}?"));
                if (!narrow || !wide || !inline1 || !inline2)
                {
                    failures.Add("a load failed");
                    continue;
                }
                if (Out(narrow->RenderAsString({})) != "F" + index)
                {
                    failures.Add("narrow " + name);
                }
                if (Text(wide->RenderAsString({})) != Str<wchar_t>(("F" + index).c_str()))
                {
                    failures.Add("wide " + name);
                }
                if (Out(inline1->RenderAsString({})) != "F" + index + "!")
                {
                    failures.Add("inline " + name);
                }
                if (Text(inline2->RenderAsString({})) != L"F0?")
                {
                    failures.Add("wide inline");
                }
            }
        });
    }
    start.CountDown();
    for (auto& worker : workers)
    {
        worker.join();
    }
    EXPECT_EQ("", failures.Summary());
    EXPECT_GT(fs.DateChecks(), 0);
    // The cache settled on one template per name
    for (int i = 0; i < 3; ++i)
    {
        const std::string name = "f" + std::to_string(i) + ".j2";
        EXPECT_TRUE(*env.LoadTemplate(name) == *env.LoadTemplate(name));
    }
}

// 3.10 threads 5: filters, tests and translations added, replaced and removed during renders. A template loaded before keeps
// its filter and test; one loaded afterwards sees the change. The translation callables are globals (as in Jinja2's
// install_gettext_callables), so a render uses the ones installed when it started
TEST(EmbeddingLifecycleTest, C3_FiltersTestsAndTranslationsChangeDuringRenders)
{
    TemplateEnv env;
    env.GetSettings().extensions.i18n = true;
    // UserCallable rather than a typed MakeCallable: a typed one with string arguments took GCC near 5 GB under ASan
    const auto filter = [](const char* text) { return UserCallable([text](const UserCallableParams&) { return Value(std::string(text)); }, { ArgInfo{ "s" } }); };
    const auto test = [](bool result) { return UserCallable([result](const UserCallableParams&) { return Value(result); }, { ArgInfo{ "s" } }); };
    const auto gettext = [](const char* prefix) {
        return UserCallable([prefix](const UserCallableParams& p) { return Value(prefix + p["message"].AsString()); }, { ArgInfo{ "message" } });
    };
    const UserCallable ngettext([](const UserCallableParams& p) { return p["s"]; }, { ArgInfo{ "s" }, ArgInfo{ "p" }, ArgInfo{ "n" } });
    env.AddFilter("flt", filter("f1"));
    env.AddTest("tst", test(true));
    env.InstallGettextCallables(gettext("t1:"), ngettext);
    const char* source = "{{ 'a'|flt }}{{ 'yes' if 'a' is tst else 'no' }}{{ _('m') }}";
    auto old = env.FromString(source);
    ASSERT_TRUE(old) << old.error().ToString();
    EXPECT_EQ("f1yest1:m", Out(old->RenderAsString({})));
    const auto isOldBinding = [](const std::string& text) { return text == "f1yest1:m" || text == "f1yest2:m" || text == "f1yest3:m"; };

    Latch start;
    Failures failures;
    std::vector<std::thread> workers;
    workers.reserve(3);
    for (int w = 0; w < 3; ++w)
    {
        workers.emplace_back([&] {
            EXPECT_TRUE(start.Wait());
            for (int i = 0; i < 50; ++i)
            {
                const std::string actual = Out(old->RenderAsString({}));
                if (!isOldBinding(actual))
                {
                    failures.Add(actual);
                }
            }
        });
    }
    start.CountDown();
    for (int i = 0; i < 50; ++i)
    {
        env.AddFilter("flt", filter(i % 2 ? "f3" : "f2"));
        env.AddTest("tst", test(i % 2 == 0));
        env.InstallGettextCallables(gettext(i % 2 ? "t3:" : "t2:"), ngettext);
        env.AddFilter("temp", filter("tmp"));
        env.RemoveFilter("temp");
    }
    for (auto& worker : workers)
    {
        worker.join();
    }
    EXPECT_EQ("", failures.Summary());

    // The last round used i = 49: f3, a test that is false and the t3 translation
    auto fresh = env.FromString(source);
    ASSERT_TRUE(fresh) << fresh.error().ToString();
    EXPECT_EQ("f3not3:m", Out(fresh->RenderAsString({})));
    EXPECT_EQ("f1yest3:m", Out(old->RenderAsString({})));
    env.RemoveFilter("flt");
    EXPECT_FALSE(env.FindFilter("flt"));
    auto gone = env.FromString("{{ 'a'|flt }}");
    ASSERT_FALSE(gone);
    EXPECT_EQ(ErrorCode::UnknownFilter, gone.error().GetCode());
    EXPECT_EQ("f1yest3:m", Out(old->RenderAsString({})));
}

// 3.10 threads 7: two threads call the macros of a module while two render the template it came from
TEST(EmbeddingLifecycleTest, C4_ModuleMacrosAndRendersTogether)
{
    TemplateEnv env;
    auto tpl = env.FromString("{% macro sq(n) %}{{ n * n }}{% endmacro %}{{ sq(3) }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    auto module = tpl->MakeModule();
    ASSERT_TRUE(module) << module.error().ToString();
    EXPECT_EQ("9", module->body);

    Latch start;
    Failures failures;
    std::vector<std::thread> workers;
    workers.reserve(4);
    for (int w = 0; w < 4; ++w)
    {
        workers.emplace_back([&, w] {
            EXPECT_TRUE(start.Wait());
            for (int i = 1; i <= 50; ++i)
            {
                if (w < 2)
                {
                    const std::string actual = Dump(CallValue(module->exports["sq"], { int64_t{ i } }));
                    if (actual != std::to_string(i * i))
                    {
                        failures.Add("macro " + std::to_string(i) + ": " + actual);
                    }
                }
                else if (Out(tpl->RenderAsString({})) != "9")
                {
                    failures.Add("render");
                }
            }
        });
    }
    start.CountDown();
    for (auto& worker : workers)
    {
        worker.join();
    }
    EXPECT_EQ("", failures.Summary());
}

// 3.10 threads 6 and destruction order: three renders wait inside the template while the environment object is
// destroyed; each ends with the includes it had left
TEST(EmbeddingLifecycleTest, C5_EnvDestroyedWhileRendersRun)
{
    CountingFs fs;
    fs.Set("a.j2", "[a{{ g }}]");
    auto env = std::make_unique<TemplateEnv>();
    env->GetSettings().templateLookup = TemplateLookup::EveryUse;
    env->AddFilesystemHandler("", fs);
    env->AddGlobal("g", "G");
    auto main = env->FromString("{% include 'a.j2' %}{{ wait() }}{% include 'a.j2' %}");
    ASSERT_TRUE(main) << main.error().ToString();

    constexpr int renders = 3;
    Latch arrived(renders);
    Latch proceed;
    const ValuesMap params{ { "wait", MakeCallable([&]() {
                                  arrived.CountDown();
                                  EXPECT_TRUE(proceed.Wait());
                                  return std::string("W");
                              }) } };
    std::vector<std::string> results(renders);
    std::vector<std::thread> workers;
    workers.reserve(renders);
    for (int w = 0; w < renders; ++w)
    {
        workers.emplace_back([&, w] { results[static_cast<size_t>(w)] = Out(main->RenderAsString(params)); });
    }
    EXPECT_TRUE(arrived.Wait());
    env.reset();
    proceed.CountDown();
    for (auto& worker : workers)
    {
        worker.join();
    }
    for (const auto& result : results)
    {
        EXPECT_EQ("[aG]W[aG]", result);
    }
    // The first include of the renders shares a cache entry (or races to make it); the second one of each render opens the file again
    EXPECT_GE(fs.Opens("a.j2"), 1 + renders);
}

// 3.10 caching 1 and 3 under threads: reloads race renders of the includer. With OncePerRender both includes of one
// render see the same version
TEST(EmbeddingLifecycleTest, C6_ReloadsRaceRendersOfTheIncluder)
{
    CountingFs fs(true);
    fs.Set("inc.j2", "v0");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    auto main = env.FromString("{% include 'inc.j2' %}|{% include 'inc.j2' %}");
    ASSERT_TRUE(main) << main.error().ToString();

    Latch start;
    Failures failures;
    std::vector<std::thread> workers;
    workers.reserve(3);
    for (int w = 0; w < 3; ++w)
    {
        workers.emplace_back([&] {
            EXPECT_TRUE(start.Wait());
            for (int i = 0; i < 50; ++i)
            {
                const std::string actual = Out(main->RenderAsString({}));
                const auto bar = actual.find('|');
                if (actual.size() < 3 || actual[0] != 'v' || bar == std::string::npos || actual.substr(0, bar) != actual.substr(bar + 1))
                {
                    failures.Add("mixed or wrong: " + actual);
                }
            }
        });
    }
    start.CountDown();
    for (int i = 1; i <= 50; ++i)
    {
        fs.Set("inc.j2", "v" + std::to_string(i));
        fs.Touch("inc.j2");
        EXPECT_TRUE(env.LoadTemplate("inc.j2"));
    }
    for (auto& worker : workers)
    {
        worker.join();
    }
    EXPECT_EQ("", failures.Summary());
    // With the threads gone one more change is seen. Not asserted before: a render that reloads while the file changes can
    // pair the new date with the old text, as Jinja2's loader can, so the last change of the loop may stay unseen
    fs.Set("inc.j2", "last");
    fs.Touch("inc.j2");
    EXPECT_TRUE(env.LoadTemplate("inc.j2"));
    EXPECT_EQ("last|last", Out(main->RenderAsString({})));
}
