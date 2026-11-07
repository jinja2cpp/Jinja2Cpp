---
status: open
priority: low
area: perf
depends: [0355]
touches: [python/src/environment.cpp, python/src/convert.cpp, python/jinja2cpp/integrations/transformers.py]
shares: [python/bench/bench_module.py]
---
# Render with the GIL released when the data is JSON-like (0248 plan D5, phase 3)

**Problem.** The module holds the GIL for the whole render (0353), as Jinja2 does, so threads
of one process render one at a time. The 0248 plan's phase 3 fast path was left out of 0355:
when the render's data is JSON-like and no global, filter or test is a Python callable (as in
`chat_template_environment()`, whose helpers are native since 0355), convert the data eagerly
and release the GIL for the C++ render, so threads render in parallel. `bench/run.py
--python-module` (0355) shows the crossing cost per case; on hf_* Render the module is
1.6-3.6x faster than Jinja2 and 2-3x slower than C++ Jinja2C++ (4-core cloud container,
2026-11-07, one run).

**Proposal.** A check at render (all values JSON-like, every callable reachable native), then
the snapshot path: `ToValue` with `JsonCopyLimits` and `Py_BEGIN_ALLOW_THREADS` around the
render; `PyErr_CheckSignals` polling moves to the end. Measure single-thread cost (the
eager copy can lose on large data the template reads little of) and multi-thread throughput,
then decide the default from the numbers.

**Done when.** A bench row with N threads shows the gain, the single-thread rows do not
regress past the gate, and the default is recorded.
