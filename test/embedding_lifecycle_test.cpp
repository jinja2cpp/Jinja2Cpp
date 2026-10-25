// Scenario suite for the embedding lifecycle contract (docs/api-2.0.md 3.10, task 0223): what a host may rely on when
// environments, templates, renders, modules and host callbacks overlap in time. Every row asserts an output, an error
// code and location, or a release (a weak_ptr that expired); a loop that only checks for "no crash" is not a row.
// The expected text of the rows with template-only semantics (E1, E6, F1-F5, D1) is Python Jinja2 3.1.6's.
// Rows that wait on other tasks are added by them: A5 and D5 (0317), E3 (0318), A6 and C7 (0264). F6 is not here: on
// master a render started by a callable does not see the global the outer render changed in place (0211 decision).
#include <jinja2cpp/error_info.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/generic_list_iterator.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <functional>
#include <istream>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <unordered_map>
#include <utility>
#include <variant>
#include <vector>

using namespace jinja2;

namespace
{
// A one-shot gate: C++17 has no std::latch. Waiting gives up after a generous time, so a broken row fails and does not hang
class Latch
{
public:
    explicit Latch(int count = 1)
        : m_count(count)
    {
    }

    void CountDown()
    {
        {
            std::scoped_lock lock(m_mutex);
            if (m_count > 0)
            {
                --m_count;
            }
        }
        m_cv.notify_all();
    }

    [[nodiscard]] bool Wait()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_cv.wait_for(lock, std::chrono::seconds(30), [this] { return m_count == 0; });
    }

private:
    std::mutex m_mutex;
    std::condition_variable m_cv;
    int m_count;
};

// A handler with settable text and modification time per file, and thread-safe counters. `reportsTime` false behaves
// like MemoryFileSystem: no date, so a cached template is never reloaded
class CountingFs : public IFilesystemHandler
{
public:
    explicit CountingFs(bool reportsTime = false)
        : m_reportsTime(reportsTime)
    {
    }

    void Set(const std::string& name, std::string text)
    {
        std::scoped_lock lock(m_mutex);
        m_files[name].text = std::move(text);
    }
    //! Moves the modification time of the file on by one second
    void Touch(const std::string& name)
    {
        std::scoped_lock lock(m_mutex);
        ++m_files[name].seconds;
    }
    //! Called from OpenStream and OpenWStream with the file name, after it is counted and with no lock held
    void SetOpenHook(std::function<void(const std::string&)> hook)
    {
        std::scoped_lock lock(m_mutex);
        m_hook = std::move(hook);
    }
    int Opens(const std::string& name) const
    {
        std::scoped_lock lock(m_mutex);
        const auto p = m_opens.find(name);
        return p == m_opens.end() ? 0 : p->second;
    }
    int WideOpens(const std::string& name) const
    {
        std::scoped_lock lock(m_mutex);
        const auto p = m_wideOpens.find(name);
        return p == m_wideOpens.end() ? 0 : p->second;
    }
    int TotalOpens(const std::string& name) const { return Opens(name) + WideOpens(name); }
    int DateChecks() const
    {
        std::scoped_lock lock(m_mutex);
        return m_dateChecks;
    }

    CharFileStreamPtr OpenStream(const std::string& name) const override
    {
        std::optional<std::string> text = Open(name, m_opens);
        if (!text)
        {
            return CharFileStreamPtr(nullptr, [](std::istream*) {});
        }
        return CharFileStreamPtr(new std::istringstream(*text), [](std::istream* s) { delete s; });
    }

    WCharFileStreamPtr OpenWStream(const std::string& name) const override
    {
        std::optional<std::string> text = Open(name, m_wideOpens);
        if (!text)
        {
            return WCharFileStreamPtr(nullptr, [](std::wistream*) {});
        }
        return WCharFileStreamPtr(new std::wistringstream(std::wstring(text->begin(), text->end())), [](std::wistream* s) { delete s; });
    }

    std::optional<std::chrono::system_clock::time_point> GetLastModificationDate(const std::string& name) const override
    {
        std::scoped_lock lock(m_mutex);
        ++m_dateChecks;
        const auto p = m_files.find(name);
        if (!m_reportsTime || p == m_files.end())
        {
            return std::nullopt;
        }
        return std::chrono::system_clock::time_point(std::chrono::seconds(1000000 + p->second.seconds));
    }

private:
    struct File
    {
        std::string text;
        int seconds = 0;
    };

    std::optional<std::string> Open(const std::string& name, std::map<std::string, int>& counter) const
    {
        std::function<void(const std::string&)> hook;
        std::optional<std::string> text;
        {
            std::scoped_lock lock(m_mutex);
            ++counter[name];
            hook = m_hook;
            const auto p = m_files.find(name);
            if (p != m_files.end())
            {
                text = p->second.text;
            }
        }
        if (hook)
        {
            hook(name);
        }
        return text;
    }

    const bool m_reportsTime;
    mutable std::mutex m_mutex;
    std::map<std::string, File> m_files;
    mutable std::map<std::string, int> m_opens;
    mutable std::map<std::string, int> m_wideOpens;
    mutable int m_dateChecks = 0;
    std::function<void(const std::string&)> m_hook;
};

