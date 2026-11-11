// The targets for templates a host's users write (docs/tasks/0267): FuzzLimits() and the settings byte of
// fuzz_common.h, host data and callables, and templates on disk.
#include "fuzz_common.h"

#include <jinja2cpp/filesystem_handler.h>
#include <jinja2cpp/generic_list.h>
#include <jinja2cpp/make_generic_list.h>
#include <jinja2cpp/reflected_value.h>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>
#include <jinja2cpp/user_callable.h>
#include <jinja2cpp/value.h>

#include <cctype>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <ios>
#include <limits>
#include <memory>
#include <optional>
#include <random>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

namespace jinja2_fuzz
{
// A host struct for the host-surface target: scalars, a nested struct by shared_ptr and a list of them
struct HostInner
{
    std::string name = "inner";
    std::int64_t weight = 7;
};

struct HostRecord
{
    std::int64_t id = 0;
    double score = 0.5;
    bool active = true;
    std::string title;
    std::wstring wideTitle;
    std::shared_ptr<HostInner> inner;
    std::vector<std::shared_ptr<HostInner>> children;
    std::vector<std::int64_t> numbers;
};
} // namespace jinja2_fuzz

namespace jinja2
{
template<>
struct TypeReflection<jinja2_fuzz::HostInner> : TypeReflected<jinja2_fuzz::HostInner>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            { "name", [](const jinja2_fuzz::HostInner& obj) { return Reflect(obj.name); } },
            { "weight", [](const jinja2_fuzz::HostInner& obj) { return Reflect(obj.weight); } },
        };
        return accessors;
    }
};

template<>
struct TypeReflection<jinja2_fuzz::HostRecord> : TypeReflected<jinja2_fuzz::HostRecord>
{
    static auto& GetAccessors()
    {
        static std::unordered_map<std::string, FieldAccessor> accessors = {
            { "id", [](const jinja2_fuzz::HostRecord& obj) { return Reflect(obj.id); } },
            { "score", [](const jinja2_fuzz::HostRecord& obj) { return Reflect(obj.score); } },
            { "active", [](const jinja2_fuzz::HostRecord& obj) { return Reflect(obj.active); } },
            { "title", [](const jinja2_fuzz::HostRecord& obj) { return Reflect(obj.title); } },
            { "titleView", [](const jinja2_fuzz::HostRecord& obj) { return Reflect(std::string_view(obj.title)); } },
            { "wideTitle", [](const jinja2_fuzz::HostRecord& obj) { return Reflect(obj.wideTitle); } },
            { "inner", [](const jinja2_fuzz::HostRecord& obj) { return Reflect(obj.inner); } },
            { "children", [](const jinja2_fuzz::HostRecord& obj) { return Reflect(obj.children); } },
            { "numbers", [](const jinja2_fuzz::HostRecord& obj) { return Reflect(obj.numbers); } },
        };
        return accessors;
    }
};
} // namespace jinja2

