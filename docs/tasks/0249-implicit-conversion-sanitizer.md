---
status: open
priority: medium
area: robustness
depends: []
touches: [cmake/sanitizer.cmake, .github/workflows/linux-build.yml]
---
# Run the sanitizer CI with `-fsanitize=implicit-conversion`

**Problem.** 0172 found host integers silently wrapping on their way into `Value`. That
bug was an explicit `static_cast`, but the same class (silent truncation or sign change
of integers) also hides in implicit conversions, which `-Wconversion` reports only at
compile time and only where the types are visible, not in template-instantiated or
value-dependent paths.

**Proposal.** Add Clang's `-fsanitize=implicit-conversion` (implicit integer truncation,
sign change and bit-field conversion; Clang only, GCC has no equivalent) to the Clang
`address+undefined` sanitizer job, with `-fno-sanitize-recover` so a hit fails the run.
Expect hits in vendored code (`robin_hood.h`, `lexertk.h`, fmt, Boost): suppress those with
an ignore list (`-fsanitize-ignorelist`), fix hits in `src/` and `include/`.

**Not covered by the sanitizer.** Explicit casts are not checked, so it would not have
caught 0172 itself. Pair it with a one-off audit of explicit integer casts at host-data
boundaries (`include/jinja2cpp/`, `src/binding/`, reflection, user callable argument
conversion): every `static_cast` to a narrower or differently signed type there either
is provably in range or goes through a checked conversion such as `detail::ToStoredInt`.

**Done when.** The Clang sanitizer job builds and runs the tests with
`implicit-conversion` enabled and green, the ignore list names only vendored code, and
the boundary-cast audit is done (fixes or filed tasks).
