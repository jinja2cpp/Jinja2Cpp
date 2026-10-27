# Third-party dependencies and SBOM

This page explains every piece of third-party code and data Jinja2C++ uses: why it is
there, who sees it, and whether it could go to make the library lighter. The data
behind it lives in [`sbom/components.json`](../sbom/components.json); the CycloneDX and
SPDX documents next to it are generated from that file by `scripts/sbom.py`, and the
format-check CI job fails when they drift from it or from the build files. Audited on
master 339b305 (2026-10-18, docs/tasks/0187).

## How to read the classification

Each component carries four independent labels.

**Role**: why it is in the tree.

| Role | Meaning |
|---|---|
| mandatory | needed in every configuration of the library |
| alternative | one of a set, exactly one of which is needed (the JSON backends) |
| optional | needed only by an opt-in feature (benchmarks, profilers, Conan mode) |
| test | needed only to build or generate the tests |
| tooling | developer or CI tooling, never part of a build product |
| redundant | requested, fetched or installed, but nothing uses it |

**Phases**: when it is needed. `build` (compiling Jinja2C++), `consumer-build` (a
consumer compiling against the installed headers), `consumer-link` (a consumer linking
the static library), `runtime` (a shared dependency loaded at run time), `test`, `dev`,
`ci`.

**Exposure**: who can see it. `public-api` (its types are in the headers every consumer
includes), `public-optional` (only in an opt-in header such as
`jinja2cpp/binding/nlohmann_json.h`, part of a package component, docs/tasks/0311), `link` (not in headers, but a static consumer must
link it), `internal` (compiled in, invisible), `none`.

**Delivery**: how it arrives. `fetched` (FetchContent, pinned by hash or tag),
`vendored` (copied into the repository), `generated` (data derived from an outside
source and committed), `system` (found on the machine), `ci` (GitHub Actions).

## The library

| Component | Version | Licence | Role | Exposure | Delivery | Why it is needed | Could it go? |
|---|---|---|---|---|---|---|---|
| expected-lite | 0.10.0 | BSL-1.0 | mandatory | public-api | fetched | `jinja2::Result<T>` in every Load/Render signature; C++17 has no `std::expected` | Keep until a C++23 floor; could be vendored into `include/` |
| {fmt} | 12.2.0 | MIT | mandatory | link | fetched | Python `str.format`, `%`-formatting and number output with runtime specs and `wchar_t`; `std::format` is C++20 | Keep; header-only mode would drop the link dependency |
| fast_float | 8.2.10 | MIT, Apache-2.0 or BSL-1.0 | mandatory | internal | fetched | decimal-to-double conversion that ignores the C locale: float literals, the `float` and `round` filters, `{% meta %}` numbers (`src/decimal_number.cpp`, 0333); `strtod` follows `LC_NUMERIC` and `std::from_chars` for `double` is missing from older AppleClang | Header-only and a build interface: not installed, not seen by static consumers; `std::from_chars` once every supported library has it |
| robin-hood-hashing | 3.11.5, modified | MIT | mandatory | internal | vendored | flat hash map for values and scopes, faster than `std::unordered_map` here | Keep the idea; upstream is archived (docs/tasks/0231) |
| lexertk | unrecorded, modified | MIT | mandatory | internal | vendored | expression tokenizer | Keep; effectively forked code |
| value_types | 57f2e7a, modified | MIT | mandatory | public-api | vendored | `polymorphic`/`indirect` in `jinja2::v2::detail::xyz`, behind `jinja2::types` (Value, GenericList) | Keep; `std::` at C++26 |
| Unicode tables | UCD 15.0.0 via Python 3.12 | Unicode-3.0 | mandatory | internal | generated | Python-identical `isprintable`, case mapping, digits without ICU | Data only; acquisition decision open in 0187/0186 |
| HTML entities | Python 3.12 `html.entities` | PSF-2.0 | mandatory | internal | generated | MarkupSafe-compatible unescape | Data only; keep |
| Boost.JSON | 1.92.0 | BSL-1.0 | alternative, and test | public-optional | fetched | the component `jinja2cpp::boost_json` (`jinja2cpp/binding/boost_json.h`) and its test; `{% meta %}` is read by `src/json_reader.cpp` since 0230 step D6 | Done: the Boost release is fetched only for this test (tests ON with `boost_json` in `JINJA2CPP_TEST_BINDINGS`, which bindings.yml sets) and never installed (0230 D7a, 0311) |
| nlohmann_json | 3.12.0 | MIT | alternative, and test | public-optional | fetched | the component `jinja2cpp::nlohmann_json`, header-only, the binding the default CI jobs test; the tests' JSON reader | Keep: tests only |
| Boost.Multiprecision | 1.92.0 | BSL-1.0 | optional, and test | public-optional | fetched | `cpp_int` behind the built-in big-integer backend with `-DJINJA2CPP_WITH_BIGINT=boost` (off by default) and the component `jinja2cpp::boost_multiprecision` (`jinja2cpp/big_int/boost_cpp_int.h`); standalone mode, so only its own and Boost.Config's headers | Done: fetched on its own with Boost.Config (boostorg/multiprecision and boostorg/config at boost-1.92.0, pinned by hash), only with tests ON or `bigint=boost` (0230 D7a) |

