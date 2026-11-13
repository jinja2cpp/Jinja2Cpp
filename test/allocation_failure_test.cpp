// Allocation-failure injection (docs/api-2.0.md 2.1, task 0253): std::bad_alloc is the only exception Load and
// Render let out. A replaced global operator new fails the Nth allocation, for every N, while a template loads or
// renders; each run must succeed or throw std::bad_alloc and leave the template and environment usable. Run under
// ASan, it also checks that the unwinding leaks nothing.
//
// With JINJA2CPP_ALLOC_EXHAUSTED=1 (the jinja2cpp_alloc_tests_exhausted run), every allocation after the Nth fails too,
// as when memory is really spent: catch handlers, unwinding and the error paths then cannot allocate either (task 0256).
//
// A separate executable: the replacement would hide the sanitizers' own operator new from every other test. With a
// Windows DLL build the library keeps its CRT's operator new, so the tests skip there.

#include <jinja2cpp/binding/nlohmann_json.h>
#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/json_adapter.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <gtest/gtest.h>

#include "binding/toy_json.h"

#if defined(__linux__) && defined(__SANITIZE_ADDRESS__)
#define JINJA2CPP_ALLOC_TESTS_LEAK_CHECK 1
#elif defined(__linux__) && defined(__has_feature)
#if __has_feature(address_sanitizer)
#define JINJA2CPP_ALLOC_TESTS_LEAK_CHECK 1
#endif
#endif
#ifdef JINJA2CPP_ALLOC_TESTS_LEAK_CHECK
#include <sanitizer/lsan_interface.h>
#endif

#include <atomic>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <exception>
#include <map>
#include <memory>
#include <new>
#include <string>
#include <utility>
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
// Set when the armed failure happens in the exhausted mode: every later allocation fails too, until disarmed
std::atomic<bool> g_spent{ false };

