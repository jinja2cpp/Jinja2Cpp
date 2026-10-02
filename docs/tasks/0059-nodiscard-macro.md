---
status: done
priority: medium
area: style
depends: [0054, 0070]
touches: [src/, include/jinja2cpp/, test/, .clang-tidy, scripts/clang_tidy_fix.py, docs/api-2.0.md]
shares: [include/jinja2cpp/]
---
# [[nodiscard]] on results that must not be dropped

**Problem.** `modernize-use-nodiscard` reports 354 functions whose result must not be
dropped (83 in public headers) and that carry no marker. The check is silent at C++14;
with the 2.0 floor at C++17 (0070) it runs in the normal tidy job.

**Proposal.** After 0070 lands, apply the check's fixes with `scripts/clang_tidy_fix.py`
(0054), `src/` first, then the public headers that 0056 keeps, and fix any result the
library itself drops. The attribute is written directly: the `JINJA2CPP_NODISCARD` macro
first chosen for C++14 builds (2026-10-02) is not needed once C++14 is dropped. Note in the 2.0 release
notes that callers get warnings where they ignore a result.

**Done when** the check reports nothing at C++17, sits in `WarningsAsErrors`, and the
full CI matrix builds warning-free.

**Done (2026-10-02)** in [#340](https://github.com/jinja2cpp/Jinja2Cpp/pull/340): 408 sites (271 `src/`, 113 public headers, 24 `test/`),
no result the library or the tests drop. The check marks only `const` member functions;
`TemplateEnv::LoadTemplate`/`FromString` were marked by hand, and `Load`/`Render`
(`Result<void>`) are left to 0080. Release note: docs/api-2.0.md section 5.5. The fix
script now drops repeated identical fixes: a template member is diagnosed once per
instantiation, and clang-apply-replacements inserted the attribute once for each
(up to 300 times on one line).
