---
status: done
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

## Result: measured, not built

Measured in PR #468 (closed unmerged, on master 544c5ab); the perf track rejected it.
Eight Render cases cost more than the +-0.5% instruction bar (up to +1.35%), and the only
gain is in callgrind's branch model, which real hardware's history-based (ITTAGE-class)
indirect predictors largely erase on an alternating text/`{{ }}` pattern. Instruction
counts are the gate because wall time in the cloud is noise, so a measured cost does not
buy an unmeasurable gain.

**Reopen if** a quiet machine with hardware counters shows a wall-time gain of at least 3%
on `mitsuhiko_table` or `macros` with no case worse than +1%. The code is on the closed
PR's commits (4434878).

There are no hardware counters in the cloud VM (no `cpu` PMU), and wall time there spreads
5-100% between runs, so the numbers are callgrind's: `Ir` and `--branch-sim=yes` (`Bcm`
conditional, `Bim` indirect mispredicts; a per-address last-target BTB, which a modern
history-based predictor beats, so the mispredict drop is an upper bound). Render, 5
iterations each, Release.

Where the base mispredicts: 95% of `Bim` on `mitsuhiko_table` (105,000 of 110,265) is the
one `nodes[r].Render(...)` call in `ComposedRenderer::Render`: a body alternates raw text
and `{{ ... }}`, and one shared indirect call site sees the target change every child.

**Tried (the narrow variant).** `ComposedRenderer::Render`, moved to `src/statements.cpp`,
switched on the child's kind byte: `RawText` and `ExprRenderer` called directly (and inline), every other kind through the
vptr. `ExpressionRenderer` rendered a plain name (`NameRef`) by a direct call to a new
`ValueRefExpression::Render`, which called its own `EvaluateRef`/`Evaluate` directly. CI
counts on 4434878:

| Render case | Ir | Mispredicts (Bcm + Bim), base -> head |
|---|---:|---:|
| `mitsuhiko_table` | -0.19% | 145,010 -> 42,747 (-71%) |
| `macros` | +0.24% | 37,141 -> 28,157 (-24%) |
| `for_loop_vars` | +1.35% | 12,959 -> 9,439 (-27%) |
| `many_tags` | +0.76% | 34,476 -> 28,044 (-19%) |
| `expressions` | +1.08% | 21,263 -> 19,554 (-8%) |
| `chat_llama` | +0.21% | 25,124 -> 24,291 (-3%) |

Every other Render case moves +0.10..+1.30% in Ir (`plain_text` +1.30%, `inheritance`
+1.07%, `substitute` +1.04%: about 13 instructions of fixed cost per render, since the loop
became an out-of-line function). Load was unchanged (-0.00..+0.04%) only because
`ComposedRenderer::Render` lived in `statements.cpp`. Trap for any retry: defined inline in
`renderer.h`, its larger body changed GCC's inlining in the parser files that include it,
and Load rose +0.06..+1.22% (`Load/expressions`: `PeekBinaryOperator` went out of line).
The added instructions are the two compares per body child (about 4 instructions); the call, prologue and return they replace
on text children do not offset them fully. Wall time (15 alternating runs pinned to one
core, `chat_mistral`, `for_loop_vars`, `macros`, `many_tags`, `mitsuhiko_table`): median
-2..-11%, minimum -3..+4%, inside this VM's noise; the verdict needs a quiet machine.

**Rejected first (the wide variant).** The same switch at every
`nodes[e].Evaluate`/`EvaluateRef` call site in the evaluator and statements (`NameRef` and `ScalarConstantExpr` by direct call): Ir +0.1..+5.5%
(`expressions` +5.54%: the inlined switches grew `BinaryExpression::Apply` past GCC's inline
limit and `BinaryMathOperation` went out of line), and mispredicts went up on `expressions`
(21,263 -> 23,256) because those sites see one kind almost always and already predict well.
Bytecode stays out of scope: the measured indirect-branch cost was one call site, and even
its removal does not pass the instruction gate.
