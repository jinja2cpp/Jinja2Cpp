---
status: open
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
