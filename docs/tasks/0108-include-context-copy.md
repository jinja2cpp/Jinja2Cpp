---
status: open
priority: medium
area: perf
depends: [0105]
touches: [src/render_context.h#Clone, src/statements.cpp#IncludedTemplateRenderer]
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
