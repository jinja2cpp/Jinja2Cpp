#include "contracts.h"

#include <string>
#include <string_view>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
void ContractFailed(const char* kind, const char* condition, const char* file, int line)
{
    // The file's name without the build machine's directories: the message reaches whoever renders
    const std::string_view path(file);
    const auto slash = path.find_last_of("/\\");
    const auto name = slash == std::string_view::npos ? path : path.substr(slash + 1);
    throw ContractViolation(std::string("jinja2cpp: ") + kind + " violated: " + condition + " (" + std::string(name) + ":" + std::to_string(line) + ")");
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
