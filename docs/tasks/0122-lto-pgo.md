---
status: open
priority: low
area: build
touches: [CMakeLists.txt, bench/README.md]
---
# Measure LTO and PGO builds

**Problem.** The library is built without link-time optimisation or profile-guided
optimisation, and we do not know what either would give an embedder. Much of the hot
path is small virtual calls and visitors, the kind of code PGO helps.

**Proposal.** An option `JINJA2CPP_WITH_LTO` (`CMAKE_INTERPROCEDURAL_OPTIMIZATION`),
measure it and a PGO build trained on the bench suite (`-fprofile-generate`/`-use`) with
wall clock and instruction counts; document how an embedder does the same.

**Done when.** The measured gains are in bench/README.md and the option exists if LTO
pays.
