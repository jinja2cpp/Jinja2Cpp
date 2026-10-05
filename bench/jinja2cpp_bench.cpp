// Google Benchmark suite for Jinja2C++ (docs/tasks/0011).
//
// Every directory under bench/cases/ is one workload: main.j2 is the template, data.json
// the render parameters, and any other *.j2 file is registered in an in-memory filesystem
// so main.j2 can include, import or extend it. An optional settings.json sets environment
// options (trim_blocks, lstrip_blocks, autoescape, as in Python) and "wide": true, which
// runs the case with TemplateW and wide strings in the data. bench/python_bench.py runs the
// same workloads with Python Jinja2, so the two engines can be compared on identical work.
//
// For each case two benchmarks are registered:
//   Load/<case>    parse main.j2 (Template::Load)
//   Render/<case>  render the loaded template to a string
//
// Usage: jinja2cpp_bench [--cases-dir=<dir>] [--dump-dir=<dir>] [google benchmark flags]
//   --cases-dir  where the cases live (default: the source tree's bench/cases)
//   --dump-dir   write each case's rendered output to <dir>/<case>.txt and exit, so
//                bench/run.py can check that both engines produce the same text
//   --count=<Load|Render>/<case> [--count-iters=N]
//                run that benchmark N times (default 10) inside CountedRegion() and exit;
//                bench/count.py runs this under callgrind, collecting only that function,
//                to get a deterministic instruction count per iteration. It also prints
//                `allocations <n> bytes <n>`: operator new calls and bytes per iteration
//   --cpu-profile=<file> --heap-profile=<prefix>
//                with --count, in a build with -DJINJA2CPP_BENCH_WITH_GPERFTOOLS=ON: write a
//                gperftools CPU profile and/or heap profile of the counted iterations
//   --threads    also register MT/Render/<case>/threads:N, rendering one shared template
//                from N threads at once (N = 1, 2, 4, ... up to the hardware threads)

#include <benchmark/benchmark.h>
#include <nlohmann/json.hpp>

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/string_helpers.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#ifdef JINJA2CPP_BENCH_GPERFTOOLS
#include <gperftools/heap-profiler.h>
#include <gperftools/profiler.h>
#endif

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <new>
#include <sstream>
#include <string>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

#ifndef JINJA2CPP_BENCH_CASES_DIR
#define JINJA2CPP_BENCH_CASES_DIR "bench/cases"
#endif

#ifndef JINJA2CPP_BENCH_GPERFTOOLS
// Counts operator new calls in --count mode: a deterministic memory metric, like the
// instruction count. Left out with gperftools, whose tcmalloc replaces operator new itself.
namespace
{
std::atomic<bool> g_countAllocations{ false };
std::atomic<uint64_t> g_allocations{ 0 };
std::atomic<uint64_t> g_allocatedBytes{ 0 };

void* CountedNew(std::size_t size)
{
    if (g_countAllocations.load(std::memory_order_relaxed))
    {
        g_allocations.fetch_add(1, std::memory_order_relaxed);
        g_allocatedBytes.fetch_add(size, std::memory_order_relaxed);
    }
    if (void* ptr = std::malloc(size ? size : 1))
    {
        return ptr;
    }
    throw std::bad_alloc();
}
} // namespace

void* operator new(std::size_t size)
{
    return CountedNew(size);
}
void* operator new[](std::size_t size)
{
    return CountedNew(size);
}
void operator delete(void* ptr) noexcept
{
    std::free(ptr);
}
void operator delete[](void* ptr) noexcept
{
    std::free(ptr);
}
void operator delete(void* ptr, std::size_t /*size*/) noexcept
{
    std::free(ptr);
}
void operator delete[](void* ptr, std::size_t /*size*/) noexcept
{
    std::free(ptr);
}
#endif

