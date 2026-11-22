---
status: open
priority: low
area: robustness
depends: [0564]
touches: [src/template_impl.h#FailedFold, src/expression_parser.cpp#FoldEvaluator]
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
