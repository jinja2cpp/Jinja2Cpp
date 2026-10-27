// Allocation-failure injection (docs/api-2.0.md 2.1, task 0253): std::bad_alloc is the only exception Load and
// Render let out. A replaced global operator new fails the Nth allocation, for every N, while a template loads or
// renders; each run must succeed or throw std::bad_alloc and leave the template and environment usable. Run under
// ASan, it also checks that the unwinding leaks nothing.
//
// A separate executable: the replacement would hide the sanitizers' own operator new from every other test. With a
// Windows DLL build the library keeps its CRT's operator new, so the tests skip there.

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/json_adapter.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include "binding/toy_json.h"

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <utility>
#include <new>
#include <string>
#include <thread>
#include <vector>
#if defined(_MSC_VER) && defined(__cpp_lib_stacktrace)
#include <iostream>
#include <stacktrace>
#endif

// NOLINTBEGIN(cppcoreguidelines-no-malloc): the replaced global allocation functions sit on malloc and free
namespace
{
// The number of allocations still to succeed before one fails; negative when no failure is armed
std::atomic<long> g_countdown{ -1 };
// The countdown last armed, for the crash report below
std::atomic<long> g_armed{ -1 };
// The allocations not freed yet, for the steady-state checks
std::atomic<long> g_live{ 0 };

// MSVC's checked iterators (_ITERATOR_DEBUG_LEVEL above 0) give every container a heap proxy, made
// in noexcept constructors such as basic_string(), so failing one ends in std::terminate. A
// debugging aid of the standard library, not the engine's: those allocations are never failed.
bool Injectable([[maybe_unused]] std::size_t size)
{
#if defined(_MSC_VER) && defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL != 0
    return size != sizeof(std::_Container_proxy);
#else
    return true;
#endif
}

void* Allocate(std::size_t size)
{
    if (g_countdown.load(std::memory_order_relaxed) >= 0 && Injectable(size)
        && g_countdown.fetch_sub(1, std::memory_order_relaxed) == 0)
    {
        throw std::bad_alloc();
    }
    if (void* p = std::malloc(size == 0 ? 1 : size))
    {
        g_live.fetch_add(1, std::memory_order_relaxed);
        return p;
    }
    throw std::bad_alloc();
}

void Release(void* p) noexcept
{
    if (p)
    {
        g_live.fetch_sub(1, std::memory_order_relaxed);
        std::free(p);
    }
}
} // namespace

void* operator new(std::size_t size)
{
    return Allocate(size);
}
void* operator new[](std::size_t size)
{
    return Allocate(size);
}
// The nothrow forms too: a sanitizer would otherwise pair its own with the free below
void* operator new(std::size_t size, const std::nothrow_t& /*tag*/) noexcept
{
    try
    {
        return Allocate(size);
    }
    catch (const std::bad_alloc&)
    {
        return nullptr;
    }
}
void* operator new[](std::size_t size, const std::nothrow_t& tag) noexcept
{
    return operator new(size, tag);
}
void operator delete(void* p, const std::nothrow_t& /*tag*/) noexcept
{
    Release(p);
}
void operator delete[](void* p, const std::nothrow_t& /*tag*/) noexcept
{
    Release(p);
}
void operator delete(void* p) noexcept
{
    Release(p);
}
void operator delete[](void* p) noexcept
{
    Release(p);
}
void operator delete(void* p, std::size_t /*size*/) noexcept
{
    Release(p);
}
void operator delete[](void* p, std::size_t /*size*/) noexcept
{
    Release(p);
}
// NOLINTEND(cppcoreguidelines-no-malloc)

