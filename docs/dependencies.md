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
`jinja2cpp/binding/nlohmann_json.h`), `link` (not in headers, but a static consumer must
link it), `internal` (compiled in, invisible), `none`.

**Delivery**: how it arrives. `fetched` (FetchContent, pinned by hash or tag),
`vendored` (copied into the repository), `generated` (data derived from an outside
source and committed), `system` (found on the machine), `ci` (GitHub Actions).

## The library

| Component | Version | Licence | Role | Exposure | Delivery | Why it is needed | Could it go? |
|---|---|---|---|---|---|---|---|
| expected-lite | 0.10.0 | BSL-1.0 | mandatory | public-api | fetched | `jinja2::Result<T>` in every Load/Render signature; C++17 has no `std::expected` | Keep until a C++23 floor; could be vendored into `include/` |
| {fmt} | 12.2.0 | MIT | mandatory | link | fetched | Python `str.format`, `%`-formatting and number output with runtime specs and `wchar_t`; `std::format` is C++20 | Keep; header-only mode would drop the link dependency |
| Boost | 1.92.0 | BSL-1.0 | mandatory | link | fetched | the nine libraries below | Yes, piece by piece (docs/tasks/0230) |
| robin-hood-hashing | 3.11.5, modified | MIT | mandatory | internal | vendored | flat hash map for values and scopes, faster than `std::unordered_map` here | Keep the idea; upstream is archived (docs/tasks/0231) |
| lexertk | unrecorded, modified | MIT | mandatory | internal | vendored | expression tokenizer | Keep; effectively forked code |
| value_types | 57f2e7a, modified | MIT | mandatory | public-api | vendored | `polymorphic`/`indirect` in `jinja2::v2::detail::xyz`, behind `jinja2::types` (Value, GenericList) | Keep; `std::` at C++26 |
| Unicode tables | UCD 15.0.0 via Python 3.12 | Unicode-3.0 | mandatory | internal | generated | Python-identical `isprintable`, case mapping, digits without ICU | Data only; acquisition decision open in 0187/0186 |
| HTML entities | Python 3.12 `html.entities` | PSF-2.0 | mandatory | internal | generated | MarkupSafe-compatible unescape | Data only; keep |
| Boost.JSON | 1.92.0 | BSL-1.0 | alternative (default) | public-optional | fetched | parses `{% meta %}` JSON for `GetMetadata` | Yes: the heaviest backend for a niche feature |
| nlohmann_json | 3.12.0 | MIT | alternative, and test | public-optional | fetched | the same, header-only; the tests' JSON reader | The natural default if Boost.JSON goes |
| RapidJSON | 24b5e7a8, patched | MIT | alternative | public-optional | fetched | the same, for RapidJSON users | Candidate to retire as a built-in backend |
| Boost.Multiprecision | 1.92.0 | BSL-1.0 | optional, and test | public-optional | fetched | `cpp_int` behind the built-in big-integer backend with `-DJINJA2CPP_WITH_BIGINT=boost` (off by default) and the opt-in adapter `jinja2cpp/big_int/boost_cpp_int.h`; standalone mode, so only its own and Boost.Config's headers | Header-only and optional; after 0230 it is fetched on its own (docs/design/0178-big-integer-plan.md 8) |

### Boost, library by library

Boost is the weight. Internal mode fetches the whole 1.92.0 CMake archive, configures
47 Boost libraries (the transitive closure of what CMake asks for) and installs all of
them: 56 MB of headers and four static libraries (`boost_filesystem`, `boost_json`,
`boost_container`, `boost_atomic`) next to `libjinja2cpp`. What the code actually
includes is small:

