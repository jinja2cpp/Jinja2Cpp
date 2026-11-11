---
status: open
priority: medium
area: parity
depends: [0457]
touches: [python/jinja2cpp/environment.py, python/src/environment.cpp]
shares: [python/tests/test_generate.py, docs/python.md]
---
# Lazy async iteration of a render, for Starlette and FastAPI streaming

**Problem.** After 0457, `Template.generate()` renders on demand on a fiber that belongs to the thread
that started it, and `next()` from another thread raises `RuntimeError` (0457 decision D2). Starlette's
`StreamingResponse` (and so FastAPI's) runs each `next()` of a synchronous iterator as a separate job in
a thread pool (`iterate_in_threadpool`), so `StreamingResponse(t.generate(...))` fails at a later item
whenever the pool picks another thread. Jinja2 allows it. Our `generate_async` hands the template to
Python Jinja2 and needs `enable_async=True`.

**Proposal.** Offer a lazy async iterator over the same fiber render: `__anext__` resumes the render on
the event-loop thread (Starlette iterates async iterators there), so the thread rule holds. Decide
whether it is `generate_async` without `enable_async` (a divergence in when it is allowed) or a module
extra; await callables only where Jinja2's async mode would. Check with Starlette's
`StreamingResponse` under `httpx`'s ASGI transport and a thread pool of several workers.

**Done when.** A Starlette app streaming a template through the new iterator sends the first chunk
before a later callable runs, never raises the thread error, and matches Jinja2's output.
