---
status: done
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

**Done** in the PR that adds this line, measured rather than added: LTO does not pay (GCC
13 renders 14% slower, Clang 18 ThinLTO is neutral), so there is no `JINJA2CPP_WITH_LTO`
option; PGO renders 17% faster, also on templates outside its training set, and LTO + PGO
19%. Numbers, the reasons and the embedder's recipe: bench/README.md "LTO and PGO". The GCC
`-Warray-bounds` false positive a profile triggers in `robin_hood.h` is kept a warning.