### Boost, library by library

The library links no Boost and its package never names it (0230 D7a). Before 0230, internal
mode fetched the whole 1.92.0 CMake archive, configured 43 Boost libraries and installed all
of them (53 MB of headers and two static libraries) next to `libjinja2cpp`. Each library the
code used now has a replacement:

| Boost library | Used for | Replaced by |
|---|---|---|
| core (`span`) | about 57 uses in 14 `src/` files | `src/span.h` (0230 D4) |
| container (`small_vector`) | about 27 hot-path uses | `src/small_vector.h`, bench-gated (0230 D5) |
| smart_ptr (`intrusive_ptr`) | InternalValue payloads (one pointer wide, 0140) | `src/intrusive_ptr.h` (0230 D4) |
| any, iterator, algorithm, predef | `unique_any`, `iterator_facade`, string helpers, endian | a `shared_ptr<void>`, a hand-written iterator, `std::string` code, compiler macros (0230 D3) |
| json | `{% meta %}` parsing | `src/json_reader.cpp` (0230 D6) |
| filesystem | `RealFileSystem` | `std::filesystem` (0230 D2) |

What is left are the opt-in pieces in the table above. With the defaults and tests OFF,
nothing of Boost is fetched; with tests ON, the default build fetches only standalone
Multiprecision and Config, and the full Boost release comes in only for the Boost.JSON
binding test, with its install rules off (`BOOST_SKIP_INSTALL_RULES`). `scripts/sbom.py
--check` fails when any other file than the Boost components (`bindings/boost_*`),
`src/big_int_builtin.cpp` and `test/binding/boost_json_binding_test.cpp` names
`<boost/...>`, and `test/package/check_package.cmake` fails when an install carries Boost or
nlohmann_json, or the core package's CMake files find or link them; only a component's own
deps file finds its library, when a consumer asks for the component (0311). The external modes find a system Boost only when a Boost piece is selected, and
Conan asks for a header-only Boost (`boost/*:header_only=True`); `external-boost` is a
deprecated alias of `internal`, removed in 3.0.

## Tests, benchmarks and tooling

| Component | Version | Licence | Role | Notes |
|---|---|---|---|---|
| googletest | v1.18.0 | BSD-3-Clause | test | archive pinned by SHA-256; no longer installed with the package (it was, in internal mode) |
| Python Jinja2 / MarkupSafe | 3.1.6 / 3.0.3 | BSD-3-Clause | test | the oracle; its expectations are committed, so builds never need it |
| google benchmark | v1.9.5 | Apache-2.0 | optional | `JINJA2CPP_BUILD_BENCHMARKS` |
| inja, minja | 3.5.0, 143465a | MIT | optional | `JINJA2CPP_BENCH_WITH_OTHER_ENGINES` |
| MiniJinja, Tera | 2.24, 2.4 | Apache-2.0, MIT | optional | `JINJA2CPP_BENCH_WITH_RUST_ENGINES`, locked in `bench/rust/Cargo.lock` |
| gperftools | system | BSD-3-Clause | optional | `JINJA2CPP_BENCH_WITH_GPERFTOOLS` |
| Conan | 2.x | MIT | optional | `JINJA2CPP_DEPS_MODE=conan-build` |
| CMake | 3.23+ | BSD-3-Clause | tooling | required to build |
| CPython, clang-tidy, clang-format, PyYAML | 3.12, 22.1.8, 18, 6.0.3 | various | tooling | CI and scripts only |
| GitHub Actions | commit SHAs | MIT | tooling | seven actions, each pinned by commit SHA with its release tag in a comment (`sha` in components.json, checked by `sbom.py --check`); two are third-party (`hendrikmuhs/ccache-action`, `ilammy/msvc-dev-cmd`) |
| CodeCoverage.cmake (Lars Bilke) | vendored | BSD-3-Clause | redundant | `cmake/code_coverage.cmake`, included by nothing (0010 owns the file) |

