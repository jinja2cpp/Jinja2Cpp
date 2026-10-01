---
status: done
priority: high
area: parity
depends: [0001, 0013, 0016]
touches: [src/expression_parser.cpp, src/expression_parser.h, src/expression_evaluator.cpp#postfix, src/internal_value.cpp#Subscript]
shares: [src/expression_evaluator.cpp, src/expression_evaluator.h, src/internal_value.cpp, src/template_parser.cpp, src/template_parser.h]
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

**Also (found in 0013).** Attribute access on a number literal is lexed as one float:
Jinja2 reads `1.e3` as `1` then attribute `e3` and `1.5.2` as `1.5` then attribute `2`
(both render as undefined); Jinja2C++ turns `1.e3` into `1000.0` and rejects `1.5.2`.
The fix belongs with numeric attributes `l.0`: the lexer should end a number before a
dot that is not followed by a digit and after a second dot.

**Scheduling.** Owns every precedence fix in the expression grammar, including `not a == b` (listed under 0015) and `is` versus `and`/`or` (listed under 0017); move those divergence lines here. String slicing uses the code-point indexing from 0016. `set a, b =` parsing is here; 0021 makes the assignment work.

**Done when.** No line of `test/parity/divergences/` names task 0014, and `ctest -R parity` passes.

**Next.** Several of these cases will then reveal value-level divergences (string slicing
depends on 0016, printed results on 0012); move their divergence lines to those tasks.

**Result.** The expression parser follows `jinja2/parser.py` level by level: `or`, `and`,
`not`, comparison chains (`CompareExpression` evaluates each operand once; a single
comparison stays a `BinaryExpression`), `+ -`, `~`, `* / // %`, left-associative `**`
above unary `+ -`, then postfix (attribute, numeric attribute, subscript, slice, call) on
any primary, then filters, `is [not] test [arg]` and calls. This also fixed precedence
bugs nobody had listed: `2 + 3 ** 2` was 25, `**` was right-associative, the right side
of `~` swallowed `and`, and `not x|length` applied the filter to `not x`. Slices use
CPython's index adjustment on lists, tuples (the result stays a tuple) and strings (by
code point). The lexer ends a number before a dot not followed by a digit and reads a
number right after a dot as an integer (`1.e3`, `1.5.2`, `l.0.1`). Parenthesised tuples
now evaluate as tuples (the leftover `literals.tuple` line of 0013). `{{ a, b }}`,
`for x in a, b` and `set x = a, b` build implicit tuples; `{% with %}` may have no
targets; `for (a, b) in` parses. Moved on: the missing tests behind `is none`,
`is sameas`, `is divisibleby 3`, `is float` (0017), `dict()` (0030), assigning
`set a, b` (0021), filters on bools (0019, `filters.filters_on_bool`) and slice errors
(0015, `subscripts.slice_step_zero`), filtered loops that unpack by name and nested
`for` targets (0021), and the parser's missing depth limit (0003). `l[a, b]` and `l[]`
index with a tuple and `{% if a, b %}` tests a tuple, as in Jinja2. Deliberate
divergence: `in`, `is`, `if` and `else` stay reserved, so `{{ in }}` is a parse error
where Jinja2 reads an undefined name.
