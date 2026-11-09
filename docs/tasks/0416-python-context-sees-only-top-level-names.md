---
status: open
priority: low
area: parity
depends: [0354]
touches: [src/render_context.h#TopLevelNames, python/src/context.cpp]
shares: [python/jinja2cpp/runtime.py, python/tests/test_context.py, docs/python.md]
---
# `pass_context`'s Context sees loop and `with` variables, Jinja2's does not

**Problem.** Jinja2's `Context` holds the render arguments, the globals and the top-level
`{% set %}`/import/macro names of the template (`context.vars`); a loop variable, a `{% with %}`
name or a macro's local is a Python local of the compiled code, and `ctx.get('i')` inside
`{% for i in ... %}` is missing. The module's `Context` (0354) reads the engine's `FindValue`,
which sees every scope: `ctx.get('i')` finds the loop variable, and `ctx.vars` and `ctx.parent`
both hold everything visible (the engine's `CollectVisibleNames` plus the globals), so the
split between a template's own names and its arguments and globals is lost.

**Proposal.** A `RenderContext` accessor over the lowest scope of the template (the one
`FindInScopesFrom(name, 0)` reads) plus the external and global scopes, to resolve and list
the names; `vars` then holds the top-level names and `parent` the arguments and globals.
Check the macro-call frames and name slots (0117), where a top-level name may live in a slot.

**Done when.** A pytest case compares `ctx.get(loop_var)`, `ctx.get(with_var)` and
`sorted(ctx.vars)` with Jinja2, and the docs/python.md "Differences" line is removed.
