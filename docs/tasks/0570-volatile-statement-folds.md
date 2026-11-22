---
status: open
priority: low
area: parser
depends: [0564]
touches: [src/template_parser.cpp, src/expression_evaluator.cpp#TryFoldTruth]
---
# Statement expressions in volatile code, and what TryFoldTruth catches

**Problem.** Found by the verifier of batch 12 A. (1) Jinja2's compiler skips the optimizer in a volatile frame
(`{% autoescape x %}` with a non-constant value, compiler.py `optimizeconst`). The expression parser knows that
only for outputs (`ExpressionParser::SetVolatile`, set by `{{ }}` and `print`); a statement's parser does not, so
`{% autoescape x %}{% set q = 10**4300 %}{% endautoescape %}` (boost backend) fails the Load with Python's
ValueError text, which Jinja2 loads (it compiles the power unfolded), and tuple-slice folds in such statements are
decided as if the code were not volatile. (2) `TryFoldTruth` (src/expression_evaluator.cpp) catches every
`std::exception`, so a `std::bad_alloc`, ResourceLimitError or RecursionLimitError while deciding an `and`/`or`
or conditional fold turns into "does not fold" instead of failing as TryFoldValue lets them.

**Proposal.** Pass InVolatileCode to each statement's ExpressionParser (the 17 sites in template_parser.cpp, or
once through the StatementsParser), and rethrow the limit and allocation errors in TryFoldTruth as
OutputFoldsAt does; check the render-path cost of the latter (TryFoldTruth runs in IsFoldableAt at render).

**Done when.** The `set` row above loads with a boost backend, a test pins a limit error raised inside an `and`
fold, and preflight's instruction gate stays within noise.
