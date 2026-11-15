#include "contracts.h"

#include <array>
#include <charconv>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <string_view>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
namespace
{
// The file's name without the build machine's directories: the message reaches whoever renders
std::string_view FileName(const char* file)
{
    const std::string_view path(file);
    const auto slash = path.find_last_of("/\\");
    return slash == std::string_view::npos ? path : path.substr(slash + 1);
}
} // namespace

void ContractFailed(const char* kind, const char* condition, const char* file, int line)
{
    const auto name = FileName(file);
    throw ContractViolation(std::string("jinja2cpp: ") + kind + " violated: " + condition + " (" + std::string(name) + ":" + std::to_string(line) + ")");
}

void ContractFailedNoexcept(const char* kind, const char* condition, const char* file, int line) noexcept
{
    // No allocation: the check may run while memory is short or during unwinding
    const auto name = FileName(file);
    std::array<char, 16> number{};
    const auto end = std::to_chars(number.data(), number.data() + number.size(), line).ptr;
    for (const std::string_view part : { std::string_view("jinja2cpp: "), std::string_view(kind), std::string_view(" violated: "), std::string_view(condition), std::string_view(" ("), name,
                                         std::string_view(":"), std::string_view(number.data(), static_cast<size_t>(end - number.data())), std::string_view(")\n") })
    {
        std::fwrite(part.data(), 1, part.size(), stderr);
    }
    std::abort();
}
} // namespace detail
} // namespace v2
} // namespace jinja2

#if defined(__cpp_contracts) && defined(JINJA2CPP_USE_LANGUAGE_CONTRACTS)
#include <contracts>

// Replaces the default handler of the whole program, which JINJA2CPP_LANGUAGE_CONTRACTS asks for
void handle_contract_violation(const std::contracts::contract_violation& violation)
{
    jinja2::detail::ContractFailed("contract", violation.comment(), violation.location().file_name(), static_cast<int>(violation.location().line()));
}
#endif
