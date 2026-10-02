---
status: open
priority: low
area: standards
depends: [0054, 0007]
touches: [src/, include/jinja2cpp/, .github/workflows/clang-tidy.yml]
---
# clang-tidy checks that need C++17 or C++20

**Problem.** At C++14 several checks are silent. A run at C++20 adds:
`modernize-use-designated-initializers` (2 340, off by choice), `use-nodiscard` (covered
at C++14 by 0059), `use-ranges` (40), `use-constraints` (32),
`bugprone-unchecked-optional-access` (15), `use-integer-sign-comparison` (13),
`concat-nested-namespaces` (10), `use-starts-ends-with` (1),
`bugprone-suspicious-stringview-data-usage` (1). The optional-access check is a bug finder
that understands `std::optional` only, which nonstd maps to at C++17.

**Proposal.** Now: an advisory C++17 clang-tidy job running only
`bugprone-unchecked-optional-access` (its 15 hits are mostly the missing `[[noreturn]]`
in 0055). After the minimum standard is raised (0007): enable the rest and apply them in
the 0057 style.

**Done when** the C++17 job is green and gating, and the modernize checks above are
applied for the new minimum standard.
