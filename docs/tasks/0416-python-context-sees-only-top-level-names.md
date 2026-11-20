---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/965
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

**Resolution (batch 11 D).** Additive public API: `jinja2::ContextView(context, ContextNames::TopLevel | Parent)`
(template.h) views a call's context through `RenderContext::FindTopLevelName`/`FindParentName`: the top level is the
running template's top scope (`TemplateFrame::baseDepth - 1`) with its frame views, the parent the scopes below it (an
including template's), the params, the external scope, the globals and the built-ins. A macro call sees its
template's top level, an included template its own. The view expires with the context it views. The Python
`Context`'s `vars`, `parent`, `keys()` and `get_all()` read the two views;
`test_context_splits_top_level_names_from_arguments` compares 14 templates with Jinja2 and the docs/python.md line
now names only what is left. Left, in 0558: a lookup (`ctx.get('i')` of a loop variable) still reads every scope.
Jinja2's lookups see the names a `set` in a loop or block body stored (`_loop_vars`, `_block_vars`, the upstream
`test_pass_context_*_loop_vars` and `*_in_blocks` tests) but not the loop's targets, and the engine's scopes do not
tell the two apart; reading only the two views failed those four upstream tests.
