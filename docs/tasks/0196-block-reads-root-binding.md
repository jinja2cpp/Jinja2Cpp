---
status: open
priority: low
area: parity
depends: [0168]
touches: [src/name_resolver.cpp, src/statements.cpp#block]
shares: [test/parity/cases/statements.py]
---
# A block reads the context, not the root's entry binding

**Problem.** Since 0168 a frame binds a name it stores later as undefined at its entry, so
that a frame inside it reading the name earlier sees undefined (Jinja2's idtracking). A
Jinja2 block is a function of its own that reads names from the context, never the root
frame's locals: in
`{% macro m() %}[{{ x }}]{% endmacro %}{% block b %}{{ x }}{{ m() }}{% endblock %}{% set x = 1 %}`
with `x=3` the block prints `3` and the macro `[]`. A Jinja2C++ block renders on the
template's top-level scope and would see the binding, so `NameResolver::EntryBinding` leaves
a name unbound when a block inside the frame reads it: the macro then prints `[3]`
(`statements.idtracking_block_and_macro_read_root_name`).

**Proposal.** Let a block's lookups pass over the entry bindings of the frames around it
(for example a marker the binding statement sets on the undefined it stores, skipped by the
lookup a block's scope view makes), then drop the block exception in `EntryBinding`.

**Done when.** No divergences line names 0196.