// A watched object: the strong reference moves into whatever the row wants to observe (a callable in the globals, a
// handler), and the row asserts that the weak one expires
class Sentinel
{
public:
    Sentinel()
        : m_owner(std::make_shared<int>(0))
        , m_watch(m_owner)
    {
    }

    std::shared_ptr<int> Hold() { return std::move(m_owner); }
    [[nodiscard]] bool Expired() const { return m_watch.expired(); }

private:
    std::shared_ptr<int> m_owner;
    std::weak_ptr<int> m_watch;
};

// Failures found on worker threads: gtest assertions stay on the test's own thread
class Failures
{
public:
    void Add(std::string text)
    {
        std::scoped_lock lock(m_mutex);
        if (m_first.empty())
        {
            m_first = std::move(text);
        }
        ++m_count;
    }
    [[nodiscard]] std::string Summary() const
    {
        std::scoped_lock lock(m_mutex);
        return m_count == 0 ? std::string() : std::to_string(m_count) + " failures, first: " + m_first;
    }

private:
    mutable std::mutex m_mutex;
    std::string m_first;
    int m_count = 0;
};

// A callable that stores every value it receives as `v`
struct Kept
{
    std::mutex mutex;
    std::vector<Value> values;
};

UserCallable KeepCallable(const std::shared_ptr<Kept>& kept)
{
    return UserCallable(
        [kept](const UserCallableParams& p) {
            std::scoped_lock lock(kept->mutex);
            kept->values.push_back(p["v"]);
            return Value(std::string());
        },
        { ArgInfo{ "v" } });
}

template<typename CharT>
struct Tag
{
};

template<typename CharT>
std::basic_string<CharT> Str(const char* text)
{
    const std::string_view view(text);
    return std::basic_string<CharT>(view.begin(), view.end());
}

Result<Template> LoadFrom(TemplateEnv& env, const std::string& name, Tag<char>)
{
    return env.LoadTemplate(name);
}

ResultW<TemplateW> LoadFrom(TemplateEnv& env, const std::string& name, Tag<wchar_t>)
{
    return env.LoadTemplateW(name);
}

Result<Template> FromStringIn(TemplateEnv& env, const char* source, Tag<char>)
{
    return env.FromString(std::string_view(source));
}

ResultW<TemplateW> FromStringIn(TemplateEnv& env, const char* source, Tag<wchar_t>)
{
    return env.FromString(Str<wchar_t>(source));
}

template<typename R>
std::string Out(const R& result)
{
    return result ? std::string(*result) : "error: " + result.error().ToString();
}

// Text of a value, the way the rows compare them; maps list `key:value,`
std::string Dump(const Value& v)
{
    if (const auto* sv = std::get_if<std::string_view>(&v.GetData()))
    {
        return std::string(*sv);
    }
    if (v.IsString())
    {
        return v.AsString();
    }
    if (v.IsInt())
    {
        return std::to_string(v.AsInt());
    }
    if (v.IsList())
    {
        std::string text = "[";
        for (const auto& item : v.ToGenericList())
        {
            text += Dump(item) + ",";
        }
        return text + "]";
    }
    if (v.IsMap())
    {
        const auto map = v.ToGenericMap();
        std::string text = "{";
        for (const auto& key : map.GetKeys())
        {
            text += key + ":" + Dump(map.GetValueByName(key)) + ",";
        }
        return text + "}";
    }
    return v.IsNone() ? "<none>" : "<other>";
}

Value CallValue(const Value& fn, ValuesList pos = {})
{
    UserCallableParams params;
    params.extraPosArgs = std::move(pos);
    return fn.As<UserCallable>().callable(params);
}

// A host record reflected by the E1 row
struct Person
{
    std::string name;
    std::vector<std::string> tags;
};
} // namespace

