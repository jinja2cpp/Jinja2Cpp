---
status: open
priority: high
area: parity
depends: [0001]
touches: [src/value_visitors.h, src/internal_value.cpp, src/string_converter_filter.cpp, src/filters.cpp, test/parity/]
---
# Print values the way Python `str()` does

**Problem.** Jinja2 prints every value through Python `str()`. Jinja2C++ prints `None` as
nothing, booleans as `true`/`false`, whole floats without `.0`, floats with 8 significant
digits, and lists, tuples, dicts and `range` objects as nothing at all. `join` drops items
that are not strings instead of converting them. The template author writes against
Jinja2's output, so every template that prints a list, a boolean or a computed float
renders differently; it is also the largest single source of noise in the parity corpus,
where a correct filter followed by a wrong repr looks like a broken filter (40 cases).

**Proposal.** One conversion routine with Python semantics, used by output, `~`, `join`,
and the `string` filter (0018):
- `None`, `True`, `False`;
- floats as Python `repr` (shortest round-trip, `1e+20`, `1e-07`, `inf`, `nan`),
  whole floats with `.0`;
- lists `[1, 'a']`, tuples `(1,)`, dicts `{'a': 1}`, nested values with `repr()` of
  strings (quote choice as Python: `"b'c"`);
- `range(0, 3)`.
Keep the old behaviour reachable only if a user depends on it: a `Jinja2CompatMode`
value is the existing hook. Wide-string output must follow the same rules.

Cases: `output.*`, `literals.bool_*`, `literals.list*`, `literals.float_*`,
`filters.join_numbers`, `operators.list_plus`, `statements.macro_varargs_print`.

**Done when.** No line of `test/parity/divergences.txt` names task 0012, and `ctest -R parity` passes.

**Next.** Changing the printed form of booleans and floats breaks existing user output;
the release notes need a migration line, and existing unit tests that encode `true` will
need updating (they were written against the old behaviour, see 0001).