namespace
{
namespace fs = std::filesystem;

std::string ReadFile(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream os;
    os << in.rdbuf();
    return os.str();
}

// The eager conversion is the common way to pass data (a ValuesMap built by the caller);
// reflection of the JSON document would measure the binding instead of the engine.
// A wide case gets std::wstring strings, as a TemplateW user would pass them.
jinja2::Value ToValue(const nlohmann::ordered_json& j, bool wide)
{
    switch (j.type())
    {
    case nlohmann::json::value_t::null:
        return jinja2::Value(jinja2::EmptyValue());
    case nlohmann::json::value_t::boolean:
        return jinja2::Value(j.get<bool>());
    case nlohmann::json::value_t::number_integer:
    case nlohmann::json::value_t::number_unsigned:
        return jinja2::Value(j.get<int64_t>());
    case nlohmann::json::value_t::number_float:
        return jinja2::Value(j.get<double>());
    case nlohmann::json::value_t::string:
        if (wide)
        {
            return jinja2::Value(jinja2::AsWString(j.get<std::string>()));
        }
        return jinja2::Value(j.get<std::string>());
    case nlohmann::json::value_t::array:
    {
        jinja2::ValuesList list;
        list.reserve(j.size());
        for (const auto& item : j)
        {
            list.push_back(ToValue(item, wide));
        }
        return jinja2::Value(std::move(list));
    }
    case nlohmann::json::value_t::object:
    {
        jinja2::ValuesMap map;
        for (const auto& item : j.items())
        {
            map.emplace(item.key(), ToValue(item.value(), wide));
        }
        return jinja2::Value(std::move(map));
    }
    default:
        return {};
    }
}

struct Case
{
    std::string name;
    bool wide = false;
    std::string source;
    std::wstring wsource; // the source of a wide case
    jinja2::ValuesMap params;
    std::unique_ptr<jinja2::TemplateEnv> env;
    // loaded once, for the multi-threaded benchmarks
    std::unique_ptr<jinja2::Template> shared;
    std::unique_ptr<jinja2::TemplateW> sharedW;
};

template<typename CharT>
const std::basic_string<CharT>& SourceOf(const Case& c)
{
    if constexpr (std::is_same_v<CharT, char>)
    {
        return c.source;
    }
    else
    {
        return c.wsource;
    }
}

template<typename CharT>
std::unique_ptr<jinja2::BasicTemplate<CharT>>& SharedOf(Case& c)
{
    if constexpr (std::is_same_v<CharT, char>)
    {
        return c.shared;
    }
    else
    {
        return c.sharedW;
    }
}

std::unique_ptr<Case> LoadCase(const fs::path& dir)
{
    auto result = std::make_unique<Case>();
    result->name = dir.filename().string();
    result->source = ReadFile(dir / "main.j2");
    result->env = std::make_unique<jinja2::TemplateEnv>();
    if (fs::exists(dir / "settings.json"))
    {
        const auto settings = nlohmann::json::parse(ReadFile(dir / "settings.json"));
        result->wide = settings.value("wide", false);
        auto& envSettings = result->env->GetSettings();
        envSettings.trimBlocks = settings.value("trim_blocks", false);
        envSettings.lstripBlocks = settings.value("lstrip_blocks", false);
        envSettings.autoescape = settings.value("autoescape", false);
    }
    if (result->wide)
    {
        result->wsource = jinja2::AsWString(result->source);
    }
    if (fs::exists(dir / "data.json"))
    {
        auto data = nlohmann::ordered_json::parse(ReadFile(dir / "data.json"));
        for (const auto& item : data.items())
        {
            result->params.emplace(item.key(), ToValue(item.value(), result->wide));
        }
    }

    auto memFs = std::make_shared<jinja2::MemoryFileSystem>();
    for (const auto& entry : fs::directory_iterator(dir))
    {
        const auto& path = entry.path();
        if (path.extension() == ".j2" && path.filename() != "main.j2")
        {
            if (result->wide)
            {
                memFs->AddFile(path.filename().string(), jinja2::AsWString(ReadFile(path)));
            }
            else
            {
                memFs->AddFile(path.filename().string(), ReadFile(path));
            }
        }
    }
    result->env->AddFilesystemHandler(std::string(), memFs);
    return result;
}

template<typename CharT>
void BenchLoad(benchmark::State& state, const Case* c)
{
    for (auto _ : state)
    {
        jinja2::BasicTemplate<CharT> tpl(c->env.get());
        auto res = tpl.Load(SourceOf<CharT>(*c), c->name);
        if (!res)
        {
            state.SkipWithError(jinja2::AsString(res.error().ToString()));
            break;
        }
        benchmark::DoNotOptimize(tpl);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * c->source.size()));
}

template<typename CharT>
void BenchRender(benchmark::State& state, const Case* c)
{
    jinja2::BasicTemplate<CharT> tpl(c->env.get());
    auto loaded = tpl.Load(SourceOf<CharT>(*c), c->name);
    if (!loaded)
    {
        state.SkipWithError(jinja2::AsString(loaded.error().ToString()));
        return;
    }
    size_t outSize = 0;
    for (auto _ : state)
    {
        auto res = tpl.RenderAsString(c->params);
        if (!res)
        {
            state.SkipWithError(jinja2::AsString(res.error().ToString()));
            break;
        }
        outSize = res.value().size();
        benchmark::DoNotOptimize(res);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * outSize));
}

