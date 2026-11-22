---
status: done
priority: low
area: parity
depends: [0543]
touches: [src/expression_parser.cpp, src/expression_parser.h, test/big_int_test.cpp]
shares: [test/parity/cases/literals.py, test/parity/divergences/literals.txt]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/987
---
# Reject integer constants past 4300 digits that survive folding

**Problem.** Found while planning 0543 (plan `/mnt/project-files/0543/plan.md`, phase P0 and
advisor note B4). Jinja2's code generator writes every constant that survives the optimizer with
`repr()`, so an integer constant past CPython's 4300-digit limit fails the compile with
ValueError, even where it never renders: a literal inside `{% if false %}` or a macro body,
`{% set x = 0x<3572 f> %}`, `x == 0x…`, `(0x…).bit_length()`, `{{ 10**5000 }}` and
`{% set x = ('f' * 3600)|int(base=16) %}` (a folded filter call). A constant that folds away
still loads: `{{ (0x…) is odd }}`, `((0x…) is odd) and x`, `[0x…]|length` and
`'%x'|format(0x…)|length` (jinja2 3.1.6 on CPython 3.12). Jinja2C++ loads all of them
(`RadixLiteralsAtTheBitCap` in `test/big_int_test.cpp` even relies on it). Corpus
`literals.int_hex_past_str_digit_limit_unrendered`, `literals.int_hex_past_str_digit_limit_set`
and `literals.int_filter_past_str_digit_limit_folded_set` (accepts);
`int_hex_past_str_digit_limit_folded` and `int_hex_at_str_digit_limit` pin the side that loads.

The boundary: `0x` with 3571 `f`s is 4300 digits and loads, 3572 is 4302 and fails;
`0b1` and 14284 zeros loads, 14285 ones fails.

**Proposal.** Follow the fold emulation (`FoldCheck`, `HasSafeRepr` and friends in
`src/expression_parser.cpp`), not the lexer, because `(0x…) is odd` must still load. Today
`FoldCheck` records candidates only around tuple-of-slices subscripts
(`src/expression_parser.h`), so this needs innermost-fold tracking for every huge constant and
folded result: a huge constant is an error at load unless an enclosing node folds it away.
Power-of-two literals know their bit count exactly; only the band near 14285 bits needs the
digit count (a 0.1 ms conversion). Check whether a big-integer backend is installed during the
load-time fold, since folded arithmetic results need one. Rewrite `RadixLiteralsAtTheBitCap` to
build its cap-sized value from data or through a folding expression.

**Done when.** The three corpus lines are deleted from `test/parity/divergences/literals.txt`,
the two loading cases still match, and the cap tests in `test/big_int_test.cpp` still cover the
2^20-bit limit.

**As built (batch 12 A).** The parser counts an integer literal past 4300 digits as a fold mark with the
tuple-slice subscripts (`FoldMarkKind::StrDigits`: it needs a fold around it, as a subscript does), and watches
(`FoldMarkKind::Watch`) the constants whose fold may give one: a literal within 10 digits of the limit, a `*` or
`**` of constants that repeats no literal sequence, and a built-in `|int` of a constant. The expressions around a
mark become fold candidates, and the Load's fold check fails with Python's ValueError text
(`ErrorCode::TemplateRuntimeError`) where a candidate folds to a value holding such an integer and no candidate
around it folds (`FoldOutcome::FoldsStrDigits`, `Repr::StrDigits`). In a check of marks only, a limit the fold
reaches is Impossible and left to the render, and without a big-integer backend an OverflowError fold takes the
integer away (Python computes it). `RadixLiteralsAtTheBitCap` reads its cap-sized value from data;
`BigIntTemplate.StrDigitsConstantsFailTheLoad` pins the boundary rows against Jinja2 3.1.6 on CPython 3.12.
Verifier round 1 fixes: no watch marks in volatile outputs; an output refuses an item past the limit whatever its
other items are (Repr flags); TryFoldTruth takes a filter's ErrorInfo. Left: statement expressions in volatile code
(0570), Load CPU on dead constant code (0569).
