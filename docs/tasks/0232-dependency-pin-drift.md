---
status: open
priority: low
area: build
touches: [conanfile.txt, thirdparty/CMakeLists.txt, .github/workflows/, sbom/components.json]
---
# Dependency pins disagree across modes and some are mutable

**Problem.** The 0187 audit (docs/dependencies.md) found the same dependency pinned
differently per deps mode, and several pins that can move under the project:
- conanfile.txt: `expected-lite/0.9.0` and `boost/1.91.0` while internal mode uses
  0.10.0 and 1.92.0; `rapidjson/cci.20250205` is an unpatched snapshot while internal mode
  applies `cmake/patches/*.patch` to commit 24b5e7a8.
- googletest (`v1.18.0`) and google benchmark (`v1.9.5`) are fetched by git tag, with no
  commit or hash.
- All seven GitHub Actions are referenced by tag (`actions/checkout@v4`, ...), including
  two third-party ones (`hendrikmuhs/ccache-action@v1.2`, `ilammy/msvc-dev-cmd@v1`) that
  run with the workflow token. PyYAML is installed unpinned in the clang-tidy job.

**Proposal.** Align the Conan pins with internal mode where ConanCenter has the version
(check with the sparse-clone method, memory `github-archive-hash-offline`) and say in
conanfile.txt why any difference stays; decide whether conan-build needs the RapidJSON
patches. Pin googletest and benchmark by commit (or URL + `URL_HASH`). Pin third-party
actions by commit SHA with the tag in a comment. Update sbom/components.json in the same
PR; `scripts/sbom.py --check` verifies the pins it records.

**Done when.** Each dependency has one version across modes or a recorded reason, tests
and actions are pinned immutably, and the SBOM check passes.
