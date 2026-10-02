---
status: open
priority: medium
area: robustness
depends: [0054, 0055, 0008]
touches: [src/, include/jinja2cpp/]
---
# clang-tidy bug-class findings that only C++23 shows

**Problem.** Two bug-finding checks need the standard library types that nonstd maps to
only from C++17: at C++23 `bugprone-unchecked-optional-access` reports 14 accesses to an
optional that the analysis cannot prove engaged, and
`bugprone-suspicious-stringview-data-usage` one `string_view::data()` passed where a
terminated string is expected. Most optional hits sit after a `ThrowRuntimeError` call,
which 0055 marks `[[noreturn]]`; the rest need a look. The modernize checks the floor
unlocks are in 0057 (mechanical), 0059 (`[[nodiscard]]`) and 0062 (`use-constraints`).

**Proposal.** After 0008 raises the floor and 0055 lands, rerun the two checks, fix each
remaining hit by hand or mark it `NOLINT(<check>)` with the reason, with a unit test for
any behaviour change.

**Done when** both checks report nothing on `src/` and `include/` at C++23 and sit in
`WarningsAsErrors`.
