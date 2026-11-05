---
status: open
priority: low
area: parity
depends: [0101, 0383]
touches: [src/filters.cpp#Tester, src/filters.cpp#Map]
shares: [test/parity/divergences/statements.txt, test/parity/divergences/filters.txt]
---
# select and map fetch one item per step

**Problem.** In Python `select`, `reject`, `selectattr`, `rejectattr` and `map` return a
generator that takes one item from the list's iterator per step. Jinja2C++ collects the
whole result when the filter is called (`Tester::Filter`, `Map::Filter` build an
`InternalValueList`). The difference shows when the list changes between the call and the
iteration, or when the source is a host generator:

- `{% for x in l|select('odd') %}{{ x }}{% do l.append(7) if loop.first %}{% endfor %}` with
  `l = [1, 2, 3]`: Python `137`, C++ `13` (`statements.for_over_select_sees_append`).
- The same with `map('string')`: Python `1237`, C++ `123`
  (`statements.for_over_map_sees_append`).
- `{% set s = l|select('odd') %}{% do l.append(5) %}{{ s|list }}`: Python `[1, 3, 5]`, C++
  `[1, 3]` (`statements.select_kept_sees_later_append`).
- A counting host generator (`MakeGenericList` with a generator) under `select`/`map` is
  drained at the call; Python fetches one item per loop step. No corpus case: the corpus
  has no host generators. Item 3 of 0390, moved here.

**Constraints.** 0101 keeps a filter result reusable (iterated again from the start), by
Ruslan's choice; a per-step result must keep that. A lazy result holds the list, the test or
filter and their arguments past the statement that made it (`set s = ...`), so it must own
them, not borrow the render's frames (lifetime rules in REVIEW.md). Measure
`Render/for_filter_if`, `Render/filters` and the chat templates with `bench/count.py`: the
eager list is cheap today.

**Proposal.** An adapter whose enumerator walks the source enumerator and applies the
test or filter per `MoveNext`, owning a copy of the source value and of the evaluated
arguments; `GetSize()` unknown. Or mark the first three deliberate next to 0101 if the cost
or the lifetime risk is too high.

**Done when.** The three corpus lines naming 0392 are gone, or marked deliberate with
Ruslan's word.
