---
status: done
priority: medium
area: perf
depends: [0105]
touches: [src/render_context.h#Clone]
---
# `include` copies every scope of the caller

**Problem.** Found with the gperftools heap profile of `Render/inheritance` after 0105.
With the include's template now resolved once per render, the rest of an `include` is
`RenderContext::Clone(true)`: it copies the whole `std::deque` of scope maps (each a
robin_hood map, every value an `InternalValue` copy) so the included template can read
the caller's names. That is 63% of the allocations left in `Render/inheritance`
(`RenderContext::RenderContext` copy constructor, robin_hood `BulkPoolAllocator`, deque
nodes), and it grows with the depth and size of the caller's scopes, not with what the
included template reads.

**Ideas.** Python builds a new context from `context.get_all()` plus the locals, which is
a flat dict copy, so the copy itself is not a parity requirement: the included template
may only read the caller's names and its own assignments must not leak back (include
never exports, unlike import). A cloned context could refer to the caller's scopes as a
read-only parent chain and start with one empty scope of its own, the way the global and
external scopes are already referred to by pointer. Writes to a mutable value
(`namespace`, list `append`) must keep today's behaviour; check them against Python.

**Done when.** `Render/inheritance` allocations per render drop by the scope copy
(`bench/count.py --baseline`), with the include/scope parity cases unchanged.

**Done** in this PR. A nested context (`Clone(true)`, and the block and `super` contexts
made with `RenderContext(other, depth)`) no longer copies the caller's scopes: it keeps a
pointer to the context it was made from and how many of its scopes it sees, and starts
with one empty scope of its own. Lookups walk its own scopes, then the parents' visible
ones; the root context keeps the old single loop, so templates without nesting do not pay
for the chain. This covers `include`, `import`, `with`, `{% set %}` and `{% filter %}`
blocks and blocks, not only `include`. A value changed in place in a nested context (a list
`append`) is now changed where it is stored, which is what Jinja2's shallow context copy
does; new corpus cases in `loader.py` pin include reads, shadowing, namespaces and appends.
`Render/inheritance` -28.5% instructions, allocations 315 -> 265 per render; `config_file`
-8%, every other render case flat or up to -2.7% (`bench/count.py --baseline`). Left:
each nested context still allocates its scope `std::deque` (2 allocations), filed as 0129.
