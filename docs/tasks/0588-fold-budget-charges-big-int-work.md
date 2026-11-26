---
status: open
priority: low
area: robustness
depends: [0569]
touches: [src/big_int.cpp#Power, src/expression_parser.h#FoldBudget]
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
