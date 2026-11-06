---
status: open
priority: medium
area: release
depends: [0226, 0006]
touches: []
---
# Correct the ConanCenter recipe license with the 2.0 version bump

**Problem.** ConanCenter's `recipes/jinja2cpp/all/conanfile.py` declares `license = "MIT"`
for 1.1.0, 1.2.1 and 1.3.2, while every Jinja2C++ release is MPL-2.0 and the recipe itself
packages the MPL `LICENSE` (docs/licensing.md, task 0226). A metadata-only change request
against old versions costs ConanCenter reviewers a round for little gain, and the 2.0 recipe
needs other changes anyway (Conan 2 layout, components, no Boost, fast_float, the notices file).

**Proposal.** Decided by Ruslan 2026-11-05: do not file a separate license fix. When 2.0 is
released, we submit the version bump to conan-center-index ourselves, and that pull request
also sets `license = "MPL-2.0"` (it applies to the whole `all/` folder, so the listed 1.x
versions are corrected too) and copies `THIRD-PARTY-NOTICES.txt` with `LICENSE` into
`licenses/` in `package()`. Coordinate the recipe content with 0006 (Conan releases).

**Done when.** The conan-center-index pull request carrying the 2.0 bump and the license
correction is linked here with its actual state; this task is done only once it is merged
and the published 2.0 package reports MPL-2.0.
