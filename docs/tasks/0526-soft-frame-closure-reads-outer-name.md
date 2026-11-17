---
status: open
priority: low
area: parity
depends: [0373]
touches: [src/statements.cpp]
shares: [test/macro_test.cpp]
---
# A closure kept past a `filter`, `with` or loop body reads the wrong value of a name the body rebound

**Problem.** Found by the verifier of batch 8 C (0491, 0504). A closure defined in a `filter` body and kept past
it, where the body rebinds a name of the frame below
(`{% set v='V' %}{% filter upper %}{% set v='F' %}{% macro inner() %}{{v}}{% endmacro %}{% set ns.f = inner %}x{% endfilter %}{{ ns.f() }}`),
prints `V` (the outer frame's copy) where Jinja2 3.1.6 prints `` (the body's `missing` after its end). The same
shape differs from Jinja2 for an include and at the top level, and for `{% with %}` and `{% for %}` bodies
(the verifier's probes i1-i4). A stopped body (0504) matches.

**Proposal.** Probe each shape against Jinja2 first; a soft frame's end should hide from its closures the names
it bound, rather than let the frame below show through (FrameEnd::Soft records nothing for them today).

**Done when.** `MacroTest` rows for filter, with and for bodies, at the top level and in an include, print
Jinja2's output.
