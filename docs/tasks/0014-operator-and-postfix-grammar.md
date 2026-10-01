---
status: open
priority: high
area: parity
depends: [0001]
touches: [src/expression_parser.cpp, src/expression_parser.h, src/template_parser.cpp, src/template_parser.h, src/expression_evaluator.cpp, src/expression_evaluator.h]
---
# Operator and postfix grammar

**Problem.** Common Jinja2 expressions do not parse: chained comparisons `a < b < c`,
`not in`, `is not test`, test arguments without parentheses (`is divisibleby 3`,
`is sameas none`), tests named by keywords (`is none`, `is true`), slices `x[1:]`,
`s[::-1]`, numeric attributes `l.0`, subscripts after a literal or a call (`'abc'[0]`,
`range(5)[2]`), parenthesised tuple targets `for (a, b) in`, tuple targets in `set a, b =`
and `{% with %}` without assignments. Each is a load-time error (30 cases).

**Proposal.** Follow Jinja2's grammar (`jinja2/parser.py`): parse comparisons as a chain,
`not in` as a compound operator, `is [not] name [arg]` with the single-argument form,
postfix operators (subscript, slice, call, attribute) in a loop over any primary, and
assignment targets as tuples. Evaluate slices with Python semantics on lists and strings
(negative and omitted bounds, negative step). This shares files with 0013, so run them one
after the other, not in parallel.

**Done when.** No line of `test/parity/divergences.txt` names task 0014, and `ctest -R parity` passes.

**Next.** Several of these cases will then reveal value-level divergences (string slicing
depends on 0016, printed results on 0012); move their divergence lines to those tasks.
