---
status: in-progress
priority: high
area: build
depends: [0230]
touches: [include/jinja2cpp/json_adapter.h, include/jinja2cpp/binding/, bindings/, test/binding/, cmake/public/, test/package/, .github/workflows/bindings.yml]
shares: [CMakeLists.txt, .github/workflows/linux-build.yml, .github/workflows/clang-tidy.yml, .github/workflows/format-check.yml, .github/workflows/conan-build.yml, scripts/preflight.sh, scripts/sbom.py, scripts/abi_namespace.py, bench/CMakeLists.txt, README.md, AGENTS.md, thirdparty/CMakeLists.txt, thirdparty/thirdparty-internal.cmake, thirdparty/external_boost_deps.cmake, thirdparty/internal_deps.cmake, thirdparty/thirdparty-conan-build.cmake, conanfile.txt, sbom/components.json, docs/dependencies.md, docs/release-2.0.md, MIGRATION.md, include/jinja2cpp/reflected_value.h]
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

**Plan decisions (Ruslan, 2026-10-25, on the architect plan `/mnt/project-files/0311/plan.md`).**
1. Any type with `JsonTraits` reflects through a partial specialisation of
   `detail::Reflector`; there is no declaration macro, and an explicit `Reflector<T>` wins.
2. The binding headers move to `bindings/<library>/include/jinja2cpp/binding/` in S5; the
   include path stays `jinja2cpp/binding/<library>.h`.
3. A traits failure in `ToValue` reports a new `ErrorCode::DataReadError` (appended);
   limits report `ResourceLimitExceeded`.
4. Every Conan `with_<library>` option defaults to False.
5. `Reflect(non-const lvalue)` keeps copying the document into an owned view, shared by
   every copy of the `Value`; `Reflect(std::as_const(doc))` and `Reflect(&doc)` borrow.
6. The view copies strings per access in 2.0; zero-copy strings need a lifetime proof
   and are a later perf task.
7. `JINJA2CPP_WITH_JSON_BINDINGS` warns that it is ignored; `rapid` is a `FATAL_ERROR`
   naming the migration entry.
8. `JsonKind::BigInt` with an optional `AsBigIntText` is part of the 2.0 contract.
9. `ToValue` errors name the JSON path where the walk stopped (`$.items[3].tags`,
   `locationDescr = "JSON"`); an optional `JsonTraits::Location(Node)` adds a byte offset,
   reported as `(char N)` like Python's `JSONDecodeError`.
10. RapidJSON is deleted, not rewritten (0303): MIGRATION.md carries a user
    `JsonTraits<rapidjson::Value>` instead.

**Proposal.** In outline (steps S1-S8 in the plan):
- `json_adapter.h`: `JsonKind`, the traits contract with capability detection, one
  generic view adapter and one `ToValue` with sequence-size and nesting limits; integers
  above `int64_t` become `BigInt` (0178).
- nlohmann, Boost.JSON (and simdjson, 0302) as thin traits specialisations under
  `bindings/<library>/`, keeping the include paths `jinja2cpp/binding/<library>.h`.
  RapidJSON is deleted by 0303 before S5, so it never becomes a component.
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

**Progress.** S1 and S2 (one PR): `include/jinja2cpp/json_adapter.h`, the nlohmann and
Boost.JSON bindings as traits, `ErrorCode::DataReadError` (20), and the conformance suite
(`test/binding/conformance.h`) on nlohmann `json` and `ordered_json`, Boost.JSON, and two
toy traits in `test/binding/toy_json.h` (Qt-style view traits; forward-only traits with
`JsonResult`, `Location` and big-integer text). The toys are instantiated in
`nlohmann_json_binding_test.cpp` until S5 can add `test/binding/json_adapter_test.cpp` to
the test list. Choices made in S1: `ToValue` errors carry the path as the last ExtraParam
and `GetLocationDescr()` reads `JSON $.path (char N)`; the optional `Elements()` range is
left for 0302 (simdjson dom) to add if it needs one. 0303 then deleted RapidJSON.

S5 (bindings as components): the headers moved to `bindings/<component>/include/` with
their include paths kept; `bindings/CMakeLists.txt` makes each one an INTERFACE target
`jinja2cpp::<component>` (`nlohmann_json`, `boost_json`, `boost_multiprecision`) with its own
export set, deps file and `jinja2cpp-<component>.pc`, and every component is installed.
`jinja2cpp-config.cmake` resolves `COMPONENTS` (unknown or unfound components fail `REQUIRED`
with a message, `OPTIONAL_COMPONENTS` keeps the core) and appends the dependency's targets
after finding them, because an exported link to the build's own fetched nlohmann_json would
have to be exported too. `JINJA2CPP_WITH_JSON_BINDINGS` is gone (a warning; `rapid` is an
error) and `JINJA2CPP_TEST_BINDINGS` (default `nlohmann_json`) picks the bindings the tests
check; the toy instantiations moved to `test/binding/json_adapter_test.cpp`, built always.
CI: the binding axis left `linux-build.yml`; `bindings.yml` runs every binding on Linux
ASan/UBSan and TSan, MSVC /MT, macOS 14, Ubuntu's b2-built Boost, a changed-line clang-tidy
with every component configured, and an install consumed through each component by
find_package and pkg-config, on touching PRs and nightly. `check_package.cmake` checks that
every component is installed, that the core package files find no Boost or nlohmann_json,
the REQUIRED/OPTIONAL/unknown component probes and, in the Boost-free job, the
nlohmann_json component of an install built without any JSON library. MIGRATION.md has the
entry.

S7 and S8: the parity corpus copies each case's context through the binding's
`jinja2::ToValue(ordered_json)` instead of its own converter, so every corpus case checks the
copy path (the test target links `jinja2cpp::nlohmann_json` whatever
`JINJA2CPP_TEST_BINDINGS` lists). The per-binding files keep only what is specific to a
binding (its conformance traits, nlohmann's pointer overloads of `Reflect`); their rendering
rows were covered by the suite, which took over the two it lacked (an attribute of a JSON
null is undefined; `Find`/`HasValue` and the 1.x accessor on a null member). The view pass
of the corpus is 0320. 0302 follows on the components.

**Next.** 0302 is rebased onto the contract (a dom view and copy, an on-demand copy).
Installing the conformance suite for third-party traits authors, and a `FromValue`
direction (Value back to a JSON document), are likely follow-ups.
