---
status: open
priority: medium
area: build
depends: [0230]
touches: [include/jinja2cpp/binding/rapid_json.h, src/binding/rapid_json_parser.h, test/binding/rapid_json_binding_test.cpp, cmake/patches/rapid_json_improvements.patch, thirdparty/internal_deps.cmake, thirdparty/thirdparty-conan-build.cmake, thirdparty/thirdparty-external.cmake]
shares: [CMakeLists.txt, .github/workflows/linux-build.yml, .github/workflows/clang-tidy.yml, conanfile.txt, sbom/components.json, docs/dependencies.md, MIGRATION.md]
---
# Retire the built-in RapidJSON backend

**Problem.** `JINJA2CPP_WITH_JSON_BINDINGS=rapid` is one of three built-in JSON backends.
It builds a fetched RapidJSON commit (24b5e7a8) with a local patch
(`cmake/patches/rapid_json_improvements.patch`), and it is the one configuration the cloud
sessions cannot build, because the download is a GitHub archive. Conan mode links a target
name only the internal mode defines (0006). The external PR #197 exists only to work around
that. The backend's only core use is parsing `{% meta %}` blocks, and 0230 replaces that
with an internal reader (decision of 2026-10-24). After that, the patched fork is carried
for one opt-in reflection header, while nlohmann (and simdjson, 0302) cover the same use.

**Decision (Ruslan, 2026-10-24).** Retire it in 2.0.

**Proposal.** Once 0230's internal meta reader lands:
- drop `rapid` from `JINJA2CPP_WITH_JSON_SUPPORTED_BINDINGS`;
- remove `include/jinja2cpp/binding/rapid_json.h`, `src/binding/rapid_json_parser.h`, the
  binding test, the patch, the RapidJSON fetch and find logic in all three dependency modes,
  the Conan requirement and the CI rows that select `rapid`;
- update `sbom/components.json` and docs/dependencies.md;
- add a MIGRATION.md entry (0077) for 1.x users: reflect a RapidJSON document by
  converting it to `jinja2::Value` themselves, or switch to the nlohmann binding;
- close #197 in the pre-freeze tracker sweep (G11, 0234) with a pointer to this task.

**Done when.** No file in the tree configures, fetches or tests RapidJSON; configuring with
`-DJINJA2CPP_WITH_JSON_BINDINGS=rapid` fails with a message that names the migration entry;
the SBOM check passes.
