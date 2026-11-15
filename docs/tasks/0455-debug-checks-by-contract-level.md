---
status: in-progress
priority: low
area: robustness
depends: [0265]
touches: [src/render_context.h, scripts/null_compare.py, src/render_suspension.cpp, src/render_suspension.h, src/contracts.h, src/contracts.cpp, test/contracts_test.cpp, test/internal_value_test.cpp, src/template_dict.cpp, src/template_dict.h, scripts/null_compare.query, fuzz/fuzz_common.h, fuzz/fuzz_targets.cpp, fuzz/untrusted_targets.cpp, fuzz/alloc_failure_target.cpp]
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

**Done (branch claude/project-thread-7kumor).** src/contracts.h gains `JINJA2CPP_FULL_CONTRACT_CHECKS`
(1 at FULL, else 0) for state kept only by invariant checks, and `JINJA2CPP_ASSERT_NOEXCEPT` for
checks in destructors and noexcept functions: it cannot throw, so at FULL a violation prints the
condition and aborts, as `assert` did. RenderContext's `m_noCachedLookups`, HomeCall's
`m_debugScopes`/`m_debugExceptions`, `Direct`'s caller members and `NoCachedLookups` are guarded by
the macro; the bare `assert`s of src/render_context.h and src/render_suspension.cpp (the
`RecordMarks`/`CheckIdle` thread-local marks too) became `JINJA2CPP_ASSERT_NOEXCEPT`. The fuzz
targets abort on an `UnexpectedException` error whose text carries " violated: "
(`AbortOnContractViolation` in fuzz/fuzz_common.h). Not covered: the property renders of
fuzz/prompt_escape_case.h, which test/filters_test.cpp shares and which report only "not rendered";
the same target's second, prompt-mode render is covered.

**Found by it.** Once reported, the alloc_failure replay caught a pre-existing invariant failure that
template text reaches: a `**` spread of a dict whose key starts with byte 0xFF, the mark of a dict key
token (`'%(a)s'|format(**{'\xFFa': 5})`, `dict(**...)`, `namespace(**...)`). `TemplateDict(InternalDict&&)`
stored the name raw, so at ON a lookup of it missed and a name spelled like another key's token could
read as that key. The constructor now escapes such names, keeping their order (regression
fuzz/regressions/kwargs-name-with-dict-key-mark.j2, TemplateDictTest). Python Jinja2 cannot hold the
byte (UTF-8 source), so there is no parity case.
