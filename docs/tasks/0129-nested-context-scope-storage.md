---
status: done
priority: low
area: perf
depends: [0108]
touches: [src/render_context.h#m_scopes]
---
# Every nested render context allocates a scope deque

**Problem.** Since 0108 a nested context (`include`, `with`, `{% set %}` and `{% filter %}`
blocks, blocks, `super`) no longer copies the caller's scopes, but it still owns a
`std::deque<InternalValueMap>`, and libstdc++ allocates a deque's map and first node on
construction even when it stays small. That is 2 of the about 5 allocations left per
`include` in `Render/inheritance` (`_M_initialize_map`, 212 calls per 2 renders in a
callgrind caller tree), and 2 of the 4 left in a trivial render (0104).

**Ideas.** Keep the first few scopes inline (an array of, say, 4 maps, robin_hood's empty
map does not allocate) and fall back to a deque beyond that. References to a scope must
stay valid across `EnterScope` (the macro code and `ScopeRef` hold them, 0104) and the
scopes stay node maps (0088, EvaluateRef), so a plain `std::vector` of maps does not do.
The backward lookup loop in `FindValue` is hot: measure it on `many_tags` and `macros`,
which regressed by 5-11% with a slower loop shape while 0108 was being written.

**Done when.** `Render/inheritance` and `Render/plain_text` allocations drop by 2 per
nested context and per render (`bench/count.py --baseline`), no render case slower.

**Done.** `ScopeStack` (src/render_context.h) keeps scopes in chunks of 8; the first chunk
lives inside the context and deeper chunks are allocated on first use and kept until the
context ends, so a scope never moves. Against master 698f881 (`bench/count.py --baseline`):
every render 2 allocations fewer, `Render/inheritance` 265 -> 157 allocations and -9.3%
instructions, `plain_text` -15%, `config_file` -5.8%, the rest within ±1.3% except
`many_tags` +1.9%, where every lookup misses the cache (each `{% set %}` starts a new epoch).

Lessons for the lookup path, measured on the way: GCC split the inlined lookup at different
points with each loop shape, and once any part of it went out of line with a pointer to the
name, every name expression paid for a stack protector (+4-8% on `many_tags`, `macros`).
`FindValueCached`, `FindValue` and the per-map lookup are now forced inline
(`JINJA2CPP_ALWAYS_INLINE`), and scopes past the first chunk are searched out of line with
the name passed by value, in a header function: the library exports nothing from src/, and
the tests use RenderContext against the shared library too (MSVC would not link). A chunk of 4 was too small: `mitsuhiko_table` nests deeper than
that in one context.
