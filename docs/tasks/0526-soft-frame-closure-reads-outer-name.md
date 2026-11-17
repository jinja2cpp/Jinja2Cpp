---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/939
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

**Resolution (batch 9 C).** A soft frame's end (FrameEnd::Soft) now gives the closures that escape it a copy in
which the names the body bound read as undefined: the names its scopes and frame views hold then
(RenderContext::HideNamesDownTo) and the names the parser saw it store, in a branch too
(NameResolver::NamesResetAtEnd, kept on the loop, `with`, `filter`, `autoescape`, block `set` and `generation` statements only
when a macro or call block is defined inside). A loop's body scope is emptied after each pass, so its stores need
the parser's list. `MacroKeptPastSoftBodyTest` (top level and include, narrow and wide, Default and Strict) and
corpus `statements.macro_kept_past_*`. A `with` or loop target read after its body still prints the undefined
value where Jinja2 prints `missing` (0165 deliberate), now instead of the frame below's value.
