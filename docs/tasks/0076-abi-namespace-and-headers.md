---
status: done
priority: high
area: release
depends: [0072, 0073, 0074, 0075, 0043]
touches: [include/, src/, CMakeLists.txt, cmake/public/, test/package/, test/fwd_test.cpp, test/value_ptr_test.cpp, test/abi_symbols.cmake, test/template_api_test.cpp, README.md, sbom/, abi/, scripts/abi_namespace.py, scripts/abi_report.sh, .git-blame-ignore-revs, .github/workflows/format-check.yml, .github/workflows/linux-build.yml]
---
# 2.0: inline ABI namespace, header layout, version

**Problem.** Nothing stops 1.x headers from linking against a 2.x library or the reverse;
users include five headers; macros and vendored code leak (`docs/api-2.0.md`, 3.8 and 5.3).

**Proposal.** `namespace jinja2 { inline namespace v2 { ... } }` in every public header;
`jinja2cpp/fwd.h` and `jinja2cpp/jinja2cpp.h`; vendored `polymorphic` into
`jinja2::detail`; `JINJA2CPP_DECLSPEC`; remove `error_handler.h`; `project(VERSION 2.0.0)`,
SOVERSION 2 and the SOVERSION policy of section 5.4 in the README. Touches every header's
namespace lines, so it runs alone after the four API tasks.

**Done when.** `nm` shows `jinja2::v2::` symbols, a test forward-declares through `fwd.h`,
and the library reports 2.0.0 / SOVERSION 2.

**Release gate (0221).** SOVERSION 2 and project version 2.0.0 are already set;
they do not replace the remaining inline namespace/header work. Add a negative
link check showing that a consumer built against 1.x headers cannot silently link
to the 2.x library (the inline namespace's job). SOVERSION tracks the ABI, not the
major (docs/api-2.0.md 5.4), so it guards ABI breaks within 2.x. Exercise both static
and shared artifacts where supported. Publish the agreed source-compatibility and
SOVERSION policy, including Settings layout changes, rather than implying every
future 2.x build is binary-compatible. Settle the API-shaping decisions listed in
0221 before the final namespace/header freeze.

**Progress.** Plan approved 2026-10-18 (decisions in the plan thread; the release-2.0
0076 plan). PR-1 (headers): `fwd.h` and `jinja2cpp.h`, vendored value_types moved to
`jinja2::detail::xyz` with `JINJA2CPP_XYZ_` macros, `JINJA2_DECLSPEC` and
`error_handler.h` removed, hidden visibility on GCC/Clang. PR-2 (ABI tag): the
script-generated `inline namespace v2` commit, the nm/link-probe/SONAME gates, the abidiff
job and the README policy. Follow-ups: 0259 (robin_hood/lexertk namespaces), 0260
(`<iosfwd>`), 0261 (Windows package link check).

**Done.** PR-1 #580. PR-2: `scripts/abi_namespace.py` wrote `inline namespace v2` into
every `namespace jinja2` of include/ and src/ (one commit, in .git-blame-ignore-revs) and
checks it in the format-check job; CTest `jinja2cpp_abi_namespace` (test/abi_symbols.cmake)
fails on a jinja2 symbol outside `v2` or a global `xyz`; test/package links
`abi_probe_v2` and expects `abi_probe_v1` (1.x mangling) not to link, in the static and
shared package rows; check_package.cmake checks the SONAME; the version asserts require
major 2; README "Compatibility and versioning". The abidiff job runs on one shared
RelWithDebInfo row per standard and reports against the last master build; committing the
baselines at the RC is 0285. Deliberate divergence from the plan: `jinja2::v2::detail` is
not suppressed in abidiff (exported detail functions are called from inline headers), and
the C++20 abi row is gcc-12, not gcc-14, so that the Release rows keep every pair; the C++17
abi row stays clang-18 (the plan preferred GCC, but the only C++17 shared row is clang-18).
The gate counts any change left after `--no-added-syms` as a break, since abidiff reports
a changed layout (a new `Settings` field) with bit 4, not bit 8. The four Unicode/HTML
generators in scripts/ emit the `v2` form, so their `--check` stays green.
