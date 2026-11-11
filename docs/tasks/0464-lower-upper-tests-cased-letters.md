---
status: open
priority: medium
area: parity
depends: [0095]
touches: [src/testers.cpp#HasNoUpperLetter, src/testers.cpp#HasNoLowerLetter, test/parity/divergences/fuzz.txt]
---
# `lower` and `upper` tests: a string with no cased letter is neither

**Problem.** Jinja2's `lower` and `upper` tests are `str.islower()` and `str.isupper()`: true
only when the string has at least one cased character and none of the other case or of
titlecase. Jinja2C++ (`HasNoUpperLetter` and `HasNoLowerLetter` in `src/testers.cpp`) checks
only that no letter of the other case is present, one `char` or `wchar_t` at a time through
`std::islower`/`std::isupper` in the global locale. So `'3' is lower`, `'' is upper` and
`'-7' is upper` are true, and the titlecase `'ǅ'` (U+01C5) counts as upper. The generated
templates of the differential check (docs/tasks/0095) found it: every divergence in 20 000
generated templates but one class was this one.

**Proposal.** Reuse `CaseCheck` of `src/value_methods.cpp` (CPython's `unicode_islower` and
`unicode_isupper` over code points, with `unicode::IsLowercase`, `IsUppercase` and
`IsTitlecase`), for narrow and wide strings alike; the `str.islower()`/`str.isupper()`
methods already go through it.

**Done when.** `fuzz.lower_test_no_cased_letter` and `fuzz.upper_test_titlecase` match and
their lines in `test/parity/divergences/fuzz.txt` are deleted (the cases move to
`cases/tests.py` if the parity steward prefers it), in narrow and wide runs.
