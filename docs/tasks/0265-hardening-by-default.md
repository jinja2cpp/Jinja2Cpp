---
status: in-progress
priority: medium
area: build
touches: [CMakeLists.txt, cmake/, .github/workflows/, src/contracts.h, src/contracts.cpp]
shares: [src/node_ref.h]
---
# Build hardening on by default; invariant checks that fail the render, not the process

**Decided (Ruslan 2026-10-18, untrusted-input review S8).** Hardening flags are on by default.
Assertion modes that abort are not wanted ("they cause crash"); C++ contracts are the more
interesting direction.

**Proposal.**
- `JINJA2CPP_HARDENING` (default ON) adds to the library build: `-fstack-protector-strong`,
  `-D_FORTIFY_SOURCE=3` (optimised builds), `-fstack-clash-protection`, `-fcf-protection` where
  supported; MSVC `/GS /guard:cf`. These trap only on actual memory corruption, where stopping is
  the only safe outcome. Measure each with `bench/count.py`; the perf track decides whether any one
  costs too much (`-fstack-protector-strong` has bitten inlined lookups before: memory notes on 0129
  and 0117).
- Not used: `_GLIBCXX_ASSERTIONS`, `_LIBCPP_HARDENING_MODE`: they abort on a precondition failure.
- Invariant checks: one macro family in `src/contracts.h` (`JINJA2CPP_EXPECTS`, `JINJA2CPP_ASSERT`)
  whose violation throws an internal exception, caught at the render boundary as an error
  `Result` (the pattern `JINJA2CPP_NODEREF_CHECKS` already follows with `InvalidNodeRef`). Under a
  C++26 build with P2900 contracts available, the macros map to `pre`/`contract_assert` with a
  violation handler that throws, so the same checks become language contracts without changing
  behaviour. Check compiler support when implementing (GCC first; Clang and MSVC later).
  Public headers keep the C++17 floor, so contracts stay in `src/`.
- CI: CodeQL `security-extended` query suite.

**Done when.** Default builds carry the flags on GCC, Clang and MSVC with the bench delta
recorded, the macro family replaces ad-hoc internal asserts, and the C++26 row exercises the
contracts mapping where the compiler supports it.

**Trust model.** When this lands, update its entry under Known gaps in [docs/security.md](../security.md) (0224).

## Result

**Landed.** `JINJA2CPP_HARDENING` (default ON, `cmake/hardening.cmake`) applies, to the library target only
and each only where the compiler takes it: `-fstack-protector-strong`, `-fstack-clash-protection`,
`-fcf-protection` (x86), `-D_FORTIFY_SOURCE=3` for Release/RelWithDebInfo/MinSizeRel (2 when a `-Werror`
probe rejects 3; skipped under sanitizers and fuzz builds, which intercept the same calls); MSVC `/GS`
`/guard:cf` (plus `/GUARD:CF` at link for a shared library; untested, the cloud has no MSVC). The configure
step prints what it applied. Ubuntu's GCC already defaults to all four (and fortify 3 at -O2), so the flags
change nothing there; they matter for Clang, other distributions, the Python wheel and `cmake` builds that
reset the flags.

`src/contracts.h` has `JINJA2CPP_EXPECTS` and `JINJA2CPP_ASSERT`; a violation throws
`detail::ContractViolation` (a `std::logic_error`) from the out-of-line `[[noreturn]] ContractFailed`
(`src/contracts.cpp`). The Load and Render boundaries report it like any `std::exception`
(`UnexpectedException` with the condition, file and line), as they do `InvalidNodeRef`. Level by
`JINJA2CPP_CONTRACT_CHECKS` (empty: FULL for Debug, sanitizer and fuzz builds, else ON; OFF none, ON
EXPECTS, FULL both). With `__cpp_contracts` and the opt-in `JINJA2CPP_LANGUAGE_CONTRACTS` the macros expand
to `contract_assert` and `contracts.cpp` defines `handle_contract_violation`. **Not exercised**: GCC 13 and
Clang 18 have no contracts, and the CI C++26 rows (gcc-14, clang-20) have none either, so that branch is
compiled out and untested; no CI row was added. Revisit when GCC 16 (`-fcontracts`) is in the matrix.

Ad-hoc `assert(` calls in `src/` became `JINJA2CPP_ASSERT`, except where a throw would only become
`std::terminate`: everything in `span.h`; `SmallVector::operator[]`, `pop_back`, the base constructor;
`NodeRef` list `Push`; `BackendObject::Out`/`Get`; `RenderWorkspace::ReturnMaps` and `Release` (reached
from `~UnitCall`); `RenderContext::~RenderContext`, `~HomeCall`, `~Direct`, `Leave`, `ExitScope()` and
`UnbindSlots` (reached from destructors). `FindValueCached`'s check of `m_noCachedLookups` stays under
`#ifndef NDEBUG`, since the counter exists only there. Vendored `robin_hood.h` and `lexertk.h` untouched.
The CodeQL workflow now runs the `security-extended` suite.

**Instruction counts** (`bench/count.py`, g++-13 -O3 Release, 26 Load and 26 Render rows, geometric mean
against a build with all four off, `-fno-stack-protector -fcf-protection=none -fno-stack-clash-protection
-U_FORTIFY_SOURCE`; the single-flag builds pass the flags to the bench driver as well):

| | all on | `-fstack-protector-strong` alone | clash + cf + fortify alone |
|---|---:|---:|---:|
| Load | +4.22% | +3.03% | +1.60% |
| Render | +6.58% | +4.64% | +2.34% |
| worst row | Render/expressions +12.09% | Render/expressions +8.94% | Load/plain_text +3.34% |

`-fstack-protector-strong` is the largest part, then the rest (most likely `endbr64` from
`-fcf-protection`; not separated). They are instruction counts, not cycles: canary checks and `endbr64`
are cheap per instruction.

**Hardening exception (perf track, 2026-11-10).** Every flag stays ON, `-fstack-protector-strong`
included (not a distro's weaker level), for every build we make, the Python wheel too: template text is
untrusted, and the canary is the flag that matters most. The table above is the recorded worst case
(against a build with no hardening at all); Ubuntu, Debian and Fedora GCC already enable all four, so
most Linux users and the CI count job pay nothing new, and a CI count row that moves more than +0.5%
goes back to the perf track. Winning the canary cost back without dropping the flag is task 0454.
