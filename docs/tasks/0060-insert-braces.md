---
status: done
priority: medium
area: style
depends: [0054, 0070]
touches: [.clang-format, src/, include/jinja2cpp/]
---
# Braces around every single-statement body

**Problem.** Ruslan chose (2026-10-02) to require braces for single-statement `if`,
`else`, `for` and `while` bodies; today braceless bodies are the dominant style
(`readability-braces-around-statements`: 1 596 hits in `src/` + `include/`).

**Proposal.** Set `InsertBraces: true` in `.clang-format` and reformat `src/` and
`include/` in one mechanical commit, like 0009. The format gate already checks those
directories whole, so enforcement is immediate. With the current `BraceWrapping`
(braces on their own lines) this adds about 3 300 lines; changing that axis is a separate
decision. `test/` follows on touched lines only, as the format gate does today. clang-format
warns that `InsertBraces` can break code around macros, so the PR proves itself with the
full CI matrix and the sanitizer build. Run while no other PR touches `src/`.

**Done when** `.clang-format` has `InsertBraces: true` and the format gate is green.

**Done (2026-10-03)** in [#345](https://github.com/jinja2cpp/Jinja2Cpp/pull/345): `InsertBraces: true`, one
mechanical commit over `src/` and `include/` (52 files, +3 280 lines, no other change),
listed in `.git-blame-ignore-revs`. The two Unicode header generators emit the braces
too, so `--check` still matches. Verified locally with GCC Debug, the nlohmann bindings and
clang ASan+UBSan before the CI matrix.
