---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/954
priority: low
area: parity
depends: [0526]
touches: [src/statements.cpp#RenderLoopInScopes]
shares: [test/macro_test.cpp]
---
# A closure kept past a recursive loop loses the names the body stored

**Problem.** Found by the 0526 verifier. A recursive loop's body is a function in Jinja2 3.1.6, which resets
nothing as it ends, so a closure defined in it and kept past the loop reads the body's names as they last were:

```jinja
{% set ns = namespace(f=none) %}{% set v = 'V' %}{% for a in [1] recursive %}{% set v = 'B' %}{% macro inner() %}[{{ v }}]{% endmacro %}{% set ns.f = inner %}{% endfor %}{{ ns.f() }}
```

prints `[B]` in Jinja2 and `[V]` here. Since 0526 a recursive loop ends as a stopped one does
(FrameEnd::Stopped, a copy of its scopes), which keeps its target and `loop`, but the body scope is emptied after
each pass, so the copy misses what the last pass stored and the frame below shows through.

**Proposal.** For a recursive loop whose body defines a closure (NameResolver::NamesResetAtEnd is not empty),
copy the body scope before it is emptied after the last pass, or keep the last pass's scope until the loop ends.

**Done when.** A `MacroKeptPastSoftBodyTest` row with the template above prints `[B]`, narrow and wide.
