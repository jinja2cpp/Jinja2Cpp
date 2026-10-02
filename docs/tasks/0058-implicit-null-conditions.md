---
status: open
priority: medium
area: style
depends: [0054]
touches: [src/, include/jinja2cpp/, scripts/null_compare.query]
---
# Implicit pointer-to-bool in conditions: rewrite and enforce

**Problem.** House style (Ruslan, 2026-10-02) is `if (ptr)` / `if (!ptr)`, never
`ptr != nullptr` in a condition. `readability-implicit-bool-conversion` with
`AllowPointerConditions` permits the implicit form but does not flag the explicit one,
and no clang-tidy check does. `scripts/null_compare.query` finds 111 such comparisons
in `src/` and `include/` (raw and smart pointers in `if`/`while`/`for`, `!`, `&&`,
`||`, `?:`). The 8 others (`return p != nullptr;`) stay explicit, since `return p;` is
the implicit conversion the tidy check rejects.

**Proposal.** A script that runs the matcher per TU, keeps matches in the repo, and
rewrites `X != nullptr` to `X` and `X == nullptr` to `!X` (parenthesising `X` unless it
is a postfix expression), applied once and reviewed. The 0054 CI job runs the matcher on
changed files and fails on a match. If clang-query proves too coarse (it reports start
locations only), turn the matcher into a clang-tidy plugin check with fix-its, built
against the same clang-tidy version CI pins.

**Done when** the matcher reports nothing on `src/`, `include/` and `test/`, and the PR
job fails on a new `if (p != nullptr)`.
