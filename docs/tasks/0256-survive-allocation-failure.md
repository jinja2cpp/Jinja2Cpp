---
status: open
priority: high
area: robustness
depends: [0252]
touches: [src/node_arena.h, src/robin_hood.h, test/oom_test.cpp, CMakeLists.txt]
---
# Survive allocation failure in Load and Render

**Release impact (0221).** G8 robustness bug, and the memory contract (docs/api-2.0.md 3.9,
0250's 2.1) promises that `std::bad_alloc` propagates cleanly. Land before 0253's boundary PR.

**Problem.** Failing the n-th allocation on master `48f6983` (probe in
[design/0252-memory-contract-plan.md](../design/0252-memory-contract-plan.md), fact 2):
`Load` throws `bad_alloc` at 19 of 24 points and crashes (SIGSEGV) at 4; `RenderAsString`
returns `UnexpectedException` at 87 of 96 and crashes at 8.
- `NodeArena::Allocate` (src/node_arena.h) `emplace_back`s the block before `new[]` succeeds,
  so cleanup reads a block with no storage.
- Vendored robin_hood tables crash when destroyed after a failed insert or rehash.
- Catch handlers that build the error allocate themselves (src/template_impl.h).

**Work.** Allocate into a local `unique_ptr`, then `emplace_back` (audit `m_objects.Reserve`,
`GrowSymbols`). Make a failed robin_hood `initData`/`rehashPowerOfTwo` leave a destroyable
table; if that takes more than about 40 lines in the vendored file, add "passes the OOM sweep"
to 0231's done criteria and land those rows with the successor. Audit allocating catch handlers.

**Tests.** A second executable `jinja2cpp_oom_tests` (it replaces global `operator new`): a
table of templates (include/extends/import, filters, `{% set %}` blocks, i18n, wide), Load and
Render failing allocation n = 0..N, plus an "exhausted" mode. Expect no crash or leak under
ASan/LSan; outcome ok, a `Result` error, or `bad_alloc` out of the call (Render reports
`UnexpectedException` until 0253 flips that one expectation); the same `Template` renders
correctly afterwards; a failed Load keeps the previous template. Add the arena's
`length_error` row (`ResourceLimitExceeded` once 0253 maps it).

**Perf.** Cold paths only: `Load/*` and `Render/*` within ±0.1% on `count.py --baseline`.
