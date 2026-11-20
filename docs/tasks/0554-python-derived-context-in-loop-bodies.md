---
status: open
priority: low
area: parity
depends: [0416]
touches: [src/render_context.h#FindTopLevelName, src/host_call_context.cpp#ContextPartMapper]
shares: [python/jinja2cpp/runtime.py, python/tests/test_context.py, docs/python.md]
---
# `pass_context` lookups: loop targets, `with` names and macro locals, and Jinja2's derived Context

**Problem.** 0416 split the module's `Context` as Jinja2's: `vars` holds the running template's top-level names
(`jinja2::ContextView(..., ContextNames::TopLevel)`, the scope at `TemplateFrame::baseDepth - 1`) and `parent` the
including template's names, the arguments and the globals. Its lookups (`get`, `resolve`, `ctx[name]`,
`in`) still read every scope (the context map itself), so `ctx.get('i')` inside `{% for i in ... %}` finds the loop
variable, as it finds a `{% with %}` name and a macro's argument or local; Jinja2's finds none of them. They cannot
read the two views only: Jinja2 3.1.6's `Context.call` takes the `_loop_vars` of the innermost loop body (or the
`_block_vars` of a block body) and, when that body has already run a `{% set %}`, calls the callable with
`context.derived(those names)`, whose `vars` is empty and whose `parent` is the old `get_all()` with the body's names
on top. The upstream tests `test_regression.py::TestBug::test_pass_context_loop_vars`, `..._scoped_loop_vars`,
`..._in_blocks` and `..._block_and_loop` check those names, and failed when the lookup read the views only. The
engine's loop scope (or slots) holds the loop's targets and the names a `set` in the body stored alike. Probe (`p`
a `pass_context` callable returning `sorted(ctx.vars)`, `ctx.get('i')` and `ctx.get('b')`):

| Template | Jinja2 | module |
|---|---|---|
| `{% set a = 1 %}{% for i in [1] %}{{ p() }}{% endfor %}` | `['a'] None None` | `['a'] 1 None` |
| `{% set a = 1 %}{% for i in [1] %}{% set b = 2 %}{{ p() }}{% endfor %}` | `[] None 2` | `['a'] 1 2` |
| `{% set a = 1 %}{% block x %}{% set b = 1 %}{{ p() }}{% endblock %}` | `[] None 1` | `['a'] None 1` |
| `{% with w = 3 %}{% set b = 2 %}{{ p() }}{% endwith %}` | `[] None None` | `[] None 2` |

**Proposal.** Tell a name a `set` in a loop or block body stored from a loop target, a `with` name and a macro's
argument (a parse-time mark on the loop's or block's slot names, say), and give the host a third view (`ContextNames`)
of the innermost such body's stored names; the Python `Context` then looks up the views only and builds a derived
context when that view is not empty. Check a loop in a macro (Jinja2's macro frames
have `_loop_vars` too) and nested loops (only the innermost body's names count).

**Done when.** The probe rows render the same in `python/tests/test_context.py` with both engines, the four
upstream tests still pass, and the `docs/python.md` sentence naming 0554 is gone.
