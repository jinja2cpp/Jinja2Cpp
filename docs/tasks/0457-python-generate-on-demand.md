---
status: in-progress
priority: medium
area: parity
depends: [0436]
touches: [include/jinja2cpp/template.h, src/render_suspension.h, src/render_suspension.cpp, src/render_workspace.cpp, src/recursion_guard.cpp, python/src/environment.cpp, python/jinja2cpp/environment.py]
shares: [python/tests/test_generate.py, python/tests/upstream/expected-failures.txt]
---
# `Template.generate` renders on demand, on the caller's thread

**Problem.** Jinja2's `generate()` runs the template as the consumer asks for items, so a server
sends the first bytes before the render ends and holds one item in memory. Since 0436 the module
yields Jinja2's items, but the whole render runs at the first `next()`.

0436 built and verified a worker-thread render (branch commit dc2a6fd9, decisions in the 0436 task
file) and dropped it: a Python callable the template calls then runs on another OS thread, which
breaks sqlite objects made in the caller's thread, `threading.local` state (Django connections),
locks the caller holds, and gevent (its patched `Thread` is a greenlet on the caller's thread); and
a daemon consumer at interpreter exit aborts.

**Proposal.** Resume the engine's render on the caller's thread: run it on a stack of its own (a
fiber: Windows fibers, `ucontext` or a small assembly switch on POSIX, with ASan's
`__sanitizer_start_switch_fiber`) and switch back at each sink write. Between two items no Python
frame of the render is live (a callable has returned before the engine writes), so CPython's thread
state needs no swapping; check that claim, the engine's stack checks on a stack of its own
(recursion_guard.cpp skips them there now), macOS arm64 support, and the cost per switch. Plan and decisions: [docs/design/0457-lazy-generate-plan.md](../design/0457-lazy-generate-plan.md).

**Done when.** `next(t.generate())` returns before a later part of the template runs (a callable
with a side effect shows it), callables run on the caller's thread, and closing the generator ends
the render.

## Decisions (Ruslan, 2026-11-11)

Plan: [docs/design/0457-lazy-generate-plan.md](../design/0457-lazy-generate-plan.md). The probes showed the
work is not module-only: two suspended renders on one thread corrupt the engine's per-thread render state,
and the engine's stack check is skipped on a fiber stack.

- D1: public `RenderOptions::suspension` before the G1 freeze; the engine swaps a render's per-thread
  state in and out around each suspending `Write`.
- D2: `next()` from another thread raises `RuntimeError` (divergence); lazy async iterator is 0461.
- D3: pinned boostorg/context `fcontext` assembly in the module, OS fibers on Windows; eager fallback on
  unverified CPython versions (3.14.0), with greenlet imported, under CET shadow stacks, on allocation
  failure and past 8 nested lazy renders.
- D4: a cycle through a suspended generator leaks until closed; fix is 0462.
- D5: Python 3.14 row in the Python CI module job.

## Steps

1. Engine facility (include/, src/, test/, docs/api-2.0.md).
2. Switch primitive and lazy `generate()` (python/).
3. Docs (docs/python.md divergences) and CI.
