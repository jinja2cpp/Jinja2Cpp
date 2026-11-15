#ifndef JINJA2CPP_SRC_CONTRACTS_H
#define JINJA2CPP_SRC_CONTRACTS_H

// Checks of preconditions and invariants whose violation fails the render, not the process
// (docs/tasks/0265). A violation throws ContractViolation, which the Load and Render
// boundaries turn into an error Result as they do any std::exception (error_boundary.h),
// the way InvalidNodeRef is reported (node_arena.h). Only JINJA2CPP_ASSERT_NOEXCEPT aborts.
//
//   JINJA2CPP_EXPECTS(cond)  a precondition at a boundary, cheap enough to keep in Release
//   JINJA2CPP_ASSERT(cond)   an internal invariant, which may cost something
//
// CMake JINJA2CPP_CONTRACT_CHECKS picks the level: OFF checks nothing, ON (the Release
// default) checks EXPECTS, FULL (Debug, sanitizer and fuzz builds) checks both. An unchecked
// condition is not evaluated but still compiled, so what it names stays "used".
//
// In a C++26 build whose compiler has P2900 contracts, and with the opt-in CMake option
// JINJA2CPP_LANGUAGE_CONTRACTS, the macros expand to contract_assert and contracts.cpp installs
// the violation handler that throws ContractViolation. That handler is program-global, which
// is why a library does not install one unasked.
//
// Do not use them in a noexcept function or a destructor: the throw would only become
// std::terminate there. JINJA2CPP_ASSERT_NOEXCEPT is the invariant check for those: it
// cannot report an error, so a violation prints the condition and aborts, as `assert` did
// before (docs/tasks/0455).
//
// JINJA2CPP_FULL_CONTRACT_CHECKS is 1 at FULL, else 0: state kept only for invariant checks
// (a counter, a size to compare on exit) is declared under `#if JINJA2CPP_FULL_CONTRACT_CHECKS`,
// so a Release (ON) build does not have it and a RelWithDebInfo sanitizer or fuzz build
// (FULL with NDEBUG) checks it. Do not guard such state with NDEBUG.

#ifndef JINJA2CPP_CONTRACT_CHECKS_LEVEL
#error "JINJA2CPP_CONTRACT_CHECKS_LEVEL comes with the jinja2cpp target (CMake option JINJA2CPP_CONTRACT_CHECKS)"
#endif

#include <jinja2cpp/config.h>

#include <stdexcept>
#include <string>

namespace jinja2
{
inline namespace v2
{
namespace detail
{
#if JINJA2CPP_CONTRACT_CHECKS_LEVEL >= 2
#define JINJA2CPP_FULL_CONTRACT_CHECKS 1
#else
#define JINJA2CPP_FULL_CONTRACT_CHECKS 0
#endif

// How much is checked: 0 (OFF), 1 (EXPECTS), 2 (EXPECTS and ASSERT)
constexpr int ContractChecks = JINJA2CPP_CONTRACT_CHECKS_LEVEL;
static_assert(ContractChecks >= 0 && ContractChecks <= 2, "JINJA2CPP_CONTRACT_CHECKS_LEVEL is 0, 1 or 2");

// A precondition or invariant that does not hold: a bug in the engine or its host, never a template's fault
class ContractViolation : public std::logic_error
{
public:
    explicit ContractViolation(const std::string& what)
        : std::logic_error(what)
    {
    }
};

// The cold path of a failed check: out of line, so a check costs one compare and a branch
[[noreturn]] JINJA2CPP_EXPORT void ContractFailed(const char* kind, const char* condition, const char* file, int line);
// The same for a check where nothing may throw: prints the message to stderr and aborts
[[noreturn]] JINJA2CPP_EXPORT void ContractFailedNoexcept(const char* kind, const char* condition, const char* file, int line) noexcept;
} // namespace detail
} // namespace v2
} // namespace jinja2

#if defined(__cpp_contracts) && defined(JINJA2CPP_USE_LANGUAGE_CONTRACTS)
#define JINJA2CPP_CONTRACT_CHECK_(level, kind, cond) \
    do \
    { \
        if constexpr (::jinja2::detail::ContractChecks >= (level)) \
        { \
            contract_assert(cond); \
        } \
    } while (false)
#else
// Unchecked, the condition sits in the discarded branch of the if constexpr: compiled, never run
#define JINJA2CPP_CONTRACT_CHECK_(level, kind, cond) \
    do \
    { \
        if constexpr (::jinja2::detail::ContractChecks >= (level)) \
        { \
            static_cast<bool>(cond) ? static_cast<void>(0) : ::jinja2::detail::ContractFailed(kind, #cond, __FILE__, __LINE__); \
        } \
    } while (false)
#endif

#define JINJA2CPP_EXPECTS(cond) JINJA2CPP_CONTRACT_CHECK_(1, "precondition", cond)
#define JINJA2CPP_ASSERT(cond) JINJA2CPP_CONTRACT_CHECK_(2, "invariant", cond)
// An invariant checked in a destructor or a noexcept function: never contract_assert, whose
// handler throws
#define JINJA2CPP_ASSERT_NOEXCEPT(cond) \
    do \
    { \
        if constexpr (::jinja2::detail::ContractChecks >= 2) \
        { \
            static_cast<bool>(cond) ? static_cast<void>(0) : ::jinja2::detail::ContractFailedNoexcept("invariant", #cond, __FILE__, __LINE__); \
        } \
    } while (false)

#endif // JINJA2CPP_SRC_CONTRACTS_H
