---
status: in-progress
priority: medium
area: parity
depends: [0355]
touches: [python/src/environment.cpp, python/jinja2cpp/environment.py, include/jinja2cpp/template.h, src/renderer.h, src/out_stream.h, src/statements.cpp]
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

- Items (part 2, done): `RenderOptions::writeEachPiece` (public, additive): a body flushes the
  render's sink stream after each node; a macro or call block holds the flush
  (`OutStream::PieceHold`), so its output is one item, as Jinja2's. It costs one register test per
  body node when off (preflight `--perf`: largest change +0.64%). `generate()` renders with it at the
  first item on the caller's thread and yields the items.
- On demand (part 1): a worker-thread render was built and verified on this branch (commit
  dc2a6fd9: daemon thread taking turns with the consumer, back-pressure of one 512-character
  batch, contextvars copy, Ctrl-C through the poll). The verifier found it cannot keep Jinja2's
  semantics: the template's callables run on another OS thread, so sqlite objects, `threading.local`
  state and locks the caller holds break, gevent's patched Thread deadlocks, and a daemon consumer at
  interpreter exit aborts (pthread_exit unwinding through noexcept destructors). Moved to 0457 for a
  render that resumes on the caller's thread (decision card 2026-11-10, recommended option, taken
  while Ruslan's answer was open).

**Result.** `test_basic_streaming` and `test_buffered_streaming` pass (upstream 626 -> 628 of 856).
Left: 0456 (items Jinja2 folds into one), 0457 (lazy rendering).
