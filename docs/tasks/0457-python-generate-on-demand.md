---
status: open
priority: medium
area: parity
depends: [0436]
touches: [python/src/environment.cpp, python/jinja2cpp/environment.py]
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
(recursion_guard.cpp skips them there now), macOS arm64 support, and the cost per switch. Needs a
plan (architect) before code.

**Done when.** `next(t.generate())` returns before a later part of the template runs (a callable
with a side effect shows it), callables run on the caller's thread, and closing the generator ends
the render.
