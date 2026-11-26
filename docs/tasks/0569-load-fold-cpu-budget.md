---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1009
priority: low
area: robustness
depends: [0564]
touches: [src/template_impl.h#FailedFold, src/expression_parser.cpp#FoldEvaluator, src/expression_parser.h#FoldBudget, src/recursion_guard.h#RenderLimits, src/big_int.cpp#IntBitsCap]
shares: [test/big_int_test.cpp, test/parity/cases/literals.py, test/parity/divergences/literals.txt, docs/security.md]
---
# Bound the CPU the Load's fold check spends on dead constant code

**Problem.** Found by the verifier of batch 12 A (0555, 0564). The fold check evaluates, at Load, every candidate
around a fold mark: since 0564 any `*` or `**` of constants, and since 0555 under StrictUndefined any built-in
filter or subscript of a constant. Python folds the same way when it compiles, so parity holds, but template text
now controls Load CPU in code that never renders. Release, boost backend: `{% if false %}{{ 1 if 3**600000 else 2
}}{% endif %}` loads in 1.1 s (0.007 s before); 100 roots of `{% if false %}{{ ("a"|upper) * 67108864 }}{% endif %}`
(4.8 KB) take 2.8 s and 64 MB of allocation per root. Allocation stays under MaxSequenceSize and the 2^20-bit cap,
and the fold runs under the render limits only when the template has some (`hasRenderLimits`); a power counts as
one step. The class existed before behind a tuple-slice subscript (20 roots took 0.34 s).

**Proposal.** Give the Load's fold context a budget of its own when the template has no render limits (steps,
and the operand size of `**` and repetition), past which a candidate is Impossible (does not fold) rather than
evaluated; check that a check of marks only then leaves the value to the render, as FoldEvaluator::TryFold does
for limits.

**Done when.** The two templates above load in under 10 ms with default settings, and the corpus still matches.

**As built (batch 13 B).** A template whose `Settings::Limits` are all defaults runs its fold checks under
`FoldBudget` (src/expression_parser.h): each check (a root) gets 64 steps of its own plus a pool of 2^14 steps shared
by the whole Load, items or characters per operation 2^16, 16 MB of values, and products and powers of at most 2^16
bits (`RenderLimits::maxIntBits`, read by big_int.cpp instead of MaxBigIntBits). Any limit reached under the budget
makes the candidate Impossible in every check (`FoldEvaluator::m_limitIsImpossible`), so a literal past 4300 digits
still fails the Load however many roots come first, while a computation past the budget is left to the render.
A template with limits of its own folds under those, as before; `maxRenderSteps` of UINT64_MAX, which only counts
steps for a host's poll (the Python module sets it for Ctrl-C), is no limit there. Release, boost: the two templates above load in
0.4 ms and 2.4 ms. A candidate whose evaluation, or a step it reads, reaches a limit does not fold and reports no strict undefined
(`FoldEvaluator::LimitedOutcomeOf`; verifier round 1 found a limit escaping `LetsUndefinedOut` and
`ConditionalOf` under StrictUndefined and failing the Load). Once the check's steps or memory are spent
(`detail::StepsSpent`), every later candidate of the check does not fold either; a size limit stops only the
candidate that reached it, so a fold beside it still folds (verifier round 2).
Deliberate divergences, past the budget only: a power or product past 2^16 bits whose value has more than 4300
digits loads (corpus `literals.int_power_past_fold_budget_unrendered`, accepts); a tuple-slice subscript, or an
integer literal past 4300 digits, whose covering fold exceeds the budget fails the Load as unfolded where Jinja2
computes the fold (`{{ ()[:, :] ~ ('a' * 100000)|first }}`; after roots that drained the pool,
`{{ (0x<3572 f>)|abs|...80 filters... is odd }}`; and `{{ (0x<20000 f>) * 1 > 5 }}`, an 80,000-bit product, which
master's 2^20-bit cap allowed). Charging big-integer work by its size is 0588.