namespace
{
using namespace jinja2;

// Fails the allocation after `succeed` more have succeeded, until disarmed
struct Injection
{
    explicit Injection(long succeed)
    {
        g_armed.store(succeed, std::memory_order_relaxed);
        g_countdown.store(succeed, std::memory_order_relaxed);
    }
    ~Injection() { g_countdown.store(-1, std::memory_order_relaxed); }
    Injection(const Injection&) = delete;
    Injection& operator=(const Injection&) = delete;
    // Whether the armed failure happened
    static bool Fired() { return g_countdown.load(std::memory_order_relaxed) < 0; }
};

constexpr long maxAllocations = 100000;

// A failure that ends the process (std::terminate, an abort, an access violation) prints no gtest
// result, so say which allocation was failing, with a stack where the library offers one (MSVC at
// C++23): the injected failures can only be traced on the platform that crashes
void ReportCrash(const char* what)
{
    g_countdown.store(-1, std::memory_order_relaxed);
    std::fprintf(stderr, "%s with allocation %ld failing\n", what, g_armed.load(std::memory_order_relaxed));
#if defined(_MSC_VER) && defined(__cpp_lib_stacktrace)
    std::cerr << std::stacktrace::current() << std::endl;
#endif
    std::fflush(stderr);
}

void InstallCrashReport()
{
    std::set_terminate([] {
        ReportCrash("std::terminate");
        std::abort();
    });
    for (const int sig : { SIGSEGV, SIGABRT })
    {
        std::signal(sig, [](int signal) {
            ReportCrash(signal == SIGSEGV ? "SIGSEGV" : "SIGABRT");
            std::_Exit(3);
        });
    }
}

// A Windows DLL allocates with its own CRT's operator new, which the replacement above does not reach
#if defined(_WIN32) && defined(JINJA2CPP_LINK_AS_SHARED)
#define JINJA2CPP_SKIP_UNREACHED_INJECTION() GTEST_SKIP() << "the library's allocations do not reach the replaced operator new"
#else
#define JINJA2CPP_SKIP_UNREACHED_INJECTION() static_cast<void>(0)
#endif

// Runs f on a new thread, so that the engine's per-thread pools and caches start cold and their
// first allocations are reached too
template<typename F>
void OnFreshThread(F&& f)
{
    InstallCrashReport();
    std::thread thread(std::forward<F>(f));
    thread.join();
}

struct Case
{
    std::string source;
    std::string expected;
};

class AllocationFailureTest : public testing::TestWithParam<Case>
{
protected:
    void SetUp() override
    {
        auto fs = std::make_shared<MemoryFileSystem>();
        fs->AddFile("header.j2", "<h1>{{ title|upper }}</h1>");
        fs->AddFile("macros.j2", "{% macro item(x, sep=', ') %}[{{ x }}]{{ sep }}{% endmacro %}");
        fs->AddFile("base.j2", "<{% block body %}base{% endblock %}>");
        m_env.AddFilesystemHandler(std::string(), fs);
    }

    [[nodiscard]] ValuesMap Params() const
    {
        return {
            { "title", "Report" },
            { "items", ValuesList{ 1, 2, 3, 4, 5 } },
            { "user", ValuesMap{ { "name", "Ann" }, { "roles", ValuesList{ "admin", "dev" } } } },
            { "greet", MakeCallable([](const std::string& name) { return "Hello, " + name + "!"; }, ArgInfo{ "name" }) },
        };
    }

