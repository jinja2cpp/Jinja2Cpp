---
status: done
priority: low
area: parity
depends: []
touches: [src/filters.cpp#Tester, test/parity/cases/filters.py]
---
# `selectattr('a', attribute='a')` fails where Python ignores the keyword

**Problem.** Found by the 0258 verifier (it fails the same way on master before 0258).
`{{ [{'a':1},{'a':0}]|selectattr('a', attribute='a')|list }}` renders `[{'a': 1}]` in
Jinja2 3.1.6, but Jinja2C++ reports an error. Python's `select_or_reject` takes the
attribute and the test name positionally and forwards the remaining positional and keyword
arguments to the test. With no test name the test is `bool`, so an extra keyword is silently
dropped. Jinja2C++ binds `selectattr` to a fixed parameter table, so `attribute=` names a
parameter that is already given by position, or is unknown.

**Proposal.** Bind `select`/`reject`/`selectattr`/`rejectattr` as Python does: the leading
positional arguments, then the rest forwarded to the test. With no test (Python calls
`bool`), drop the extra arguments, or reject them exactly where Python does. Add corpus
cases for `selectattr` and `rejectattr` with no test plus an extra keyword, and with a test
plus a keyword the test does not accept.

**Done when** those cases match Python in narrow and wide renders.

**Done.** `Tester` binds as `select_or_reject` does: the attribute and the test name only by
position, every other argument (keywords included) to the test, and none at all without a
test. A missing attribute fails at the first item ("Missing parameter for attribute name"),
so `selectattr(attribute='a')`, a Jinja2C++-only spelling, is now an error as in Python; the
two unit rows that used it pass the attribute by position. Cases: `filters.select*_extra_*`,
`filters.select_lazy_errors_on_empty` and four in `errors.py`.
