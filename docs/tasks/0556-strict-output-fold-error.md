---
status: done
priority: low
area: parity
depends: [0546]
touches: [src/expression_parser.cpp#FoldCheckPasses]
shares: [test/parity/cases/subscripts.py, test/parity/divergences/subscripts.txt]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/987
---
# Strict output whose fold fails with an error that is no UndefinedError

**Problem.** Under StrictUndefined an output's fold candidate fails the Load whenever its evaluation fails
(`FoldCandidate::failsLoad` is always true for an output). Jinja2's `visit_Output` catches every exception of
the output's as_const and compiles the output unfolded, so only the optimizer's own UndefinedError (an `and`,
`or`, `~` or conditional inside it) fails the compile. `{% if false %}{{ ([1][:, :] is undefined)|length }}{% endif %}b`
renders `b` in Jinja2 and fails the Load here ("']' expected"); the 0546 verifier found 32 such templates
(`[1:]`, `|max`, `is sameas true` on a bool), on master too. Corpus
`subscripts.tuple_of_slices_strict_output_fold_error_unrendered` (rejects).

**Proposal.** Treat an output candidate's failure as the non-output candidate's (Impossible unless
`LetsUndefinedOut`), while the tuple-of-slices subscript inside it still needs a covering fold: check against
Jinja2 which node covers it when the output does not fold (the optimizer's fold of the operand) before
changing the rule.

**Done when.** The corpus line is deleted and the existing `tuple_of_slices_strict_output_*` cases still match.

**As built (batch 12 A).** An output candidate never fails the Load: a failing evaluation, a strict undefined or
a value str() refuses only means it does not fold (`FoldEvaluator::OutcomeOf`), and its non-output duplicate,
evaluated next, decides as Jinja2's optimizer does once visit_Output gives up. `FoldCandidate::failsLoad` is false
for outputs.
