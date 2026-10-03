---
status: open
priority: low
area: build
touches: [include/jinja2cpp/polymorphic_value/polymorphic_cxx14.h]
---
# GCC Release builds warn `-Wmaybe-uninitialized` in `polymorphic_cxx14.h`

**Problem.** Found while building the benchmark suite (0011) with
`-DCMAKE_BUILD_TYPE=Release` and GCC 13: compiling `src/error_info.cpp` prints
`polymorphic_cxx14.h:322:29: warning: '<anonymous>' may be used uninitialized` three
times. The strict warning set demotes `maybe-uninitialized` to a warning
(`-Wno-error=maybe-uninitialized`), so the build passes, but the library is not
warning-free at `-O2`/`-O3`, and Debug CI never sees it.

**Proposal.** Find out whether it is the usual GCC false positive in inlined
`polymorphic_value` copies or a real read of an indeterminate value, then either fix
the initialisation or suppress it locally with a comment saying why.

**Done when.** A GCC Release build of the library prints no warnings.
