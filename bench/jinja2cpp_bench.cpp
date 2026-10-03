// Google Benchmark suite for Jinja2C++ (docs/tasks/0011).
//
// Every directory under bench/cases/ is one workload: main.j2 is the template, data.json
// the render parameters, and any other *.j2 file is registered in an in-memory filesystem
// so main.j2 can include, import or extend it. bench/python_bench.py runs the same
// workloads with Python Jinja2, so the two engines can be compared on identical work.
//
// For each case two benchmarks are registered:
//   Load/<case>    parse main.j2 (Template::Load)
//   Render/<case>  render the loaded template to a string
//
// Usage: jinja2cpp_bench [--cases-dir=<dir>] [--dump-dir=<dir>] [google benchmark flags]
//   --cases-dir  where the cases live (default: the source tree's bench/cases)
//   --dump-dir   write each case's rendered output to <dir>/<case>.txt and exit, so
//                bench/run.py can check that both engines produce the same text

#include <benchmark/benchmark.h>
#include <nlohmann/json.hpp>

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/value.h>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifndef JINJA2CPP_BENCH_CASES_DIR
#define JINJA2CPP_BENCH_CASES_DIR "bench/cases"
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
jinja2::Value ToValue(const nlohmann::ordered_json& j)
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
        return jinja2::Value(j.get<std::string>());
    case nlohmann::json::value_t::array:
    {
        jinja2::ValuesList list;
        list.reserve(j.size());
        for (const auto& item : j)
        {
            list.push_back(ToValue(item));
        }
        return jinja2::Value(std::move(list));
    }
    case nlohmann::json::value_t::object:
    {
        jinja2::ValuesMap map;
        for (const auto& item : j.items())
        {
            map.emplace(item.key(), ToValue(item.value()));
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
    std::string source;
    jinja2::ValuesMap params;
    std::unique_ptr<jinja2::TemplateEnv> env;
};

std::unique_ptr<Case> LoadCase(const fs::path& dir)
{
    auto result = std::make_unique<Case>();
    result->name = dir.filename().string();
    result->source = ReadFile(dir / "main.j2");
    if (fs::exists(dir / "data.json"))
    {
        auto data = nlohmann::ordered_json::parse(ReadFile(dir / "data.json"));
        for (const auto& item : data.items())
        {
            result->params.emplace(item.key(), ToValue(item.value()));
        }
    }

    result->env = std::make_unique<jinja2::TemplateEnv>();
    auto memFs = std::make_shared<jinja2::MemoryFileSystem>();
    for (const auto& entry : fs::directory_iterator(dir))
    {
        const auto& path = entry.path();
        if (path.extension() == ".j2" && path.filename() != "main.j2")
        {
            memFs->AddFile(path.filename().string(), ReadFile(path));
        }
    }
    result->env->AddFilesystemHandler(std::string(), memFs);
    return result;
}

void BenchLoad(benchmark::State& state, const Case* c)
{
    for (auto _ : state)
    {
        jinja2::Template tpl(c->env.get());
        auto res = tpl.Load(c->source, c->name);
        if (!res)
        {
            state.SkipWithError(res.error().ToString().c_str());
            break;
        }
        benchmark::DoNotOptimize(tpl);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * c->source.size()));
}

void BenchRender(benchmark::State& state, const Case* c)
{
    jinja2::Template tpl(c->env.get());
    auto loaded = tpl.Load(c->source, c->name);
    if (!loaded)
    {
        state.SkipWithError(loaded.error().ToString().c_str());
        return;
    }
    size_t outSize = 0;
    for (auto _ : state)
    {
        auto res = tpl.RenderAsString(c->params);
        if (!res)
        {
            state.SkipWithError(res.error().ToString().c_str());
            break;
        }
        outSize = res.value().size();
        benchmark::DoNotOptimize(res);
    }
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations() * outSize));
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

int Dump(const std::vector<std::unique_ptr<Case>>& cases, const fs::path& dumpDir)
{
    fs::create_directories(dumpDir);
    int failures = 0;
    for (const auto& c : cases)
    {
        jinja2::Template tpl(c->env.get());
        auto loaded = tpl.Load(c->source, c->name);
        std::ofstream out(dumpDir / (c->name + ".txt"), std::ios::binary);
        if (!loaded)
        {
            std::cerr << c->name << ": " << loaded.error().ToString() << '\n';
            ++failures;
            continue;
        }
        auto res = tpl.RenderAsString(c->params);
        if (!res)
        {
            std::cerr << c->name << ": " << res.error().ToString() << '\n';
            ++failures;
            continue;
        }
        out << res.value();
    }
    return failures == 0 ? 0 : 1;
}
} // namespace

int main(int argc, char** argv)
{
    const fs::path casesDir = TakeFlag(argc, argv, "cases-dir", JINJA2CPP_BENCH_CASES_DIR);
    const std::string dumpDir = TakeFlag(argc, argv, "dump-dir", "");

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

    for (const auto& c : cases)
    {
        benchmark::RegisterBenchmark("Load/" + c->name, BenchLoad, c.get());
        benchmark::RegisterBenchmark("Render/" + c->name, BenchRender, c.get());
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
