---
status: open
priority: low
area: parity
depends: [0479]
touches: [src/statements.cpp#RenderMacroBody]
shares: [test/macro_test.cpp]
---
# A closure made in a `with` or filter body of a stopped macro is kept with the macro's frame

**Problem.** Left by 0479. When a StopIteration ends a macro call (the call gives undefined, as Jinja2's
Context.call does), RenderMacroBody captures the closures of the macro's frame, and a stopped loop captures its
own. A closure made inside a `{% with %}` or `{% filter %}` body of the stopped macro and stored outside it
(`{% set ns.f = inner %}`) is captured with the macro's frame instead of its own soft frame, so a name that body
binds may read differently from Jinja2 when `ns.f()` runs after the call:

```
{% set ns = namespace(f=none) %}{% macro m() %}{% with v='W' %}{% macro inner() %}{{ v }}{% endmacro %}
{% set ns.f = inner %}{{ 1|stop }}{% endwith %}{% endmacro %}[{{ m() }}]{{ ns.f() }}      Jinja2: "[]W"
```

**Proposal.** Close the soft frames of `with` and filter bodies on the stop path as the stopped loop does
(CaptureClosures(Soft) before the frame goes). Probe the case first: it may already match.

**Done when.** The case above matches Jinja2 with a MacroStopTest row, under ASan+UBSan.
