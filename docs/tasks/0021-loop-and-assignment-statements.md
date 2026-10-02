---
status: done
priority: high
area: parity
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/320
depends: [0001, 0014, 0020]
touches: []
shares: [src/statements.cpp, src/statements.h, src/template_parser.cpp, src/template_parser.h, src/internal_value.cpp, src/internal_value.h, src/global_functions.cpp, src/render_context.h, src/renderer.h, include/jinja2cpp/template_env.h]
---
# Loop controls, loop object, namespace, tuple assignment

**Problem.** `{% break %}` and `{% continue %}` (the loopcontrols extension) do not parse;
`loop.revindex`, `loop.revindex0`, `loop.changed()` and `loop.depth` are missing;
`namespace()` cannot be assigned to (`{% set ns.c = ... %}` reports "not supported"),
which is the only way to carry state out of a loop in Jinja2; `{% set a, b = [1, 2] %}`
assigns nothing; `do l.append(x)` parses but cannot mutate (11 cases).

**Also (found in 0014).** A loop with an `if` filter unpacks each item by name instead of
by position, so `{% for a, b in [[1, 2]] if a %}` renders nothing
(`statements.for_unpack_filter`, `ForStatement`'s filtered adapter in `src/statements.cpp`).
Nested tuple targets (`for (a, b), c in ...`) do not parse (`statements.for_nested_target`).

**Done in 0027.** Tuple targets now unpack by position from any iterable, with Jinja2's
count check, in `for` (filtered or not) and `set` (`statements.set_multiple`,
`set_unpack_list`, `for_unpack_filter` match). A mapping on the right still assigns its
values by name (`{% set first, last = person %}`), as Jinja2C++ always did; Python would
assign the keys. Nested targets are still open here.

**Proposal.** Add `break`/`continue` statements behind an `Extensions::LoopControls` flag
next to `Extensions::Do`, so that without it they stay an error as in Jinja2; complete the loop object,
implement `namespace` as a mutable mapping with attribute assignment, and tuple
assignment from any sequence. `do` mutation depends on the reference semantics designed
in 0020.

**Done when.** No line of `test/parity/divergences/` names task 0021, and `ctest -R parity` passes.

**Done.** `break`/`continue` parse behind `Settings::Extensions::LoopControls` (an error
without it, and outside a loop of the same macro, call or block body, as in Jinja2). They
set a pending `LoopControl` on the `RenderContext`; `ComposedRenderer` stops at it, `with`,
`filter` and block `set` hand it back from their cloned contexts (dropping their output,
as Jinja2 does), and the loop takes it. As in Jinja2, `for ... else` renders its `else`
body unless some pass finished without `break` or `continue`. The loop object gains
`revindex`, `revindex0` (lazy for filtered loops), `changed(*values)`, and `depth`/`depth0`
outside recursive loops. `for` and `set` targets are trees (`AssignTarget`): nested
tuples, and in `set` namespace attributes, also inside a tuple (`set ns.a, b = ...`).
`namespace(...)` takes `dict()`'s arguments and is a shared mapping marked `IsNamespace()`;
assigning an attribute of anything else raises. Left over: `namespace()` is still a
mapping rather than an opaque object (0042), and calling a missing attribute renders empty (0026).
