---
status: open
priority: medium
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

**Done when.** The script migrates a copy of our own 1.x-style tests to warning-free code,
in CI.
