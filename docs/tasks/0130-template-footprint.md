---
status: open
priority: low
area: perf
touches: [src/template_parser.cpp, src/template_parser.h, src/expression_parser.cpp, src/renderer.h, src/statements.h, src/expression_evaluator.h]
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
