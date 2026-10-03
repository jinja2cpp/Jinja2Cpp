---
status: done
priority: medium
area: robustness
depends: [0054, 0055, 0070]
touches: [src/, include/jinja2cpp/]
---
# clang-tidy bug-class findings that only C++17 shows

**Problem.** Two bug-finding checks need the standard library types that nonstd maps to
only from C++17: there `bugprone-unchecked-optional-access` reports 14 accesses to an
optional that the analysis cannot prove engaged, and
`bugprone-suspicious-stringview-data-usage` one `string_view::data()` passed where a
terminated string is expected. Most optional hits sit after a `ThrowRuntimeError` call,
which 0055 marks `[[noreturn]]`; the rest need a look. The modernize checks the floor
unlocks are in 0057 (mechanical) and 0059 (`[[nodiscard]]`).

**Proposal.** After 0070 raises the floor and 0055 lands, rerun the two checks, fix each
remaining hit by hand or mark it `NOLINT(<check>)` with the reason, with a unit test for
any behaviour change.

**Done when** both checks report nothing on `src/` and `include/` at C++17 and sit in
`WarningsAsErrors`.

## Outcome

Done with 0055 in one PR. `[[noreturn]]` cleared the `filesizeformat` hit; the other 13
were guarded (`ListAdapter::Iterator`, `Enumerator::Transfer`, the indexed accessors'
`GetItemByIndex`/`IsEqual`, the loop's `GetLength`, `tojson`'s `NewLine`) or use
`value_or`. The `string_view::data()` hit is a sized constructor, marked NOLINT. Both
checks are part of `bugprone-*`, which enters `WarningsAsErrors` with 0063 (`test/` has
10 optional-access hits).
