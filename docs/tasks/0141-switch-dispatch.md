---
status: open
priority: low
area: perf
touches: [src/expression_evaluator.h, src/expression_evaluator.cpp, src/statements.h]
---
# Every node visit is a virtual call

**Problem.** The evaluator walks a tree of virtual nodes: one vtable load and one indirect
branch per node. MiniJinja and Tera 2 compile to flat bytecode instead.

**Proposal.** Experiment only, after 0140: dispatch the hottest node kinds with a `switch` on
the 1-byte kind 0118 P3 adds, keeping virtual calls for the rest. Trigger: I1 misses or
branch mispredictions still a top cost in 0137's columns after 0140. Bytecode is not
considered before this experiment.

**Done when.** Measured; kept or rejected with numbers.
