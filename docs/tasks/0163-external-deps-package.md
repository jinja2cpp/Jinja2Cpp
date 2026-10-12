---
status: open
priority: low
area: build
depends: [0068]
touches: [thirdparty/thirdparty-external.cmake, thirdparty/thirdparty-external-boost.cmake, cmake/public/jinja2cpp-config-deps-external.cmake.in, cmake/public/jinja2cpp-config-deps-external-boost.cmake.in, cmake/public/jinja2cpp-config-deps-conan.cmake.in, .github/workflows/linux-build.yml]
---
# The external deps modes do not configure, and no CI job builds them

**Finding (2026-10-12, while fixing 0068).** `JINJA2CPP_DEPS_MODE=external` and
`external-boost` fail at generate time on master: `thirdparty-external.cmake` wraps the
found packages in `imported_target_alias` targets and then calls `install(TARGETS ...)` on
`expected-lite`, `fmt-header-only` and `RapidJson`, which are imported targets in that
mode (*checked by the 0068 architect review on 587e5ed*). No workflow configures either
mode, so the breakage went unnoticed. `cmake/public/jinja2cpp-config-deps-conan.cmake.in`
belongs to a `conan` mode that has no `thirdparty-conan.cmake` and is dead.

**Proposal.**
- Find the dependencies by their own package configs (`find_package(expected-lite CONFIG)`,
  `find_package(fmt CONFIG)`, Boost by components) and link their namespaced targets; drop
  the alias wrappers and the `install(TARGETS)` of imported targets.
- Keep the deps files 0068 left (`find_dependency` of the same packages) and remove the dead
  conan deps file (needs the owner's OK to delete).
- Add one CI row, or a step on an existing row, that installs Boost, fmt and expected-lite
  from the distribution or a prefix, configures `external`, and runs
  `test/package/check_package.cmake` against the install.

**Done when.** Both modes configure, build and pass `check_package.cmake` in CI.
