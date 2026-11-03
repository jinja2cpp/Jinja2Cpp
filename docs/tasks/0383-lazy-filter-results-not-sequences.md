---
status: in-progress
priority: low
area: parity
depends: [0101]
touches: [src/filters.cpp#SequenceAccessor, src/expression_evaluator.cpp#SliceExpression]
shares: [test/parity/divergences/filters.txt, test/parity/divergences/subscripts.txt]
---
# A lazy filter result is no sequence: `last` and slicing it

**Problem.** In Python `map`, `select`, `reject`, `selectattr`, `rejectattr`, `batch`,
`slice`, `unique` and `items` return a generator, and `reverse` a list's
`list_reverseiterator`. Neither is a sequence: `|last` calls `reversed()` on it and a
slice indexes it, so both raise TypeError.

- `{{ [1, 2]|select|last }}` fails with "'generator' object is not reversible".
  Jinja2C++ renders `2`.
- `{{ ([1, 2]|map('string'))[0:1] }}` fails with "'generator' object is not
  subscriptable". Jinja2C++ renders `['1']`.
- `{{ [1, 2]|reverse|last }}` fails ("'list_reverseiterator' object is not reversible").
  Jinja2C++ renders `1`.

Jinja2C++ results are lazy too, but they answer as lists. The corpus pins this with
`filters.last_of_lazy_result` and `subscripts.slice_lazy_result`, listed as `accepts`
under this task. Found by 0362, which made the other values without items or `reversed()`
raise as Python does.

**Decision needed.** This is the same trade as 0101, where reuse of a lazy result was
kept as a deliberate divergence. Matching Python turns templates that render today into
errors. `|list|last` and `(x|list)[a:b]` work in both engines. The options:
- (a) Raise as Python does. This needs a generator mark on the lazy `ListAdapter`s these
  filters make, which `reverse` needs too.
- (b) Keep answering and record the divergence as deliberate in docs/parity.md next to
  0101's row.

**Done when.** Ruslan has chosen. The two corpus lines are then either gone, with the
mark in place and checked narrow and wide, or marked deliberate.

**Decision (Ruslan, 2026-11-03).** (a): raise as Python does. 2.0 is the breaking release,
and a template shared with Python (chat templates) that works here but fails there is the
worse surprise; `|list` fixes it in both engines.

**As built.** `ListAdapter::CreateIterator(items, IteratorKind)` makes the same shared list
adapter with an iterator kind (three bits of `ValueObject`'s flags, read without a virtual call).
The lazy filters mark their results `Generator`; `reverse` marks `ListReverse`,
`TupleReverse`, `RangeIterator` or `DictReverseKey` by its input, and reverses an iterator into
a plain list, as Jinja2's `list(value)` fallback does; so is reverse of a host object without reversed(). A marked list refuses `|last`
(not reversible), `|length`, `|count` and `|random` (no len()) and slices (not subscriptable),
indexes to undefined (Jinja2 swallows getitem's TypeError), is no sequence, has no list methods,
and `PythonTypeName` names its Python type. Iteration, `|first`, `|list`, `|join`, `|sort`,
`in` and for loops are unchanged. Left for 0387: `==`, `+`/`*`, tojson and the printed form;
the MIGRATION.md note is 0388.
