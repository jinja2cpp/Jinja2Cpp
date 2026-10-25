---
status: open
priority: low
area: parity
touches: [src/statements.cpp, test/parity/divergences/statements.txt]
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
