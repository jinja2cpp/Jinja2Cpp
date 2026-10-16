---
status: done
priority: low
area: parity
depends: [0170]
touches: [src/testers.cpp, src/value_visitors.h]
---
# A dictsort pair has no identity, and sort folds case inside tuples

**Problem.** Two gaps the 0170 verifier found next to the dictsort pair work, both present on
master before it:

- `is sameas` is false for a dictsort pair even against itself (`{% set r = p %}{{ r is sameas p }}`
  gives False, Python True): `IsSameObject` in `src/testers.cpp` compares list and map identities
  and has no case for `TestKind::KVPair`, which is held by value in a box.
- `|sort` with the default `case_sensitive=false` lowercases strings nested in tuples:
  `[('B', 2), ('a', 3)]|sort` gives `a,B` where Python gives `B,a`. Jinja2's `ignore_case`
  postprocessor lowers only a top-level string item; tuples compare as they are.

**Proposal.** Give the boxed pair an identity (the box address) in `IsSameObject`; in the sort
key, apply case folding only to string items, not recursively into lists and tuples.

**Done when** the corpus cases `filters.dictsort_pair_sameas` and `filters.sort_tuple_case`
match and their lines in `test/parity/divergences/filters.txt` are deleted.

**Done.** `IsSameObject` compares a pair's box. The case folding moved out of the nested
comparisons: `BinaryMathOperation` compares list, tuple and dict items case-sensitively, so
`sort`, `min`, `max`, `unique`, `groupby` and `dictsort` all fold only a top-level string, as
Jinja2's `ignore_case` does. Pinned by `filters.min_max_tuple_case` and `filters.groupby_tuple_case`.
