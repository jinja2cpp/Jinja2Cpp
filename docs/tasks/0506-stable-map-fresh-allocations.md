---
status: done
priority: medium
area: perf
depends: [0231]
touches: [src/ankerl/unordered_dense.h, src/render_context.h, src/template_impl.h, test/name_map_test.cpp, test/allocation_failure_test.cpp, sbom/components.json, bench/rust, bench/README.md]
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

**Result.** Where the fresh maps are, from callgrind's callers of `operator new` per render: the
block scope of each `{% block %}` (a block renders in a context of its own, `RenderContext(other,
depth)`, whose spare scope starts empty), the top scope of an imported module (its macros, which
then become the module's export map), and the scope of a macro body that sets a name
(`Render/hf_gpt_oss`). They are made in different places and some of them escape (a module's
exports), so recycling each would take a mechanism per site. The extra allocation is the same
in all of them, though: the `std::vector` of block pointers. So the fix is in the map:

- `segmented_vector` with the default allocator keeps its first block pointers in the tail of its
  first block (`colocated_blocks`, a local change in src/ankerl/unordered_dense.h): the first
  block is allocated with room for 8 pointers after its 8 values (512 + 64 bytes for an
  `InternalValueMap`), and a map that outgrows them moves the list to an array of its own that
  doubles. The list stays on the heap, so an iterator survives a move or swap of the map as it
  did with the vector. Other allocators keep the vector. Freeing is out of line behind an inline
  test: inline, the bigger destructor stopped GCC inlining `~table` into the loops (+0.3% on
  `Render/mitsuhiko_table`).
- The new-name insert inlines its placement where a new name is the common case:
  `table::try_emplace_placing_inline` (local change), used by `ScopeRef::ForName` and the loop
  that converts a render's params. Making the miss path inline for every string-keyed insert
  instead saved the same on `Render/many_tags` but cost +1.03% on `Render/config_file` and
  +0.76% on `Render/hf_deepseek_v31`.

bench/count.py against master 01fbec9 (GCC, Release): the four rows allocate one to three times
less per render (`Render/inheritance` 35, -3; `Render/config_file` 908, -2; `Render/hf_gpt_oss`
155, -1; `Render/html_autoescape` 674, -1), and so do `Load/hf_gpt_oss` (-4) and `Load/macros`
(-1). Bytes are +56 per fresh map (the 64-byte tail less the vector's 8). Instructions: no row
above +0.2%; `Render/substitute` -1.9%, `Render/config_file` -1.7%, `Render/plain_text` -0.95%,
`Render/inheritance` -0.85%, `Render/many_tags` -0.79%.

Against robin_hood (612663b, the base of #891, with this PR's cases): the same allocation counts
on all four rows, and `Render/many_tags` +0.39% (732.8 k against 730.0 k). Bytes stay above
robin_hood's by 0.5 to 1.6 KB a render on those rows: 512-byte segments against its node pool.
What a new name still costs more is the key's string made by an out-of-line `_M_construct` call
inside the placement, and the search for a free slot in the group.

Also in this PR, from the 2026-11-15 engines baseline: MiniJinja's `chat_mistral` dropped out of
the comparison because the Rust driver read `data.json` into sorted maps (serde_json and
MiniJinja without `preserve_order`), so `tool.items()` iterated in another order than Python's.
The driver now keeps the insertion order and sorts keys in its `tojson` as Jinja2's does (not for
the transformers `hf_*` cases): MiniJinja renders 20 cases as Python does, `chat_mistral` and
`hf_qwen3` included.
