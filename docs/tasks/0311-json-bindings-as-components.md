---
status: in-progress
priority: high
area: build
depends: [0230]
touches: [include/jinja2cpp/json_adapter.h, include/jinja2cpp/binding/, bindings/, test/binding/, cmake/public/, .github/workflows/bindings.yml]
shares: [CMakeLists.txt, thirdparty/internal_deps.cmake, thirdparty/thirdparty-conan-build.cmake, conanfile.txt, sbom/components.json, docs/dependencies.md, docs/release-2.0.md, MIGRATION.md, include/jinja2cpp/reflected_value.h]
---
# JSON bindings as components on a public adapter contract

**Problem.** Once 0230 D6 gives `{% meta %}` its own reader, nothing in `src/` includes a
JSON library, yet the package still treats the JSON binding as a variant of the library.
`JINJA2CPP_WITH_JSON_BINDINGS` picks one of Boost.JSON, nlohmann and RapidJSON at
configure time, the installed config and pkg-config file ask for that library, and a user
of another library gets a jinja2cpp packaged around the wrong dependency. The three
headers in `include/jinja2cpp/binding/` are hand copies of the same map, list and value
accessors (about 240 lines each), so the 0125 lifetime rules live in three places, and
"works with any JSON library" means only the three we ship. On the other side, simdjson
(0302) cannot fit the copies at all: its on-demand API is forward-only and cannot back a
view that answers repeated lookups.

**Decision (Ruslan, 2026-10-25, design doc "JSON bindings as separate packages").**
- B1: one repository and one version; one component per JSON library (CMake
  `COMPONENTS`, a pkg-config file per binding, Conan components, vcpkg features, Bazel
  targets for 0245).
- B2: a public, header-only `JsonTraits<Json>` contract in `jinja2cpp/json_adapter.h`;
  our bindings are rewritten on it before 2.0.
- B3: two ways in, a `Reflect` view and a `ToValue` deep copy; traits that only offer a
  forward pass (simdjson on-demand) get the copy only.
- B4: the internal `{% meta %}` reader stays private.
- B5: default PR jobs test nlohmann only; full binding validation (every binding, the
  Boost release tarball) runs on PRs that touch the bindings, `json_adapter.h` or
  `reflected_value.h`, and nightly on master.

**Proposal.** The architect plan (`/mnt/project-files/0311/plan.md`) fixes the contract,
the steps and the open decisions. In outline:
- `json_adapter.h`: `JsonKind`, the traits contract with capability detection, one
  generic view adapter and one `ToValue` with sequence-size and nesting limits; integers
  above `int64_t` become `BigInt` (0178).
- nlohmann, Boost.JSON (and simdjson, 0302) as thin traits specialisations under
  `bindings/<library>/`, keeping the include paths `jinja2cpp/binding/<library>.h`.
  RapidJSON follows 0303.
- `test/binding/conformance.h`: one typed suite every binding instantiates, for both the
  view and the copy.
- Packaging: `JINJA2CPP_WITH_JSON_BINDINGS` is removed, `JINJA2CPP_TEST_BINDINGS`
  (default `nlohmann_json`) picks the binding tests; every binding header and component
  file is installed, and a component's `find_dependency` runs only when it is requested.
  The Boost.Multiprecision adapter header becomes the `boost_multiprecision` component.
- CI as in B5, plus a check that a Boost-free install contains no Boost.

Until 0230 D7a merges, only `include/`, `test/binding/` and `bindings/` change; D7a
owns the binding plumbing in `CMakeLists.txt` and `thirdparty/` until then.

**Done when.** `find_package(jinja2cpp COMPONENTS nlohmann_json)` and
`pkg-config jinja2cpp-nlohmann_json` work from an install that was built without any JSON
library; every shipped binding passes the conformance suite as view and copy; a user
traits specialisation in a test (not shipped) passes it too; the full-validation workflow
runs on a touching PR and nightly; `JINJA2CPP_WITH_JSON_BINDINGS` no longer exists and
MIGRATION.md says what replaces it; the view path shows no instruction-count regression
against the old bindings in `bench/count.py`.

**Next.** 0302 is rebased onto the contract (a dom view and copy, an on-demand copy).
Installing the conformance suite for third-party traits authors, and a `FromValue`
direction (Value back to a JSON document), are likely follow-ups.
