---
status: done
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

**Resolution** ([#355](https://github.com/jinja2cpp/Jinja2Cpp/pull/355)). A GCC false
positive: the empty allocator base shares its address with `cb_`, and every constructor
passed `alloc_base::get()` on (to `clone()`, `move()`, `create_control_block()`) while
`cb_` was still unset, so GCC saw a reference into unwritten storage (copy constructor in
`src/template.cpp`; the in_place constructor under `-Wextra` in tests). `cb_` now has a
default member initialiser. The same PR fixes the initializer-list in_place constructor,
which built a `T` instead of the requested `U` (`test/value_ptr_test.cpp`). GCC 13 and
clang 18 Release/Debug: no warnings under the strict set. Wider flags are 0092.
