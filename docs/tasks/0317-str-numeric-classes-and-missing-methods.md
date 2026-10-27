---
status: open
priority: low
area: parity
touches: [src/value_methods.cpp#StrOps, scripts/gen_unicode_case.py, src/unicode_case.h, src/unicode_tables.h]
shares: [test/parity/cases/methods.py, test/parity/divergences/methods.txt]
---
# str methods: Unicode digit and numeric classes, casefold, isascii, isidentifier, isprintable, format leftovers

**Problem.** Found while doing 0314. `str.isdigit()` and `str.isnumeric()` test the decimal
class only, so `'²'.isdigit()` and `'½'.isnumeric()` are false here and true in Python
(`methods.str_digit_numeric_unicode`). `casefold()`, `isascii()`, `isidentifier()` and
`isprintable()` are not implemented at all ("has no attribute"),
`methods.str_casefold_isascii`. Two str.format leftovers, found by the 0314 verifier: a
field's `.attr` part also finds a dict item (`'{x.y}'.format(x={'y': 1})` gives `1`, Python
AttributeError, `methods.str_format_attr_is_not_item`), and `!a` converts as `!r` without
escaping non-ASCII characters (`'é'`, Python `'\xe9'`, `methods.str_format_ascii_conversion`).

**Proposal.** Generate Digit and Numeric range tables from the pinned Python (as
`gen_unicode_case.py` does for case), and a `str.casefold()` mapping table with the same
run/multi layout as the case tables. `isprintable` can reuse `src/unicode_printable.h`
(repr's table); `isidentifier` needs XID_Start/XID_Continue ranges. `isascii` needs no data.
`ApplyAccessors` looks up `.attr` as an attribute only (methods and reflected fields), and
`!a` applies `ascii()`'s escaping to the repr. Markup gets each method too
(`MarkupMethods` must stay the same size as `StrMethods`).

**Done when.** The four corpus cases match and their lines are gone from
`test/parity/divergences/methods.txt`.
