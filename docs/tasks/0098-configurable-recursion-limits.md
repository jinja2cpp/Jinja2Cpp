---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/615
priority: low
area: api
depends: [0003]
touches: [src/recursion_guard.h, src/recursion_guard.cpp]
shares: [include/jinja2cpp/template_env.h, src/expression_parser.cpp, src/template_parser.cpp]
---
# Configurable recursion limits, closer to Python's

**Problem.** The limits of 0003 (`src/recursion_guard.h`) are compile-time constants:
expression nesting 64, operator depth 400, open blocks 128, render recursion 256, and a
stack reserve of 128 KiB (512 KiB under ASan). Two consequences:
- An embedder cannot choose them. One rendering untrusted templates on a worker thread
  may want a tight render depth that fails the same way on every platform, rather than
  at whatever depth its stack runs out; one rendering deep but trusted recursion (a tree
  of 1000 levels) cannot raise them at all.
- Where Jinja2C++ stops is near Python but not equal (measured against Jinja2 3.1.6 by
  the 0003 verifier): nested brackets 64 against about 68, nested `{% block %}` about 124
  against 240, `~` chains 400 against 500, and in Debug or ASan builds heavy recursion
  (200 macro levels, each evaluating a long expression) hits the stack reserve where
  Python renders.

**Proposal.** Add the four counts to `Settings` (defaults as today) and read them in the
parser and `RenderDepthGuard` instead of the constants; keep the stack check as the
backstop it is. Count `{% block %}` nesting apart from other blocks, as Python's limit for
blocks is twice that for `if`, and raise the defaults where the stack allows it. Document
the stack each default needs (2 MiB for 200 macro levels in Debug, less in Release).

**Done when.** The limits are settable per environment and covered by
`test/recursion_limits_test.cpp`, and the cases above render as in Python with the
default settings on an 8 MiB stack.

**Moved (2026-10-18).** The four counts become fields of `Settings::Limits` in 0262 (Ruslan chose
one nested struct, enforced at 2.0). This task keeps the default re-tuning against Python and the
separate `{% block %}` count.

**Done (this PR).** Measured with the oracle (Python 3.12, Jinja2 3.1.6): Python renders 69 nested
brackets (75 lists), 491 unary operators, 492 operands of `+`, 328 filters, and 247 nested `block`,
`with`, `filter` (246 `set` blocks); it stops nested `if` and `macro` at 98 (CPython's 100
indentation levels) and `for` at 20 (CPython's static block limit). Ruslan chose (2026-10-23) one
count for every block kind: defaults are now `maxExpressionDepth` 70, `maxExpressionOperators` 491,
`maxBlockNesting` 248, so Jinja2C++ stops exactly where Python does for parentheses, unary operators, `+` chains and
`block`/`with`/`filter` nesting, and renders the deeper `if`/`for` that CPython's compiler refuses.
Remaining differences, all near the boundary: nested list and dict literals stop at 69 (Python 75),
`set` blocks at 247 (Python 246), filter chains at 491 (Python 197 to 328). Under ASan a thread
stack no larger than the 512 KiB reserve is never checked (src/recursion_guard.cpp), so deep
nesting overflows it, as it did with the old limits. `maxRenderDepth` stays 256
(Python: 247 macro levels). `~` chains stay nested operators (Python's are flat): 0289. Tests:
`RecursionLimitsTest.NestingJinja2AcceptsRenders` and `NestedBlocks` check both sides of each
boundary.
