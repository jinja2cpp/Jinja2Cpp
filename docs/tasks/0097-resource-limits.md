---
status: open
priority: medium
area: robustness
depends: [0003]
touches: [src/global_functions.cpp, src/expression_evaluator.cpp]
shares: [include/jinja2cpp/template_env.h, src/filters.cpp]
---
# Templates can ask for unbounded time and memory

**Problem.** Found while fuzzing (0003): nothing bounds the work a template asks for.
`{{ range(100000000)|list|length }}` builds a list of 10^8 values and `{{ ('x' *
100000000)|length }}` a 100 MB string; both ran for over a minute in a Debug build before
being killed. String repetition costs a couple of microseconds per character there, where
Python builds the string in one allocation. `{% for i in range(10**9) %}` runs for
hours. libFuzzer reports such inputs as timeouts or out-of-memory, which the fuzz
workflow tolerates, so they hide real slowdowns too.

Python Jinja2's default `Environment` has no limits either, but its
`SandboxedEnvironment` raises `OverflowError` for a `range` longer than `MAX_RANGE`
(100000) and checks operator use through `intercept_unop`/`call_binop`. Embedders who
render templates written by users need a comparable switch; Jinja2C++ has none.

**Hard cap (done).** A size-reporting list such as `range(2**63 - 1, -2**63 + 1, -922375807)`
made `|list` reserve its whole length at once: 1.44 TB, which ASan aborts on as
allocation-size-too-big (found by the fuzz job on PR #430). Building a list from a size hint,
`'s' * n`, `center` and `indent` now fail the render once the result would exceed
`MaxSequenceSize` (2^31 items or characters, `src/internal_value.h`), with an
`UnexpectedException` whose message says the sequence is too long. Python raises
`MemoryError` for `|list`, `'s' * n`, `center` and `indent`; it keeps `range(2**40)[1:]`
lazy (Jinja2C++ builds the list, so this still diverges) and rejects `range + list` and
`range * n` with `TypeError`. `reverse` and `tojson(indent)` got the same check, and
`random` over a range longer than 2^31 no longer overflows an `int`. The cap is fixed and far above what a template should
need (2^31 list items take about 150 GB); it is not the opt-in limit proposed below, which
would be much lower and configurable. `fuzz/regressions/huge-range-list.j2` replays the finding.

**Proposal.**
- Make `'x' * n` and `[x] * n` reserve once and fill, instead of appending in a loop.
- Add opt-in limits to `Settings`: maximum `range` length, maximum string or list size a
  single operation may produce, and optionally a step budget (expression nodes evaluated
  per render) checked where the stack checks of 0003 already are. Exceeding one fails
  the render with a new `ErrorCode` (Python's `OverflowError`/`SecurityError`).
- Use the limits in the fuzz targets, so that timeouts and out-of-memory inputs become
  findings again, and fail the fuzz job on them.

**Done when.** The three templates above fail fast with the new error when the limits
are set and render as before when they are not; `'x' * 10**8` takes well under a second
in a Release build; the fuzz workflow no longer ignores timeouts and OOMs.
