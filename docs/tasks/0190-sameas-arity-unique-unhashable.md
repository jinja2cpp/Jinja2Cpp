---
status: done
priority: low
area: parity
touches: [src/testers.cpp, src/filters.cpp]
---
# `sameas` takes any number of arguments, `unique` takes unhashable items

**Problem.** Two gaps the 0176 verifier found, both present before it:

- `1 is sameas()` renders False and `x is sameas(a, b)` renders a result; Python raises
  `TypeError` (`test_sameas()` takes exactly one argument).
- `[['a'], ['A']]|unique` yields both items; Python raises `TypeError: unhashable type: 'list'`
  because `unique` keeps a `set` of seen keys. Dicts as items fail the same way.

**Proposal.** Make `other` a required argument of `sameas` and reject extra positional
arguments; in `Unique`, raise for list and map keys (tuples of hashable items stay allowed).

**Done when** `tests.sameas_arity` and `filters.unique_unhashable` match and their lines in
`test/parity/divergences/` are deleted.

**Done.** `divisibleby`, `in` and `sameas` raise the argument error they already recorded
(missing or extra argument), as Jinja2's `test_*()` signatures do; `unique` raises
`unhashable type` for a list or dict key, also inside a tuple or a dictsort pair, and with
`attribute=`. Ranges, tuples of hashable items and namespace objects stay allowed.
Corpus: `tests.{sameas,divisibleby,in}_{arity,extra_arg}`, `filters.unique_unhashable*`,
`filters.unique_hashable_tuple_range`.
