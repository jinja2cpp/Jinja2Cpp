---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/630
priority: medium
area: build
depends: [0271, 0273]
touches: [include/jinja2cpp/big_int/boost_cpp_int.h, src/big_int_builtin.cpp, src/big_int.cpp, CMakeLists.txt, thirdparty/, cmake/public/, jinja2cpp.pc.in, sbom/components.json, .github/workflows/, test/parity/divergences/operators.txt, test/parity/divergences/filters.txt, docs/parity.md, docs/dependencies.md, docs/api-2.0.md, README.md, test/big_int_test.cpp, test/errors_test.cpp, test/parity/parity_test.cpp, test/package/]
---
# Boost.Multiprecision backend and `JINJA2CPP_WITH_BIGINT` (0178 phase 4)

**Problem.** 0271 provides the socket; users need a ready adapter and a build that turns it on.

**Proposal** (plan sections 3.5, 8). A public header-only adapter over `cpp_int`
(`jinja2cpp/big_int/boost_cpp_int.h`), also the documented sample. `-DJINJA2CPP_WITH_BIGINT=none|boost`
selects the built-in default; Multiprecision is fetched standalone (`BOOST_MP_STANDALONE`) so the
core stays Boost-free (0230). Installed config exports `jinja2cpp_BIGINT`, pkg-config a variable;
SBOM lists the optional component. CI: boost on linux-build.yml rows 57, 62, 64, 67 (pairwise kept),
one sanitizer, one Windows, one macOS row and the fuzz build. Coordinate `thirdparty/`/`sbom/` with
0230.

**Done when.** The four arithmetic cases of 0178 are `@nobigint` lines that match in boost rows,
`scripts/sbom.py --check` passes, the package consumer test checks the backend name, and the perf
gate holds on the boost build.

Plan: [docs/design/0178-big-integer-plan.md](../design/0178-big-integer-plan.md).

**As built.** `BoostCppIntBackend()` in `include/jinja2cpp/big_int/boost_cpp_int.h` (header-only,
standalone Multiprecision: only its own and Boost.Config's headers, taken from the deps mode's Boost
through the `jinja2cpp_boost_multiprecision` interface target). `src/big_int_builtin.cpp` holds
`GetBuiltinBigIntBackend()` and is the one library file that may include it. The tests check the
adapter in every build (`BigIntBoost.*`) and the built-in default in boost builds
(`BigIntBuiltin.MatchesTheBuildOption`); the parity suite records `# features: bigint` when the
default env has a backend, and the three arithmetic lines of operators.txt are `rejects@nobigint`
(`filters.round_int_overflow` already matched after 0270). CI: `bigint: boost` on five
linux-build rows (one with the package check), the gcc ASan row, the shared MSVC row, one macOS
row and the `render` fuzz target.

**Left for 0006 (packaging).** The Conan recipe gets an option `bigint=[none, boost]` and the
vcpkg port a feature `bigint-boost`, both mapping to `-DJINJA2CPP_WITH_BIGINT` (plan section 8).

**Closed (2026-10-24).** Merged in #630 (train 31, #633).