| Boost library | Used for | C++17 alternative | Cost to replace |
|---|---|---|---|
| core (`span`) | about 57 uses in 14 `src/` files | none before C++20 `std::span` | small: an internal span |
| container (`small_vector`) | about 27 hot-path uses | none | medium: perf-sensitive, measure with bench/ |
| smart_ptr (`intrusive_ptr`) | InternalValue payloads (one pointer wide, 0140) | none (`shared_ptr` is two pointers) | small: about 60 lines |
| any (`unique_any`) | holds the parsed metadata document | `std::any` needs copyable payloads | small |
| iterator (`iterator_facade`) | one internal iterator | hand-written iterator | small |
| filesystem | `RealFileSystem`: path joining, `last_write_time` | `std::filesystem` | small; removes two compiled libraries |
| algorithm (string) | `replace_all`, `is_alnum`, `all(is_space)` | a few lines of `std::string` code | trivial |
| predef (endian) | RapidJSON's UTF-16/32 choice | compiler macros | trivial |
| json | the default metadata backend | nlohmann (header-only) or an internal reader | small, behind the existing backend switch |

`lexical_cast`, `numeric_conversion`, `variant` and `optional` are no longer requested
(nothing includes them since 0071), and the dead includes of `boost/unordered_map.hpp`,
`boost/cast.hpp`, `boost/core/null_deleter.hpp`, `algorithm/string/trim_all.hpp` and
`algorithm/string/join.hpp` are gone (docs/tasks/0229). `container` is now requested by
name: `lexical_cast` used to pull it in, and `small_vector` needs it. `optional` and the
other leaf libraries still arrive as dependencies of `algorithm`, `any` and `json`.

## Tests, benchmarks and tooling

| Component | Version | Licence | Role | Notes |
|---|---|---|---|---|
| googletest | v1.18.0 | BSD-3-Clause | test | git tag, not a hash; no longer installed with the package (it was, in internal mode) |
| Python Jinja2 / MarkupSafe | 3.1.6 / 3.0.3 | BSD-3-Clause | test | the oracle; its expectations are committed, so builds never need it |
| google benchmark | v1.9.5 | Apache-2.0 | optional | `JINJA2CPP_BUILD_BENCHMARKS` |
| inja, minja | 3.5.0, 143465a | MIT | optional | `JINJA2CPP_BENCH_WITH_OTHER_ENGINES` |
| MiniJinja, Tera | 2.24, 2.4 | Apache-2.0, MIT | optional | `JINJA2CPP_BENCH_WITH_RUST_ENGINES`, locked in `bench/rust/Cargo.lock` |
| gperftools | system | BSD-3-Clause | optional | `JINJA2CPP_BENCH_WITH_GPERFTOOLS` |
| Conan | 2.x | MIT | optional | `JINJA2CPP_DEPS_MODE=conan-build` |
| CMake | 3.23+ | BSD-3-Clause | tooling | required to build |
| CPython, clang-tidy, clang-format, PyYAML | 3.12, 22.1.8, 18, unpinned | various | tooling | CI and scripts only |
| GitHub Actions | tags | MIT | tooling | seven actions, all pinned by mutable tags; two are third-party (`hendrikmuhs/ccache-action`, `ilammy/msvc-dev-cmd`) |
| CodeCoverage.cmake (Lars Bilke) | vendored | BSD-3-Clause | redundant | `cmake/code_coverage.cmake`, included by nothing (0010 owns the file) |

## What a consumer actually needs

| Consumer | Needs |
|---|---|
| compiles against the installed headers | expected-lite only (plus the JSON library of a binding header they include) |
| links the static library | also fmt and the Boost libraries (filesystem, atomic; json and container with the default backend) |
| links the shared library built in internal mode | expected-lite only; fmt and Boost are linked in |

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
5. **Pins that disagree.** Conan mode builds against expected-lite 0.9.0, Boost 1.91.0
   and an unpatched RapidJSON while internal mode uses 0.10.0, 1.92.0 and a patched
   commit; googletest, benchmark and every GitHub Action are pinned by mutable tags
   (docs/tasks/0232). The audit in `scripts/sbom.py --check` makes such drift visible but
   records it rather than resolving it.

Distribution notices for the redistributed code (Boost, fmt and expected-lite in an
internal-mode install, value_types in the installed headers) are docs/tasks/0226.

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
