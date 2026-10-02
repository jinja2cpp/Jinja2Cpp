---
status: open
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
