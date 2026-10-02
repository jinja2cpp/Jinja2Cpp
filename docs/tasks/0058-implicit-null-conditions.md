---
status: done
priority: medium
area: style
depends: [0054, 0070]
touches: [src/, include/jinja2cpp/, scripts/null_compare.query, scripts/null_compare.py, .github/workflows/clang-tidy.yml]
---
# Implicit pointer-to-bool in conditions: rewrite and enforce

**Problem.** House style (Ruslan, 2026-10-02) is `if (ptr)` / `if (!ptr)`, never
`ptr != nullptr` in a condition. `readability-implicit-bool-conversion` with
`AllowPointerConditions` permits the implicit form but does not flag the explicit one,
and no clang-tidy check does. `scripts/null_compare.query` finds 111 such comparisons
in `src/` and `include/` (raw and smart pointers in `if`/`while`/`for`, `!`, `&&`,
`||`, `?:`). The 8 others (`return p != nullptr;`) stay explicit, since `return p;` is
the implicit conversion the tidy check rejects.

**Proposal.** `scripts/null_compare.py` (0054) runs the matcher per TU and lists the
matches in the repository; CI runs it on changed lines in report-only mode. This task
adds a `--fix` mode that rewrites `X != nullptr` to `X` and `X == nullptr` to `!X` (parenthesising `X` unless it
is a postfix expression), applied once and reviewed. Then add `--fail` to the step in
`.github/workflows/clang-tidy.yml` so a new match fails the PR. If clang-query proves too coarse (it reports start
locations only), turn the matcher into a clang-tidy plugin check with fix-its, built
against the same clang-tidy version CI pins.

**Done when** the matcher reports nothing on `src/`, `include/` and `test/`, and the PR
job fails on a new `if (p != nullptr)`.

**Done (2026-10-02)** in [#339](https://github.com/jinja2cpp/Jinja2Cpp/pull/339). `--fix` rewrote 83 sites. 25 of the 111
were not conditions in tidy's sense (`return p != nullptr || q != nullptr;`,
`bool b = p != nullptr && x;`): there the implicit form is the pointer-to-bool conversion
`readability-implicit-bool-conversion` rejects, so the query now follows that check's
`AllowPointerConditions` rule (parentheses, `!`, `&&`, `||` up to an `if`/`while`/`for`/`do`
or `?:` condition) and those sites stay explicit. Three tests of a `bool*` variable
stay explicit too: `if (flag)` reads as the flag's value, and
`bugprone-bool-pointer-implicit-conversion` warns on it, so the query skips them. clang-query's range underline was
enough for the rewrite; no plugin check needed. CI passes `--fail`.
