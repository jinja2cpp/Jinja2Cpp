// Allocation-failure injection (docs/api-2.0.md 2.1, task 0253): std::bad_alloc is the only exception Load and
// Render let out. A replaced global operator new fails the Nth allocation, for every N, while a template loads or
// renders; each run must succeed or throw std::bad_alloc and leave the template and environment usable. Run under
// ASan, it also checks that the unwinding leaks nothing.
//
// A separate executable: the replacement would hide the sanitizers' own operator new from every other test. With a
// Windows DLL build the library keeps its CRT's operator new, so the tests skip there.

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <memory>
#include <utility>
#include <new>
#include <string>
#include <thread>
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
        return p;
    }
    throw std::bad_alloc();
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
    std::free(p);
}
void operator delete[](void* p, const std::nothrow_t& /*tag*/) noexcept
{
    std::free(p);
}
void operator delete(void* p) noexcept
{
    std::free(p);
}
void operator delete[](void* p) noexcept
{
    std::free(p);
}
void operator delete(void* p, std::size_t /*size*/) noexcept
{
    std::free(p);
}
void operator delete[](void* p, std::size_t /*size*/) noexcept
{
    std::free(p);
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
                                         Case{ "{% extends 'base.j2' %}{% block body %}{{ super() }}+{{ title }}{% endblock %}", "<base+Report>" }));
} // namespace