// Many threads render one shared template, as a server does; real time, items per second
template<typename CharT>
void BenchRenderShared(benchmark::State& state, Case* c)
{
    for (auto _ : state)
    {
        auto res = SharedOf<CharT>(*c)->RenderAsString(c->params);
        if (!res)
        {
            state.SkipWithError(jinja2::AsString(res.error().ToString()));
            break;
        }
        benchmark::DoNotOptimize(res);
    }
    state.SetItemsProcessed(state.iterations());
}

template<typename CharT>
bool LoadShared(Case& c)
{
    auto& shared = SharedOf<CharT>(c);
    shared = std::make_unique<jinja2::BasicTemplate<CharT>>(c.env.get());
    return shared->Load(SourceOf<CharT>(c), c.name).has_value();
}

// Pulls --name=value out of argv so the rest can go to google benchmark.
std::string TakeFlag(int& argc, char** argv, const char* name, std::string defaultValue)
{
    const std::string prefix = std::string("--") + name + "=";
    for (int i = 1; i < argc; ++i)
    {
        if (std::strncmp(argv[i], prefix.c_str(), prefix.size()) != 0)
        {
            continue;
        }
        std::string value = argv[i] + prefix.size();
        std::copy(argv + i + 1, argv + argc, argv + i);
        --argc;
        return value;
    }
    return defaultValue;
}

#if defined(_MSC_VER)
#define JINJA2CPP_BENCH_NOINLINE __declspec(noinline)
#else
#define JINJA2CPP_BENCH_NOINLINE __attribute__((noinline))
#endif

// The only code callgrind collects in --count mode (--toggle-collect=*CountedRegion*)
template<typename Fn>
JINJA2CPP_BENCH_NOINLINE void CountedRegion(Fn& fn, int iterations)
{
    for (int i = 0; i < iterations; ++i)
    {
        fn();
    }
}

struct ProfileOptions
{
    std::string cpuProfile;
    std::string heapProfile;
};

template<typename Fn>
void RunCounted(Fn& fn, int iterations, const ProfileOptions& profile)
{
#ifdef JINJA2CPP_BENCH_GPERFTOOLS
    if (!profile.cpuProfile.empty())
    {
        ProfilerStart(profile.cpuProfile.c_str());
    }
    if (!profile.heapProfile.empty())
    {
        HeapProfilerStart(profile.heapProfile.c_str());
    }
    CountedRegion(fn, iterations);
    if (!profile.heapProfile.empty())
    {
        HeapProfilerDump("end");
        HeapProfilerStop();
    }
    if (!profile.cpuProfile.empty())
    {
        ProfilerStop();
    }
#else
    if (!profile.cpuProfile.empty() || !profile.heapProfile.empty())
    {
        std::cerr << "profiles need a build with -DJINJA2CPP_BENCH_WITH_GPERFTOOLS=ON\n";
    }
    g_allocations = 0;
    g_allocatedBytes = 0;
    g_countAllocations = true;
    CountedRegion(fn, iterations);
    g_countAllocations = false;
    std::cout << "allocations " << g_allocations / iterations << " bytes " << g_allocatedBytes / iterations << '\n';
#endif
}

template<typename CharT>
int CountCase(const Case& c, bool load, int iterations, const ProfileOptions& profile)
{
    bool ok = true;
    if (load)
    {
        auto fn = [&c, &ok] {
            jinja2::BasicTemplate<CharT> tpl(c.env.get());
            ok = ok && tpl.Load(SourceOf<CharT>(c), c.name).has_value();
        };
        fn(); // warm-up: first-use initialisation stays out of the count
        RunCounted(fn, iterations, profile);
    }
    else
    {
        jinja2::BasicTemplate<CharT> tpl(c.env.get());
        if (!tpl.Load(SourceOf<CharT>(c), c.name))
        {
            std::cerr << c.name << ": failed to load\n";
            return 1;
        }
        auto fn = [&tpl, &c, &ok] { ok = ok && tpl.RenderAsString(c.params).has_value(); };
        fn();
        RunCounted(fn, iterations, profile);
    }
    if (!ok)
    {
        std::cerr << c.name << ": failed\n";
    }
    return ok ? 0 : 1;
}

