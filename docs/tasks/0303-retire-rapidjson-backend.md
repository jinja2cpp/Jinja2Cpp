---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/TBD
priority: medium
area: build
depends: [0230, 0311]
touches: [include/jinja2cpp/binding/rapid_json.h, test/binding/rapid_json_binding_test.cpp, cmake/patches/rapid_json_improvements.patch, cmake/patches/0001-fix-custom_command-error.patch, thirdparty/internal_deps.cmake, thirdparty/thirdparty-conan-build.cmake, thirdparty/thirdparty-external.cmake]
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
- add a MIGRATION.md entry (0077) for 1.x users with a ready
  `jinja2::JsonTraits<rapidjson::Value>` (and `<rapidjson::Document>`) on the 0311 contract,
  about 40 lines (`/mnt/project-files/0311/plan.md`, section 8), so `Reflect` and `ToValue`
  keep working with their own RapidJSON; or switch to the nlohmann binding;
- close #197 in the pre-freeze tracker sweep (G11, 0234) with a pointer to this task.

**Order (0311 plan, Ruslan 2026-10-25).** Delete, do not rewrite on the contract. Run
right after 0230 D7a and before 0311's S5, so RapidJSON never becomes a component; 0311's
S1/S2 leave `rapid_json.h` alone (it keeps using `ReflectedDataHolder`, which stays).

**Done when.** No file in the tree configures, fetches or tests RapidJSON; configuring with
`-DJINJA2CPP_WITH_JSON_BINDINGS=rapid` fails with a message that names the migration entry;
the SBOM check passes. The MIGRATION.md
snippet was compiled once against unpatched RapidJSON (the pinned 24b5e7a8) in a scratch
build, recorded in the PR description; it is not built in CI.
