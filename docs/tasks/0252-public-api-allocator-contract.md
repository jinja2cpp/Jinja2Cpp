---
status: done
priority: high
area: api
touches: [docs/api-2.0.md]
shares: [include/jinja2cpp/template.h, include/jinja2cpp/template_env.h]
---
# Allocator and memory contract for the 2.0 public API

**Release impact (0221).** Accepting a memory resource changes exported signatures, and
an allocator-aware `Value` would change its layout, so this is decided before the
2.0 header freeze, next to the error contract (0250). An explicit "no allocator
parameters in 2.0" with a documented contract is a valid outcome. Needs an architect plan.

**Problem.** Embedders that budget memory (servers with per-request arenas, games,
real-time and embedded hosts) cannot control where Jinja2C++ allocates, and nothing
says what the library allocates or when:
- `Load` builds the parse tree in an internal pmr arena (0118) on global `new`; the
  host cannot supply the upstream `std::pmr::memory_resource` or cap it.
- `Render` uses a thread-local `RenderWorkspace` (src/render_workspace.h) that keeps
  its buffers for the thread's lifetime; there is no way to pass, size or release it.
- `RenderAsString` returns a fresh `std::string`; reusing a caller buffer or writing to
  an output iterator/sink without an `std::ostream` is not offered.
- `Value`, `ValuesList` and `ValuesMap` use `std::allocator`; host data is copied into
  them with no way to place it in the host's arena.
- There is no statement of which calls allocate, what is cached per thread or per
  `TemplateEnv`, or how out-of-memory surfaces (`std::bad_alloc` to the host, or a
  `Result` error; ties to 0250).

The tension: every allocator hook is API and ABI surface that 2.x must keep, and an
allocator-aware value model (pmr containers inside `Value`) costs size and speed on the
hot path that the perf track spent 0140/0151 shrinking; doing nothing leaves embedders
with global `new` only.

**Questions for the plan.**
1. Which entry points take an optional `std::pmr::memory_resource*` (Load upstream for
   the arena, a per-render scratch resource), and whether a `RenderContext`/options
   struct carries it instead of more overloads.
2. Whether `Value` and its containers stay `std::allocator` (recommended starting point:
   yes, keep host data out of scope) or get pmr variants.
3. Render output: caller-owned `std::string&` reuse and a non-stream sink, measured
   against the bench suite.
4. The thread-local workspace: keep, make releasable (`ReleaseThreadCaches()`), or
   replace with a caller-passed workspace.
5. Out-of-memory behaviour and `noexcept` guarantees, written together with 0250.
6. Where the contract is documented (docs/api-2.0.md and the README section 0076 adds).

**Done when.** The decisions are written in docs/api-2.0.md, the memory contract is
documented for users, and implementation tasks are filed for any API it adds.

**Outcome (Ruslan 2026-10-18, all as recommended).** No allocator parameters in 2.0;
`bad_alloc` only for system OOM, budgets are `ResourceLimitExceeded` (with 0250);
`Render(ostream&)` all-or-nothing; `RenderTo` into a string or a sink plus a header-only
`ContainerSink`; `ReleaseThreadCaches()`. Decisions in docs/api-2.0.md 3.9 and decision 7,
plan in [design/0252-memory-contract-plan.md](../design/0252-memory-contract-plan.md).
Implementation: [0256](0256-survive-allocation-failure.md) (OOM crashes),
[0257](0257-render-to-string-and-sink.md) (`RenderTo`, sink),
[0275](0275-release-thread-caches.md), [0276](0276-memory-contract-docs.md) (docs).
