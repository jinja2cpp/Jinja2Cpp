---
status: open
priority: low
area: parity
depends: [0479]
touches: [src/out_stream.h, src/out_stream.cpp]
shares: [python/tests/test_values.py, docs/python.md]
---
# A macro that stops after its output reached the sink keeps the RuntimeError

**Problem.** Left by 0479. A call the template makes that a StopIteration escapes gives undefined, and the call's
output is rolled back (OutStream::RollbackTo). A sink stream (RenderTo, the module's generate() and stream()) can
roll back only while nothing reached the sink since the call began, which is up to its 512-character buffer. A
rollback fails whenever the stream's buffer reached the sink since the call began, which depends on where the
buffer boundary falls, not only on the macro's own output length: `{{ 'p' * 510 }}[{{ m() }}]` with a macro of two
characters fails. A macro that wrote more before stopping, or any call that crossed the boundary, keeps the failure, which the module reports as
`RuntimeError: generator raised StopIteration`. Jinja2 buffers a macro's output, so it yields nothing of it:

```
{% macro m() %}{{ 'x' * 600 }}{{ 1|stop }}{% endmacro %}[{{ m() }}]
    Jinja2 generate(): "[" "" "]"     module generate(): RuntimeError     module render(): "[]"
```

**Proposal.** While a guarded call of a template callable runs on a sink stream, spill what the buffer would
deliver into a reused member string instead of the sink (still counted against Settings::Limits::maxOutputSize),
and deliver it when the outermost guarded call returns. Keep writeEachPiece and lazy-render suspension working;
no allocation when nothing spills.

**Done when.** The template above gives `[]` through RenderTo and the module's generate(), with a C++ row in
MacroStopOutputTest and the long generate() row in python/tests/test_values.py turned around.
