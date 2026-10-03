---
status: done
priority: low
area: build
depends: [0057]
touches: [src/, include/jinja2cpp/]
---
# Include what you use (misc-include-cleaner)

**Problem.** `misc-include-cleaner` reports 697 hits in `src/` + `include/` (and 226 in
test/): symbols used through transitive includes (`std::make_shared`,
`nonstd::expected_lite::make_unexpected`, `jinja2::ErrorCode`). Code compiles by accident
of include order, which breaks when a header stops including another, and compile times
carry unneeded includes.

**Proposal.** Configure `IgnoreHeaders` and a mapping for the nonstd `*-lite` and Boost
umbrella headers (the check otherwise asks for internal detail headers), then apply in
one PR per directory. Switch the check on in `.clang-tidy` after that.

**Done when** the check is on and reports nothing.

## Outcome

Measured again at C++17 after 0055: 692 hits in `src/` and 267 in `test/`, all in `.cpp`
files (no header reports a symbol it reaches transitively). They come down to 225 and
143 distinct include insertions, plus 26 removals. `.clang-tidy` sets
`misc-include-cleaner.IgnoreHeaders` so that the check never asks for libstdc++ `bits/`,
Boost, fmt, nonstd, gtest, nlohmann, `detail/` or vendored headers, and the check is on.

`clang-apply-replacements` refuses two insertions at one offset, which is every
multi-header fix here, so the edits were applied by merging same-offset insertions, then
regrouped (own header, local, `<jinja2cpp/...>`, third-party, standard, each sorted).
Three includes the cleaner calls unused are needed and carry `// IWYU pragma: keep`
with the reason: `out_stream.h` (twice; `GetStreamOnString` returns an `OutStream` by
value) and `jinja2cpp/generic_list_iterator.h` in a test (`GenericList::iterator` is only
declared in `generic_list.h`, so any user iterating a `GenericList` must include it;
see 0069). The JSON binding sources for nlohmann and RapidJSON are not compiled in the
default configuration, so the check has not seen them.
