# Licensing and distribution notices

Task [0226](tasks/0226-license-metadata-and-distribution.md). What license Jinja2C++ is
under, which third-party code and data its artifacts carry, and where the notices for them
are. The machine-readable inventory is [sbom/components.json](../sbom/components.json)
(task 0187); this page records the conclusions a packager needs.

## Jinja2C++ is MPL-2.0, in every release

The `LICENSE` file is the Mozilla Public License 2.0 at every tag of the repository:
`v0.5-alpha`, `v0.6-alpha`, `v0.9`, `0.9.1`, `0.9.2`, `1.0.0`, `1.1.0`, `1.2.0-beta`,
`1.2.1`, `1.3.0-beta`, `1.3.1`, `1.3.2` and master (checked 2026-11-05 with
`git show <tag>:LICENSE`). The README badge says Mozilla, and the SBOM records `MPL-2.0`
for the root component. No release was ever published under MIT, and nothing here
relicenses anything.

### Metadata that disagrees

| Where | Declares | Status |
|---|---|---|
| ConanCenter recipe [`recipes/jinja2cpp/all/conanfile.py`](https://github.com/conan-io/conan-center-index/blob/master/recipes/jinja2cpp/all/conanfile.py), versions 1.1.0, 1.2.1, 1.3.2 (`config.yml`) | `license = "MIT"` (line 15), while its `package()` copies the MPL-2.0 `LICENSE` into `licenses/` | Wrong; the correction below is **not filed yet** |
| Historical recipe repositories (jinja2cpp/conan-recipes, the conan-center-index fork) | MIT | Not maintained; their fate is decided in task 0227 |
| Owned metadata in this repository: `sbom/components.json` (and the generated SPDX and CycloneDX documents), `LICENSE`, README badge | MPL-2.0 | Correct. `conanfile.txt` and `jinja2cpp.pc.in` carry no license field |

Proposed upstream correction (an issue or pull request on conan-io/conan-center-index,
filed by a maintainer): change `license = "MIT"` to `license = "MPL-2.0"` in
`recipes/jinja2cpp/all/conanfile.py`, which covers every packaged version (1.1.0, 1.2.1,
1.3.2: their `LICENSE` is MPL-2.0); for a 2.0 recipe, also copy `THIRD-PARTY-NOTICES.txt`
into `licenses/`. Link the submitted request here with its actual state; a submitted
request is not a published package update.

## Third-party code and data in the artifacts

`THIRD-PARTY-NOTICES.txt` in the repository root holds the copyright and license text of
each component below. CMake installs it with `LICENSE` to `${CMAKE_INSTALL_DOCDIR}`
(`share/doc/Jinja2Cpp/`, next to the SBOM in `sbom/`), in every dependency mode, because
everything in the table that is compiled into the library is compiled in whatever the mode.

| Component | License | Source archive | Library binary | Installed headers | Notice |
|---|---|---|---|---|---|
| robin-hood-hashing (`src/robin_hood.h`, vendored) | MIT | yes | every build | no | in the file, NOTICES |
| lexertk (`src/lexertk.h`, vendored) | MIT | yes | every build | no | in the file, NOTICES |
| value_types (`include/jinja2cpp/polymorphic_value/`, vendored) | MIT | yes | every build | yes | in the files, NOTICES |
| Unicode Character Database 15.0 (generated `src/unicode_*.h`, `scripts/unicode_name_aliases.txt`) | Unicode-3.0 | yes | every build | no | NOTICES |
| Python 3.12 `html.entities.html5` (generated `src/html_entities.h`) | PSF-2.0 (see below) | yes | every build | no | NOTICES |
| fast_float | MIT OR Apache-2.0 OR BSL-1.0, used under MIT | no (fetched or found) | every build (header-only) | no | NOTICES |
| {fmt} | MIT | no (fetched or found) | linked; a shared library contains it | internal mode installs fmt's headers, library and package | NOTICES |
| expected-lite | BSL-1.0 | no (fetched or found) | header-only | internal mode installs `include/nonstd/expected.hpp` | NOTICES |
| Boost.Multiprecision, Boost.Config | BSL-1.0 | no | only with `-DJINJA2CPP_WITH_BIGINT=boost` | no (the `boost_multiprecision` component asks the consumer for Boost) | NOTICES |

"NOTICES" is `THIRD-PARTY-NOTICES.txt`. Redistributed means the row's artifact contains the
component's code or data; externally supplied means the consumer provides it (external and
Conan modes take fmt and expected-lite from the package manager, which ships their
licenses; the JSON binding components link the consumer's nlohmann/json or Boost.JSON).

The html5 entity table is a list of the WHATWG HTML named character references, taken
from Python's standard library so that unescaping matches Python. A list of names and code
points carries little or no copyright of its own; the inventory records PSF-2.0, the
license of the file it was read from, and the notice ships, so the conservative reading is
covered.

Not in any installed artifact, only in the source archive and its build: tests (GoogleTest,
nlohmann/json, Boost.JSON, the JSON test suite cases in `test/json_test_suite_cases.inc`,
the chat templates in `test/chat_templates/templates/` whose per-model licenses are in
`test/chat_templates/SOURCES.json`), benchmarks and their comparison engines, the fuzzers,
and `cmake/code_coverage.cmake` (BSD-3-Clause, notice in the file). Each is in
`sbom/components.json` with its license; the full SBOM profile (`sbom/jinja2cpp-full.*`)
lists them.

## For packagers

- Declare `MPL-2.0` for Jinja2C++ itself.
- Ship `LICENSE` and `THIRD-PARTY-NOTICES.txt` (Conan: copy both into `licenses/`; Debian:
  `debian/copyright` can cite the sections of the notices file). A package that installs
  through CMake gets both in `share/doc/Jinja2Cpp/`.
- When fmt and expected-lite come from your package manager, their own packages carry their
  licenses; the notices file still lists them because an internal-mode install contains
  them.
- The release artifact check (task 0225, `scripts/check_release_artifact.py`) fails when
  an install lacks either file.
