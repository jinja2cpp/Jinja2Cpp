// Runs every fuzz target over the given files and directories without libFuzzer, so the
// regression inputs in fuzz/regressions/ are checked by ctest under every compiler
// (GCC, MSVC) and sanitizer configuration, not only in clang fuzzing builds.
// jinja2cpp_fuzz_alloc_replay is this file built with JINJA2CPP_FUZZ_ALLOC_REPLAY: it runs only the
// allocation-failure target, whose operator new replacement the other targets must not see.
// With --json it renders each input once and prints the outcome as a JSON line instead,
// for the differential check against Python Jinja2 (fuzz/differential.py).
// With --mutate N it runs the structure-aware mutator (template_mutator.h) N times over each input, and N times as a
// crossover with the next one, and feeds the mutants to the targets that bound their work (parse, untrusted, host), so
// the mutator is checked under every compiler (ctest jinja2cpp_fuzz_mutator_smoke).
#include "fuzz_common.h"
#include "template_mutator.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <string>
#include <vector>

namespace
{

bool g_json = false;
std::size_t g_mutations = 0;

// Stands in for LLVMFuzzerMutate: changes, inserts or erases one byte
std::size_t MutateOneByte(std::uint8_t* data, std::size_t size, std::size_t maxSize)
{
    static std::mt19937 rng(95);
    std::uniform_int_distribution<std::size_t> pick(0, size == 0 ? 0 : size - 1);
    const std::size_t pos = pick(rng);
    switch (rng() % 3)
    {
    case 0:
        if (size < maxSize)
        {
            std::copy_backward(data + pos, data + size, data + size + 1);
            data[pos] = static_cast<std::uint8_t>(rng());
            return size + 1;
        }
        [[fallthrough]];
    case 1:
        data[pos] = static_cast<std::uint8_t>(rng());
        return size;
    default:
        if (size > 1)
        {
            std::copy(data + pos + 1, data + size, data + pos);
            return size - 1;
        }
        return size;
    }
}

void RunMutants(const std::vector<char>& bytes, const std::vector<char>& next)
{
    const auto* input = reinterpret_cast<const std::uint8_t*>(bytes.data());
    const auto* other = reinterpret_cast<const std::uint8_t*>(next.data());
    std::vector<std::uint8_t> buffer(jinja2_fuzz::MaxInputSize);
    for (unsigned seed = 0; seed < g_mutations; ++seed)
    {
        for (const auto& layout : { jinja2_fuzz::PlainTemplate, jinja2_fuzz::SettingsPrefix })
        {
            const auto runTargets = [&layout, &buffer](std::size_t size) {
                if (layout.prefixLength == 0)
                {
                    jinja2_fuzz::FuzzParse(buffer.data(), size);
                }
                else
                {
                    jinja2_fuzz::FuzzUntrusted(buffer.data(), size);
                    jinja2_fuzz::FuzzHostSurface(buffer.data(), size);
                }
            };
            const std::size_t size = std::min(bytes.size(), buffer.size());
            std::copy(input, input + size, buffer.begin());
            runTargets(jinja2_fuzz::MutateTemplate(buffer.data(), size, buffer.size(), seed, layout, &MutateOneByte));
            const std::size_t crossed = jinja2_fuzz::CrossOverTemplates(input, size, other, std::min(next.size(), buffer.size()), buffer.data(),
                                                                        buffer.size(), seed, layout);
            if (crossed != 0)
            {
                runTargets(crossed);
            }
        }
    }
}

// Output bytes go out as hex: they need not be valid UTF-8
std::string Hex(const std::string& bytes)
{
    static const char digits[] = "0123456789abcdef";
    std::string result;
    result.reserve(bytes.size() * 2);
    for (const unsigned char c : bytes)
    {
        result.push_back(digits[c >> 4]);
        result.push_back(digits[c & 0xF]);
    }
    return result;
}

void PrintOutcome(const std::filesystem::path& path, const std::uint8_t* data, std::size_t size)
{
    static const char* const kinds[] = { "rendered", "parse_error", "render_error" };
    const auto outcome = jinja2_fuzz::RenderOutcome(data, size);
    std::cout << R"({"file": ")" << path.filename().string() << R"(", "kind": ")" << kinds[outcome.kind] << R"(", "code": )"
              << static_cast<int>(outcome.code) << R"(, "output": ")" << Hex(outcome.output) << "\"}\n";
}

std::vector<char> ReadFile(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    return { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
}

void RunFile(const std::filesystem::path& path, const std::filesystem::path& next)
{
    const std::vector<char> bytes = ReadFile(path);
    if (g_mutations != 0)
    {
        std::cout << "mutate " << path.generic_string() << '\n'
                  << std::flush;
        RunMutants(bytes, ReadFile(next));
        return;
    }
    const auto* data = reinterpret_cast<const std::uint8_t*>(bytes.data());
    if (g_json)
    {
        PrintOutcome(path, data, bytes.size());
        return;
    }
    std::cout << "replay " << path.generic_string() << '\n'
              << std::flush;
#if defined(JINJA2CPP_FUZZ_ALLOC_REPLAY)
    // jinja2cpp_fuzz_alloc_replay: only the target that replaces operator new (alloc_failure_target.cpp)
    jinja2_fuzz::FuzzAllocFailure(data, bytes.size());
#else
    jinja2_fuzz::FuzzParse(data, bytes.size());
    jinja2_fuzz::FuzzRender(data, bytes.size());
    jinja2_fuzz::FuzzRenderWide(data, bytes.size());
    jinja2_fuzz::FuzzMetadata(data, bytes.size());
    jinja2_fuzz::FuzzPromptEscape(data, bytes.size());
    jinja2_fuzz::FuzzUntrusted(data, bytes.size());
    jinja2_fuzz::FuzzHostSurface(data, bytes.size());
    jinja2_fuzz::FuzzLoader(data, bytes.size());
#endif
}

} // namespace

int main(int argc, char* argv[])
{
    std::vector<std::filesystem::path> files;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--json")
        {
            g_json = true;
            continue;
        }
        if (std::string(argv[i]) == "--mutate" && i + 1 < argc)
        {
            g_mutations = std::stoul(argv[++i]);
            continue;
        }
        const std::filesystem::path arg(argv[i]);
        if (std::filesystem::is_directory(arg))
        {
            std::vector<std::filesystem::path> found;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(arg))
            {
                if (entry.is_regular_file() && entry.path().filename().string()[0] != '.')
                {
                    found.push_back(entry.path());
                }
            }
            std::sort(found.begin(), found.end());
            files.insert(files.end(), found.begin(), found.end());
        }
        else
        {
            files.push_back(arg);
        }
    }
    for (std::size_t i = 0; i < files.size(); ++i)
    {
        RunFile(files[i], files[(i + 1) % files.size()]);
    }
    const std::size_t count = files.size();
    if (!g_json)
    {
        std::cout << "replayed " << count << " inputs" << '\n';
    }
    return 0;
}
