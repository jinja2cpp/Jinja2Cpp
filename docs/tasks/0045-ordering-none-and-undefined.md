---
status: open
priority: low
area: parity
depends: [0034]
touches: [src/filters.cpp#CompareForOrder]
shares: [src/filters.cpp]
---
# `sort`, `min` and `max` over `None`, undefined values or dicts

**Problem.** Found in 0027. Jinja2 raises `TypeError` when `sort`, `min`, `max` or
`dictsort(by='value')` compare `None` with anything, and `UndefinedError` when a sort key
is undefined (`sort(attribute=...)` on an item without that attribute). Jinja2C++ orders
both as "not less" and renders (`errors.sort_with_none`, `errors.sort_undefined_attribute`).
`CompareForOrder` in `src/filters.cpp` already rejects values of different kinds, but
`None` and undefined are the same empty value until 0034 tells them apart, and an
undefined operand is let through on purpose. `sort` also lets two dicts through
(`errors.sort_dicts`): Jinja2 compares sort keys wrapped in lists, so equal dicts sort
fine and only unequal ones raise, and Jinja2C++ cannot test mappings for equality yet.

**Proposal.** Once 0034 lands, classify `None` as unordered in `GetOrderKind` and make an
undefined operand raise as Jinja2's `Undefined` does (respecting the undefined policy:
`ChainableUndefined` and friends behave the same for `<`). With dict
equality in place, make `CompareForOrder` raise for unequal dicts under `sort`.

**Progress (wave 3 train).** With 0015's dict equality and `==`-first comparison in
`sort`, and 0034's distinct `None`, the three `sort` cases match and their lines are gone.
`min`, `max` and `dictsort(by='value')` over `None` still go through `CompareForOrder`,
which lets an empty operand through; the corpus has no case for them yet, so add one
before closing this task.

**Done when.** No line of `test/parity/divergences/` names task 0045, and `ctest -R parity` passes.
