---
status: open
priority: high
area: release
touches: [CMakeLists.txt#JINJA2CPP_INSTALL, docs/licensing.md]
---
# Reconcile package license metadata and ship required notices

**Problem.** The source tree's LICENSE is MPL-2.0, while both historical selected
Conan recipe repositories and the [public ConanCenter recipe](https://github.com/conan-io/conan-center-index/blob/master/recipes/jinja2cpp/all/conanfile.py)
inspected on 2026-10-17 declare MIT. A local internal-mode install of master 498c671 contained no license,
copying or copyright files. These are concrete distribution gaps, separate from
0187's broader machine-readable provenance and SBOM proposal.

**Proposal.** Verify the applicable licenses against the actual source and release
history; do not resolve conflicting metadata by silently relicensing anything.
Record the result and correct owned packaging metadata. File the corresponding
update with the maintained upstream ConanCenter recipe, identifying the versions
affected. Treat the old conan-recipes and conan-center-index forks according to the
support decision in 0227 rather than assuming they publish current packages.

Make the CMake install and release artifacts include the project's license and the
notices required for code/data they redistribute. Inventory bundled implementation
headers and generated Unicode/HTML data, and distinguish redistributed dependencies
from externally supplied ones. Agree notice locations with downstream packagers.
Feed that inventory into 0187; a full SBOM generator is not required to fix a known
metadata mismatch or missing license file.

**Done when.** Documented license conclusions agree with the candidate's source and
owned package metadata, required notices appear in source and installed artifacts,
and 0225 asserts their presence. The upstream recipe correction is linked with its
actual status; a submitted request is not described as a published package update.
