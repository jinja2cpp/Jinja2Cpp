---
status: done
priority: low
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/733
area: parity
touches: [src/statements.cpp, test/parity/divergences/statements.txt, src/render_context.h, test/parity/divergences/loader.txt, test/parity/cases/loader.py]
---
# A `self` an include stored does not see the names the includer set

**Problem.** Found by the 0223 scenario suite (row F2). A `self` stored by an included template and
called after the include ended renders the block without the names the including template set
with `{% set %}`:

```jinja
{# main #}{% set ns = namespace(s=none) %}{% set x = 'X' %}{% include 'inc' %}|{{ ns.s.blk() }}
{# inc  #}{% block blk %}B{{ x }}{% endblock %}{% set ns.s = self %}
```

Python Jinja2 3.1.6 prints `BX|BX`; Jinja2C++ prints `BX|B` (a name passed as a render
parameter is seen). Pinned by `statements.stored_self_sees_includer_names`.

**Proposal.** A stored `self` keeps the context the include rendered in (Python's block
functions close over the include's context, which includes the includer's names), as 0207 did
for its frame.

**Done when.** The corpus case matches and its divergence line is deleted; ASan clean.

**Fix (parity batch 2, group C).** An included template's context ends before the render
(`RenderContext::SetEndsBeforeRender`): its top-level macros and its `self` register a
`ClosureCapture` (see 0326), and the include copies every name they see, the includer's
included, when it ends (`RenderIncludedTemplate`). A stored `self` renders its ended blocks with
the copy in the block context's scope, and a `self` inside them is made from the same site. Also
fixed: `loader.include_defined_macro_escapes`, `loader.self_of_ended_include_reads_names`
(0165's deliberate lines). Corpus rows `loader.include_macro_kept_*`, `loader.include_nested_macro_kept`,
`loader.self_of_ended_include_last_one`, `loader.self_of_ended_include_calls_self`.