namespace jinja2_fuzz
{
namespace
{
std::string Narrow(const std::uint8_t* data, std::size_t size)
{
    return { reinterpret_cast<const char*>(data), size };
}

// The settings byte, then the template, narrow or wide, with the support templates, in `env`
template<typename CharT>
void RenderUntrusted(jinja2::TemplateEnv& env, std::uint8_t settingsByte, const std::basic_string<CharT>& source, const jinja2::ValuesMap& params)
{
    ConfigureUntrustedEnv(env, settingsByte);
    auto fs = std::make_shared<jinja2::MemoryFileSystem>();
    AddSupportTemplates(*fs, source, DelimitersOf(settingsByte));
    env.AddFilesystemHandler(std::string(), fs);
    jinja2::BasicTemplate<CharT> tpl(&env);
    // A Load or Render error is a normal outcome
    if (tpl.Load(source, "self"))
    {
        (void)tpl.RenderAsString(params);
    }
}

void RenderInput(jinja2::TemplateEnv& env, const std::uint8_t* data, std::size_t size, const jinja2::ValuesMap& params)
{
    const std::uint8_t settingsByte = data[0];
    if ((settingsByte & settings_byte::Wide) != 0)
    {
        RenderUntrusted(env, settingsByte, DecodeUtf8(data + 1, size - 1), params);
    }
    else
    {
        RenderUntrusted(env, settingsByte, Narrow(data + 1, size - 1), params);
    }
}

// Host data and callables, made afresh for each input, so that a render that keeps one past its end reads freed
// memory, which ASan reports, once the next input has run
jinja2::ValuesMap MakeHostContext()
{
    using jinja2::ArgInfo;
    using jinja2::MakeCallable;
    using jinja2::Value;
    auto params = MakeContext();

    auto record = std::make_shared<HostRecord>();
    record->id = 42;
    record->title = "record <one> & \"two\"";
    record->wideTitle = L"wide \u00e9 record";
    record->inner = std::make_shared<HostInner>();
    for (int n = 0; n < 3; ++n)
    {
        auto child = std::make_shared<HostInner>();
        child->name = "child" + std::to_string(n);
        child->weight = n;
        record->children.push_back(std::move(child));
    }
    record->numbers = { 5, 3, 9, 1 };
    auto second = std::make_shared<HostRecord>();
    second->id = 7;
    second->title = "second";
    std::vector<std::shared_ptr<HostRecord>> records{ record, second };

    params["rec"] = jinja2::Reflect(record);
    // Reflect borrows an lvalue container or struct and owns an rvalue one: the values must own theirs, as this
    // function's locals end before the render starts
    params["recs"] = jinja2::Reflect(std::move(records));
    params["recValue"] = jinja2::Reflect(HostRecord(*record));
    params["ints"] = jinja2::Reflect(std::vector<std::int64_t>{ 9, 0, 8, 1, 7 });
    params["strs"] = jinja2::Reflect(std::vector<std::string>{ "b", "a", "c" });
    params["gen"] = jinja2::MakeGenericList([n = 0]() mutable -> std::optional<Value> {
        if (n >= 5)
        {
            return std::nullopt;
        }
        return Value(n++);
    });

    params["add"] = MakeCallable(
        [](std::int64_t a, std::int64_t b) {
            if ((b > 0 && a > std::numeric_limits<std::int64_t>::max() - b) || (b < 0 && a < std::numeric_limits<std::int64_t>::min() - b))
            {
                throw std::overflow_error("add overflows");
            }
            return a + b;
        },
        ArgInfo{ "a" }, ArgInfo{ "b", false, 1 });
    params["concat"] = MakeCallable(
        [](const jinja2::ValuesList& args) {
            std::string result;
            for (const auto& arg : args)
            {
                if (arg.IsString())
                {
                    result += arg.AsString();
                }
            }
            return result;
        },
        ArgInfo{ "*args" });
    params["kw"] = MakeCallable([](const jinja2::ValuesMap& kwargs) { return kwargs; }, ArgInfo{ "**kwargs" });
    params["lookup"] = MakeCallable([](const jinja2::GenericMap& context, const std::string& name) { return context.GetValueByName(name); },
                                    ArgInfo{ "*context" }, ArgInfo{ "name" });
    params["identity"] = MakeCallable([](const Value& v) { return v; }, ArgInfo{ "v" });
    // A fresh host object on each call, owned only by the value returned
    params["make"] = MakeCallable(
        [](std::int64_t id) {
            auto made = std::make_shared<HostRecord>();
            made->id = id;
            made->title = "made " + std::to_string(id);
            made->inner = std::make_shared<HostInner>();
            return jinja2::Reflect(std::move(made));
        },
        ArgInfo{ "id", false, 0 });
    // Render reports a host exception of any type as an error (docs/api-2.0.md 2.1)
    params["fail"] = MakeCallable(
        [](std::int64_t how) -> Value {
            if (how == 1)
            {
                throw 1; // NOLINT(hicpp-exception-baseclass): what a host may throw is not up to the engine
            }
            throw std::runtime_error("host failure");
        },
        ArgInfo{ "how", false, 0 });
    return params;
}

// The loader target's files. <dir>/plain and <dir>/linked hold the same templates; <dir>/canary.txt and
// <dir>/outside/canary.txt lie outside both. Only linked/ has links to them: RealFileSystem follows links, as Python's
// FileSystemLoader does, so it serves plain/, and ConfinedFileSystem, which must not follow them, serves linked/
struct LoaderTree
{
    std::filesystem::path dir;
    std::filesystem::path plain;
    std::filesystem::path linked;
    std::vector<std::filesystem::path> canaries;

