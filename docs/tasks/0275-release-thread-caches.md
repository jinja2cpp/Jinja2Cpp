---
status: in-progress
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

**As built (2026-10-23).** `RenderWorkspace::ReleaseCaches()` (named apart from the existing
`Release(SlotFrame)`) refuses while maps are lent or frames taken; it moves the chunks, frames,
lent tables and globals out before they are destroyed, so a host destructor that renders sees an
empty workspace, and keeps the frame generation counting so that a stale handle never resolves.
Loop frames leave their reserved vector one at a time. `ReleaseThisThread()` lives in
render_workspace.cpp so that `ForThisThread()` stays inline in template.cpp (an out-of-line copy
there cost Render/plain_text 12 instructions). The HTML-escape buffer is freed through
`detail::HtmlEscapeBuffer()`. The pending-destroy array (internal_value.cpp) needed no change: it
is allocated only during a destruction nested 64 deep and freed when that destruction ends, so it
is always empty and unallocated between renders. `LookupCache` keeps no memory to free (128 fixed
entries) and its stale entries are never used (epochs). Added to the `noexcept` list in
docs/api-2.0.md section 2 item 6.
