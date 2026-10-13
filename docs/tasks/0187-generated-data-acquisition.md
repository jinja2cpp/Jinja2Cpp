---
status: open
priority: low
area: build
touches: [scripts/gen_unicode_printable.py, scripts/gen_unicode_tables.py, scripts/gen_unicode_case.py, scripts/gen_html_entities.py, thirdparty/internal_deps.cmake]
---
# How generated tables and third-party data are acquired (SBOM)

**Problem.** The library embeds data generated from outside sources: four headers derived
from Python's `unicodedata`, `str` methods and `html.entities` (generators in `scripts/`,
checked for staleness by `parity-expectations.yml` since 0048, PR #502), plus vendored code
(`robin_hood.h`, `lexertk.h`) and pinned dependencies hashed in
`thirdparty/internal_deps.cmake`. Their provenance (source, version, licence) is spread
over comments and is not machine-readable, so there is no SBOM a downstream packager can
consume, and each generated header records its source only in a comment line (raised by
Ruslan 2026-10-13 in the 0048 thread).

**Proposal.** One manifest of every embedded or fetched component (name, version, origin
URL, licence, hash, how it is regenerated), from which an SPDX or CycloneDX SBOM is emitted
at install time; the generators write their source version into the manifest.

**Alternative to weigh** (Ruslan: "quite good, but a big rework"): generate the Unicode
tables at build time, or with a committed generator, from pinned Unicode Character Database
files (`UnicodeData.txt`, `SpecialCasing.txt`, `DerivedCoreProperties.txt`) and the WHATWG
`entities.json`, stored in the repo or fetched by hash. Builds stay Python-free and
reproducible and the Unicode version becomes an explicit pin, but the tables are no longer
derived from Python itself, so Python-specific rules (`str.isprintable`, `int()`'s digit
and space transform, Final_Sigma) must be re-implemented and cross-checked against Python
in CI. Decide together with 0186.

**Done when.** A manifest lists every embedded or fetched component with version and
licence, installs ship an SBOM generated from it, and the table-source decision is recorded.
