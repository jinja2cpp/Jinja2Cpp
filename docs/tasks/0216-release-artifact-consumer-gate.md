---
status: open
priority: high
area: release
touches: [scripts/check_release_artifact.py, test/release/]
shares: [.github/workflows/linux-build.yml, .github/workflows/windows-build.yml, .github/workflows/macos-build.yml]
---
# Validate the release source artifact from a clean downstream consumer

**Problem.** The installed-package tests (0068) validate a configured checkout's
exports. They do not prove that a published source archive contains everything
needed, works without Git metadata or local caches, or supports consumers who build
with tests disabled. Conan dependency-mode CI is also different from testing an
actual published Jinja2Cpp package (0006).

**Proposal.** Add a reusable candidate-artifact check that:

1. Builds the candidate's source archive in a fresh directory without .git or the
   maintainer's build outputs, then configures with JINJA2CPP_BUILD_TESTS=OFF.
2. Installs to a scratch prefix and uses a separate consumer through find_package
   and, where supported, pkg-config, reusing test/package rather than duplicating it.
3. Checks version, public headers, exported definitions, static/shared linkage and
   required notices (0217). Exercise each advertised dependency mode and JSON binding
   with a documented representative matrix, including external/Conan dependencies.
4. Records the exact archive hash, toolchain, dependency versions and results for RC
   signoff (0212). Distinguish expected dependency downloads from accidental reliance
   on a checkout, network generation step, or prepopulated cache.

Reuse existing platform CI and package checks. Coordinate Conan recipe publication
and actual package-consumer checks through 0006. Do not require every matrix product
or a fully offline build with missing dependencies; document network prerequisites
and test offline consumption only when the required dependencies are preprovided.

**Done when.** The same check can validate the final distributed source artifact,
runs in release validation on the supported platforms, and builds/runs its consumer
with tests disabled in the library. A missing public header, wrong package version,
missing notice, or undeclared checkout dependency makes the check fail.
