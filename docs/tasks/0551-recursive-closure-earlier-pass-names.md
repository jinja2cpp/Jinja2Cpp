---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/967
priority: low
area: parity
depends: [0537]
touches: [src/statements.cpp#RenderLoopInScopes]
shares: [test/macro_test.cpp]
---
# A closure of a recursive loop reads a name its last pass did not store from the live frame below

**Problem.** Found by the verifier of 0537. Since 0537 a closure kept past a recursive loop reads the names the body's
last pass left. A name an earlier pass stored and the last one did not is read from the frame below the loop as it is
when the closure is called. Jinja2 3.1.6 resolves the body function's locals at the start of each pass, so the
closure reads what the name was below the loop then:

```jinja
{% set ns = namespace(f=none) %}{% for a in [1, 2] recursive %}{% if a == 1 %}{% set w = 'W1' %}{% macro m() %}[{{ w }}]{% endmacro %}{% set ns.f = m %}{% endif %}{% endfor %}{% set w = 'out' %}{{ ns.f() }}
```

prints `[]` in Python and `[out]` here (master too). When `w` is set before the loop both print its value then.

**Proposal.** When the recursive loop's last pass is restored for its closures (`CaptureLastPass`), add each name the
body stores (NameResolver::NamesResetAtEnd) that the last pass did not with the value the frame below had at the
loop's start, or undefined when it had none.

**Done when.** A `MacroKeptPastSoftBodyTest` row with the template above prints `[]`, narrow and wide.
