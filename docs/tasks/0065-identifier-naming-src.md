---
status: open
priority: low
area: style
depends: [0057, 0056]
touches: [src/, .clang-tidy]
---
# readability-identifier-naming for src/

**Problem.** The conventions are consistent but unenforced: `CamelCase` types and
functions, `m_`/`s_` members and statics, `camelBack` locals. On two large TUs the check
(options already in `.clang-tidy`) reports ~190 names, nearly all deliberate: STL-shaped
container methods (`ordered_map::insert`, `begin`), `*_t` aliases, enum constants such
as `RM_MetaBegin` in `src/template_parser.h`. Ruslan has not decided yet whether to adopt it.

**Proposal.** Extend the ignore patterns until only real deviations remain, rename those
in `src/` (renames are fix-its), switch the check on for `src/`. `include/` follows the
2.0 API decision (0056).

**Done when** the check is on for `src/` and reports nothing.
