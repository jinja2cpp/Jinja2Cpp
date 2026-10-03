---
status: done
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
as `RM_MetaBegin` in `src/template_parser.h`. Ruslan chose to adopt it (2026-10-03).

**Proposal.** Extend the ignore patterns until only real deviations remain, rename those
in `src/` (renames are fix-its), switch the check on for `src/`. `include/` follows the
2.0 API decision (0056).

**Done when** the check is on for `src/` and reports nothing.

## Outcome

On after 0064. Function-local statics are locals (`s_` only for class and file-scope
statics: `ClassMemberPrefix`), class constants are `CamelCase` (`MaxReprDepth`), value
template parameters `camelBack` or a single capital (`N`). Ignored as deliberate:
STL-shaped methods (`try_emplace`, `max_size`), `boost::iterator_facade` hooks, fmt's
`parse`/`format`, the public `is*`/`as*`/`get*` accessors of `Value`, `swap`, and the
`RM_` match kinds. `sv_to_string` is gone: C++17 constructs `std::basic_string` from a
`string_view` directly (Ruslan, 2026-10-03). Renamed: `modify_time`, `num_keys`, the `ProtectedValue` lambdas, `make_result`,
`m_invalidIndex` (a class constant, now `InvalidIndex`) and `CanModify`. `include/`'s one
hit (`UserCallable::m_gen`) is NOLINT until 0056's names land; `test/` has the check off.
