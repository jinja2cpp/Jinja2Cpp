---
status: open
priority: low
area: parity
depends: [0020]
touches: [src/internal_value.cpp#InputValueConvertor, src/internal_value.cpp#ValuesListAdapter, src/internal_value.cpp#ValuesMapAdapter, src/render_context.h]
shares: [src/value_methods.cpp, src/expression_evaluator.cpp]
---
# Mutation follow-ups: aliases of context data, cycles, loops over changing lists

**Problem.** Since 0020, lists and dicts the template builds are shared, as Python's are,
but data from the render context stays borrowed (`ByRef` into the caller's const
`ValuesMap`, reused across renders). A mutating method copies a borrowed container on its
first write and stores the copy back where the receiver came from (the variable, or the
owning list or dict). Every other name that already referred to the old data keeps it:

- `{% set y = l %}{% do y.append(9) %}{{ l }}` prints `[3, 1, 2]`; Python `[3, 1, 2, 9]`
  (corpus `methods.context_alias`).
- `{% for i in l %}{% do l.pop() %}{{ i }}{% endfor %}` over a context `l` iterates the
  old list (`312`; Python `31`). With a list the template built it matches.
- a loop variable over a context list of dicts (`{% for m in messages %}{% do m.update(..) %}`)
  changes a copy that is dropped at the end of the iteration;
- a variable found in a scope copied by `with`, a set block or a filter block, and a bound
  method value (`{% set f = l.append %}`) on borrowed data, write to a copy.

Two smaller leftovers of the same change:

- Cycles: since 0110 a list, dict or namespace may contain itself, as in Python, and the
  render frees such cycles when it ends (`CycleCollector`). A cycle through a bound method's
  receiver or a key-value pair, which the collector cannot follow, still raises "a list or
  dict cannot contain itself through a bound method or a key-value pair"
  (`methods.list_contains_its_bound_method`); cycles through other objects leak (0251).
- `loop.length`, `loop.last` and `loop.revindex` are taken before the body runs; a list
  that grows or shrinks inside the loop is iterated live, so they can be wrong.
- `for` reads one item ahead (for `loop.last` and `loop.nextitem`), Jinja2 only when
  those are used: removing from the list being iterated visits a different set of items
  (`{% set q = [1, 2, 3] %}{% for x in q %}{{ x }}{% do q.remove(x) %}{% endfor %}` gives
  `12`, Python `13`). Appending during the loop matches.

And methods 0020 left out, all raising or diverging without crashing: `list.sort(key=,
reverse=)`, `d.get([1])` (an unhashable key, Python TypeError), nested format fields
(`'{:{}}'.format(1, 5)`), non-ASCII case mapping in `upper`/`capitalize`/`swapcase` (shared
with the filters), and `|attr('upper')` / `|map(attribute='upper')`, which look up items
only.

**Proposal.** Convert each borrowed container once per render into a lazily-filled
copy-on-write state shared by every value converted from it (a per-render cache keyed by
the source address, owned by the `RenderContext`), so all aliases see the first write.
Keep the read path free of allocations for templates that never mutate.

**Done when.** No line of `test/parity/divergences/` names 0049, and an ASan/LSan run of
the corpus is clean.

**Plan.** Approved 2026-10-25 (D1-D8), `/mnt/project-files/release-2.0/0049-plan.md`; phases
P1 (collector, #663, done), P2 (borrowed cells), P3 (loops, folds 0304), P4 (0212 keys and
callables), P5 (JSON identity and unpacking).

**P2, borrowed cells.** An adapter of the host's data (`ValuesListAdapter`, `ValuesMapAdapter`,
`GenericListAdapter`, `GenericMapAdapter`) carries a `CellLink` (src/borrowed_cells.h). The first
write copies the data into a `BorrowedCell` that the render's registry (`BorrowedCells`, one per
render and one per template module) keys by the data's identity; the writing adapter and every
other adapter of the same data read the cell from then on (a global epoch tells them to look).
The host's data is never written (D2), and the registry detaches its adapters when the render
ends, so the thread's globals read their own data again in the next render. `list()` of the
host's data now copies it, as Python does. `methods.context_alias` and
`methods.operand_changed_by_call` match; rows `methods.alias_*` and test/borrowed_cells_test.cpp
pin the rest.

A container a render hands out (an argument of a user callable, which the host may keep) is a
view while the render runs, so the host sees the render's later writes, as Python's object
would. When a render that wrote to the host's data ends, the containers the host still holds
become a copy of what they show then, down to their nested containers (`ExportSlot`,
`FreezeExports`): a view would read the host's data again once the cells are detached, so a
nested write would revert under a kept container (review of #673). Only kept containers are
copied, once each, at the render's end. A range, a list of unknown length, a namespace, a loop,
a host object, a dict with keys that are not text, a cycle, anything deeper than 256 containers,
and a container whose copy throws (out of memory, a host accessor) stay views.

Perf: an item adapter in a block (`ItemAdapters`) has a `CellLink` whose destructor does
nothing; the block releases its slots, and skips that when no cell existed since it was made
(`QuietEpoch`). An adapter on its own is an `OwnedAdapter`, which only adds a destructor that
releases the link, so the adapter's code is compiled once. A map item of the caller's
data skips the convertor's visit (`LendMapItem`). Every host-data row is within +1% of master
c39c06a (`bench/count.py`, convert and reflect).

Left for later phases:
- the loop rows (`pop` in a loop over context data, `loop.length` after an append): P3;
- a JSON value read twice gives two accessors (`{% set y = j.a %}{% do j.a.append(2) %}{{ y }}`
  prints `[1]`, Python `[1, 2]`): the binding has no stable identity until P5's `IJsonNode`
  marker (plan section 9);
- the write-back of a copy into its variable or owner (`EvaluateMutableRoot`,
  `EvaluateIndices(forMutation)`) stays for the containers that are not the host's data (a
  context map of the engine, a computed list), and stores the same value back for a cell.
