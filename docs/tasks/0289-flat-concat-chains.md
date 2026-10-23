---
status: open
priority: low
area: parity
depends: [0098]
touches: [src/expression_parser.cpp, src/expression_evaluator.h, src/expression_evaluator.cpp]
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
