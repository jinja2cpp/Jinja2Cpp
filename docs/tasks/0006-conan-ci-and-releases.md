---
status: in-progress
priority: high
area: release
---
# Conan package in CI and resumed releases

**Problem.** The `conan-build` dependency mode and `conanfile.txt` exist, but nothing
builds them, so they drift: the last release predates the 2024 dependency bump, the
conan pins differ from `thirdparty/internal_deps.cmake` (e.g. fmt 10.1.1 vs 12.1.0,
variant-lite 2.0.0 vs 3.0.0, Boost 1.85 vs 1.90), and `thirdparty-conan-build.cmake` had a
bug no build would let pass (the Boost.JSON component variable was misspelt; fixed in
PR #291). In conan mode the rapid bindings link a target name only internal mode defines
(`RapidJson`), and nlohmann bindings are only found when tests are on. Users who consume Jinja2C++ through a package
manager get an old version.

**Proposal.**
1. CI job (`.github/workflows/conan-build.yml`, PR #291; ConanCenter is not reachable
   from the cloud sandbox, so CI is the first place it runs): `conan install` + configure with `-DJINJA2CPP_DEPS_MODE=conan-build` + build +
   tests, so the mode stays working.
2. Fix rapid/nlohmann bindings in conan mode and add them to the job.
3. Keep `conanfile.txt` versions aligned with `internal_deps.cmake` (one check script).
   ConanCenter lags upstream, so the check must allow "newest version ConanCenter has":
   after the October 2026 bump internal mode uses Boost 1.92.0 and expected-lite 0.10.0
   while ConanCenter tops out at Boost 1.91.0 and expected-lite 0.9.0.
4. Release process: tag, changelog, update the ConanCenter recipe and vcpkg port.

**Done when.** The Conan CI job is green and a new release is published to ConanCenter.
