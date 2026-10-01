---
status: in-progress
priority: medium
area: style
---
# Converge the tree on one clang-format style

**Problem.** The committed code follows no single clang-format configuration. The old
`.clang-format` did not even parse (a duplicated key), so it described intent, not reality.
A style gate on the whole tree would demand thousands of unrelated changed lines; no
gate lets every PR add its own style.

**Measurement (Oct 2026).** Lines that clang-format would change, vendored files excluded
(19.7k lines in `src/`+`include/`, 6.5k in `test/`):

| config | src+include | test | total |
|---|---|---|---|
| derived from committed code (now in `.clang-format`) | 1202 | 1622 | 2824 |
| derived + `ColumnLimit: 160` | 4479 | 1937 | 6416 |
| derived + `ColumnLimit: 120` | 4398 | 1978 | 6376 |
| Microsoft defaults | 4448 | 1979 | 6427 |
| WebKit defaults | 7387 | 2031 | 9418 |
| Mozilla defaults | 13930 | 3679 | 17609 |
| LLVM defaults | 15865 | 4071 | 19936 |

The derived config was found by coordinate descent over ~55 options, scoring each
candidate by lines changed. Results are the same with clang-format 18 and 23.

What is left under the derived config:
- `test/`: hand-aligned `InputOutputPair` tables inside `INSTANTIATE_TEST_SUITE_P`, which
  clang-format cannot reproduce. Most of the residual.
- `src/`: `case` labels indented in some files and not others; continuation indent of 2
  in some places and 4 in others; trailing whitespace.
- `ColumnLimit: 0` (keep the author's line breaks) is the biggest single win. Any hard
  limit re-wraps thousands of hand-broken lines.

**Proposal.**
1. Done in PR #291: derived `.clang-format`, `.clang-format-ignore` for vendored code, CI
   checks only lines a PR touches.
2. Mark data tables in `test/` with `// clang-format off/on`.
3. One mechanical commit that formats `src/` and `include/` with the derived config
   (about 1.2k lines), listed in `.git-blame-ignore-revs`. After that a whole-file check can
   replace the changed-lines check for those directories.
4. Optionally, later: a second mechanical commit that adopts a column limit. It is a
   readability decision, so it waits for the maintainer.

**Done when.** `clang-format --dry-run --Werror` passes on `src/` and `include/`.