    TemplateEnv m_env;
};

TEST_P(AllocationFailureTest, LoadSucceedsOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    const auto& param = GetParam();
    long n = 0;
    for (; n < maxAllocations; ++n)
    {
        Template tpl(&m_env);
        bool threw = false;
        bool loaded = false;
        bool fired = false;
        OnFreshThread([&] {
            Injection injection(n);
            try
            {
                loaded = tpl.Load(param.source).has_value();
            }
            catch (const std::bad_alloc&)
            {
                threw = true;
            }
            fired = Injection::Fired();
        });
        if (!threw)
        {
            ASSERT_TRUE(loaded) << "allocation " << n << " failed and Load returned an error\n"
                                << param.source;
        }
        else
        {
            ASSERT_TRUE(tpl.Load(param.source).has_value()) << param.source;
        }
        auto result = tpl.RenderAsString(Params());
        ASSERT_TRUE(result.has_value()) << "after allocation " << n << " failed: " << result.error().ToString();
        EXPECT_EQ(param.expected, result.value()) << "after allocation " << n << " failed";
        if (!fired)
        {
            break;
        }
    }
    RecordProperty("allocations", static_cast<int>(n));
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

TEST_P(AllocationFailureTest, RenderSucceedsOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    const auto& param = GetParam();
    Template tpl(&m_env);
    ASSERT_TRUE(tpl.Load(param.source).has_value()) << param.source;
    const auto params = Params();
    long n = 0;
    for (; n < maxAllocations; ++n)
    {
        bool threw = false;
        bool fired = false;
        Result<std::string> result;
        OnFreshThread([&] {
            Injection injection(n);
            try
            {
                result = tpl.RenderAsString(params);
            }
            catch (const std::bad_alloc&)
            {
                threw = true;
            }
            fired = Injection::Fired();
        });
        if (!threw)
        {
            ASSERT_TRUE(result.has_value()) << "allocation " << n << " failed and Render returned an error: " << result.error().ToString();
            EXPECT_EQ(param.expected, result.value()) << "allocation " << n << " failed and Render printed something else";
        }
        auto again = tpl.RenderAsString(params);
        ASSERT_TRUE(again.has_value()) << "after allocation " << n << " failed: " << again.error().ToString();
        EXPECT_EQ(param.expected, again.value()) << "after allocation " << n << " failed";
        if (!fired)
        {
            break;
        }
    }
    RecordProperty("allocations", static_cast<int>(n));
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

TEST_F(AllocationFailureTest, MetadataSucceedsOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    const std::string source = R"({% meta %}{"title": "Report", "tags": ["a", "b"]}{% endmeta %}x)";
    long n = 0;
    for (; n < maxAllocations; ++n)
    {
        Template tpl(&m_env);
        ASSERT_TRUE(tpl.Load(source).has_value());
        bool threw = false;
        bool fired = false;
        Result<GenericMap> metadata;
        OnFreshThread([&] {
            Injection injection(n);
            try
            {
                metadata = tpl.GetMetadata();
            }
            catch (const std::bad_alloc&)
            {
                threw = true;
            }
            fired = Injection::Fired();
        });
        if (!threw)
        {
            ASSERT_TRUE(metadata.has_value()) << "allocation " << n << " failed: " << metadata.error().ToString();
        }
        auto again = tpl.GetMetadata();
        ASSERT_TRUE(again.has_value()) << "after allocation " << n << " failed: " << again.error().ToString();
        EXPECT_TRUE(again.value().HasValue("title"));
        if (!fired)
        {
            break;
        }
    }
    RecordProperty("allocations", static_cast<int>(n));
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

// A big integer read from JSON text either arrives exact or the copy or the view throws std::bad_alloc; running out
// of memory is never reported as a malformed integer (DataReadError) or read as none (task 0311)
TEST(JsonAllocationFailureTest, BigIntegerIsExactOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    const std::string text = "[123456789012345678901234567890123456789012345678901234567890]";
    const std::string digits = text.substr(1, text.size() - 2);
    for (const bool view : { false, true })
    {
        SCOPED_TRACE(view ? "Reflect" : "ToValue");
        long n = 0;
        for (; n < maxAllocations; ++n)
        {
            ToyStreamDoc stream(toyjson::Parser(text, true).Parse());
            ToyViewDoc doc(toyjson::Parser(text, true).Parse());
            bool threw = false;
            bool fired = false;
            std::string read;
            std::string failure;
            OnFreshThread([&] {
                Injection injection(n);
                try
                {
                    if (view)
                    {
                        const Value item = Reflect(&doc).ToGenericList().GetAccessor()->GetIndexer()->GetItemByIndex(0);
                        read = item.IsBigInt() ? item.AsBigInt().ToString() : "<not a BigInt>";
                    }
                    else
                    {
                        auto copy = ToValue(stream);
                        if (copy)
                        {
                            const Value& item = copy->AsList()[0];
                            read = item.IsBigInt() ? item.AsBigInt().ToString() : "<not a BigInt>";
                        }
                        else
                        {
                            failure = copy.error().ToString();
                        }
                    }
                }
                catch (const std::bad_alloc&)
                {
                    threw = true;
                }
                fired = Injection::Fired();
            });
            if (!threw)
            {
                ASSERT_EQ("", failure) << "allocation " << n;
                ASSERT_EQ(digits, read) << "allocation " << n;
            }
            if (!fired)
            {
                break;
            }
        }
        EXPECT_LT(0, n) << "no allocation reached the injection";
        EXPECT_LT(n, maxAllocations);
    }
}

// A module the host keeps and calls once per request: what a call imports with context ends
// with the call, so the module stops growing after its first calls (docs/tasks/0301)
class ModuleCallTest : public AllocationFailureTest
{
protected:
    jinja2::TemplateModule MakeModule(const std::string& body)
    {
        auto fs = std::make_shared<MemoryFileSystem>();
        fs->AddFile("lib", "{% macro x() %}X{% endmacro %}{% set v = [1, 2, 3] %}");
        fs->AddFile("fails", "{% import 'lib' as l with context %}{{ 1 // 0 }}");
        m_env.AddFilesystemHandler(std::string(), fs);
        EXPECT_TRUE(!!m_tpl.Load("{% macro m() %}" + body + "{% endmacro %}"));
        auto module = m_tpl.MakeModule();
        EXPECT_TRUE(!!module);
        return module ? std::move(*module) : jinja2::TemplateModule{};
    }

    static std::string Call(const jinja2::Value& fn)
    {
        const jinja2::UserCallableParams params;
        return fn.As<jinja2::UserCallable>().callable(params).AsString();
    }

    Template m_tpl{ &m_env };
};

TEST_F(ModuleCallTest, CallsStayFlat)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    for (const char* body : { "{% import 'lib' as l with context %}{{ l.x() }}{% include 'lib' %}",
                              "{% import 'lib' as l %}{{ l.x() }}",
                              "{% from 'lib' import x with context %}{{ x() }}",
                              // A module shared without context whose top level fails each time
                              "{% import 'fails' as f %}",
                              // A call that fails leaves no scope on the module's context
                              "{% for i in [1] %}{% set q = 2 %}{{ 1 // 0 }}{% endfor %}",
                              // An invalid name fails each call, with nothing kept
                              "{% include 42 %}" })
    {
        auto module = MakeModule(body);
        const auto& m = module.exports["m"];
        const auto callMany = [&m](int count) {
            for (int i = 0; i != count; ++i)
            {
                try
                {
                    Call(m);
                }
                catch (const jinja2::BadResultAccess<>&) // NOLINT(bugprone-empty-catch): the failing case fails every call
                {
                }
            }
        };
        callMany(100);
        const long before = g_live.load(std::memory_order_relaxed);
        callMany(2000);
        // Each call kept about 30 allocations before 0301
        EXPECT_LE(g_live.load(std::memory_order_relaxed) - before, 16) << body;
    }
}

// A call that fails at any allocation ends what it imported, and the module goes on working
TEST_F(ModuleCallTest, CallSucceedsOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    auto module = MakeModule("{% set ns = namespace() %}{% import 'lib' as l with context %}{% set ns.f = l.x %}{{ ns.f() }}{{ l.v|length }}");
    const auto& m = module.exports["m"];
    ASSERT_EQ("X3", Call(m));
    long n = 0;
    for (; n != maxAllocations; ++n)
    {
        bool fired = false;
        bool threw = false;
        std::string result;
        OnFreshThread([&] {
            const Injection injection(n);
            try
            {
                result = Call(m);
            }
            catch (const std::bad_alloc&)
            {
                threw = true;
            }
            fired = Injection::Fired();
        });
        ASSERT_EQ("X3", Call(m)) << "allocation " << n;
        if (!threw)
        {
            ASSERT_EQ("X3", result) << "allocation " << n;
        }
        if (!fired)
        {
            break;
        }
    }
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

// A module call whose copy on write fails leaves what the host kept from an earlier call as it
// was, and the next call copies again (docs/tasks/0331, D8)
TEST_F(AllocationFailureTest, KeptModuleSnapshotSurvivesAFailedCopy)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro add(x) %}{% set _ = l.append(x) %}{% set _ = d.update(b=x) %}{% set _ = keep(l) %}"
                           "{% set _ = keep(d) %}{% endmacro %}"));
    const auto show = [](const Value& v) {
        Template shown;
        EXPECT_TRUE(!!shown.Load("{{ k }}"));
        return shown.RenderAsString(ValuesMap{ { "k", v } }).value();
    };
    long n = 0;
    for (; n != maxAllocations; ++n)
    {
        std::vector<Value> kept;
        ValuesMap params{ { "l", ValuesList{ 0 } }, { "d", ValuesMap{ { "a", 0 } } } };
        params["keep"] = UserCallable(
            [&kept](const UserCallableParams& p) {
                kept.push_back(p["v"]);
                return Value();
            },
            { ArgInfo{ "v" } });
        auto module = tpl.MakeModule(params);
        ASSERT_TRUE(!!module);
        const auto call = [&module](int64_t x) {
            UserCallableParams callParams;
            callParams.extraPosArgs = ValuesList{ x };
            module->exports["add"].As<UserCallable>().callable(callParams);
        };
        call(1);
        ASSERT_EQ(2u, kept.size());
        const Value list = kept[0];
        const Value dict = kept[1];
        bool fired = false;
        OnFreshThread([&] {
            const Injection injection(n);
            try
            {
                call(2);
            }
            catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): a failed call is one of the outcomes checked
            {
            }
            fired = Injection::Fired();
        });
        call(3);
        ASSERT_EQ("[0, 1]", show(list)) << "allocation " << n;
        ASSERT_EQ("{'a': 0, 'b': 1}", show(dict)) << "allocation " << n;
        if (!fired)
        {
            break;
        }
    }
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

