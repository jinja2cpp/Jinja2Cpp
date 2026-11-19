---
status: open
pr:
priority: low
area: parity
depends: [0531]
touches: [src/statements.cpp#RenderLoopInScopes]
shares: [test/borrowed_cells_test.cpp]
---
# A failure through a nested level of a recursive loop loses the template's top-level names for its closures

**Problem.** Found by the verifier of 0531. A render nested in another one that shares its globals fails inside a
level of a recursive loop that `loop(...)` started; the host swallows the failure and the outer render calls a
closure that level stored. With `inner()` rendering

```jinja
{% set v = 'V' %}{% for a in [[1], 2] recursive %}{% if a is iterable %}{{ loop(a) }}{% else %}{% macro m() %}[{{ v }}]{% endmacro %}{% set _ = g.append(m) %}{{ 1 // 0 }}{% endif %}{% endfor %}
```

and the outer `{{ inner() }}{{ g[-1]() }}`, Python Jinja2 3.1.6 prints `[V]` and Jinja2C++ prints `[]` (master too).
The same shape through a macro calling itself prints the right `[V0]`, as do loops nested without recursion: only
the failure capture of a level that runs through the `loop(...)` call (its `HomeCall`, the link to the scopes around
the outermost level) misses the scopes below the loop.

**Proposal.** On the failure path of `RenderLoopInScopes` for an inner level (`site` not null), close over the
frames up to the site's scopes as the level's own end does, or let the level's `HomeCall` end its view by capturing
before it unwinds.

**Done when.** A `BorrowedCellsSingle.NestedRenderThatFailsKeepsItsClosures` row with the template above prints `[V]`,
ASan clean.
