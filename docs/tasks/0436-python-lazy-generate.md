---
status: in-progress
priority: medium
area: parity
depends: [0355]
touches: [python/src/generate.cpp, python/src/generate.h, python/src/environment.cpp, python/src/poll.h, python/jinja2cpp/environment.py, include/jinja2cpp/template.h, src/renderer.h, src/out_stream.h]
shares: [python/tests/upstream/expected-failures.txt]
---
# `Template.generate` renders on demand, in Jinja2's chunks

**Problem.** Jinja2's `generate()` and `stream()` are lazy: the template runs as the consumer
asks for items, one item per output node (`"<ul>"`, then `"<li>"`, ...), so a server can send the
first bytes before the render ends, and memory holds one item. Since 0355 the module's
`generate()` renders through the engine's `OutputSink` and yields its chunks (up to 512
characters), but the whole render runs at the first `next()`, and the chunk boundaries are the
engine's (`test_api.py::TestStreaming::test_basic_streaming`, `test_buffered_streaming` in
`python/tests/upstream/expected-failures.txt`).

**Proposal.** Two parts, decided apart:
- On-demand rendering: the engine renders on a worker thread that takes the GIL for Python
  callables and hands each chunk to the consumer (back-pressure: one chunk in flight); closing the
  generator ends the render through the sink. To settle first: `contextvars` (run in a copy of
  the consumer's context), Ctrl-C (the poll hook runs off the main thread), recursion limits,
  and the cost per chunk. Or a coroutine-style engine render (larger).
- Jinja2's item boundaries: a sink mode that flushes per output node rather than per 512
  characters, if callers depend on them (Flask's `stream_template` does not).

**Done when.** `next(t.generate())` returns before a later part of the template runs (a
callable with a side effect shows it), and the two upstream tests pass or are `deliberate`.

## Decisions

- On demand (part 1): `_jinja2cpp._Generation` runs the render on a Python daemon thread
  (`threading.Thread`, so the stack is a Python thread's: 16 MiB on macOS) inside
  `contextvars.copy_context().run`. Worker and consumer take turns under one mutex: one runs while the
  other waits without the GIL, so items, errors and Python objects are only touched by the side whose
  turn it is. The worker hands over once it gathered 512 characters of items (or at the end); the
  template thus runs at most one batch ahead of the consumer (recommended option of the decision
  card, 2026-11-10, taken while Ruslan's answer was open). Close, `del` and the generator's finalizer
  cancel the render through the sink and the poll; an `atexit` hook closes parked renders before
  finalization; at finalization a parked worker is left to the process exit.
- Ctrl-C: a consumer on the main thread wakes every 50 ms, asks the worker for the GIL (the worker's
  poll yields it), runs `PyErr_CheckSignals`; a KeyboardInterrupt cancels the render, is sent to a
  Python callable running on the worker (`PyThreadState_SetAsyncExc`) and is raised in the consumer.
- Items (part 2): `RenderOptions::writeEachPiece` (public, additive): a body flushes the render's
  sink stream after each node, a macro or call block holds the flush (`OutStream::PieceHold`) so its
  output is one item, as Jinja2's. Costs one register test per body node when off.
- Measured: `"".join(generate())` of a 1.7 MB output takes about 2x `render()` (a handoff about
  25 us per 512 characters); a 130-character template 0.4 ms against 0.2 ms (thread start).

**Result.** `test_basic_streaming` and `test_buffered_streaming` pass (upstream 626 -> 628 of 856).
Left (task 0452): Jinja2's constant folding (`a{{ 1 }}b` is one item), `super()` and a recursive
loop's call as one item.