namespace jinja2
{
template<>
struct TypeReflection<Person> : TypeReflected<Person>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            { "name", [](const Person& obj) { return Reflect(obj.name); } },
            { "tags", [](const Person& obj) { return Reflect(obj.tags); } },
        };
        return accessors;
    }
};
} // namespace jinja2

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
    EXPECT_EQ(Str<CharT>("A"), first->RenderAsString({}).value());
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
    EXPECT_EQ(Str<CharT>("v1"), held->RenderAsString({}).value());
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
    EXPECT_EQ(Str<CharT>("v2"), fresh->RenderAsString({}).value());
    // The template the host held keeps its text
    EXPECT_EQ(Str<CharT>("v1"), held->RenderAsString({}).value());
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
    EXPECT_EQ(Str<CharT>("v1"), second->RenderAsString({}).value());
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
    EXPECT_EQ(Str<CharT>("v1"), second->RenderAsString({}).value());
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
        EXPECT_EQ(Str<CharT>("[aG]XF"), kept->RenderAsString({}).value());
        EXPECT_EQ(1, fs->TotalOpens("a.j2"));
    }
    // The host lets go of the handler it gave away: the template's environment state still holds it
    fs.reset();
    ASSERT_FALSE(watch.expired());
    for (int render = 1; render <= 3; ++render)
    {
        EXPECT_EQ(Str<CharT>("[aG]XF"), kept->RenderAsString({}).value());
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
    EXPECT_EQ(Str<CharT>("hi xT"), user.RenderAsString({ { "hi", module->exports["hi"] } }).value());
    EXPECT_EQ(Str<CharT>("hi xT"), user.RenderAsString({ { "hi", module->exports["hi"] } }).value());
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
    env.AddGlobal("sub", MakeCallable([&sub](int64_t n) { return sub->RenderAsString({ { "n", n } }).value(); }, ArgInfo{ "n" }));
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
                if (wide->RenderAsString({}).value() != Str<wchar_t>(("F" + index).c_str()))
                {
                    failures.Add("wide " + name);
                }
                if (Out(inline1->RenderAsString({})) != "F" + index + "!")
                {
                    failures.Add("inline " + name);
                }
                if (inline2->RenderAsString({}).value() != L"F0?")
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
    const auto filter = [](const char* text) { return MakeCallable([text](const std::string&) { return std::string(text); }, ArgInfo{ "s" }); };
    const auto test = [](bool result) { return MakeCallable([result](const std::string&) { return result; }, ArgInfo{ "s" }); };
    const auto gettext = [](const char* prefix) {
        return MakeCallable([prefix](const std::string& message) { return prefix + message; }, ArgInfo{ "message" });
    };
    const auto ngettext = MakeCallable([](const std::string& singular, const std::string&, int64_t) { return singular; }, ArgInfo{ "s" }, ArgInfo{ "p" }, ArgInfo{ "n" });
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

// ---------------------------------------------------------------------------------------------------------------------
// D. Reentrancy

// 3.10 reentrancy: a callable renders the same template recursively (bounded by a parameter) and another template of the
// environment; the nested render keeps the limits of its own template
TEST(EmbeddingLifecycleTest, D1_NestedRendersOfTheSameAndOtherTemplates)
{
    TemplateEnv env;
    auto main = env.FromString("{% if n > 0 %}[{{ n }}{{ rec(n - 1) }}<{{ other(n) }}>]{% endif %}");
    auto other = env.FromString("o{{ n * 2 }}");
    ASSERT_TRUE(main && other);
    env.AddGlobal("rec", MakeCallable([&main](int64_t n) { return main->RenderAsString({ { "n", n } }).value(); }, ArgInfo{ "n" }));
    env.AddGlobal("other", MakeCallable([&other](int64_t n) { return other->RenderAsString({ { "n", n } }).value(); }, ArgInfo{ "n" }));
    EXPECT_EQ("[3[2[1<o2>]<o4>]<o6>]", Out(main->RenderAsString({ { "n", int64_t{ 3 } } })));

    // The inner template comes from an environment with a small output limit; the outer one has none
    TemplateEnv limited;
    limited.GetSettings().limits.maxOutputSize = 5;
    auto inner = limited.FromString("xxxxxxxxxxxx");
    ASSERT_TRUE(inner) << inner.error().ToString();
    TemplateEnv free;
    auto outer = free.FromString("a{{ bridge() }}b");
    ASSERT_TRUE(outer) << outer.error().ToString();
    std::optional<ErrorInfo> innerError;
    const ValuesMap params{ { "bridge", MakeCallable([&]() {
                                  auto r = inner->RenderAsString({});
                                  if (r)
                                  {
                                      return std::string("fits");
                                  }
                                  innerError = r.error();
                                  return std::string(r.error().GetExtraParams().at(1).AsString());
                              }) } };
    EXPECT_EQ("amaxOutputSizeb", Out(outer->RenderAsString(params)));
    ASSERT_TRUE(innerError.has_value());
    EXPECT_EQ(ErrorCode::ResourceLimitExceeded, innerError.value().GetCode());
}

// 3.10 reentrancy: a callable loads templates, builds one from a string and changes globals and filters mid-render; the
// running render keeps its snapshot and the next one sees the changes
TEST(EmbeddingLifecycleTest, D2_ConfigurationChangesFromACallable)
{
    CountingFs fs;
    fs.Set("x.j2", "X");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    env.AddGlobal("g", "old");
    env.AddGlobal("gone", "here");
    std::string inner;
    env.AddGlobal("mutate", MakeCallable([&]() {
                      env.AddGlobal("g", "new");
                      env.RemoveGlobal("gone");
                      env.AddFilter("shout", MakeCallable([](const std::string& s) { return s + "!"; }, ArgInfo{ "s" }));
                      auto loaded = env.LoadTemplate("x.j2");
                      auto built = env.FromString("{{ g }}{{ 'a'|shout }}");
                      inner = loaded.value().RenderAsString({}).value() + built.value().RenderAsString({}).value();
                      return std::string("M");
                  }));
    auto tpl = env.FromString("{{ g }}{{ gone }}{{ mutate() }}{{ g }}{{ gone }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    EXPECT_EQ("oldhereMoldhere", Out(tpl->RenderAsString({})));
    EXPECT_EQ("Xnewa!", inner);
    EXPECT_TRUE(env.FindFilter("shout"));
    EXPECT_EQ("newMnew", Out(tpl->RenderAsString({})));
}

// 3.10 threads 7 and reentrancy: macro -> callable -> the same module's macro; a nested render's error goes back to the
// outer callable, which carries on; an exception thrown inside a nested render reaches the outer render at the outer
// location
TEST(EmbeddingLifecycleTest, D3_ModuleReentryAndNestedErrors)
{
    std::weak_ptr<TemplateModule> watchModule;
    {
        TemplateEnv env;
        Template tpl(&env);
        ASSERT_TRUE(tpl.Load("{% macro f(n) %}{% if n > 0 %}{{ n }}{{ back(n - 1) }}{% endif %}{% endmacro %}"));
        auto module = std::make_shared<TemplateModule>();
        watchModule = module;
        env.AddGlobal("back", MakeCallable([module](int64_t k) { return CallValue(module->exports["f"], { k }); }, ArgInfo{ "k" }));
        *module = tpl.MakeModule().value();
        EXPECT_EQ("321", Dump(CallValue(module->exports["f"], { int64_t{ 3 } })));
        // The callable holds the module, and the module's macros hold the globals they were made with, `back` among
        // them: the host ends the cycle by removing the global and dropping the exports
        env.RemoveGlobal("back");
        module->exports.clear();
    }
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_TRUE(watchModule.expired());
    {
        TemplateEnv env;
        auto bad = env.FromString("{{ nosuch.attr }}", "inner.j2");
        ASSERT_TRUE(bad) << bad.error().ToString();
        std::optional<ErrorInfo> innerError;
        env.AddGlobal("tryInner", MakeCallable([&]() {
                          auto r = bad->RenderAsString({});
                          if (r)
                          {
                              return std::string("ok");
                          }
                          innerError = r.error();
                          return "E" + std::to_string(static_cast<int>(r.error().GetCode()));
                      }));
        auto outer = env.FromString("a{{ tryInner() }}b{{ tryInner() }}c");
        ASSERT_TRUE(outer) << outer.error().ToString();
        EXPECT_EQ("aE11bE11c", Out(outer->RenderAsString({})));
        ASSERT_TRUE(innerError.has_value());
        EXPECT_EQ(ErrorCode::UndefinedError, innerError.value().GetCode());
        EXPECT_EQ("inner.j2", innerError.value().GetErrorLocation().fileName);
    }
    {
        TemplateEnv env;
        auto deep = env.FromString("{{ boom() }}", "deep.j2");
        ASSERT_TRUE(deep) << deep.error().ToString();
        env.AddGlobal("boom", MakeCallable([]() -> std::string { throw std::runtime_error("deep"); }));
        env.AddGlobal("bridge", MakeCallable([&]() -> std::string {
                          auto r = deep->RenderAsString({});
                          if (!r)
                          {
                              std::rethrow_exception(r.error().GetException());
                          }
                          return *r;
                      }));
        auto outer = env.FromString("line1\n{{ bridge() }}", "outer.j2");
        ASSERT_TRUE(outer) << outer.error().ToString();
        auto result = outer->RenderAsString({});
        ASSERT_FALSE(result);
        EXPECT_EQ(ErrorCode::UserCallableException, result.error().GetCode());
        EXPECT_EQ("outer.j2", result.error().GetErrorLocation().fileName);
        EXPECT_EQ(2u, result.error().GetErrorLocation().line);
        ASSERT_FALSE(result.error().GetExtraParams().empty());
        EXPECT_EQ("deep", result.error().GetExtraParams()[0].AsString());
    }
}

namespace
{
// Renders a template on every Write, and records what that made
class NestingSink final : public OutputSink
{
public:
    explicit NestingSink(const Template& nested)
        : m_nested(nested)
    {
    }

    void Write(std::string_view text) override
    {
        m_out.append(text);
        m_expectedNested += "[" + std::to_string(text.size()) + "]";
        m_nestedOut += m_nested.RenderAsString({ { "w", static_cast<int64_t>(text.size()) } }).value();
    }

    std::string m_out;
    std::string m_nestedOut;
    std::string m_expectedNested;

private:
    const Template& m_nested;
};
} // namespace

// 3.10 reentrancy: a render started from a sink's Write, from finalize, from a gettext callable, from the autoescape
// selector (at Load) and from a handler that loads another name of the same environment (no lock is held across the
// handler call; a deadlock would hang this row)
TEST(EmbeddingLifecycleTest, D4_RendersFromHostHooks)
{
    Template nested;
    ASSERT_TRUE(nested.Load("[{{ w }}]"));
    Template wrap;
    ASSERT_TRUE(wrap.Load("<{{ v }}>"));
    Template translated;
    ASSERT_TRUE(translated.Load("({{ m }})"));

    {
        TemplateEnv env;
        Template tpl(&env);
        ASSERT_TRUE(tpl.Load("ab{{ x }}cd{{ 'ef' }}"));
        NestingSink sink(nested);
        ASSERT_TRUE(tpl.RenderTo(sink, { { "x", "X" } }));
        EXPECT_EQ("abXcdef", sink.m_out);
        EXPECT_FALSE(sink.m_nestedOut.empty());
        EXPECT_EQ(sink.m_expectedNested, sink.m_nestedOut);
    }
    {
        TemplateEnv env;
        env.GetSettings().finalize = UserCallable([&wrap](const UserCallableParams& p) { return Value(wrap.RenderAsString({ { "v", p["v"] } }).value()); }, { ArgInfo{ "v" } });
        auto tpl = env.FromString("{{ 1 }}-{{ 'a' }}");
        ASSERT_TRUE(tpl) << tpl.error().ToString();
        EXPECT_EQ("<1>-<a>", Out(tpl->RenderAsString({})));
    }
    {
        TemplateEnv env;
        env.GetSettings().extensions.i18n = true;
        env.InstallGettextCallables(MakeCallable([&translated](const std::string& message) { return translated.RenderAsString({ { "m", message } }).value(); }, ArgInfo{ "message" }),
                                    MakeCallable([](const std::string& singular, const std::string&, int64_t) { return singular; }, ArgInfo{ "s" }, ArgInfo{ "p" }, ArgInfo{ "n" }));
        auto tpl = env.FromString("{{ gettext('hi') }}|{{ _('yo') }}");
        ASSERT_TRUE(tpl) << tpl.error().ToString();
        EXPECT_EQ("(hi)|(yo)", Out(tpl->RenderAsString({})));
    }
    {
        TemplateEnv env;
        std::vector<std::string> seen;
        std::string nestedOut;
        env.GetSettings().autoescapeSelector = AutoescapeSelector([&](const std::string& name) {
            seen.push_back(name);
            nestedOut += nested.RenderAsString({ { "w", int64_t{ 7 } } }).value();
            return true;
        });
        auto tpl = env.FromString("{{ '<' }}", "page.html");
        ASSERT_TRUE(tpl) << tpl.error().ToString();
        EXPECT_EQ("&lt;", Out(tpl->RenderAsString({})));
        ASSERT_EQ(1u, seen.size());
        EXPECT_EQ("page.html", seen[0]);
        EXPECT_EQ("[7]", nestedOut);
    }
    {
        CountingFs fs;
        fs.Set("main.j2", "M{% include 'dep.j2' %}");
        fs.Set("dep.j2", "D");
        fs.Set("side.j2", "S");
        TemplateEnv env;
        env.AddFilesystemHandler("", fs);
        std::string sideOut;
        fs.SetOpenHook([&](const std::string& name) {
            if (name == "main.j2")
            {
                sideOut = Out(env.LoadTemplate("side.j2").value().RenderAsString({}));
            }
        });
        auto main = env.LoadTemplate("main.j2");
        ASSERT_TRUE(main) << main.error().ToString();
        EXPECT_EQ("S", sideOut);
        EXPECT_EQ("MD", Out(main->RenderAsString({})));
    }
}

// ---------------------------------------------------------------------------------------------------------------------
// E. Values across the boundary

// 3.10 values 3 and 7: a callable returns reflected data, an rvalue (owned), a const& to a host object (borrowed) and a
// shared_ptr (shared); later statements of the render and an include use it
TEST(EmbeddingLifecycleTest, E1_ReflectedReturnValues)
{
    const auto repeat = [](const char* text) {
        std::string result;
        for (int i = 0; i < 14; ++i)
        {
            result += text;
        }
        return result;
    };
    const Person hostPerson{ repeat("bor"), { "p", "q", "r" } };

    CountingFs fs;
    fs.Set("inc.j2", "{{ o.name }}{{ b.tags|length }}{{ s.tags[0] }}{% for t in o.tags %}{{ t }}{% endfor %}");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    env.AddGlobal("owned", MakeCallable([&] { return Reflect(Person{ repeat("own"), { "x", "y" } }); }));
    env.AddGlobal("borrowed", MakeCallable([&hostPerson] { return Reflect(hostPerson); }));
    env.AddGlobal("shared", MakeCallable([&] { return Reflect(std::make_shared<Person>(Person{ repeat("shr"), { "m" } })); }));
    auto tpl = env.FromString(
        "{% set o = owned() %}{% set b = borrowed() %}{% set s = shared() %}{{ o.name }},{{ b.name }},{{ s.name }}|{{ o.tags|join('+') }},{{ b.tags|join('+') }},{{ s.tags|join('+') }}|{% include 'inc.j2' %}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    const std::string expected = repeat("own") + "," + repeat("bor") + "," + repeat("shr") + "|x+y,p+q+r,m|" + repeat("own") + "3mxy";
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ(expected, Out(tpl->RenderAsString({})));
    }
}

// 3.10 values 2: lists, dicts, ranges, map results, namespaces and loop items the template made, kept by a callable, are
// readable after the render, the template and the environment are gone, and after another render reused the thread's tables
TEST(EmbeddingLifecycleTest, E2_TemplateMadeValuesOutliveRenderAndEnv)
{
    const std::string a(40, 'a');
    const std::string b(40, 'b');
    struct Case
    {
        const char* source;
        std::string expected;
    };
    const Case cases[] = {
        { "{{ keep([1, 2, [3, 4]]) }}", "[1,2,[3,4,],]" },
        { "{{ keep({'a': [1, 2]}) }}", "{a:[1,2,],}" },
        { "{{ keep(range(3)) }}", "[0,1,2,]" },
        { "{{ keep(items | map('upper')) }}", "[" + std::string(40, 'A') + "," + std::string(40, 'B') + ",]" },
        { "{% set l = [1, 2] %}{{ keep(l) }}", "[1,2,]" },
        { "{% for x in [[1], [2]] %}{{ keep(x) }}{% endfor %}", "[1,];[2,]" },
        { "{{ keep(namespace(x=5)) }}", "{x:5,}" },
        { "{{ keep(dict(x=[7])) }}", "{x:[7,],}" },
    };
    for (const auto& testCase : cases)
    {
        SCOPED_TRACE(testCase.source);
        auto kept = std::make_shared<Kept>();
        {
            TemplateEnv env;
            Template tpl(&env);
            ASSERT_TRUE(tpl.Load(testCase.source));
            const ValuesMap params{ { "items", ValuesList{ a, b } }, { "keep", KeepCallable(kept) } };
            ASSERT_TRUE(tpl.RenderAsString(params));
            // A second render on the thread reuses the workspace tables
            Template other;
            ASSERT_TRUE(other.Load("{% set q = [9, 9, 9] %}{{ q }}{{ {'zz': 'yy'} }}"));
            ASSERT_TRUE(other.RenderAsString({}));
        }
        std::string text;
        for (const auto& value : kept->values)
        {
            text += (text.empty() ? "" : ";") + Dump(value);
        }
        EXPECT_EQ(testCase.expected, text);
    }
}

// 2.1 and values 4: callables that throw jinja2::Exception, std::runtime_error and a type that is no exception give the
// error code, the location and the exception_ptr; the template and the thread's caches keep working
TEST(EmbeddingLifecycleTest, E4_ThrowingCallables)
{
    TemplateEnv env;
    Template tpl(&env);
    ASSERT_TRUE(tpl.Load("ab\n{{ f() }}", "e.j2"));
    const auto thrower = [](int kind) {
        return MakeCallable([kind]() -> std::string {
            if (kind == 0)
            {
                throw Exception(ErrorCode::InvalidValueType, "typed failure");
            }
            if (kind == 1)
            {
                throw std::runtime_error("std failure");
            }
            throw 42;
        });
    };

    auto typed = tpl.RenderAsString({ { "f", thrower(0) } });
    ASSERT_FALSE(typed);
    EXPECT_EQ(ErrorCode::InvalidValueType, typed.error().GetCode());
    EXPECT_EQ("e.j2", typed.error().GetErrorLocation().fileName);
    EXPECT_EQ(2u, typed.error().GetErrorLocation().line);
    ASSERT_FALSE(typed.error().GetExtraParams().empty());
    EXPECT_EQ("typed failure", typed.error().GetExtraParams()[0].AsString());

    auto standard = tpl.RenderAsString({ { "f", thrower(1) } });
    ASSERT_FALSE(standard);
    EXPECT_EQ(ErrorCode::UserCallableException, standard.error().GetCode());
    EXPECT_EQ("e.j2", standard.error().GetErrorLocation().fileName);
    EXPECT_EQ(2u, standard.error().GetErrorLocation().line);
    ASSERT_FALSE(standard.error().GetExtraParams().empty());
    EXPECT_EQ("std failure", standard.error().GetExtraParams()[0].AsString());
    ASSERT_TRUE(standard.error().GetException());
    EXPECT_THROW(std::rethrow_exception(standard.error().GetException()), std::runtime_error);

    auto foreign = tpl.RenderAsString({ { "f", thrower(2) } });
    ASSERT_FALSE(foreign);
    EXPECT_EQ("e.j2", foreign.error().GetErrorLocation().fileName);
    EXPECT_EQ(2u, foreign.error().GetErrorLocation().line);
    ASSERT_TRUE(foreign.error().GetException());
    try
    {
        std::rethrow_exception(foreign.error().GetException());
        ADD_FAILURE() << "rethrow_exception returned";
    }
    catch (int value)
    {
        EXPECT_EQ(42, value);
    }

    // The same template and thread render again, and so does another template of the environment
    EXPECT_EQ("ab\nok", Out(tpl.RenderAsString({ { "f", MakeCallable([]() { return std::string("ok"); }) } })));
    EXPECT_EQ("fine", Out(env.FromString("fine").value().RenderAsString({})));
    EXPECT_TRUE(ReleaseThreadCaches());
}

// 3.10 values 6: exports made with MakeModule(params) that refer to the params are read after the params, the template
// and the module are gone; an item read from an exported list is valid while the list lives
TEST(EmbeddingLifecycleTest, E5_ModuleExportsOwnTheirStorage)
{
    const std::string a(40, 'a');
    const std::string b(40, 'b');
    std::optional<TemplateModule> module;
    {
        Template tpl;
        ASSERT_TRUE(tpl.Load("{% set x = items %}{% set y = items | map('upper') | list %}{% set z = data %}"));
        const ValuesMap params{ { "items", ValuesList{ a, b } }, { "data", ValuesMap{ { "k", ValuesList{ 1, 2 } } } } };
        auto made = tpl.MakeModule(params);
        ASSERT_TRUE(made) << made.error().ToString();
        module = std::move(*made);
    }
    const std::string upper = "[" + std::string(40, 'A') + "," + std::string(40, 'B') + ",]";
    EXPECT_EQ("[" + a + "," + b + ",]", Dump(module->exports["x"]));
    EXPECT_EQ(upper, Dump(module->exports["y"]));
    EXPECT_EQ("{k:[1,2,],}", Dump(module->exports["z"]));

    Value x = module->exports["x"];
    Value y = module->exports["y"];
    Value z = module->exports["z"];
    GenericList xs = x.ToGenericList();
    std::optional<Value> firstItem;
    for (const auto& item : xs)
    {
        firstItem = item;
        break;
    }
    module.reset();
    EXPECT_TRUE(ReleaseThreadCaches());
    EXPECT_EQ("[" + a + "," + b + ",]", Dump(x));
    EXPECT_EQ(upper, Dump(y));
    EXPECT_EQ("{k:[1,2,],}", Dump(z));
    ASSERT_TRUE(firstItem.has_value());
    EXPECT_EQ(a, Dump(firstItem.value()));
}

namespace
{
struct StringMapAccessor : IMapItemAccessor
{
    std::map<std::string, std::string> items;

    [[nodiscard]] size_t GetSize() const override { return items.size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return items.count(name) != 0; }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        const auto p = items.find(name);
        return p == items.end() ? Value() : Value(p->second);
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> keys;
        keys.reserve(items.size());
        for (const auto& item : items)
        {
            keys.push_back(item.first);
        }
        return keys;
    }
};

struct IntMapAccessor : IMapItemAccessor
{
    std::map<std::string, int64_t> items;

    [[nodiscard]] size_t GetSize() const override { return items.size(); }
    [[nodiscard]] bool HasValue(const std::string& name) const override { return items.count(name) != 0; }
    [[nodiscard]] Value GetValueByName(const std::string& name) const override
    {
        const auto p = items.find(name);
        return p == items.end() ? Value() : Value(p->second);
    }
    [[nodiscard]] std::vector<std::string> GetKeys() const override
    {
        std::vector<std::string> keys;
        keys.reserve(items.size());
        for (const auto& item : items)
        {
            keys.push_back(item.first);
        }
        return keys;
    }
};

template<typename CharT>
void CheckGenericContainersAcrossStatements()
{
    // Borrowed: the host's vector and accessor outlive every render. Owned: the callable and accessor live in the Value
    const std::vector<std::string> hostList{ "a", "b" };
    StringMapAccessor hostMap;
    hostMap.items = { { "a", "A" }, { "b", "B" } };
    const std::vector<int64_t> ownedItems{ 1, 2, 3 };
    IntMapAccessor ownedMap;
    ownedMap.items = { { "x", 1 }, { "y", 2 } };

    CountingFs fs;
    fs.Set("inc.j2", "{% for k, v in mo|dictsort %}{{ k }}={{ v }};{% endfor %}{{ mb.a }}{{ lb|length }}");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    auto tpl = FromStringIn(env, "{% macro show(l) %}{% for x in l %}{{ x }},{% endfor %}{% endmacro %}{{ show(lo) }}|{{ show(lb) }}|{% include 'inc.j2' %}", Tag<CharT>{});
    ASSERT_TRUE(tpl);
    for (int i = 0; i < 3; ++i)
    {
        // A generated list is walked once, so each render gets its own
        ValuesMap params{
            { "lo", MakeGenericList([items = ownedItems, pos = size_t{ 0 }]() mutable -> std::optional<Value> {
                  if (pos == items.size())
                  {
                      return std::nullopt;
                  }
                  return Value(items[pos++]);
              }) },
            { "lb", MakeGenericList(hostList.begin(), hostList.end()) },
            { "mo", GenericMap([accessor = ownedMap]() { return &accessor; }) },
            { "mb", GenericMap([&hostMap]() { return &hostMap; }) },
        };
        EXPECT_EQ(Str<CharT>("1,2,3,|a,b,|x=1;y=2;A2"), tpl->RenderAsString(params).value());
    }
}
} // namespace

// 3.10 values 1: GenericList and GenericMap params, owned and borrowed, read across macros, loops and an include, again on later renders
TEST(EmbeddingLifecycleTest, E6_GenericContainersAcrossStatements)
{
    CheckGenericContainersAcrossStatements<char>();
    CheckGenericContainersAcrossStatements<wchar_t>();
}

// ---------------------------------------------------------------------------------------------------------------------
// F. Stored macro, self, super and caller crossing frames. The plain forms are in the parity corpus; these need the host

// 3.10 threads 1 and values: a macro stored in a namespace inside a loop and called after it, with the frames reused
// by a second loop of another shape; on four threads
TEST(EmbeddingLifecycleTest, F1_MacroStoredInLoopCalledAfterIt)
{
    Template tpl;
    ASSERT_TRUE(tpl.Load("{% set ns = namespace(m=none) %}{% set base = 'B' %}{% for i in [1, 2, 3] %}{% macro mm(x) %}[{{ base }}:{{ x }}]{% endmacro %}{% if i == 2 %}{% set ns.m = mm %}{% endif %}{% endfor %}"
                         "{% for a, b in [(10, 20), (30, 40)] %}{% set t = a + b %}{{ t }},{% endfor %}{{ ns.m(w) }}"));
    Latch start;
    Failures failures;
    std::vector<std::thread> workers;
    workers.reserve(4);
    for (int t = 0; t < 4; ++t)
    {
        workers.emplace_back([&, t] {
            EXPECT_TRUE(start.Wait());
            for (int i = 0; i < 50; ++i)
            {
                const std::string w = "w" + std::to_string((t * 100) + i);
                const std::string actual = Out(tpl.RenderAsString({ { "w", w } }));
                if (actual != "30,70,[B:" + w + "]")
                {
                    failures.Add(actual);
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

// 3.10 caching 1 and 3: `self` stored by an include and called after the include ended (0207), with the include
// reloaded in between; the stored one keeps the old template
TEST(EmbeddingLifecycleTest, F2_SelfStoredByIncludeSurvivesReload)
{
    CountingFs fs(true);
    fs.Set("inc.j2", "{% block blk %}B{{ y }}{% endblock %}{% set ns.s = self %}");
    TemplateEnv env;
    env.AddFilesystemHandler("", fs);
    env.AddGlobal("reload", MakeCallable([&]() {
                      fs.Set("inc.j2", "{% block blk %}N{{ y }}{% endblock %}{% set ns.s = self %}");
                      fs.Touch("inc.j2");
                      return env.LoadTemplate("inc.j2").has_value() ? std::string() : std::string("reload failed");
                  }));
    auto tpl = env.FromString("{% set ns = namespace(s=none) %}{% include 'inc.j2' %}{{ reload() }}|{{ ns.s.blk() }}");
    ASSERT_TRUE(tpl) << tpl.error().ToString();
    // The variable is a render parameter: one the includer sets itself is not seen by the stored `self` (see the task notes)
    const ValuesMap params{ { "y", "Y" } };
    EXPECT_EQ("BY|BY", Out(tpl->RenderAsString(params)));
    EXPECT_EQ("NY|NY", Out(tpl->RenderAsString(params)));
}

// 3.10 destruction order: `super` stored in a block and called after the block ended, while the environment, which
// cached the parent, is destroyed from inside the block
TEST(EmbeddingLifecycleTest, F3_SuperStoredInBlockAfterEnvDestroyed)
{
    CountingFs fs;
    fs.Set("parent.j2", "{% block b %}P{% endblock %}|{{ ns.s() }}");
    fs.Set("child.j2", "{% extends 'parent.j2' %}{% set ns = namespace(s=none) %}{% block b %}{% set ns.s = super %}C{{ super() }}{{ drop() }}{% endblock %}");
    auto env = std::make_unique<TemplateEnv>();
    env->AddFilesystemHandler("", fs);
    auto child = env->LoadTemplate("child.j2");
    ASSERT_TRUE(child) << child.error().ToString();
    const ValuesMap params{ { "drop", MakeCallable([&env]() {
                                  env.reset();
                                  return std::string();
                              }) } };
    EXPECT_EQ("CP|P", Out(child->RenderAsString(params)));
    EXPECT_FALSE(env);
    EXPECT_EQ("CP|P", Out(child->RenderAsString(params)));
}

// 3.10 caching 3 and 4: macros of two imported modules stored by a macro and called after it returned, with caching off,
// and after the environment object is gone
TEST(EmbeddingLifecycleTest, F4_ImportedMacrosCalledAfterImporterReturned)
{
    for (const bool dropEnv : { false, true })
    {
        SCOPED_TRACE(dropEnv);
        CountingFs fs;
        fs.Set("lib.j2", "{% macro hi(x) %}hi {{ x }}{% endmacro %}");
        fs.Set("lib2.j2", "{% macro hi(x) %}ho {{ x }}{% endmacro %}");
        auto env = std::make_unique<TemplateEnv>();
        env->GetSettings().cacheSize = 0;
        env->AddFilesystemHandler("", fs);
        auto tpl = env->FromString(
            "{% set ns = namespace(f=none, g=none) %}{% macro get() %}{% import 'lib.j2' as lib %}{% import 'lib2.j2' as lib2 %}{% set ns.f = lib.hi %}{% set ns.g = lib2.hi %}{% endmacro %}"
            "{{ get() }}{{ drop() }}{{ ns.f('a') }}|{{ ns.g('b') }}");
        ASSERT_TRUE(tpl) << tpl.error().ToString();
        const ValuesMap params{ { "drop", MakeCallable([&]() {
                                      if (dropEnv)
                                      {
                                          env.reset();
                                      }
                                      return std::string();
                                  }) } };
        EXPECT_EQ("hi a|ho b", Out(tpl->RenderAsString(params)));
        EXPECT_EQ("hi a|ho b", Out(tpl->RenderAsString(params)));
        EXPECT_EQ(!dropEnv, !!env);
    }
}

// 3.10 values: `caller` stored by a macro and called after the macro returned (a closure over the enclosing macro's names is
// not pinned: it fails on master, see the task notes)
TEST(EmbeddingLifecycleTest, F5_CallerCalledAfterItsMacroReturned)
{
    const char* source = "{% set ns = namespace(c=none) %}{% set v = 'V' %}{% macro m() %}{% set ns.c = caller %}x{% endmacro %}{% call m() %}body {{ v }}{% endcall %}|{{ ns.c() }}";
    Template narrow;
    ASSERT_TRUE(narrow.Load(source));
    TemplateW wide;
    ASSERT_TRUE(wide.Load(Str<wchar_t>(source)));
    for (int i = 0; i < 3; ++i)
    {
        EXPECT_EQ("x|body V", Out(narrow.RenderAsString({})));
        EXPECT_EQ(L"x|body V", wide.RenderAsString({}).value());
    }
}
