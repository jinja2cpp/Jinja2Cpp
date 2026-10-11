---
status: open
priority: low
area: perf
depends: [0117]
touches: [src/render_context.h, src/expression_evaluator.h]
---
# A new name in a scope invalidates every cached lookup

**Problem.** The lookup cache (0100, rekeyed by symbol in 0118 phase 6) validates an
entry by the render context's epoch. Any name added to any scope (`ScopeRef::ForName`,
`operator[]`), a scope cleared or left, and a slot bound for the first time start a new
epoch, so every cached lookup misses once afterwards, also for names that the new binding
cannot hide. A `set` in a loop body, and the body scope cleared after each pass, do this on
every pass. On master 8ede9da the miss path (`RenderContext::FindValueWithViews`) is 2.1%
of `Render/chat_mistral` (67 misses a render); 0129 measured `many_tags` +1.9% for the
same reason (each root `set` starts an epoch). Found while measuring 0117 P3, which would
not remove these epochs: a slot bound for the first time starts one too.

**Proposal.** Measure first which epochs cause the misses (a Debug counter per epoch
source on chat_mistral and many_tags). If `set` of a name and the body-scope clear dominate,
narrow the invalidation to the name: for example an epoch per symbol, or skip the epoch
when the name added is not one the cache holds an entry for in a scope further out. The
Debug cross-check of every hit against `FindValue` stays.

**Done when** the misses after `set` are gone or the measurement shows they are not worth
the code, with Render chat_mistral and many_tags measured and no case worse than +0.3%.
