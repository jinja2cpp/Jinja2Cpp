---
status: open
priority: low
area: parity
depends: [0020]
touches: [src/internal_value.cpp#InputValueConvertor, src/internal_value.cpp#ValuesListAdapter, src/internal_value.cpp#ValuesMapAdapter, src/render_context.h]
shares: [src/value_methods.cpp, src/expression_evaluator.cpp]
---
# Mutation follow-ups: aliases of context data, cycles, live loop length

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

- Shared ownership cannot free a cycle, so storing a list or dict in itself
  (`x.append(x)`, through `update`, `insert`, `setdefault`) raises "a list or dict cannot
  contain itself" where Python prints `[...]`. Values captured by macros or user
  callables are not inspected, so a cycle through them leaks.
- `loop.length`, `loop.last` and `loop.revindex` are taken before the body runs; a list
  that grows or shrinks inside the loop is iterated live, so they can be wrong.

**Proposal.** Convert each borrowed container once per render into a lazily-filled
copy-on-write state shared by every value converted from it (a per-render cache keyed by
the source address, owned by the `RenderContext`), so all aliases see the first write.
Keep the read path free of allocations for templates that never mutate. For cycles, a
render-scoped arena that owns template containers (freed when the render ends) would allow
Python's behaviour.

**Done when.** No line of `test/parity/divergences/` names 0049, and an ASan/LSan run of
the corpus is clean.
