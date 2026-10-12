---
status: open
priority: low
area: parity
depends: [0052]
touches: [src/filters.cpp, test/parity/cases/fuzz.py, test/parity/divergences/fuzz.txt]
---
# Sequence filters on a value Python cannot iterate render nothing

**Problem.** Found while fixing `join` of a bool (0094). The filters that iterate their
input (`list`, `first`, `last`, `sort`, `unique`, `sum`, `min`, `max`, `map`, `select`,
`reject`, `groupby`, `slice`, `batch`, `length`, `random`, `reverse`, `dictsort`) return
an empty value when given a number, a bool or None: `ConvertToList` fails and the filter
returns `InternalValue()`. Python raises: TypeError "'int' object is not iterable" (or "has
no len()", "is not reversible"), FilterArgumentError "argument must be iterable" for
`reverse`, AttributeError for `dictsort`. Undefined still iterates as empty in Python
(`nope|list` is `[]`), so only defined non-iterables fail. `join` already raises (0094).
Pinned by `fuzz.list_int`, `fuzz.first_bool`, `fuzz.sort_none`, `fuzz.length_int` and
`fuzz.dictsort_int`; `5|batch(2)` and `None|batch(2)` are already listed under 0048.

**Proposal.** One helper next to `ConvertToList` that raises Python's TypeError for a
defined value that is neither a sequence, a string, a mapping nor a generator, used at each
`if (!isConverted)` in `src/filters.cpp`. Land it after 0052, which edits the same
functions for undefined values.

**Done when.** The five `fuzz.*` lines tagged 0175 are gone from
`test/parity/divergences/fuzz.txt`.
