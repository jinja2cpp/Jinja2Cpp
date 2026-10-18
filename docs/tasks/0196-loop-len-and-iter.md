---
status: done
priority: low
area: parity
depends: []
touches: [src/statements.cpp#LoopAccessor, src/filters.cpp#SequenceAccessor]
shares: [test/parity/divergences/statements.txt]
---
# `loop` as a sized iterable

**Problem.** Jinja2's `LoopContext` defines `__len__` (the loop's length) and `__iter__`
(the remaining items). Jinja2C++ stores `loop` as a map, so `loop|length` counts its keys
(`statements.loop_len`: Jinja2 prints `22`, Jinja2C++ `1313`), and iterating `loop`,
`loop|list` or `loop is iterable` see the key names. Its repr is the map, where Jinja2
prints `<LoopContext 1/2>`. Found while fixing 0191, which made the non-recursive loop
callable and `loop is mapping` false.

**Proposal.** Give the loop accessor a length and an item sequence that the sequence
filters, `for` and the iterable test use ahead of the map keys, and Jinja2's repr. Making
`loop` an object (`GetObjectClass()`) is the wrong tool: objects are not sized or iterable.

**Done when.** No line of `test/parity/divergences/` names 0196.

**Outcome.** `len(loop)` and the repr landed here; iterating `loop` (and refusing it in
`dictsort`/`items`) is split out to 0219.
