---
status: open
priority: medium
area: release
depends: [0230]
touches: [MODULE.bazel, BUILD.bazel, .bazelrc, .bazelversion, .github/workflows/bazel.yml]
---
# Bazel build and Bazel Central Registry module

**Problem.** Jinja2C++ can be consumed through CMake (FetchContent, installed package) and
Conan. Bazel users have no supported route (raised by Ruslan 2026-10-18 for 2.0
distribution). Each dependency needs a Bazel module: fmt and nlohmann_json are in the
Bazel Central Registry (BCR); Boost's BCR modules are per-library and community-maintained;
expected-lite and the vendored headers (robin_hood, lexertk, polymorphic) would be pinned
`http_archive`s or vendored sources. The generated headers (`version.h`, Unicode tables)
must not need CMake or Python at Bazel build time.

**Proposal.** After the Boost-free core (0230) removes the hardest dependency: a Bzlmod
`MODULE.bazel` and a `cc_library(name = "jinja2cpp")` with the public headers, generated
`version.h` from a `genrule` or `expand_template`, and the JSON bindings as separate
targets. A CI job builds it and runs the unit tests (`cc_test` over the gtest binary
sources). Then submit the module to the BCR with a `presubmit.yml`, and keep its version in
step with CMake's `project(VERSION)` through a check script.

**Done when.** `bazel test //...` passes in CI on Linux, macOS and Windows, a downstream
example depends on the module through Bzlmod, and the BCR submission is linked with its
actual status (submitted is not published). docs/release-2.0.md D15 records whether this
is a 2.0 condition.
