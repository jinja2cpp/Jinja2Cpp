---
status: done
priority: low
area: perf
depends: [0117]
touches: [src/render_context.h, src/expression_evaluator.h, src/statements.cpp, test/helpers_tests.cpp, test/slot_frame_test.cpp]
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

## Done (PR link in the PR)

**Measured** (a counter per epoch source, Release bench on master 39dc889, 100 renders):
- chat_mistral: 114 lookups a render, 43 hits, 71 misses after an epoch: a name added by
  `set` (17), the loop body scope cleared after each pass (14), a loop's slots bound (17)
  and unbound (14), a nested context made (9).
- many_tags: 1,500 lookups, 600 hits, 300 entry collisions (more than 128 names) and 596
  misses, all after a root `set`.

**Change.** The context epoch stays for changes that can move any lookup (a module scope
bound, a scope entered with names, a context made or copied). A name added to or erased from
a scope, a scope cleared or left, and a slot bound for the first time or unbound now drop
only the cached entries of the names concerned:
- `LookupCache` groups its 128 entries in 32 buckets of 4 by the name's hash.
  `NewSlot(hash)` gives a name expression an entry of its name's bucket (in turn within
  it), so dropping a name clears the 4 keys of its bucket. Keys sit in their own array so
  that this is two stores. The hit path is unchanged.
- Each scope records the buckets of the names added to it (32 bits in `ScopeStack`), so that
  clearing or leaving it forgets those buckets without hashing or walking its names. A scope
  entered with names it did not record (a loop's kept body scope) starts a context epoch
  and forgets every bucket when it goes.
- `BindSlot` and `UnbindSlots` take the slot names' hashes (the loop, filter and macro
  callers have them).
- Scope values must stay where they are while other names are added: a static assert keeps
  `InternalValueMap` a node map.

**Result** (callgrind, Release bench, against 39dc889): many_tags -9.6%, macros -3.8%,
chat_mistral -0.6%, config_file -0.5%, dict_ops -0.4%, chat_qwen -0.2%, chat_llama -0.1%.
Costs: mitsuhiko_table +0.7%, for_filter_if +0.6%, for_range +0.3%, inheritance +0.6..0.7%,
substitute +11 and plain_text +3 instructions. In a loop, forgetting by name costs four bucket
clears per run of the loop (bind `loop` and the item, unbind both) where two epochs were
enough, about +20 instructions a run; mitsuhiko_table runs its inner loop 1,000 times a
render. Keeping epochs for loop slots instead was measured: mitsuhiko_table +0.1%, but
chat_mistral +0.15%, config_file +0.4%, inheritance +0.5%, so loops forget by name too. These cases miss this task's +0.3% bar (the CI gate is +3%);
the trade is put to the performance track for a verdict.

