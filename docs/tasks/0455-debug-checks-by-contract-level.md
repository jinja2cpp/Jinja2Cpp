---
status: open
priority: low
area: robustness
depends: [0265]
touches: [src/render_context.h]
---
# RenderContext's debug-only invariants follow the contract level, not NDEBUG

**Problem.** 0265 made `JINJA2CPP_ASSERT` checks follow `JINJA2CPP_CONTRACT_CHECKS` (FULL in
Debug, sanitizer and fuzz builds). The members that RenderContext keeps only for its debug
invariants (`m_noCachedLookups`, `m_debugScopes`, `m_debugExceptions`, the `Direct` members in
src/render_context.h) are still compiled under `#ifndef NDEBUG`, so a RelWithDebInfo sanitizer
or fuzz build at FULL checks every other invariant but not these, and the `FindValueCached`
check stays a bare `#ifndef NDEBUG` block.

**Proposal.** Guard those members and their checks with `detail::ContractChecks >= 2` (or a
macro of src/contracts.h) instead of `NDEBUG`, so the sanitizer and fuzz rows check them too.
Measure the Release build stays unchanged (`bench/count.py`: the members must not exist at ON).

**Done when** a RelWithDebInfo FULL build has the members and runs the checks, and a Release
ON build's instruction counts are unchanged.

**Also.** Fuzz and sanitizer builds now run `JINJA2CPP_ASSERT` at FULL, and a violation becomes an
`UnexpectedException` error Result instead of an abort, which a fuzz target accepts as an ordinary
error. The targets in `fuzz/` should treat an error whose text carries " violated: " as a finding
(abort there), so that an invariant failure the fuzzer reaches is reported (0265 verifier).