// JINJA2CPP_ALLOC_EXHAUSTED=1: memory stays spent once the armed failure happens. Read with malloc, before any test
bool ReadExhaustedMode() noexcept
{
#ifdef _MSC_VER
    // std::getenv is C4996 under /W4 /WX
    char* value = nullptr;
    _dupenv_s(&value, nullptr, "JINJA2CPP_ALLOC_EXHAUSTED");
    const bool exhausted = value != nullptr && value[0] == '1';
    std::free(value);
    return exhausted;
#else
    const char* value = std::getenv("JINJA2CPP_ALLOC_EXHAUSTED");
    return value != nullptr && value[0] == '1';
#endif
}
const bool g_exhausted = ReadExhaustedMode();

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
    if (g_spent.load(std::memory_order_relaxed) && Injectable(size))
    {
        throw std::bad_alloc();
    }
    if (g_countdown.load(std::memory_order_relaxed) >= 0 && Injectable(size)
        && g_countdown.fetch_sub(1, std::memory_order_relaxed) == 0)
    {
        g_spent.store(g_exhausted, std::memory_order_relaxed);
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

// Fails the allocation after `succeed` more have succeeded (and, in the exhausted mode, every one after it), until
// disarmed
struct Injection
{
    explicit Injection(long succeed)
    {
        g_armed.store(succeed, std::memory_order_relaxed);
        g_countdown.store(succeed, std::memory_order_relaxed);
    }
    ~Injection()
    {
        g_countdown.store(-1, std::memory_order_relaxed);
        g_spent.store(false, std::memory_order_relaxed);
    }
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
    g_spent.store(false, std::memory_order_relaxed);
    std::fprintf(stderr, "%s with allocation %ld failing%s\n", what, g_armed.load(std::memory_order_relaxed), g_exhausted ? " (and every one after it)" : "");
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

// Under LeakSanitizer (the ASan jobs), each test ends with a leak check, not only the process: the first test that
// fails it made the leak (a check reports every leak still unreachable, so the tests after it fail too, task 0256)
class LeakCheckedTest : public testing::Test
{
protected:
    void TearDown() override
    {
#ifdef JINJA2CPP_ALLOC_TESTS_LEAK_CHECK
        EXPECT_EQ(0, __lsan_do_recoverable_leak_check()) << "LeakSanitizer found leaks (reported above)";
#endif
    }
};

// Stops LeakSanitizer reporting what this thread allocates while it lives, when `pause` is set
class LeakCheckPause
{
public:
    explicit LeakCheckPause([[maybe_unused]] bool pause)
#ifdef JINJA2CPP_ALLOC_TESTS_LEAK_CHECK
        : m_paused(pause)
    {
        if (m_paused)
        {
            __lsan_disable();
        }
    }
    ~LeakCheckPause()
    {
        if (m_paused)
        {
            __lsan_enable();
        }
    }
#else
    {
    }
    ~LeakCheckPause() = default;
#endif
    LeakCheckPause(const LeakCheckPause&) = delete;
    LeakCheckPause& operator=(const LeakCheckPause&) = delete;

private:
#ifdef JINJA2CPP_ALLOC_TESTS_LEAK_CHECK
    bool m_paused;
#endif
};

struct Case
{
    std::string source;
    std::string expected;
    // Makes reference cycles: when memory stays spent, the walk at the end of the render that breaks them cannot
    // allocate its tables and they leak (known gap, task 0486), so the exhausted mode does not leak-check its Render
    bool makesCycles = false;
};

// The sources and outputs are ASCII: a wide row widens each character
template<typename CharT>
std::basic_string<CharT> Widen(const std::string& text)
{
    return std::basic_string<CharT>(text.begin(), text.end());
}

class AllocationFailureTest : public LeakCheckedTest
    , public testing::WithParamInterface<Case>
{
protected:
    void SetUp() override
    {
        auto fs = std::make_shared<MemoryFileSystem>();
        fs->AddFile("header.j2", "<h1>{{ title|upper }}</h1>");
        fs->AddFile("macros.j2", "{% macro item(x, sep=', ') %}[{{ x }}]{{ sep }}{% endmacro %}");
        fs->AddFile("base.j2", "<{% block body %}base{% endblock %}>");
        m_env.AddFilesystemHandler(std::string(), fs);
        // The i18n extension, with a catalog the host translates through (task 0256)
        m_env.GetSettings().extensions.i18n = true;
        const std::map<std::string, std::string> catalog{ { "Hello %(name)s!", "Hallo %(name)s!" }, { "%(n)s item", "%(n)s Ding" } };
        m_env.InstallGettextCallables(
            MakeCallable(
                [catalog](const std::string& message) {
                    const auto found = catalog.find(message);
                    return found == catalog.end() ? message : found->second;
                },
                ArgInfo{ "message" }),
            MakeCallable([](const std::string& singular, const std::string& plural, int64_t n) { return n == 1 ? singular : plural; },
                         ArgInfo{ "singular" },
                         ArgInfo{ "plural" },
                         ArgInfo{ "n" }),
            MakeCallable([](const std::string& context, const std::string& message) { return context + ":" + message; }, ArgInfo{ "context" }, ArgInfo{ "message" }));
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

    // Fails each allocation of a Load in turn: Load succeeds or throws std::bad_alloc, and the template loads and
    // renders after it
    template<typename CharT>
    void SweepLoad(const Case& param)
    {
        const auto source = Widen<CharT>(param.source);
        const auto expected = Widen<CharT>(param.expected);
        long n = 0;
        for (; n < maxAllocations; ++n)
        {
            BasicTemplate<CharT> tpl(&m_env);
            bool threw = false;
            bool loaded = false;
            bool fired = false;
            OnFreshThread([&] {
                Injection injection(n);
                try
                {
                    loaded = tpl.Load(source).has_value();
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
                ASSERT_TRUE(tpl.Load(source).has_value()) << param.source;
            }
            auto result = tpl.RenderAsString(Params());
            ASSERT_TRUE(result.has_value()) << "after allocation " << n << " failed: " << ToString(result.error().ToString());
            EXPECT_EQ(expected, result.value()) << "after allocation " << n << " failed";
            if (!fired)
            {
                break;
            }
        }
        RecordProperty("allocations", static_cast<int>(n));
        EXPECT_LT(0, n) << "no allocation reached the injection";
        EXPECT_LT(n, maxAllocations);
    }

    // Fails each allocation of a Render in turn: Render succeeds with the expected output or throws std::bad_alloc,
    // and the next Render prints the expected output
    template<typename CharT>
    void SweepRender(const Case& param)
    {
        const auto source = Widen<CharT>(param.source);
        const auto expected = Widen<CharT>(param.expected);
        BasicTemplate<CharT> tpl(&m_env);
        ASSERT_TRUE(tpl.Load(source).has_value()) << param.source;
        const auto params = Params();
        long n = 0;
        for (; n < maxAllocations; ++n)
        {
            bool threw = false;
            bool fired = false;
            Result<std::basic_string<CharT>, CharT> result;
            OnFreshThread([&] {
                const LeakCheckPause pause(g_exhausted && param.makesCycles);
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
                ASSERT_TRUE(result.has_value()) << "allocation " << n << " failed and Render returned an error: " << ToString(result.error().ToString());
                EXPECT_EQ(expected, result.value()) << "allocation " << n << " failed and Render printed something else";
            }
            auto again = tpl.RenderAsString(params);
            ASSERT_TRUE(again.has_value()) << "after allocation " << n << " failed: " << ToString(again.error().ToString());
            EXPECT_EQ(expected, again.value()) << "after allocation " << n << " failed";
            if (!fired)
            {
                break;
            }
        }
        RecordProperty("allocations", static_cast<int>(n));
        EXPECT_LT(0, n) << "no allocation reached the injection";
        EXPECT_LT(n, maxAllocations);
    }

    TemplateEnv m_env;
};

TEST_P(AllocationFailureTest, LoadSucceedsOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    SweepLoad<char>(GetParam());
}

TEST_P(AllocationFailureTest, RenderSucceedsOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    SweepRender<char>(GetParam());
}

// The same rows as TemplateW: the wide lexer, the conversions of narrow host data and files (task 0256)
TEST_P(AllocationFailureTest, WideLoadSucceedsOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    SweepLoad<wchar_t>(GetParam());
}

TEST_P(AllocationFailureTest, WideRenderSucceedsOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    SweepRender<wchar_t>(GetParam());
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
using JsonAllocationFailureTest = LeakCheckedTest;

TEST_F(JsonAllocationFailureTest, BigIntegerIsExactOrThrowsBadAlloc)
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

// An nlohmann::json document the host keeps, read through a view (Reflect of a const reference; a non-const lvalue
// would be copied into an owned view) or copied (ToValue):
// running out of memory while reading it is std::bad_alloc, and the document stays as it was. A document the library
// owns (Reflect of an rvalue, a std::shared_ptr) is not covered: nlohmann::json's destructor allocates, so running out
// of memory while the last Value of it drops ends the process (docs/api-2.0.md 3.9, task 0279)
TEST_F(JsonAllocationFailureTest, NlohmannDocumentReadsOrThrowsBadAlloc)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    const auto doc = nlohmann::json::parse(R"({"a": [1, 2, 3], "o": {"k": "v", "n": [true, null, 2.5]}})");
    const std::string expected = R"(1,2,3|v|3|{"a": [1, 2, 3], "o": {"k": "v", "n": [true, null, 2.5]}})";
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{{ d.a|join(',') }}|{{ d.o.k }}|{{ d.o.n|length }}|{{ d|tojson }}"));
    for (const bool view : { true, false })
    {
        SCOPED_TRACE(view ? "Reflect" : "ToValue");
        long n = 0;
        for (; n < maxAllocations; ++n)
        {
            bool threw = false;
            bool fired = false;
            Result<std::string> result;
            std::string failure;
            OnFreshThread([&] {
                const Injection injection(n);
                try
                {
                    if (view)
                    {
                        result = tpl.RenderAsString(ValuesMap{ { "d", Reflect(doc) } });
                    }
                    else if (auto copy = ToValue(doc))
                    {
                        result = tpl.RenderAsString(ValuesMap{ { "d", std::move(*copy) } });
                    }
                    else
                    {
                        failure = copy.error().ToString();
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
                ASSERT_TRUE(result.has_value()) << "allocation " << n << ": " << result.error().ToString();
                ASSERT_EQ(expected, result.value()) << "allocation " << n;
            }
            if (!fired)
            {
                break;
            }
        }
        EXPECT_LT(0, n) << "no allocation reached the injection";
        EXPECT_LT(n, maxAllocations);
    }
    EXPECT_EQ(3u, doc["a"].size());
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
#ifdef _LIBCPP_VERSION
    // libc++abi allocates an exception's message with operator new and frees it with free(), so
    // the counter sees one live allocation per failing call (JINJA2CPP_DEBUG_CONTAINERS=libc++, 0344)
    GTEST_SKIP() << "the allocation counter cannot follow libc++abi's exception messages";
#endif
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

// With TemplateLookup::EveryUse and no cache every use parses the template again: a reload whose source did not
// change is dropped, so the module keeps no tree per call (docs/tasks/0313)
TEST_F(ModuleCallTest, EveryUseReloadsStayFlat)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
#ifdef _LIBCPP_VERSION
    GTEST_SKIP() << "the allocation counter cannot follow libc++abi's exception messages";
#endif
    m_env.GetSettings().templateLookup = jinja2::TemplateLookup::EveryUse;
    m_env.GetSettings().cacheSize = 0;
    // A template takes the environment's settings when it is made
    m_tpl = Template(&m_env);
    for (const char* body : { "{% include 'lib' %}{{ 1 }}", "{% import 'lib' as l with context %}{{ l.x() }}", "{% import 'lib' as l %}{{ l.x() }}",
                              "{% from 'lib' import x %}{{ x() }}", "{% for i in range(3) %}{% include 'lib' %}{% endfor %}" })
    {
        auto module = MakeModule(body);
        const auto& m = module.exports["m"];
        const auto callMany = [&m](int count) {
            for (int i = 0; i != count; ++i)
            {
                Call(m);
            }
        };
        callMany(100);
        const long before = g_live.load(std::memory_order_relaxed);
        callMany(1000);
        // Each call kept a tree, about 11 allocations, before
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

// A call that succeeds hands out values that keep its final state, also when recording that state
// at the call's end runs out of memory: the call then fails instead (0335, Codex review). Here the
// kept list is a template list, so the end of the call records every cell of the module
TEST_F(AllocationFailureTest, SucceededCallKeepsItsStateOrFails)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro add(x) %}{% set _ = a.append(x) %}{% set _ = b.append(x) %}{% set _ = keep([a, b]) %}{% endmacro %}"));
    const auto show = [](const Value& v) {
        Template shown;
        EXPECT_TRUE(!!shown.Load("{{ k }}"));
        return shown.RenderAsString(ValuesMap{ { "k", v } }).value();
    };
    long n = 0;
    for (; n != maxAllocations; ++n)
    {
        std::vector<Value> kept;
        ValuesMap params{ { "a", ValuesList{ 0 } }, { "b", ValuesList{ 0 } } };
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
        bool fired = false;
        bool succeeded = false;
        OnFreshThread([&] {
            const Injection injection(n);
            try
            {
                call(1);
                succeeded = true;
            }
            catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): a failed call is one of the outcomes checked
            {
            }
            fired = Injection::Fired();
        });
        if (succeeded)
        {
            call(2);
            ASSERT_EQ(2u, kept.size()) << "allocation " << n;
            ASSERT_EQ("[[0, 1], [0, 1]]", show(kept[0])) << "allocation " << n;
            ASSERT_EQ("[[0, 1, 2], [0, 1, 2]]", show(kept[1])) << "allocation " << n;
        }
        if (!fired)
        {
            break;
        }
    }
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

// A module's sort() whose key function makes the host call the module again, and that call hands
// the emptied list to the host: the sort then copies the list on write (D8), and when that copy
// fails the list gets its items back, not the ones already moved out (Codex review)
TEST_F(AllocationFailureTest, ModuleSortKeepsItsItemsWhenTheCopyFails)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro srt() %}{% set _ = l.sort(key=k) %}{% endmacro %}"
                           "{% macro k(x) %}{% set _ = reexp() %}{{ x[0] }}{% endmacro %}"
                           "{% macro exp() %}{% set _ = keep(l) %}{% endmacro %}"
                           "{% macro show() %}{{ l }}{% endmacro %}"));
    long n = 0;
    for (; n != maxAllocations; ++n)
    {
        std::vector<Value> kept;
        const TemplateModule* self = nullptr;
        ValuesMap params{ { "l", ValuesList{ Value(ValuesList{ 3 }), Value(ValuesList{ 1 }), Value(ValuesList{ 2 }) } } };
        params["keep"] = UserCallable(
            [&kept](const UserCallableParams& p) {
                kept.push_back(p["v"]);
                return Value();
            },
            { ArgInfo{ "v" } });
        params["reexp"] = UserCallable(
            [&self](const UserCallableParams&) {
                self->exports.at("exp").As<UserCallable>().callable(UserCallableParams{});
                return Value();
            },
            {});
        auto module = tpl.MakeModule(params);
        ASSERT_TRUE(!!module);
        self = &*module;
        const auto call = [&module](const std::string& name) { return module->exports[name].As<UserCallable>().callable(UserCallableParams{}).AsString(); };
        bool fired = false;
        bool threw = false;
        OnFreshThread([&] {
            const Injection injection(n);
            try
            {
                call("srt");
            }
            catch (const std::bad_alloc&)
            {
                threw = true;
            }
            fired = Injection::Fired();
        });
        const auto shown = call("show");
        if (threw)
        {
            ASSERT_TRUE(shown == "[[3], [1], [2]]" || shown == "[[1], [2], [3]]") << shown << " at allocation " << n;
        }
        else
        {
            ASSERT_EQ("[[1], [2], [3]]", shown) << "allocation " << n;
        }
        if (!fired)
        {
            break;
        }
    }
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

// A render nested in one that wrote a global through an alias reads that render's snapshot of the
// globals, and so its cell (docs/tasks/0324); when the nested render fails on an allocation, the
// next one it makes still reads that cell (Codex review, docs/tasks/0331)
TEST_F(AllocationFailureTest, NestedRenderAfterAFailedGlobalsConversion)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    TemplateEnv env;
    env.AddGlobal("g", ValuesList{ 0 });
    Template inner(&env);
    ASSERT_TRUE(!!inner.Load("{{ g }}"));
    Template outer(&env);
    ASSERT_TRUE(!!outer.Load("{% set y = g %}{% set _ = y.append(1) %}{{ inner() }}|{{ g }}"));
    long n = 0;
    for (; n != maxAllocations; ++n)
    {
        bool fired = false;
        ValuesMap params;
        params["inner"] = MakeCallable([&inner, &fired, n]() {
            {
                const Injection injection(n);
                try
                {
                    static_cast<void>(inner.RenderAsString({}));
                }
                catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): a failed render is one of the outcomes checked
                {
                }
                fired = Injection::Fired();
            }
            return inner.RenderAsString({}).value();
        });
        std::string result;
        OnFreshThread([&] { result = outer.RenderAsString(params).value(); });
        ASSERT_EQ("[0, 1]|[0, 1]", result) << "allocation " << n;
        if (!fired)
        {
            break;
        }
    }
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

// A cache miss that fails to allocate, while it inserts or evicts, leaves the cache as it was or
// complete: a later load finds the file and renders it (docs/tasks/0264)
TEST_F(AllocationFailureTest, CacheMissAfterAFailedInsert)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    long n = 0;
    for (; n != maxAllocations; ++n)
    {
        auto fs = std::make_shared<MemoryFileSystem>();
        fs->AddFile("a.j2", std::string("A"));
        fs->AddFile("b.j2", std::string("B"));
        TemplateEnv env;
        env.GetSettings().cacheSize = 1;
        env.AddFilesystemHandler("", fs);
        ASSERT_TRUE(!!env.LoadTemplate("b.j2"));
        bool fired = false;
        {
            const Injection injection(n);
            try
            {
                static_cast<void>(env.LoadTemplate("a.j2"));
            }
            catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): a failed load is one of the outcomes checked
            {
            }
            fired = Injection::Fired();
        }
        for (const char* name : { "a.j2", "b.j2", "a.j2" })
        {
            auto tpl = env.LoadTemplate(name);
            ASSERT_TRUE(!!tpl) << "allocation " << n;
            std::string out;
            OnFreshThread([&] { out = tpl->RenderAsString({}).value_or("error"); });
            EXPECT_EQ(name[0] == 'a' ? "A" : "B", out) << "allocation " << n;
        }
        if (!fired)
        {
            break;
        }
    }
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

// Two names of the host's data in a module: an allocation failure while the second attaches to
// the cell the first made leaves it looking again, not reading the host's data for good
// (docs/tasks/0049)
TEST_F(AllocationFailureTest, AliasAttachesAfterAFailedLink)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro m() %}{% set _ = a.append(9) %}{{ a|length }}={{ b|length }}{% endmacro %}"));
    const std::vector<int64_t> data{ 1, 2, 3 };
    long n = 0;
    for (; n != maxAllocations; ++n)
    {
        // One accessor behind both names: a copy of a GenericList made by MakeGenericList copies
        // its accessor, which is other data
        auto made = std::make_shared<GenericList>(MakeGenericList(data.begin(), data.end()));
        const GenericList list([made] { return made->GetAccessor(); });
        auto module = tpl.MakeModule(ValuesMap{ { "a", list }, { "b", list } });
        ASSERT_TRUE(!!module);
        const auto call = [&module]() { return module->exports["m"].As<UserCallable>().callable(UserCallableParams{}).AsString(); };
        bool fired = false;
        OnFreshThread([&] {
            const Injection injection(n);
            try
            {
                call();
            }
            catch (const std::bad_alloc&) // NOLINT(bugprone-empty-catch): a failed call is one of the outcomes checked
            {
            }
            fired = Injection::Fired();
        });
        const auto result = call();
        const auto eq = result.find('=');
        ASSERT_NE(std::string::npos, eq) << result;
        ASSERT_EQ(result.substr(0, eq), result.substr(eq + 1)) << "allocation " << n;
        if (!fired)
        {
            break;
        }
    }
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

// The end of a module call records what the values it handed out read, then marks the recorded
// cells shared: an allocation failure in between records nothing. A kept value then shows the
// call's final state or, when the call failed, the host's data, which nothing writes (task 0361),
// never the outer list's new items over the host's nested list; and later calls do not change it
// (docs/tasks/0338, step D)
TEST_F(AllocationFailureTest, FailedCallsPinIsAllOrNothing)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    Template tpl;
    ASSERT_TRUE(!!tpl.Load("{% macro add(x) %}{% set _ = a[0].append(x) %}{% set _ = a.append(x + 1) %}{% set _ = keep(a) %}{% endmacro %}"));
    const auto show = [](const Value& v) {
        Template shown;
        EXPECT_TRUE(!!shown.Load("{{ k }}"));
        return shown.RenderAsString(ValuesMap{ { "k", v } }).value();
    };
    long n = 0;
    for (; n != maxAllocations; ++n)
    {
        std::vector<Value> kept;
        ValuesMap params{ { "a", ValuesList{ Value(ValuesList{ 0 }) } } };
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
        bool fired = false;
        bool threw = false;
        OnFreshThread([&] {
            const Injection injection(n);
            try
            {
                call(1);
            }
            catch (const std::bad_alloc&)
            {
                threw = true;
            }
            fired = Injection::Fired();
        });
        if (!kept.empty())
        {
            const auto first = show(kept[0]);
            if (threw)
            {
                // A failed pin leaves the view reading the host's data (docs/tasks/0361)
                ASSERT_TRUE(first == "[[0, 1], 2]" || first == "[[0]]") << first << " at allocation " << n;
            }
            else
            {
                ASSERT_EQ("[[0, 1], 2]", first) << "allocation " << n;
            }
            call(3);
            call(5);
            ASSERT_EQ(first, show(kept[0])) << "allocation " << n;
        }
        if (!fired)
        {
            break;
        }
    }
    EXPECT_LT(0, n) << "no allocation reached the injection";
    EXPECT_LT(n, maxAllocations);
}

// A global written through an alias and through its own name: an allocation failure while the
// cell is made or an adapter attaches to it leaves the render's view of the global whole, and
// never writes the host's data (docs/tasks/0338, step D)
TEST_F(AllocationFailureTest, GlobalAliasesAfterAFailedAdopt)
{
    JINJA2CPP_SKIP_UNREACHED_INJECTION();
    TemplateEnv env;
    env.AddGlobal("g", ValuesList{ 0 });
    Template tpl(&env);
    ASSERT_TRUE(!!tpl.Load("{% set y = g %}{% set _ = y.append(1) %}{% set _ = g.append(2) %}{{ y }}|{{ g }}"));
    Template plain(&env);
    ASSERT_TRUE(!!plain.Load("{{ g }}"));
    long n = 0;
    for (; n != maxAllocations; ++n)
    {
        bool fired = false;
        bool threw = false;
        Result<std::string> result;
        OnFreshThread([&] {
            {
                const Injection injection(n);
                try
                {
                    result = tpl.RenderAsString({});
                }
                catch (const std::bad_alloc&)
                {
                    threw = true;
                }
                fired = Injection::Fired();
            }
            // The same thread, whose snapshot of the globals the failed render may have left attached
            if (!threw)
            {
                ASSERT_TRUE(result.has_value()) << result.error().ToString();
                ASSERT_EQ("[0, 1, 2]|[0, 1, 2]", result.value()) << "allocation " << n;
            }
            ASSERT_EQ("[0, 1, 2]|[0, 1, 2]", tpl.RenderAsString({}).value()) << "after allocation " << n;
            ASSERT_EQ("[0]", plain.RenderAsString({}).value()) << "after allocation " << n;
        });
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
                                               "2",
                                               true },
                                         // Writes to the host's data through its aliases, and a cycle through
                                         // its cell (docs/tasks/0049)
                                         Case{ "{% set y = items %}{% set r = user.roles %}{{ y.append(6) or '' }}{{ r.append('ops') or '' }}"
                                               "{% set f = user.update %}{{ f(x=1) or '' }}{{ items.append(items) or '' }}{{ items|length }}{{ user.roles|length }}{{ user.x }}",
                                               "731",
                                               true },
                                         // The i18n extension: {% trans %} with a variable and a plural, and the
                                         // gettext globals (task 0256)
                                         Case{ "{% trans name=user.name %}Hello {{ name }}!{% endtrans %}|{% trans n=items|length %}{{ n }} item"
                                               "{% pluralize %}{{ n }} items{% endtrans %}|{% trans %}{{ title }} 100%{% endtrans %}",
                                               "Hallo Ann!|5 items|Report 100%" },
                                         Case{ "{{ _('Hello %(name)s!', name=user.name) }}|{{ ngettext('%(num)d apple', '%(num)d apples', 3) }}|"
                                               "{{ pgettext('menu', 'Open') }}|{{ gettext('%(n)s item', n=1) }}",
                                               "Hallo Ann!|3 apples|menu:Open|1 Ding" }));
} // namespace
