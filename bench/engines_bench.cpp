// Other C++ template engines on the bench/cases workloads (docs/tasks/0120): inja and minja
// (the Jinja subset llama.cpp used for chat templates), built only with
// -DJINJA2CPP_BENCH_WITH_OTHER_ENGINES=ON, which fetches them.
//
// The same case directories as jinja2cpp_bench: main.j2, data.json, other *.j2 files as
// includes, settings.json. Each engine gets the template source as is and the data in its
// own native form (inja: nlohmann::json, minja: minja::Value), built once outside
// the timed loop, as jinja2cpp_bench builds a ValuesMap once. A case an engine cannot run
// (a settings.json option it lacks, a parse or render error) is left out; bench/run.py
// also leaves out a case whose output differs from Python Jinja2's, so every number in its
// table is the same work.
//
// Benchmarks: <engine>/Load/<case> (parse main.j2) and <engine>/Render/<case>.
//
// Usage: engines_bench [--cases-dir=<dir>] [--dump-dir=<dir>] [google benchmark flags]
//   --dump-dir  write <dir>/<engine>/<case>.txt for every case the engine renders (and
//               <case>.err with the reason for one it does not) and exit

#include <benchmark/benchmark.h>
#include <inja/inja.hpp>
#include <minja/minja.hpp>
#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#ifndef JINJA2CPP_BENCH_CASES_DIR
#define JINJA2CPP_BENCH_CASES_DIR "bench/cases"
#endif

namespace
{
namespace fs = std::filesystem;
using Json = nlohmann::ordered_json;

std::string ReadFile(const fs::path& path)
{
    std::ifstream in(path, std::ios::binary);
    std::ostringstream os;
    os << in.rdbuf();
    return os.str();
}

struct Case
{
    std::string name;
    std::string source;
    std::string injaSource;                      // main.inja: the same template in inja's dialect, when main.j2 is not
    std::map<std::string, std::string> includes; // the other *.j2 files
    Json data = Json::object();
    Json settings = Json::object();
};

Case LoadCase(const fs::path& dir)
{
    Case result;
    result.name = dir.filename().string();
    result.source = ReadFile(dir / "main.j2");
    if (fs::exists(dir / "main.inja"))
    {
        result.injaSource = ReadFile(dir / "main.inja");
    }
    if (fs::exists(dir / "data.json"))
    {
        result.data = Json::parse(ReadFile(dir / "data.json"));
    }
    if (fs::exists(dir / "settings.json"))
    {
        result.settings = Json::parse(ReadFile(dir / "settings.json"));
    }
    for (const auto& entry : fs::directory_iterator(dir))
    {
        const auto& path = entry.path();
        if (path.extension() == ".j2" && path.filename() != "main.j2")
        {
            result.includes.emplace(path.filename().string(), ReadFile(path));
        }
    }
    return result;
}

// Jinja2 drops one trailing newline of a template (keep_trailing_newline=False); inja keeps
// it, so it gets the source as Jinja2 sees it. Done once, outside the timed loop.
std::string DropTrailingNewline(std::string source)
{
    if (!source.empty() && source.back() == '\n')
    {
        source.pop_back();
        if (!source.empty() && source.back() == '\r')
        {
            source.pop_back();
        }
    }
    return source;
}

// One engine on one case: Load parses, Render renders what Prepare parsed
class Runner
{
public:
    virtual ~Runner() = default;
    // Empty when the engine can run the case, else why not
    virtual std::string Prepare() = 0;
    virtual void Load() = 0;
    virtual std::string Render() = 0;
};

class InjaRunner : public Runner
{
public:
    explicit InjaRunner(const Case& c)
        : m_case(c)
    {
    }

    std::string Prepare() override
    {
        if (m_case.settings.value("autoescape", false))
        {
            return "no autoescape";
        }
        if (m_case.settings.value("wide", false))
        {
            return "no wide strings";
        }
        m_env.set_trim_blocks(m_case.settings.value("trim_blocks", false));
        m_env.set_lstrip_blocks(m_case.settings.value("lstrip_blocks", false));
        m_env.set_search_included_templates_in_files(false);
        for (const auto& [name, source] : m_case.includes)
        {
            m_env.include_template(name, m_env.parse(DropTrailingNewline(source)));
        }
        m_data = inja::json::parse(m_case.data.dump());
        m_source = DropTrailingNewline(m_case.injaSource.empty() ? m_case.source : m_case.injaSource);
        m_template = m_env.parse(m_source);
        return {};
    }
    void Load() override { benchmark::DoNotOptimize(m_env.parse(m_source)); }
    std::string Render() override { return m_env.render(m_template, m_data); }

private:
    const Case& m_case;
    inja::Environment m_env;
    inja::json m_data;
    std::string m_source;
    inja::Template m_template;
};

class MinjaRunner : public Runner
{
public:
    explicit MinjaRunner(const Case& c)
        : m_case(c)
    {
    }

