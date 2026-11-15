---
status: open
priority: medium
area: perf
depends: [0231]
touches: [src/hash_map.h, src/render_context.h, src/statements.cpp]
---
# A fresh scope map allocates once more than robin_hood did, and a new name costs more to add

**Problem.** Left by 0231. `detail::StableMap` (unordered_dense's segmented map) keeps its values in
512-byte segments listed in a `std::vector` of block pointers, so the first insert into a map that no
render recycled makes three allocations (block list, segment, index) where robin_hood made two (table,
node pool). On master after the swap that is +1 allocation per render on `Render/hf_gpt_oss` and
`Render/html_autoescape`, +2 on `Render/config_file` and +3 on `Render/inheritance` (bench/count.py;
bytes are lower than with 1024-byte segments and within +1.4 KB). Adding a name that is not in the scope
yet also costs about 43 instructions more (`ScopeRef::ForName`, 140 to 183 per `set` of a new name in
`Render/many_tags`, which is +1.98% overall): the miss goes through unordered_dense's out-of-line
`find_or_place_miss`.

**Proposal.** Find the maps that are made fresh per render or per call (macro call scopes, include and
block contexts, module exports) and recycle them as the loop body scope and the render's top scope
already are (RenderWorkspace::LendMaps, RenderContext::m_spareScope), or give the block list a first
entry inline. For the insert, measure keeping `place_new` inline for our key (the probe was kept inline
the same way through `key_compare_is_call`, src/hash_map.h) against the caller-loop cost the library's
comment describes.

**Done when.** Those four rows allocate what they did with robin_hood (master before 0231), and
`Render/many_tags` is back within +0.5% of its robin_hood count, or the remaining cost is recorded here
with its cause.
