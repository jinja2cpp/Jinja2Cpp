---
status: open
priority: medium
area: build
depends: [0271, 0273]
touches: [include/jinja2cpp/big_int/boost_cpp_int.h, src/big_int_builtin.cpp, CMakeLists.txt, thirdparty/, cmake/public/, jinja2cpp.pc.in, sbom/components.json, .github/workflows/, test/parity/divergences/operators.txt, test/parity/divergences/filters.txt, docs/parity.md, docs/dependencies.md, README.md]
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
