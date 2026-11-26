---
status: in-progress
priority: low
area: parser
depends: [0564]
touches: [src/template_parser.cpp, src/template_parser.h#StatementsParser, src/expression_evaluator.cpp#TryFoldTruth, src/expression_parser.cpp#AddFoldCandidate]
shares: [test/big_int_test.cpp, test/limits_test.cpp, test/parity/cases/subscripts.py]
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

**As built (batch 13 B).** `StatementsParser::Parse` works out `InVolatileCode` once per tag (`m_volatile`) and
every statement's ExpressionParser takes it (`SetVolatile`), `print` included. `AddFoldCandidate` records no
candidate in volatile code except an output's whole value, which visit_Output folds itself: so a tuple-slice
subscript inside a volatile statement, or inside a part of a volatile output, stays unfolded and fails the Load as
in Jinja2 (corpus `subscripts.tuple_of_slices_volatile_*`; the output part was a divergence too, not listed).
`TryFoldTruth` rethrows HostUnrecoverableError, bad_alloc, ResourceLimitError and RecursionLimitError as
OutputFoldsAt does; the Load's fold check maps a limit there to "does not fold" where TryFold maps it to Impossible
(`FoldEvaluator::Decides`), so a check of marks only behaves as before. Pinned by
`LimitsEnvTest.FoldsAtLoad` (an `and`, `or` and conditional test past `maxSequenceSize` fail the Load with
ResourceLimitExceeded, not as an unfolded subscript) and the volatile rows of
`BigIntTemplate.StrDigitsConstantsFailTheLoad`.
