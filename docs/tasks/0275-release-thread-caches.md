---
status: open
priority: medium
area: api
depends: [0252]
touches: [include/jinja2cpp/template.h, src/template.cpp, src/render_workspace.h, src/render_workspace.cpp, src/markup.h, src/internal_value.cpp, test/template_api_test.cpp]
---
# `ReleaseThreadCaches()`

**Release impact (0221).** API wave: one exported function, lands before the G1 freeze.

**Problem.** The thread-local `RenderWorkspace` (src/render_workspace.h) keeps slot frames,
loop frames, parameter tables, buffers and the last environment's converted globals for the
thread's lifetime. A thread pool cannot give the memory back, and the globals (with whatever
host callables they capture) stay alive on every thread that rendered after the `TemplateEnv`
is destroyed.

**Decided (0252, docs/api-2.0.md 3.9).** `JINJA2CPP_EXPORT bool ReleaseThreadCaches() noexcept;`
frees them on the calling thread; returns `false` and frees nothing while a render runs on that
thread (frames taken or maps lent). Implemented as `RenderWorkspace::Release()`: loop frames
cleared, `Globals()` reset, HTML-escape buffer and pending-destroy array shrunk when empty.
Add it to 0250's `noexcept` table.

**Tests.** Render, release (`true`), render again with identical output; from a user callable
during a render: `false`. Env retention: a global callable capturing a `shared_ptr` sentinel;
destroy the env; the sentinel is still held; release; it is freed. TSan: four threads rendering
and releasing in turn.

**Perf.** Not on the render path: every case ±0.1%.