INSTANTIATE_TEST_SUITE_P(Templates,
                         AllocationFailureTest,
                         testing::Values(Case{ "Hello, {{ user.name }}!", "Hello, Ann!" },
                                         Case{ "{% for i in items if i is odd %}{{ loop.index }}:{{ i * 2 }};{% endfor %}", "1:2;2:6;3:10;" },
                                         Case{ "{{ items|map('string')|join('-') }} {{ user.roles|sort|first }} {{ '%s=%d'|format('a', 1) }}",
                                               "1-2-3-4-5 admin a=1" },
                                         Case{ "{% set d = {'k': [1, 'two', 3.5]} %}{{ d.k|length }}{{ d|tojson }}{{ greet(user.name) }}",
                                               "3{\"k\": [1, \"two\", 3.5]}Hello, Ann!" },
                                         Case{ "{% include 'header.j2' %}{% from 'macros.j2' import item %}{% for i in items[:2] %}{{ item(i) }}{% endfor %}",
                                               "<h1>REPORT</h1>[1], [2], " },
                                         Case{ "{% extends 'base.j2' %}{% block body %}{{ super() }}+{{ title }}{% endblock %}", "<base+Report>" },
                                         // Reference cycles, freed when the render ends: none leaks when an
                                         // allocation fails (docs/tasks/0283, LeakSanitizer in the sanitizer job)
                                         Case{ "{% set ns = namespace(l=[]) %}{% set ns.self = ns %}{% set l = [1] %}{{ l.append(l) or '' }}"
                                               "{% set ns.c = cycler(ns) %}{% set ns.f = ns.l.append %}{{ l|length }}",
                                               "2" },
                                         // Writes to the host's data through its aliases, and a cycle through
                                         // its cell (docs/tasks/0049)
                                         Case{ "{% set y = items %}{% set r = user.roles %}{{ y.append(6) or '' }}{{ r.append('ops') or '' }}"
                                               "{% set f = user.update %}{{ f(x=1) or '' }}{{ items.append(items) or '' }}{{ items|length }}{{ user.roles|length }}{{ user.x }}",
                                               "731" }));
} // namespace
