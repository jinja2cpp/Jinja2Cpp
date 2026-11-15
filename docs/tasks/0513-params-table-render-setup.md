---
status: in-progress
pr:
priority: medium
area: perf
depends: [0512]
touches: [src/params_table.h, src/render_context.h, src/render_workspace.h, src/render_workspace.cpp, src/template_impl.h, src/statements.h, src/statements.cpp, src/expression_evaluator.h, src/expression_evaluator.cpp, test/params_table_test.cpp, test/slot_frame_test.cpp, CMakeLists.txt, docs/api-2.0.md]
---
# The params of a render as a flat table, and a slimmer per-render setup

**Problem.** After 0512, `Render/substitute` (two params) cost 2,179 instructions, about 600 of them
for the params: each one was copied as a `std::string` key, hashed, inserted into the lent
`InternalValueMap`, converted, and erased again by `EmptyMaps` when the render ended. Substitute is the
one row where Tera is ahead (0.72x, perf-track engines baseline 2026-11-15). `Render/plain_text` is
almost all fixed setup: about 1,000 instructions with nothing to render.

**Design** (architect plan `perf-track/0513-params-setup-plan.md`, option C). A lazy view of the host's
map, a Load-time slot list and a cache of converted params across renders were each rejected there: no
gain on rows that read every param, a host accessor running mid-render (0481), or unsafe (ABA, 0049).

**Change.**
- P1. `ParamsTable` (`src/params_table.h`): a contiguous array of `{name view, value}` lent by the
  workspace (`RenderMaps::params`), filled once per render and never grown after, so lookup results
  and the lookup cache stay valid. The names are views of the host's keys (a `ValuesMap`'s nodes, or
  the names of the values a `GenericMap` gave). Up to 8 params a lookup compares names, with no hash;
  past 8 the table also keeps an open-addressing index by the keyed `HashName` (0231), cross-checked
  against a scan at FULL contract checks. A `GenericMap` may repeat a key: the last value wins.
- `RenderContext` searches the table where it searched the external scope (`FindEntryFrom`,
  `FindValueWhere`, `FindForWrite`, `CollectVisibleNames`). The copy and child constructors copy it;
  `CloneWith` (a module, an include without context) does not, as Python's `import "m"` without context
  sees no params. A top-level render's external scope is the shared empty map.
- The 0481 keep is decided before the params are converted. A kept render converts them once, as owned
  copies (`OwnedArgument`), instead of borrowing them and converting them again.
- A `GenericMap`'s values live in a lent `std::vector`, reserved before the first one, instead of a
  `std::list`: no node allocation per key.
- `ValueRefExpression::EvaluateRef` keeps its lookup by name out of line. Inlined, the longer
  `FindEntryFrom` made GCC save six registers before every slot read (for_loop_vars +4.7%,
  mitsuhiko_table +2.7%).
- P2. `GetGlobalsSnapshot` takes the workspace `RenderCore` already fetched, with the hit inline and
  the conversion out of line (`ConvertGlobalsSnapshot`). `TemplateRenderer::RenderNamed` is inline: a
  template with no blocks and no `extends` goes straight to `RenderBody`, without a frame of its own.
  `RenderContext` no longer has an empty map of its own for `Clone(false)`.
- docs/api-2.0.md, values crossing the boundary, item 1: the render borrows the params map's keys too.

**Result** (bench/count.py, GCC Release, pinned name-hash key, against master 9118188):
- substitute Render -25.2% (2,179 to 1,631), Sink -29.1%, To -27.6%.
- plain_text Render -6.0% (1,012 to 951), Sink -8.7%, To -7.5%.
- hf rows -1.0..-1.5%, chat rows -0.3..-0.5%, inheritance -2.0%, filters -2.0%, large_static
  -1.8..-2.9%; every other row -0.1..-0.8%, Module/keep_written +0.04%.
- Allocations: chat and hf rows -1 a render (-22 bytes); config_file and html_autoescape -64 bytes.

**Not done.** P3 (a template that looks up no name skips the globals) and P4 (the snapshot read
without a `shared_ptr` copy, a lifetime change for the advisor) from the plan. What remains of
plain_text's setup is the output string's `reserve` and malloc (about 130), `Flush` (85), and
`RenderContext`'s constructor and destructor (about 45 each, mostly member initialization).

**Next contradiction.** An external level now has two forms: the params table of a top-level render and
the `InternalValueMap` of a module's or a `with context` import's names. A rework of the lookup cache or
of root slots (0117 P4) has to handle both, or move modules onto `ParamsTable` first.

**Done when.** The count job on the PR shows no row above +0.3% and no new allocations, and the rows
above move as measured.
