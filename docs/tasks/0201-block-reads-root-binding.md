---
status: done
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
(`statements.idtracking_block_and_macro_read_root_name`). Any other read before the store goes
wrong with it: in
`{% block b %}{% for j in [1] %}{{ x }}{% endfor %}{% endblock %}{% for j in [1] %}{{ x }}{% endfor %}{% set x = 1 %}`
Jinja2 prints `3` and Jinja2C++ `33`. An `include` or `import ... with context` before the
store drops the binding the same way (Jinja2 hands them the outer value); the same marker
would let both keep it.

**Proposal.** Let a block's lookups pass over the entry bindings of the frames around it
(for example a marker the binding statement sets on the undefined it stores, skipped by the
lookup a block's scope view makes), then drop the block exception in `EntryBinding`.

**Done when.** No divergences line names 0201.

**Done.** `BindUndefinedStatement` marks the undefined it binds (`UndefinedValue::atEntry`)
and flags its context. A block's context (`RenderBlockAt`), and an include's with context,
call `RenderContext::PassOverEntryBindings`: when a binding was made, their lookups take the
slow path and pass over marked values in the scopes below the context's own, reaching the
value outside; an import with context leaves them out of the names it copies
(`CollectVisibleNames`). The resolver no longer drops a name a block reads, nor one stored
after an include or import with context (`AddContextRead` is gone); a read in a block does
not count as a read inside. Cases: `statements.idtracking_block_macro_reads_context`,
`idtracking_scoped_block_passes_over_binding`,
`idtracking_child_block_passes_over_parent_binding`,
`idtracking_include_before_store_keeps_binding`, `idtracking_import_before_store_keeps_binding`.
The mark carries the low bits of the name's hash, so a copy stored under another name
(`{% with a = x %}` before the root stores `x`) is not passed over; a copy stored back under
the same name in a frame inside (`{% set x = x %}` in a loop, then a scoped block reading `x`)
still is, where Jinja2 sees Undefined: accepted, as no realistic template does it. Further
cases: `idtracking_block_loop_then_reads_context`, `idtracking_block_macro_in_loop_then_after`,
`idtracking_block_changes_list_in_place`, `idtracking_scoped_block_reads_copy_of_binding`,
`idtracking_include_loop_then_reads_context`.
