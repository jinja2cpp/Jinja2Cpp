---
status: open
priority: medium
area: style
depends: [0054]
touches: [include/jinja2cpp/config.h, src/]
shares: [include/jinja2cpp/]
---
# [[nodiscard]] at every standard through JINJA2CPP_NODISCARD

**Problem.** `modernize-use-nodiscard` is silent at C++14, the default standard, so 354
functions whose result must not be dropped (83 in public headers) carry no marker. With
`ReplacementString: JINJA2CPP_NODISCARD` (already in `.clang-tidy`) the check runs at
C++14 and inserts the macro.

**Proposal.** Define `JINJA2CPP_NODISCARD` in `include/jinja2cpp/config.h` as
`[[nodiscard]]` when `__cplusplus` (or `_MSVC_LANG` on MSVC) is at least 201703L,
otherwise empty; apply the check's fixes to `src/` first, then to the public headers that
0056 keeps unchanged. Build at C++14, 17 and 20 to catch results the library itself drops.
Note in the release notes that C++17 users get warnings where they ignore a result.

**Done when** the check reports nothing at C++14 and the C++17/20 CI jobs build
warning-free.
