---
status: in-progress
priority: low
area: parity
depends: [0098]
touches: [src/expression_parser.cpp, src/expression_evaluator.h, src/expression_evaluator.cpp, src/node_arena.h, src/node_arena.cpp, src/recursion_guard.h, test/recursion_limits_test.cpp, test/expressions_test.cpp, test/autoescape_test.cpp, test/undefined_test.cpp, test/node_arena_test.cpp, test/parity/cases/operators.py, test/parity/cases/autoescape.py, test/parity/cases/undefined.py, test/parity/expected/operators.json, test/parity/expected/autoescape.json, test/parity/expected/undefined.json]
---
# `~` chains as one node, as Jinja2's `Concat`

**Problem.** Found while re-tuning the depth limits against Python (0098). Jinja2 parses
`a ~ b ~ c ...` into one `Concat` node with a list of operands, so Python 3.12 renders a chain of
6000 `~` and more. Jinja2C++ builds a left-leaning tree of `BinaryExpression::StringConcat` nodes
(`ExpressionParser::ParseBinary`), so the chain counts against `maxExpressionOperators` (491) and
fails to load past it; `test/recursion_limits_test.cpp` `LongChains` pins the rejection of 20000
`'a' ~`. The tree also builds the result once per operator: `n` operands copy O(n^2) characters.
Comparison chains are already flat (`CompareExpression`).

**Proposal.** Parse a `~` run at one precedence level into a `ConcatExpression` holding its operands
in the node arena (as `CompareExpression` does), evaluate it into one string reserved to the sum of
the operand lengths, keep the folding (`SetFolded`) and the markup-join rules of `ConcatAsStrings`,
and do not count its operands as nested operators. Perf: `bench/count.py`, the chat templates use
`~`.

**Tests.** `LongChains`: a 20000-operand `~` chain renders (move it out of the rejected list);
markup join under autoescape and folded chains unchanged (existing rows).

**As built.** `ConcatExpression` (`NodeKind::ConcatExpr`, a trivial node) holds the operands of one `~`
run as an `ArenaSpan<NodeRef<Expression>>` and a `folded` flag. `ParseBinary` collects the operands of
consecutive `~` into a pending list and makes the node when another operator or the end of the call
follows; the run is folded (`SetFolded`) when every operand is foldable, as before. In the operator count
the first `~` counts as one operator (`OperatorChain::AfterRight`) and the later ones join the run
(`AfterRightFlat`: the deepest operand, no increment), so a 20000-operand chain loads and the operands'
own sub-expressions count as before. `Evaluate` evaluates every operand once and only then runs `CheckUndefinedUse` on each, as Jinja2 builds the operand list before joining (so `missing ~ f()` calls `f`, and `missing ~ 1 / 0` fails dividing);
escapes the non-Markup operands when any is Markup under autoescape (and not folded or volatile), and
builds one string of the template's width reserved to the sum of the operand lengths (the fallback renders
non-strings with `ValueRenderer`). `BinaryExpression` lost its `StringConcat` case and folded flag; the
enumerator stays because the parser's operator table returns it.
