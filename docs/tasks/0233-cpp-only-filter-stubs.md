---
status: done
priority: medium
area: api
issues: ["https://github.com/jinja2cpp/Jinja2Cpp/issues/41"]
touches: [src/filter_factories.cpp, src/string_converter_filter.cpp, src/serialize_filters.cpp, src/filters.h, test/filters_test.cpp, docs/cpp-only-filters.md]
shares: [docs/parity.md]
---
# C++-only filters `camelize`, `underscorize`, `escapecpp`, `toxml`, `toyaml` do nothing

**Problem.** The five names are registered in `src/filter_factories.cpp` (lines 62, 71,
106, 107, 116), but no code handles their modes: `Serialize` only implements `JsonMode`
(`src/serialize_filters.cpp`), and the `StringConverter` switch has no case for
`CamelMode`, `UnderscoreMode` or `EscapeCppMode`. `{{ "a_b c"|camelize }}|{{ {"a":1}|toyaml }}`
renders `|` with no error (found by the 0221 GitHub scout, 2026-10-18, issue #41 of the
"Release 1.3" milestone). `docs/parity.md` advertises them as deliberate extensions.
Since 0193 an unknown filter is a Load error, so a silently empty filter is now the
only way a filter name can fail quietly.

**Proposal.** Decide per filter, before the 2.0 API freeze (docs/release-2.0.md, D16):
implement it with a defined output (camelize/underscorize: identifier case conversion;
escapecpp: C++ string-literal escaping; toyaml/toxml: a documented subset), or remove it
from the factory so that it is an unknown-filter Load error, as in Jinja2. `applymacro`
and the `startsWith` test work and stay.

**Done when.** Each name either renders its documented output with rows in
`test/filters_test.cpp`, or fails Load with `UnknownFilter`; `docs/parity.md` and the 2.0
migration notes (0077) say which.

**Outcome.** Ruslan chose D16 a (2026-10-18). `camelize(upper_first=true)`, `underscorize`,
`escapecpp` and `toyaml` render the output documented in `docs/cpp-only-filters.md`, with
rows in `test/filters_test.cpp`; `toxml` is removed and fails Load with `UnknownFilter`
(corpus case `filters.unknown_filter_toxml`). Release note: `docs/api-2.0.md` 5.7, for
0077's `MIGRATION.md`.
