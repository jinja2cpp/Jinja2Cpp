---
status: done
priority: low
area: build
touches: [conanfile.txt, thirdparty/CMakeLists.txt, .github/workflows/, .github/actions/, sbom/components.json, scripts/sbom.py, .claude/hooks/prefetch_deps.py]
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

**Progress (0230 step D7c).** Done. Every `uses:` in the workflows and the local composite
action is a 40-character commit SHA with a `# vX.Y.Z` comment (decision R7: GitHub-owned
actions included; the newest release of the major that was referenced). PyYAML is
`pyyaml==6.0.3`. googletest v1.18.0 and benchmark v1.9.5 are `URL` archives with
`URL_HASH`, written as literals; the hashes come from `git archive | gzip -n -6`, a method
checked first against fmt 12.2.0's existing pin, and the prefetch hook already clones them
because it scans `thirdparty/CMakeLists.txt`. The Conan pins that differ from internal mode
stay, each with its reason in `conanfile.txt` and a `conan_why` in components.json
(expected-lite 0.9.0 and Boost 1.91.0 are ConanCenter's newest; RapidJSON cci.20250205 is
the same commit and the local patches only touch CMake). `scripts/sbom.py --check` now fails
a conan version that differs without `conan_why`, and compares every workflow `uses:` with
the `sha` and release tag of its `gha-*` component. Nothing refreshes the SHAs yet: a
Dependabot `github-actions` entry is a follow-up.
