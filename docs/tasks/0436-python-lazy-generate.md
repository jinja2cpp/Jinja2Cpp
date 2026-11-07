---
status: open
priority: medium
area: parity
depends: [0355]
touches: [python/src/environment.cpp, python/jinja2cpp/environment.py]
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
