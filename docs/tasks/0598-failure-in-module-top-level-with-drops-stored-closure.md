---
status: open
priority: low
area: parity
depends: [0586]
touches: [src/statements.cpp#WithStatement::Render, src/statements.cpp#ClosesOverOnFailure, src/statements.cpp#ForStatement, src/render_context.h]
shares: [test/template_module_test.cpp]
---
# A failure inside a module's top-level `with` drops the names its stored closure reads

**Problem.** Found by the verifier of batch 14 C (0586), identical on master c4561b9. A `Template::MakeModule` macro
`keep()` imports `lib` with context and fails; `use()` calls `ns.f()`. With `lib` =
`{% set v = 5 %}{% with a = 1 %}{% macro f() %}{{ v }}{{ a }}{% endmacro %}{% set ns.f = f %}{{ 1 // 0 }}{% endwith %}`
`use()` prints nothing where Jinja2 3.1.6 prints `51`: the `with` never reaches its end, so its closure keeps `a`
(FrameEnd::Stopped would), and the module stays. A failure after the `endwith` matches Jinja2 since 0586 (`5`).
WithStatement::Render runs its body under RenderSoftBody but takes no capture on a failure, as RenderFailingFrame does
for a block body (0531).

**Done when** the row prints `51` on each later call, narrow and wide, in test/template_module_test.cpp next to
`ClosureInATopLevelWithKeepsTheModuleNames`.

**Progress (batch 15 A).** ClosesOverOnFailure also holds in a context RenderContext::InEndingModule finds, and the
loops' failure paths ask it. Rows `FailureInATopLevelBodyKeepsTheClosureNames` (with, filter, loop, loop with a filter,
second pass, `else`, `with` in a loop, block, scoped block), narrow and wide. The same in a macro body: 0608.
