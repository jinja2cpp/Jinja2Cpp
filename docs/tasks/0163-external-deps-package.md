---
status: done
priority: low
area: build
depends: [0068]
touches: [CMakeLists.txt, thirdparty/thirdparty-external.cmake, thirdparty/thirdparty-external-boost.cmake, thirdparty/external_boost_deps.cmake, test/package/check_package.cmake, cmake/public/jinja2cpp-config-deps-external.cmake.in, cmake/public/jinja2cpp-config-deps-external-boost.cmake.in, .github/workflows/linux-build.yml]
---
# The external deps modes do not configure, and no CI job builds them

**Finding (2026-10-12, while fixing 0068).** `JINJA2CPP_DEPS_MODE=external` and
`external-boost` fail at generate time on master: `thirdparty-external.cmake` wraps the
found packages in `imported_target_alias` targets and then calls `install(TARGETS ...)` on
`expected-lite`, `fmt-header-only` and `RapidJson`, which are imported targets in that
mode (*checked by the 0068 architect review on 587e5ed*). No workflow configures either
mode, so the breakage went unnoticed.

**Proposal.**
- Find the dependencies by their own package configs (`find_package(expected-lite CONFIG)`,
  `find_package(fmt CONFIG)`, Boost by components) and link their namespaced targets; drop
  the alias wrappers and the `install(TARGETS)` of imported targets.
- Keep the deps files 0068 left (`find_dependency` of the same packages). The empty deps
  file of the old `conan` mode was deleted with Ruslan's OK (2026-10-12).
- Add one CI row, or a step on an existing row, that installs Boost, fmt and expected-lite
  from the distribution or a prefix, configures `external`, and runs
  `test/package/check_package.cmake` against the install.

**Done when.** Both modes configure, build and pass `check_package.cmake` in CI.

**Resolution (2026-10-12, PR TBD).**
- `external` finds expected-lite, fmt, nlohmann_json (tests or nlohmann bindings) and
  RapidJSON (rapid bindings) by their package configs and links their namespaced targets.
  `external-boost` keeps the fetched fmt and expected-lite.
- `thirdparty/external_boost_deps.cmake` asks Boost for per-library targets and falls back to
  `Boost::headers` plus the compiled libraries when they are missing: a b2-built Boost such
  as Ubuntu's 1.83 reports the header-only components found but defines no targets for them.
  The components it used go into the installed deps file, so the consumer finds the same.
- Both modes link `JINJA2_PRIVATE_LIBS_INT`/`JINJA2_PUBLIC_LIBS_INT` like conan-build; the
  `imported_target_alias` wrappers and the `install(TARGETS)` of imported targets are gone.
  The `external-boost` deps file now also finds the fmt installed next to the library,
  which a static build's exported link interface names.
- CI job `external-deps` (linux-build.yml): `external` on GCC 14 Release static and
  `external-boost` on Clang 20 Debug shared, both against Ubuntu's Boost 1.83, with the
  tests and `check_package.cmake` (which gained `DEPS_PREFIX` for dependencies outside the
  default search path).

Not covered: `internal_deps.cmake` patches RapidJSON before building with it, so
`external` with rapid bindings and an unpatched system copy is untested and may not compile.