    LoaderTree()
    {
        std::random_device seed;
        dir = std::filesystem::temp_directory_path() / ("jinja2cpp-fuzz-loader-" + std::to_string(seed()) + "-" + std::to_string(seed()));
        plain = dir / "plain";
        linked = dir / "linked";
        std::filesystem::create_directories(dir / "outside");
        canaries = { dir / "canary.txt", dir / "outside" / "canary.txt" };
        for (const auto& root : { plain, linked })
        {
            std::filesystem::create_directories(root / "sub" / "deeper");
        }
        WriteSupport(Delimiters{ DelimiterSet(0) });
        // Links out of the root: absolute, relative with `..`, and a directory; and one inside it. Creating them may
        // fail (Windows without the privilege); the target then fuzzes the names only
        std::error_code ignored;
        std::filesystem::create_symlink(canaries[0], linked / "escape-abs", ignored);
        std::filesystem::create_symlink(std::filesystem::path("..") / "outside" / "canary.txt", linked / "escape-up", ignored);
        std::filesystem::create_directory_symlink(dir / "outside", linked / "escape-dir", ignored);
        std::filesystem::create_symlink(std::filesystem::path("deeper") / "leaf", linked / "sub" / "inside-link", ignored);
#if !defined(_WIN32)
        // Without the links the confined half would check nothing
        if (!std::filesystem::is_symlink(linked / "escape-abs") || !std::filesystem::is_symlink(linked / "escape-up") || !std::filesystem::is_symlink(linked / "escape-dir"))
        {
            std::abort();
        }
#endif
    }
    ~LoaderTree()
    {
        std::error_code ignored;
        std::filesystem::remove_all(dir, ignored);
    }
    LoaderTree(const LoaderTree&) = delete;
    LoaderTree& operator=(const LoaderTree&) = delete;
    LoaderTree(LoaderTree&&) = delete;
    LoaderTree& operator=(LoaderTree&&) = delete;

    // The templates beside the input, written with its delimiters
    void WriteSupport(const Delimiters& delims) const
    {
        for (const auto& root : { plain, linked })
        {
            Write(root / "base", WithDelimiters<char>("<{% block content %}base{% endblock %}|{% block other %}{% endblock %}>", delims));
            Write(root / "header", WithDelimiters<char>("[{{ foo }}|{{ bar }}]", delims));
            Write(root / "macros", WithDelimiters<char>("{% macro m(a, b=2) %}{{ a }}-{{ b }}{% endmacro %}{% set v = 42 %}", delims));
            Write(root / "sub" / "item", WithDelimiters<char>("{% include 'header' %}<{{ x }}>", delims));
            Write(root / "sub" / "deeper" / "leaf", WithDelimiters<char>("leaf {{ x }}", delims));
        }
    }

