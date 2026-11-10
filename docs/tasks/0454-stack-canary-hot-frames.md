---
status: open
priority: medium
area: perf
depends: [0265]
touches: [src/expression_evaluator.cpp, src/value_visitors.h]
---
# Hot frames that gain a stack canary under -fstack-protector-strong

**Problem.** 0265 turned `-fstack-protector-strong` on by default (perf track's hardening
exception). Against a build with no hardening it costs +3.03% on Load and +4.64% on Render
(`bench/count.py`, g++-13 -O3, geomean), worst `Render/expressions` +8.94%
(docs/tasks/0265, per-row data in the project files under 0265/). The flag puts a canary in each
function with a local array, a local whose address is taken, or an aggregate on the stack, which
the evaluator's hot visitors are likely to have.

**Proposal.** Find which hot frames gained the canary: callgrind of `Render/expressions` with and
without the flag, then `objdump -d` of those functions looking for `__stack_chk_fail`. Remove what
triggers it without dropping the flag: a local buffer moved to a member or a caller, an address-taken
local passed by value or returned instead, a large local split out of the hot path into a cold
function.

**Done when** `Render/expressions` and the Render geomean recover most of the +4.6% with the flag on
(target: the canary's share under +1%), measured with `bench/count.py`, and the flag stays on.
