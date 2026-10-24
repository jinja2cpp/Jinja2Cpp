---
status: open
priority: medium
area: build
touches: [CMakeLists.txt, cmake/, .github/workflows/, src/contracts.h]
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
