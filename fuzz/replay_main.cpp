// Runs every fuzz target over the given files and directories without libFuzzer, so the
// regression inputs in fuzz/regressions/ are checked by ctest under every compiler
// (GCC, MSVC) and sanitizer configuration, not only in clang fuzzing builds.
// With --json it renders each input once and prints the outcome as a JSON line instead,
// for the differential check against Python Jinja2 (fuzz/differential.py).
#include "fuzz_common.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace
{

bool g_json = false;

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

void RunFile(const std::filesystem::path& path)
{
    std::ifstream in(path, std::ios::binary);
    const std::vector<char> bytes((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
    const auto* data = reinterpret_cast<const std::uint8_t*>(bytes.data());
    if (g_json)
    {
        PrintOutcome(path, data, bytes.size());
        return;
    }
    std::cout << "replay " << path.generic_string() << '\n' << std::flush;
    jinja2_fuzz::FuzzParse(data, bytes.size());
    jinja2_fuzz::FuzzRender(data, bytes.size());
    jinja2_fuzz::FuzzRenderWide(data, bytes.size());
}

} // namespace

int main(int argc, char* argv[])
{
    std::size_t count = 0;
    for (int i = 1; i < argc; ++i)
    {
        if (std::string(argv[i]) == "--json")
        {
            g_json = true;
            continue;
        }
        const std::filesystem::path arg(argv[i]);
        if (std::filesystem::is_directory(arg))
        {
            std::vector<std::filesystem::path> files;
            for (const auto& entry : std::filesystem::recursive_directory_iterator(arg))
            {
                if (entry.is_regular_file() && entry.path().filename().string()[0] != '.')
                {
                    files.push_back(entry.path());
                }
            }
            std::sort(files.begin(), files.end());
            for (const auto& file : files)
            {
                RunFile(file);
                ++count;
            }
        }
        else
        {
            RunFile(arg);
            ++count;
        }
    }
    if (!g_json)
    {
        std::cout << "replayed " << count << " inputs" << '\n';
    }
    return 0;
}
