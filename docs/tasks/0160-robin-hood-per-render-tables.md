---
status: done
priority: medium
area: perf
depends: [0154]
touches: [src/template_impl.h, src/render_context.h, src/render_workspace.h, src/render_workspace.cpp]
---
# Per-render robin_hood tables the bench used to hide

**Finding.** #454 (0154 part A) routed robin_hood's allocations through `operator new`, so
the bench counts them now. The Performance track's verdict on CI job 114316741240 showed:
- every Render case: +2 allocations, +558 B per render;
- Render/many_tags: +13 allocations, +76,837 B per render (peak +76,888).

This is heap work every render already did; only the counting is new.

**Likely sources (not yet measured; inferred from the code).**
- The 2 per render: `intParams` in `TemplateImpl::Render` (template_impl.h), an
  `InternalValueMap` that the params are converted into on every render. A robin_hood table
  allocates its bucket array and a node block on first insert, which would be 2 allocations.
- many_tags: scope maps in `ScopeStack` (render_context.h, `InternalValueMap maps[ChunkSize]`).
  Each scope that takes a name allocates its own table, and a table that grows past a power
  of two re-allocates. 76 KB suggests one scope collects many names (many_tags `set`s), or
  scopes are re-created per iteration.

**Measure first.** Use `count.py --allocs` or a heaptrack/massif run on Render/plain_text
and Render/many_tags with a breakpoint on `robin_hood::detail::...::allocate`. Record which
table each allocation belongs to.

**Ideas, once measured.**
- Render params without the conversion map: look them up in the caller's `ValuesMap` (0115
  user data) or reserve `intParams` to the param count.
- Reuse scope tables across renders (keep a cleared table in the per-template workspace) and
  across loop iterations, instead of dropping them.
- Reserve a scope table once its final size is known at Load (0117 slots already cover most
  names; what still lands in scope maps?).

**Done when** Render/plain_text shows no robin_hood allocation, and many_tags's robin_hood
bytes per render are at most one table that is reused across renders.

## Done (PR link in the PR)

**Measured** (callgrind call graph on master 1ca9c15, Release bench):
- every Render case with params: 2 allocations, 558 B: the bucket array and first node block of
  `intParams`, rebuilt each render;
- many_tags: those 2, plus 11 for the top scope. Its 300 top-level `set`s land in the
  RenderContext's first scope, which rehashed 6 times per render (to 512 buckets, 176k
  instructions, 15% of the render) and allocated 4 node blocks. Slots do not cover them:
  top-level names stay in scope maps, since an include or import sees them.

**Change.** The thread's `RenderWorkspace` lends each render a pair of maps (params and top
scope) and takes them back emptied: `clear()` keeps the bucket array and returns the nodes to
the table's pool, so a steady render allocates nothing for either. A render started inside
another (a user callable rendering a template) is lent the next pair. A table past 1024
buckets, or of a render nested deeper than four levels, is let go rather than kept on
the thread. The top scope moves into the context only
when it has a table, and out of it at the end of a clean render; a failed render drops it.

**Result** (CI instruction counts, job 114471708298, against 1ca9c15): many_tags -4.6% (15 -> 2 allocations,
82,467 -> 5,630 B, peak 82,520 -> 5,632 B), substitute -13.2%, large_static -1.7%, others
-0.3%..+0.4%, plain_text +1.5% (+15 instructions: the thread-local workspace lookup and lend).
Every case with params: -2 allocations, -558 B.

