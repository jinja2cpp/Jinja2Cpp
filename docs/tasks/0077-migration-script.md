---
status: open
priority: high
area: release
depends: [0072, 0074, 0076]
touches: [scripts/jinja2cpp_migrate.py, MIGRATION.md, README.md]
---
# 2.0 migration script and notes

**Problem.** 2.0 renames the `Value` accessors, `Settings` fields and a few types. Users need
to move without reading the whole diff (`docs/api-2.0.md`, section 5).

**Proposal.** `scripts/jinja2cpp_migrate.py`: reads a build log (GCC, Clang, MSVC formats),
keeps deprecation warnings whose message starts with `jinja2cpp-2:`, and replaces the old
identifier at each location with the name in the message (Clang points at the identifier,
GCC at the call's `(`); a small table handles the hard breaks (`Settings` fields,
`useLineStatements`, removed headers). `MIGRATION.md` lists every change; the docs site
(jinja2cpp.github.io) follows.

The table also covers code that leans on expected-lite beyond the `std::expected` subset
(raised by Ruslan 2026-10-02, 0070): `nonstd::expected<T, ErrorInfo>` →
`jinja2::Result<T>`, `r.get_unexpected()` → `nonstd::make_unexpected(r.error())`. These are
not 2.0 breaks (expected-lite stays, pinned), but `MIGRATION.md` should name the subset so a
later switch of `Result<T>` to `std::expected` (a C++23 floor) is a recompile for users.
Our own tests are the first client: `test/undefined_test.cpp` spells
`nonstd::make_unexpected`.

Template behaviour changes belong in `MIGRATION.md` too, for example the `format` filter:
since 0048 it is printf-style only and a string with `{}` placeholders raises "not all
arguments converted"; such templates move to the `'{}'.format(x)` string method.

`MIGRATION.md` also lists the template syntax 2.0 stops accepting, all of which Jinja2
rejects too (0094, Ruslan decided 2026-10-12): `{% macro m %}` and `{% call m %}` without
parentheses (write `m()`), `{'a' = 1}` dict literals (write `{'a': 1}`), and `+}}` (a `+`
before the end of an expression is the operator; `+%}` on statements is unchanged).
A positional argument after a keyword one is a syntax error too (0294): `f(a=1, 2)`, and the
old `applymacro(macro='m', x)` order, become `f(2, a=1)` and `applymacro(x, macro='m')`.

`Value(float)` stores a `float` since 0039: `IsDouble()`, `AsDouble()` and
`GetIf<double>()` no longer see it, so host code that read a C++ `float` back as a `double`
tests `IsFloat()` too (no compiler warning points at it; `MIGRATION.md` names it).

**Done when.** The script migrates a copy of our own 1.x-style tests to warning-free code,
in CI.

**Release gate (0221).** Cover behavioral changes as well as compiler renames:
None/undefined, value printing, whitespace, format syntax and macro scoping all
need before/after examples. Keep current requirements consistent across README
and the versioned website: C++17, CMake >= 3.23, current dependencies and supported
compiler versions. Replace the README's find_package(jinja2cpp 1 REQUIRED) and
historical Conan snippets with commands actually verified against the 2.0 candidate.
Coordinate the supported repository/example inventory through 0227.

The external-consumer upgrade trial is tracked separately as 0222. Its failures
feed this task's migration notes/tool; passing the project's own warning-rewrite
tests alone does not establish a successful downstream upgrade.