    std::string Prepare() override
    {
        if (m_case.settings.value("autoescape", false))
        {
            return "no autoescape";
        }
        if (m_case.settings.value("wide", false))
        {
            return "no wide strings";
        }
        if (!m_case.includes.empty())
        {
            return "no include, import or extends";
        }
        m_options.trim_blocks = m_case.settings.value("trim_blocks", false);
        m_options.lstrip_blocks = m_case.settings.value("lstrip_blocks", false);
        m_options.keep_trailing_newline = false;
        m_template = minja::Parser::parse(m_case.source, m_options);
        // Arrays and objects in a minja::Value are shared: built once, as the data is for the
        // other engines. Each render gets a fresh child scope, so `set` does not leak into
        // the next render. Keys go in sorted (through nlohmann::json): minja's tojson keeps
        // insertion order where Jinja2's sorts, and with sorted data the two agree.
        m_globals = minja::Context::make(minja::Value(Json(nlohmann::json(m_case.data))));
        return {};
    }
    void Load() override { benchmark::DoNotOptimize(minja::Parser::parse(m_case.source, m_options)); }
    std::string Render() override { return m_template->render(minja::Context::make(minja::Value::object(), m_globals)); }

private:
    const Case& m_case;
    minja::Options m_options{};
    std::shared_ptr<minja::TemplateNode> m_template;
    std::shared_ptr<minja::Context> m_globals;
};

struct Engine
{
    const char* name;
    std::function<std::unique_ptr<Runner>(const Case&)> make;
};

const std::vector<Engine>& Engines()
{
    static const std::vector<Engine> engines = {
        { "inja", [](const Case& c) -> std::unique_ptr<Runner> { return std::make_unique<InjaRunner>(c); } },
        { "minja", [](const Case& c) -> std::unique_ptr<Runner> { return std::make_unique<MinjaRunner>(c); } },
    };
    return engines;
}

// Prepares and renders once; an exception or a refusal becomes the reason it cannot run
std::unique_ptr<Runner> TryPrepare(const Engine& engine, const Case& c, std::string& output, std::string& error)
{
    auto runner = engine.make(c);
    try
    {
        error = runner->Prepare();
        if (error.empty())
        {
            output = runner->Render();
            return runner;
        }
    }
    catch (const std::exception& e)
    {
        error = e.what();
    }
    return nullptr;
}

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
    std::vector<Case> cases;
    cases.reserve(dirs.size());
    for (const auto& dir : dirs)
    {
        cases.push_back(LoadCase(dir));
    }

    // Runners live until the benchmarks have run
    std::vector<std::unique_ptr<Runner>> runners;
    for (const auto& engine : Engines())
    {
        for (const auto& c : cases)
        {
            std::string output;
            std::string error;
            auto runner = TryPrepare(engine, c, output, error);
            if (!dumpDir.empty())
            {
                const auto dir = fs::path(dumpDir) / engine.name;
                fs::create_directories(dir);
                std::ofstream(dir / (c.name + (runner ? ".txt" : ".err")), std::ios::binary) << (runner ? output : error);
                continue;
            }
            if (!runner)
            {
                continue;
            }
            Runner* r = runner.get();
            benchmark::RegisterBenchmark(std::string(engine.name) + "/Load/" + c.name, [r](benchmark::State& state) {
                for (auto _ : state)
                {
                    r->Load();
                }
            });
            benchmark::RegisterBenchmark(std::string(engine.name) + "/Render/" + c.name, [r](benchmark::State& state) {
                for (auto _ : state)
                {
                    auto res = r->Render();
                    benchmark::DoNotOptimize(res);
                }
            });
            runners.push_back(std::move(runner));
        }
    }
    if (!dumpDir.empty())
    {
        return 0;
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