int Count(const std::vector<std::unique_ptr<Case>>& cases, const std::string& name, int iterations, const ProfileOptions& profile)
{
    const auto slash = name.find('/');
    const std::string kind = name.substr(0, slash);
    const std::string caseName = slash == std::string::npos ? std::string() : name.substr(slash + 1);
    auto found = std::find_if(cases.begin(), cases.end(), [&](const auto& c) { return c->name == caseName; });
    if (found == cases.end() || (kind != "Load" && kind != "Render"))
    {
        std::cerr << "unknown benchmark: " << name << '\n';
        return 1;
    }
    const Case* c = found->get();
    return c->wide ? CountCase<wchar_t>(*c, kind == "Load", iterations, profile)
                   : CountCase<char>(*c, kind == "Load", iterations, profile);
}

// Renders the case to `out` as UTF-8, so a wide case compares with Python's output too
template<typename CharT>
bool RenderToUtf8(const Case& c, std::ostream& out, std::string& error)
{
    jinja2::BasicTemplate<CharT> tpl(c.env.get());
    auto loaded = tpl.Load(SourceOf<CharT>(c), c.name);
    if (!loaded)
    {
        error = jinja2::AsString(loaded.error().ToString());
        return false;
    }
    auto res = tpl.RenderAsString(c.params);
    if (!res)
    {
        error = jinja2::AsString(res.error().ToString());
        return false;
    }
    out << jinja2::AsString(res.value());
    return true;
}

int Dump(const std::vector<std::unique_ptr<Case>>& cases, const fs::path& dumpDir)
{
    fs::create_directories(dumpDir);
    int failures = 0;
    for (const auto& c : cases)
    {
        std::ofstream out(dumpDir / (c->name + ".txt"), std::ios::binary);
        std::string error;
        const bool ok = c->wide ? RenderToUtf8<wchar_t>(*c, out, error) : RenderToUtf8<char>(*c, out, error);
        if (!ok)
        {
            std::cerr << c->name << ": " << error << '\n';
            ++failures;
        }
    }
    return failures == 0 ? 0 : 1;
}
} // namespace

int main(int argc, char** argv)
{
    const fs::path casesDir = TakeFlag(argc, argv, "cases-dir", JINJA2CPP_BENCH_CASES_DIR);
    const std::string dumpDir = TakeFlag(argc, argv, "dump-dir", "");
    const std::string countName = TakeFlag(argc, argv, "count", "");
    const int countIterations = std::stoi(TakeFlag(argc, argv, "count-iters", "10"));
    ProfileOptions profile;
    profile.cpuProfile = TakeFlag(argc, argv, "cpu-profile", "");
    profile.heapProfile = TakeFlag(argc, argv, "heap-profile", "");
    bool threaded = false;
    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--threads") == 0)
        {
            threaded = true;
            std::copy(argv + i + 1, argv + argc, argv + i);
            --argc;
            break;
        }
    }

    std::vector<fs::path> dirs;
    for (const auto& entry : fs::directory_iterator(casesDir))
    {
        if (entry.is_directory() && fs::exists(entry.path() / "main.j2"))
        {
            dirs.push_back(entry.path());
        }
    }
    std::sort(dirs.begin(), dirs.end());

    std::vector<std::unique_ptr<Case>> cases;
    cases.reserve(dirs.size());
    for (const auto& dir : dirs)
    {
        cases.push_back(LoadCase(dir));
    }

    if (!dumpDir.empty())
    {
        return Dump(cases, dumpDir);
    }
    if (!countName.empty())
    {
        return Count(cases, countName, countIterations, profile);
    }

    for (const auto& c : cases)
    {
        benchmark::RegisterBenchmark("Load/" + c->name, c->wide ? BenchLoad<wchar_t> : BenchLoad<char>, c.get());
        benchmark::RegisterBenchmark("Render/" + c->name, c->wide ? BenchRender<wchar_t> : BenchRender<char>, c.get());
    }
    if (threaded)
    {
        const int maxThreads = static_cast<int>(std::max(1U, std::thread::hardware_concurrency()));
        for (const auto& c : cases)
        {
            if (!(c->wide ? LoadShared<wchar_t>(*c) : LoadShared<char>(*c)))
            {
                continue;
            }
            auto* bench = benchmark::RegisterBenchmark("MT/Render/" + c->name,
                                                       c->wide ? BenchRenderShared<wchar_t> : BenchRenderShared<char>, c.get());
            for (int n = 1; n <= maxThreads; n *= 2)
            {
                bench->Threads(n);
            }
            bench->UseRealTime();
        }
    }

    benchmark::Initialize(&argc, argv);
    if (benchmark::ReportUnrecognizedArguments(argc, argv))
    {
        return 1;
    }
    benchmark::RunSpecifiedBenchmarks();
    benchmark::Shutdown();
    return 0;
}
