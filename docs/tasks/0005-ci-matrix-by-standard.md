---
status: in-progress
priority: high
area: ci
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/291
---
# CI matrix organised by C++ standard, pairwise-sparse

**Problem.** A full Cartesian matrix (compilers × standards × build types × linkage ×
JSON bindings) grows multiplicatively: 6 compilers × 3 standards × 2 configs alone is 36
jobs, and adding bindings and linkage would be over 200. Most of those jobs test the same
code paths. On the other side, dropping axes loses coverage of real interactions (a
binding that only breaks in C++20, a shared-library export that only breaks on one
compiler).

What users actually choose is the language standard, not the compiler patch version, so
the standard is the primary axis; compiler versions only need their oldest and newest
supported releases.

**Proposal.** Every standard gets the full set of compiler families; the remaining axes
(compiler version, build type, linkage, JSON bindings) are covered pairwise: every pair
of values of any two axes appears in at least one job. Pairwise coverage catches most
interaction bugs at a fraction of the cost.

**Done when.** The Linux workflow lists the pairwise matrix, each standard row is
explained in the workflow, and adding C++23 (0007) is a matter of adding rows.

**Next.** Pairwise misses three-way interactions; if a bug ever escapes because of one,
add that specific triple rather than going back to the full product.

**Load trim (2026-10-02, Ruslan: one build type per compiler and standard).** Linux and
macOS already ran one job per compiler (or runner image) and standard. Windows ran three
per standard, one per CRT value, but `JINJA2CPP_BUILD_SHARED=ON` forces `/MD`, so two of
the nine jobs duplicated others and `/MT` was only ever built once (C++20 Debug static).
Windows now runs one job per standard, rotating build type, linkage and CRT so that each
value appears once: 9 jobs (about 84 runner-minutes per push) become 3 (about 28).
MSVC Release+shared is no longer built; Linux keeps Release+shared rows on GCC and Clang.
The full measurement is in the PR description.
