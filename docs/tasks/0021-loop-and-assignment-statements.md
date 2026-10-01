---
status: open
priority: high
area: parity
depends: [0001]
touches: [src/statements.cpp, src/statements.h, src/template_parser.cpp, src/template_parser.h, src/internal_value.cpp, include/jinja2cpp/template_env.h]
---
# Loop controls, loop object, namespace, tuple assignment

**Problem.** `{% break %}` and `{% continue %}` (the loopcontrols extension) do not parse;
`loop.revindex`, `loop.revindex0`, `loop.changed()` and `loop.depth` are missing;
`namespace()` cannot be assigned to (`{% set ns.c = ... %}` reports "not supported"),
which is the only way to carry state out of a loop in Jinja2; `{% set a, b = [1, 2] %}`
assigns nothing; `do l.append(x)` parses but cannot mutate (11 cases).

**Proposal.** Add `break`/`continue` statements behind an `Extensions::LoopControls` flag
next to `Extensions::Do`, so that without it they stay an error as in Jinja2; complete the loop object,
implement `namespace` as a mutable mapping with attribute assignment, and tuple
assignment from any sequence. `do` mutation depends on the reference semantics designed
in 0020.

**Done when.** No line of `test/parity/divergences.txt` names task 0021, and `ctest -R parity` passes.
