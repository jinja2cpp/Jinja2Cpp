---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/1020
priority: low
area: robustness
depends: [0569]
touches: [src/big_int.cpp#Power, src/expression_parser.h#FoldBudget, src/recursion_guard.h, src/recursion_guard.cpp, src/template_impl.h#FailedFold]
---
# Charge the Load's fold budget for the size of big-integer work

**Problem.** Found by the verifier of batch 13 B (0569). The fold budget (FoldBudget) counts a product or power
as one step and caps its result at 2^16 bits, so each root's 64 own steps can still multiply 2^16-bit integers 64
times. Release, boost backend: `{% if false %}{{ ((3**20000*3**20000) % 7) + ... }}{% endif %}` (ten such terms)
repeated 100 times, 32.8 KB, loads in 7.5 s and 400 repeats (131 KB) in 31 s; Python 3.12 needs 0.81 s for the
100-repeat file. Master takes the same time, so 0569 did not make it worse, but the budget does not yet bound Load
CPU by template size for big-integer arithmetic.

**Proposal.** Under the budget, charge a product, power, division or radix conversion steps in proportion to its
operands' words (for example bits / 64 per operand, or the backend's quadratic cost), drawn from the same per-check
and Load-wide steps, so that a candidate past them is Impossible as for any other limit.

**Done when.** The 100-repeat template above loads in under 50 ms on the Release boost build, and the corpus and
`BigIntTemplate.FoldBudgetBoundsDeadConstants` still pass.

**Progress.** In the fold check (`RenderLimits::bigIntWorkPerStep`, set to
`FoldBudget::WordProductsPerStep` = 1024 only there) a product or power step, a floor division or
modulo, and a backend decimal conversion (`fromDecimal`/`toDecimal`) take one step per 1024 products
of the operands' 64-bit words (`ChargeBigIntWork` in `src/big_int.cpp`, through the new
`detail::ChargeSteps`), before the work; past the budget the candidate is Impossible as for any limit.
Renders charge nothing new. With the 0543 P4 word entries the huge results of the template no longer
convert to decimal either. Release, boost build, median of 7: the 100-repeat template (36.8 KB) loads
in 15.7 ms (was 7.5 s), 400 repeats (147 KB) in 34 ms (was 31 s). Test
`BigIntTemplate.FoldBudgetChargesBigIntWork` (roots of big products spend the Load's steps, roots of
small ones do not; the 0588 template loads); `FoldBudgetBoundsDeadConstants` and the corpus pass.
