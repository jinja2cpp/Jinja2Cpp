---
status: open
priority: medium
area: perf
touches: [src/expression_parser.cpp, src/expression_parser.h, src/error_handling.h]
---
# Every operand descends all eleven precedence levels at Load

**Problem.** Found re-profiling the MiniJinja Load gap (0136) on master 50dec81. The
expression parser is one function per precedence level: `ParseTupleOrExpression` →
`ParseFullExpression` → `ParseLogicalOr` → `And` → `Not` → `Compare` → `PlusMinus` →
`StringConcat` → `MulDiv` → `Pow` → `UnaryPlusMinus` → `ParseValueExpression`. A bare
name or literal, which is most operands in a template, passes through all of them. Each
level builds an `OperatorChain` (the recursion limit) and returns a
`ParseResult<NodeRef<Expression>>`, an `expected` whose error side is a `ParseError`
holding a `Token` (with a 72-byte `InternalValue`, 0140) and a vector of related tokens,
so every level moves and destroys a large object to pass a 4-byte `NodeRef` up.

Self cost of the eleven level functions plus the `expected` operations around them
(callgrind, `jinja2cpp_bench --count`, Release):

| Case | Load instructions | Levels | `expected` ops | Together |
|---|---:|---:|---:|---:|
| `Load/chat_llama` | 554,741 | 8.3% | 2.1% | 10.4% |
| `Load/many_tags` | 13,197,410 | 8.8% | 2.3% | 11.1% |
| `Load/substitute` | 10,713 | 9.8% | 1.7% | 11.5% |

In `substitute` that is about 720 instructions per `{{ name }}` before the parser
reaches the name. MiniJinja's parser is recursive descent too, but its levels pass a
`Result<ast::Expr>` whose error is a boxed pointer.

**Proposal.** Either or both, measured separately:
1. Precedence climbing for the binary levels (`or` down to `**`): one loop driven by a
   table of (operator, precedence, associativity), so an operand without an operator
   costs one call and one table probe. Comparison chains, `not`, unary minus and the
   string-concatenation `~` keep their own rules; the parity corpus and the
   expression-parser tests pin them, and the recursion limit
   (`MaxExpressionOperators`, `RecursionLimitExceeded`) and fuzz depth limits (0003)
   must still hold.
2. A small error side for `ParseResult`: keep the error out of line (one pointer, or an
   index into the parser's error list) so success moves 8 bytes.

**Done when.** `bench/count.py --baseline` on master: Load -6% or better on
`chat_llama`, `many_tags` and `substitute`, no Render change, the parser tests and the
corpus unchanged, the sanitizer and fuzz configurations clean.

**Next.** With 0142 (one scanner) and 0118 P5c (a Seal that copies instead of
relocating each node), the remaining Load gap to MiniJinja should be re-measured
(0136 estimates about 0.85x after all three).