## What a consumer actually needs

| Consumer | Needs |
|---|---|
| compiles against the installed headers | expected-lite only (plus the library of a component they link, such as nlohmann_json for `jinja2cpp::nlohmann_json`) |
| links the static library | also fmt |
| links the shared library built in internal mode | expected-lite only; fmt is linked in |

## The contradictions this exposes

1. **A lightweight library on a heavyweight portability layer.** Boost entered when
   C++11/14 lacked `any`, `optional`, `variant` and `filesystem`. The C++17 floor (2.0)
   removed that reason, but Boost stayed, now held in place by small utilities
   (`span`, `small_vector`, `intrusive_ptr`) that the performance work adopted. Each is a
   few dozen lines to own; the payoff is a library whose only external dependencies are
   fmt and a header-only expected. That move creates the next tension: owned utilities
   must be kept correct and fast, so each swap needs the bench/ gate (docs/tasks/0230).
2. **A default backend chosen for a niche feature.** The default JSON backend exists
   only to parse `{% meta %}` blocks, yet it brings the two largest compiled Boost
   libraries. Making the header-only backend the default (or parsing metadata with a
   small internal reader) resolves it without touching the opt-in binding headers.
3. **Python as oracle, not as dependency.** Parity needs Python's exact Unicode and
   entity behaviour; committing generated tables keeps builds Python-free, and the
   staleness checks keep them honest. The open question (0187, 0186) is whether the
   tables should come from pinned Unicode data instead of the oracle interpreter.
4. **Vendoring trades a dependency for a fork.** robin_hood, lexertk and value_types are
   all locally modified, and two have no recorded upstream revision. The inventory now
   records what changed; robin_hood's upstream being archived is the first place where
   the fork has to be paid for (docs/tasks/0231).
5. **Pins that disagree.** Conan mode builds against expected-lite 0.9.0, a header-only
   Boost 1.91.0 (tests and `bigint=boost` only) while internal mode uses 0.10.0 and
   1.92.0. ConanCenter has no newer expected-lite or Boost, so each difference carries a `conan_why` in components.json and a
   comment in `conanfile.txt`; `scripts/sbom.py --check` fails a difference without one
   (docs/tasks/0232). googletest, benchmark and every GitHub Action are now pinned
   immutably (SHA-256 or commit SHA).

Distribution notices for the redistributed code (fmt and expected-lite in an
internal-mode install, Boost.Multiprecision compiled into a `bigint=boost` build, fast_float compiled into every build, value_types in the installed headers) are docs/tasks/0226.

## Maintaining the inventory

- Adding, bumping or removing a dependency: edit `sbom/components.json`, run
  `python3 scripts/sbom.py`, commit the four regenerated `sbom/jinja2cpp*.json` files.
- `python3 scripts/sbom.py --check` fails when a generated file is stale, when a pin or
  version recorded in the manifest no longer appears in the build file it names, or when
  the build uses something the manifest does not list: a FetchContent source, a Conan or
  pip requirement, a GitHub Action, a Boost header, or a file in `.clang-format-ignore`
  (the list of vendored files).
- `--validate` also checks the documents against the CycloneDX 1.6 schema and the SPDX
  2.3 validator (CI installs pinned `cyclonedx-python-lib` and `spdx-tools`).
- The library profile (`jinja2cpp.cdx.json`, `jinja2cpp.spdx.json`) is installed to
  `share/doc/Jinja2Cpp/sbom/`; the `-full` profile adds tests, benchmarks and tooling.
  Both describe all configurations at once: each component's `selected-by` property
  says which option brings it in.
