---
status: in-progress
priority: low
area: perf
touches: [src/template_parser.cpp, src/template_parser.h, src/expression_parser.cpp, src/renderer.h, src/statements.h, src/expression_evaluator.h, src/expression_evaluator.cpp, src/function_base.h, src/filters.cpp, src/filters.h, src/testers.cpp, src/testers.h, src/global_functions.cpp]
---
# A loaded template keeps 30 times its source

**Problem.** 0121's memory figures (`bench/count.py`, `Retained` on `Load/*`, master
698f881) show that a template with many tags costs far more memory than its text:

| Case | Source | Retained after Load | Ratio |
|---|---:|---:|---:|
| `many_tags` (2,400 tags) | 39 KB | 1,138,584 B | 29x |
| `chat_llama` | 3.5 KB | 38,424 B | 11x |
| `large_static` (mostly text) | 37 KB | 53,064 B | 1.4x |

About 470 bytes per tag: each `{{ x }}` and `{% if %}` becomes several heap nodes
(renderers, expression nodes, `shared_ptr` control blocks, `std::string` names, vectors
with spare capacity). A server caching thousands of per-tenant templates pays this per
template, and the scattered nodes also cost cache misses at render time.

A smaller oddity from the same run: `Render/dict_ops` peaks at 35 KB for 1.8 KB of output,
so something it builds per render (sorted dict copies?) is large; check it while here.

**Proposal.** Measure where the bytes go (`--heap-profile` in a gperftools build, bench
README), then shrink the biggest: `make_shared` instead of `shared_ptr(new)`,
`shrink_to_fit` or exact `reserve` on node vectors after parsing, `string_view` into the
kept source instead of copied names, an arena for the AST (that last one is a design
change; ask for an architect plan first).

**Done when.** `Load/many_tags` retains under half of today's 1.14 MB with no instruction
regression on `Render/*`, and `dict_ops`'s peak is explained or reduced.

**Progress: phases P2a and P2b of the 0118 plan** (`/mnt/project-files/perf-track/0118-parse-tree-arena-plan.md`,
section 5; PR #418):
- **P2a, filter and tester arguments.** `FunctionBase` kept a `ParsedArgumentsInfo` (272 B:
  a name-keyed map of nodes, both extra-argument lists) and a `std::string` error. It now
  keeps `BoundArguments` (a pointer to the kind's static `ArgumentsTable` and one exact-size
  array of nodes, by declared position, allocated only when the call passes an argument)
  and a `unique_ptr<std::string>` error: 24 B instead of 304. An unbound parameter reads
  the table's default, so no default becomes a node, at Load or at render (the
  `ConstantExpression` that `SetDefaultArg` made for every defaulted parameter of a filter
  created during a render, as in `map('upper')`, is gone, and with it `ArgumentInfo::defaultExpr`).
  Extra arguments go straight to the filter's own `CallParamsInfo`.
  `map(attribute=...)` binds no `'attr'` constant node. `ExpressionFilter`'s error string
  became a `unique_ptr` too. The gettext alias `_` calls the `gettext` in scope through
  `CallExpression::CallValue` instead of building a `CallExpression` and a
  `ValueRefExpression` per call.
- **P2b.** `ExpressionRenderer` lost its 72-byte `finalize` copy; templates with
  `Settings::finalize` get a `FinalizedExpressionRenderer`. The unused statement visitor
  (`ast_visitor.h`, an extra vptr on every renderer) is removed. The root body gives back
  its spare capacity after the parse. Decision 5 (dropping structural equality and the
  virtual `IComparable` base) stays a separate PR.
- **Fixed on the way:** `map(attribute='x', foo=1)` ignored `foo`; it now fails like
  Jinja2's `FilterArgumentError` (corpus case `filters.map_attribute_unexpected_kwarg`).
  The `startsWith` test without its argument dereferenced a null node; it now tests
  against an empty prefix (`TestersTest.StartsWithWithoutArgument`).
- **Decision 5** (0118 plan): parse trees no longer compare structurally. Every
  `IsEqual` override, the `operator==` of node pointers, call parameters, macro
  parameters, assign targets and bound arguments, and the virtual `IComparable` base of
  renderers, filters and testers are gone (about 1,150 lines). `TemplateImpl::operator==`
  compares source, settings and environment, which determine the tree. Instructions
  against master 0892811: Load -1.3..+0.2%, Render -0.5..+0.4%, retained up to -9.7 KB
  (`many_tags`). Removing the overrides made GCC stop inlining `SetStatement::AssignBody`
  (`Render/many_tags` +0.9%); the set statements now call `AssignTo` directly.

`bench/count.py --baseline` against master faea865 (Release, GCC 13):

| Case | Retained before | after | Instructions (Load) | Instructions (Render) |
|---|---:|---:|---:|---:|
| `many_tags` | 1,060,112 | 784,352 (-26%) | -1.79% | -1.79% |
| `chat_llama` | 39,504 | 31,456 (-20%) | -1.24% | -1.07% |
| `filters` | 11,504 | 6,584 (-43%) | -1.24% | -1.89% |
| `large_static` | 53,376 | 48,720 (-9%) | -2.61% | -4.07% |

Render instructions drop on every case but `plain_text` (+1.16%, 27 instructions):
`mitsuhiko_table` -3.72%, `expressions` -3.88%, `html_autoescape` -3.28%, `for_range`
-5.23%. Load allocations rise on filter-heavy templates (`many_tags` 9,659 -> 10,260),
one exact-size argument array per filter call; the 0118 arena (P4) removes them.

**`dict_ops` peak, explained.** `Render/dict_ops` peaks at 34 KB because `dictsort`
over its 100-entry dict holds, at once, the copied entries (`GetEntries`, 100 x 104 B),
the sort keys (100 x 40 B), the result list (100 x 72 B) and one heap `KeyValuePair`
per result item (about 112 B each): about 330 B per entry, proportional to the dict, not
to the 1.8 KB output. Python's `sorted(d.items())` keeps a comparable list of tuples.
Not reduced here.

**Left for 0118 P3-P5:** the goal of under half of 1.14 MB (0.53 MB) needs the arena:
`shared_ptr` control blocks and malloc headers (~270 KB on `many_tags`), 16-byte links
and `std::string` names.

