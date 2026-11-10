#include "contracts.h"

#include <string>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
void ContractFailed(const char* kind, const char* condition, const char* file, int line)
{
    throw ContractViolation(std::string("jinja2cpp: ") + kind + " violated: " + condition + " (" + file + ":" + std::to_string(line) + ")");
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
