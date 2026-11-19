---
status: open
priority: low
area: parity
depends: [0536]
touches: [src/expression_parser.cpp#FoldCheckPasses]
shares: [test/limits_test.cpp]
---
# Tuple-of-slices fold check: argument steps, subscript levels, kept values and the per-render constant

**Problem.** Left by 0536, which links filter, binary and unary candidates as steps of a chain at Load:

- a candidate inside a filter's or call's argument, or a subscript level of nested brackets
  (`[1][:, :][0][0]...`), is no step of the candidate around it: the outer one evaluates it again, so
  nesting N levels evaluates about N^2/2 nodes (bounded by maxExpressionDepth and the render limits);
  comparison chains and tests (`x is y`) are not linked either;
- a kept step value lives until the check ends: a chain of large values (`|center(3000000)` over
  hundreds of steps) holds them all, bounded by maxExpressionOperators times the largest value;
- per render: Jinja2 replaces the folded subscript by its constant, while the sealed tree keeps a
  TupleSliceExpression that evaluates its operand and parts and builds the undefined's name each time
  it runs. Caching it needs per-template storage for the constants the Load computed, since sealed
  nodes are shared and immutable (Codex P2 on #865).

**Proposal.** Link SubscriptExpression and IsExpression candidates on their value operand as 0536 links
operators, drop a step's value once its last later step has read it, and measure whether the per-render
constant is worth a per-template table.

**Done when.** A strict nesting of N subscript levels around a tuple-of-slices subscript evaluates O(N) nodes
at Load (a LimitsEnvTest step-limit test), and the kept values are freed as the chain passes them.
