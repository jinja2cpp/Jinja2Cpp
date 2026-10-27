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

The methods 0020 left out are split off: `list.sort`, nested format fields and Unicode case
mapping in the str methods went to 0314. `d.get([1])`, `|attr('upper')` and
`|map(attribute='upper')` were fixed since.

**Proposal.** Convert each borrowed container once per render into a lazily-filled
copy-on-write state shared by every value converted from it (a per-render cache keyed by
the source address, owned by the `RenderContext`), so all aliases see the first write.
Keep the read path free of allocations for templates that never mutate.

**Done when.** No line of `test/parity/divergences/` names 0049, and an ASan/LSan run of
the corpus is clean.
