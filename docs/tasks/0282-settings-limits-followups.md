---
status: open
priority: low
area: api
depends: [0262]
touches: [src/recursion_guard.h, src/recursion_guard.cpp, src/template_parser.cpp, src/filters.cpp, docs/tasks/0118-parse-tree-arena.md, docs/tasks/0152-expression-precedence-descent.md, docs/design/0117-name-slots-plan.md]
---
# `Settings::Limits` follow-ups (0262 leftovers)

Found while `Settings::Limits` was built and verified (0262). None is in its scope. Memory bounds
(operations that grow strings or lists without checking `maxSequenceSize`, captured output) are
0263's work and not repeated here.

1. **One step may overrun `maxRenderTime`.** The clock is polled every 16 steps, after host
   callables and at render end, but nothing interrupts a step. A filter over a string near the
   2^26 cap runs for seconds in an ASan build. Long filters (`replace`, `upper`, `center`,
   `wordwrap`, ...) could count their work in steps, or poll the clock every N characters, while a
   render with limits runs.
2. **`maxBlockNesting = 0` parses deep nesting in quadratic time.** Without a block limit, the
   stack check is the only bound, and the 0262 verifier saw parse time grow with the square of the
   depth. Find the walk that causes it and make it incremental, or document 0 as "trusted input
   only".
3. **The depth counters are per thread.** A nested render started from a host callable gets its own
   `maxRenderDepth` budget but shares the thread's stack check with the outer render, so the inner
   render can fail with `RecursionLimitExceeded` before its own limit. Decide whether that is the
   contract (the stack is real) and document it in `template_env.h`.
4. **Stale names in docs.** `docs/tasks/0118`, `docs/tasks/0152` and
   `docs/design/0117-name-slots-plan.md` still name `MaxExpressionOperators`, `MaxExpressionDepth`
   and `MaxRenderDepth`, which 0262 replaced with `Settings::Limits` fields.

**Trust model.** When this lands, update its entry under Known gaps in [docs/security.md](../security.md) (0224).
