---
status: in-progress
priority: low
area: parity
depends: [0020]
touches: [src/value_methods.cpp#StrOps, src/value_methods.cpp#ListMethods, src/case_mapping.h, src/unicode_case.h, scripts/gen_unicode_case.py]
shares: [src/string_converter_filter.cpp, test/parity/cases/methods.py, test/parity/divergences/methods.txt]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/662
---
# Methods 0020 left out: list.sort, nested format fields, Unicode case

**Problem.** Split from 0049 (plan section 2.8, phase P0). Three Python methods diverged
without crashing: `list.sort()` did not exist ("no attribute 'sort'"), `str.format` refused
a field nested in a format spec (`'{:{}}'.format(1, 5)`, "Nested replacement fields are not
supported"), and the case methods `upper`, `lower`, `capitalize`, `swapcase` and `title`
mapped ASCII only (`'straße'.upper()` gave `STRAßE`), although the filters had used the full
Unicode mappings since 0048. `isupper`/`islower` were ASCII only too, and `istitle` was
missing.

**Proposal.** 
- `list.sort(*, key=None, reverse=False)`: keyword-only, stable, `<` only (equal dicts or
  Nones are a TypeError, as in Python, unlike the `sort` filter), keys taken once per item
  through the template's call path (a macro or a bound method works as `key`), `reverse`
  by truth value. As in CPython the list reads as empty while keys run and compare, a list
  changed meanwhile is a ValueError, and a failing key or comparison leaves it unchanged.
- `str.format`: port CPython's `MarkupIterator`/`parse_field`: a `[key]` may hold `}` or
  `:`, a spec ends at the balancing `}`, nested fields are expanded after the value is
  looked up, sharing the auto numbering, one level deep ("Max string recursion exceeded").
  `Markup.format` escapes the nested values too, as markupsafe does.
- Case: the shared code points helpers move from `string_converter_filter.cpp` into
  `src/case_mapping.h`, which adds CPython's `do_swapcase` and `do_title`. The generated
  `unicode_case.h` gains the Uppercase, Lowercase and titlecase sets
  (`scripts/gen_unicode_case.py`), which `swapcase`, `title`, `isupper`, `islower` and the
  new `istitle` need. ASCII strings keep their fast paths.

**Done when.** The `methods.list_sort*`, `methods.str_format_nested*`,
`methods.str_case_*`, `methods.str_istitle` and `methods.markup_*` corpus cases match in
the narrow and wide runs, and 0049's task file no longer lists these methods.
