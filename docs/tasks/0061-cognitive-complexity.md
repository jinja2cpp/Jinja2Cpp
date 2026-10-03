---
status: open
priority: low
area: style
depends: [0054]
touches: [src/string_converter_filter.cpp, src/value_methods.cpp, src/python_format.cpp, src/filters.cpp, src/template_parser.h, src/testers.cpp, src/expression_parser.cpp, src/expression_evaluator.cpp]
---
# Bring functions under cognitive complexity 25

**Problem.** `readability-function-cognitive-complexity` is on at 25 (Ruslan's choice).
Of 743 scored functions, 35 exceed it, 13 exceed 50 and 3 exceed 100: `WordWrap` 158
(`src/string_converter_filter.cpp:111`), `FormatValue` 155 (`src/value_methods.cpp:715`),
the urlize filter 132 (`src/string_converter_filter.cpp:955`), `Directive` 98
(`src/python_format.cpp:225`), `filesizeformat` 89 (`src/filters.cpp:1539`), `Format` 88,
`HtmlUnescape` 84, a filter at `src/filters.cpp:627` 80, `SplitImpl` 67,
`MarkMacroSpecialNames` 63, a tester at `src/testers.cpp:271` 61, `ParseSubscript` 57,
`ParseCallParamsImpl` 54.

**Proposal.** In 0057, mark the 35 with
`// NOLINT(readability-function-cognitive-complexity): score N, see 0061`, a greppable
debt list, so the PR job holds new functions to 25 at once. Then split one function per
PR, largest first, with the parity corpus and unit tests as the safety net; each split
deletes its NOLINT. When the list is short, step the threshold down (20, then 15).

**Done when** no NOLINT for this check remains at threshold 25.

**Progress.** The 35 markers landed with 0065
(`// NOLINTNEXTLINE(readability-function-cognitive-complexity): score N, split in
docs/tasks/0061`; `grep -rn "split in docs/tasks/0061" src` lists them). The check is in
`WarningsAsErrors`, so a new function over 25 now fails the pull-request job. Left: the
splits, one per PR, largest first.

- `Directive` (`src/python_format.cpp`, 98): split into `Key`, `ParseSpec` and one
  conversion function per family (`ConvertText`, `ConvertInteger`, `ConvertFloat`,
  `ConvertChar`); 33 markers remain.
