---
status: open
priority: medium
area: parity
touches: [src/expression_parser.cpp, src/expression_evaluator.cpp#CallExpression]
shares: [test/parity/divergences/statements.txt]
---
# `f(*args, **kwargs)` in calls

**Problem.** Found by the 0188/0196 verifier. Jinja2 accepts Python's argument unpacking in
any call: `range(*[1, 3])` renders `[1, 2]` and `m(**{'a': 1, 'b': 2})` passes keywords to a
macro. Jinja2C++ fails at Load with `Unexpected token: '*'` (`statements.call_star_args`,
`statements.call_star_kwargs`). Templates that forward argument lists between macros
(`caller(*varargs)`, `m(**kwargs)`) cannot be written.

**Proposal.** Parse `*expr` and `**expr` after the other arguments, as Jinja2's parser does
(`*` after keywords is allowed, a second `*` is not), keep them in `ArenaCallParams`, and
splice them into `CallParams` when the call is evaluated, raising Python's TypeErrors for a
non-iterable `*` value, a non-mapping `**` value and a keyword given twice. The fast paths
that read `posParams` directly (`LoopAttrExpression::TryCallCycle`, the method calls) must
see the spliced arguments or step aside.

**Done when.** No line of `test/parity/divergences/` names 0212.