    static void Write(const std::filesystem::path& path, std::string_view text)
    {
        std::ofstream out(path, std::ios::binary | std::ios::trunc);
        out.write(text.data(), static_cast<std::streamsize>(text.size()));
    }
};

// Text the input cannot know: new for every input, so the fuzzer cannot learn it from the comparisons of one run
std::string NewCanary()
{
    static std::mt19937_64 generator{ std::random_device{}() };
    static const char digits[] = "0123456789abcdef";
    std::string canary = "CANARY-";
    for (int n = 0; n < 24; ++n)
    {
        canary.push_back(digits[generator() & 0xF]);
    }
    return canary;
}
// The self-check of .github/workflows/fuzz.yml (docs/tasks/0267): with JINJA2CPP_FUZZ_INJECT_SLOW set, an input that
// starts with SlowSeed hangs, so the job can show that a timeout of this target fails it
constexpr std::string_view SlowSeed = "\xFFjinja2cpp-fuzz-slow-seed";

void InjectSlowSeed(const std::uint8_t* data, std::size_t size)
{
    static const bool enabled = [] {
#if defined(_MSC_VER)
        char* value = nullptr;
        std::size_t length = 0;
        const bool set = _dupenv_s(&value, &length, "JINJA2CPP_FUZZ_INJECT_SLOW") == 0 && value;
        std::free(value); // NOLINT(cppcoreguidelines-no-malloc): _dupenv_s allocates with malloc
        return set;
#else
        return std::getenv("JINJA2CPP_FUZZ_INJECT_SLOW") != nullptr; // NOLINT(concurrency-mt-unsafe): read once, nothing sets it
#endif
    }();
    if (enabled && Narrow(data, size).rfind(SlowSeed, 0) == 0)
    {
        std::this_thread::sleep_for(std::chrono::minutes(10));
    }
}
// Whether the output holds the canary's random tail, in any case and either direction: what reaches it through
// upper, lower, title, capitalize, reverse or a slice that keeps the tail
bool HoldsCanary(std::string output, const std::string& canary)
{
    for (auto& c : output)
    {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    const std::string tail = canary.substr(canary.find('-') + 1);
    const std::string reversed(tail.rbegin(), tail.rend());
    return output.find(tail) != std::string::npos || output.find(reversed) != std::string::npos;
}
} // namespace

void ConfigureUntrustedEnv(jinja2::TemplateEnv& env, std::uint8_t settingsByte)
{
    ConfigureEnv(env);
    auto& settings = env.GetSettings();
    settings.limits = FuzzLimits();
    ApplySettingsByte(settings, settingsByte);
}

void FuzzUntrusted(const std::uint8_t* data, std::size_t size)
{
    if (size == 0 || size > MaxInputSize)
    {
        return;
    }
    InjectSlowSeed(data, size);
    jinja2::TemplateEnv env;
    RenderInput(env, data, size, MakeContext());
}

void FuzzHostSurface(const std::uint8_t* data, std::size_t size)
{
    using jinja2::ArgInfo;
    using jinja2::MakeCallable;
    if (size == 0 || size > MaxInputSize)
    {
        return;
    }
    jinja2::TemplateEnv env;
    env.AddFilter("twice", MakeCallable([](const jinja2::Value& v) { return jinja2::ValuesList{ v, v }; }, ArgInfo{ "v" }));
    env.AddFilter("ctxget",
                  MakeCallable([](const std::string& name, const jinja2::GenericMap& context) { return context.GetValueByName(name); }, ArgInfo{ "name" },
                               ArgInfo{ "*context" }));
    env.AddTest("positive", MakeCallable([](std::int64_t v) { return v > 0; }, ArgInfo{ "v" }));
    env.AddGlobal("grec", jinja2::Reflect(std::make_shared<HostRecord>()));
    RenderInput(env, data, size, MakeHostContext());
}

void FuzzLoader(const std::uint8_t* data, std::size_t size)
{
    if (size < 2 || size > MaxInputSize)
    {
        return;
    }
    static const LoaderTree tree;
    const std::string canary = NewCanary();
    for (const auto& path : tree.canaries)
    {
        LoaderTree::Write(path, canary);
    }
    // Narrow only: the names are the point here
    const auto settingsByte = static_cast<std::uint8_t>(data[0] & ~settings_byte::Wide);
    const bool confined = (data[1] & 1) != 0;
    const std::string source = Narrow(data + 2, size - 2);
    const auto& root = confined ? tree.linked : tree.plain;
    LoaderTree::Write(root / "self", source);
    tree.WriteSupport(DelimitersOf(settingsByte));

    jinja2::TemplateEnv env;
    ConfigureUntrustedEnv(env, settingsByte);
    if (confined)
    {
        env.AddFilesystemHandler(std::string(), std::make_shared<jinja2::ConfinedFileSystem>(root.string()));
    }
    else
    {
        env.AddFilesystemHandler(std::string(), std::make_shared<jinja2::RealFileSystem>(root.string()));
    }
    // By name from the root, and from the source text. Error text counts too: the input cannot hold the canary, so an
    // error that quotes it quotes a file read outside the root
    std::string output;
    const auto take = [&output](const auto& result) {
        if (result)
        {
            output += result.value();
        }
        else
        {
            output += result.error().ToString();
        }
    };
    auto loaded = env.LoadTemplate("self");
    if (loaded)
    {
        take(loaded.value().RenderAsString(MakeContext()));
    }
    else
    {
        output += loaded.error().ToString();
    }
    jinja2::Template tpl(&env);
    auto parsed = tpl.Load(source, "self");
    if (parsed)
    {
        take(tpl.RenderAsString(MakeContext()));
    }
    else
    {
        output += parsed.error().ToString();
    }
    // Neither handler may reach a canary by a name, nor ConfinedFileSystem through a link
    if (HoldsCanary(std::move(output), canary))
    {
        std::abort();
    }
}

} // namespace jinja2_fuzz
