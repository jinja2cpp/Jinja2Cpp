---
status: open
priority: medium
area: perf
depends: [0057]
touches: [src/, include/jinja2cpp/]
---
# clang-tidy: fixes that change signatures, copies or linkage

**Problem.** Some 0054 hits need review because the fix changes a signature or ownership:
`performance-unnecessary-value-param` (65; `FilterParams` is copied on every filter call,
`src/filters.cpp:946` and others), `modernize-pass-by-value` (19),
`performance-move-const-arg` (12), `unnecessary-copy-initialization`,
`noexcept-move-constructor`, `cppcoreguidelines-missing-std-forward` (16),
`rvalue-reference-param-not-moved`, `prefer-member-initializer`,
`readability-convert-member-functions-to-static` (26), `misc-use-anonymous-namespace`
(17), `google-explicit-constructor` (66, of which 24 in public headers),
`cppcoreguidelines-special-member-functions` (22), `readability-implicit-bool-conversion`
(16 after the allowed pointer and integer conditions).

**Proposal.** One PR per two or three checks, fixes applied with the 0054 script and
reviewed. `Value`'s converting constructors are implicit by design: they get
`// NOLINT(google-explicit-constructor)` unless 0056 decides otherwise. Measure the
`FilterParams` change with the perf benchmarks if any exist (otherwise note the
allocation count).

**Done when** these checks report nothing and sit in `WarningsAsErrors`.
