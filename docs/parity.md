# Jinja2C++ feature parity map

How far Jinja2C++ is from Python [Jinja2](https://jinja.palletsprojects.com/) 3.1, area by
area, and which task in `docs/tasks/` closes each gap.

Every statement marked with a case id is backed by the differential corpus in
`test/parity/` (1196 templates rendered by both engines, see
[test/parity/README.md](../test/parity/README.md)); `ctest -R parity` re-checks all of
them. Statements in the last section (API level) are read from the headers and are not
corpus-checked yet.

Legend: ✅ matches Jinja2 · 🟡 works with differences · ❌ missing or broken ·
➖ not applicable to C++.

## Summary

Snapshot of `python3 test/parity/generate.py --report` (Jinja2 3.1.6, Oct 2026):

| area | cases | match | output | rejects | accepts | unsupported | unordered | crash | tasks |
|---|---|---|---|---|---|---|---|---|---|
| autoescape | 83 | 77 | 6 | 0 | 0 | 0 | 0 | 0 | 0051 |
| custom | 28 | 28 | 0 | 0 | 0 | 0 | 0 | 0 |  |
| errors | 71 | 70 | 0 | 0 | 1 | 0 | 0 | 0 | 0036 |
| filters | 233 | 222 | 6 | 1 | 4 | 0 | 0 | 0 | 0048 |
| globals | 39 | 35 | 1 | 0 | 3 | 0 | 0 | 0 | 0026, 0042 |
| literals | 56 | 50 | 6 | 0 | 0 | 0 | 0 | 0 | 0015, 0031, 0036, 0041 |
| loader | 58 | 58 | 0 | 0 | 0 | 0 | 0 | 0 |  |
| methods | 89 | 84 | 1 | 0 | 0 | 0 | 4 | 0 | 0043, 0049 |
| operators | 109 | 106 | 0 | 3 | 0 | 0 | 0 | 0 | 0015 |
| options | 35 | 32 | 0 | 0 | 0 | 3 | 0 | 0 | 0029 |
| output | 35 | 35 | 0 | 0 | 0 | 0 | 0 | 0 |  |
| sequences | 39 | 34 | 3 | 0 | 2 | 0 | 0 | 0 | 0037 |
| statements | 155 | 148 | 4 | 0 | 2 | 0 | 1 | 0 | 0026, 0031, 0038, 0042 |
| subscripts | 39 | 38 | 0 | 0 | 1 | 0 | 0 | 0 | 0026 |
| tests | 36 | 36 | 0 | 0 | 0 | 0 | 0 | 0 |  |
| undefined | 41 | 25 | 3 | 0 | 6 | 7 | 0 | 0 | 0026, 0047 |
| whitespace | 50 | 49 | 1 | 0 | 0 | 0 | 0 | 0 | 0044 |
| **total** | **1196** | **1127** | **31** | **4** | **19** | **10** | **5** | **0** | |

*output*: both render, text differs. *rejects*: C++ errors on a valid template.
*accepts*: C++ renders a template Jinja2 rejects. *unsupported*: needs an Environment
option C++ lacks. *unordered*: depends on hash order, so it matches on some standard
libraries and not others. *crash*: skipped because it hits undefined behaviour.

Two gaps account for most of the visible damage, because nearly every template prints
values or calls methods:

1. **Printing values** (0012, done): `True`/`False`, `2.0`, lists, tuples and dicts now
   print as Python does, and `None` prints as `None` now that it is told apart from
   undefined (0034).
2. **Expression grammar** (0013, 0014, done): literals, slices, `a < b < c`, `not in`,
   `is not`, `is divisibleby 3` and Jinja2's operator precedence all parse as in Jinja2.

Next come Python methods on values (0020, `s.strip()`, `d.items()`, used heavily by LLM
chat templates), arithmetic semantics (0015) and the missing filters and tests (0017,
0018).

The earlier audit probe (56/159 matching) understated parity: many of its empty outputs
came from printing lists and from `join` over numbers, which this corpus isolates.

## Tasks

| # | Gap | Priority | Cases |
|---|---|---|---|
| [0012](tasks/0012-python-value-stringification.md) | Print values the way Python `str()` does | high | done |
| [0013](tasks/0013-literal-syntax.md) | Literal syntax: `none`, numeric forms, dict and tuple literals | high | done |
| [0014](tasks/0014-operator-and-postfix-grammar.md) | Operator and postfix grammar: chained compare, `not in`, `is not`, slices | high | done |
| [0015](tasks/0015-arithmetic-and-logic-semantics.md) | Python arithmetic, comparison and `and`/`or` semantics | high | 4 |
| [0016](tasks/0016-strings-as-sequences.md) | Strings behave as sequences | high | done |
| [0017](tasks/0017-builtin-tests.md) | Complete the builtin tests | medium | done |
| [0018](tasks/0018-missing-builtin-filters.md) | Missing builtin filters (`string`, `safe`, `indent`, ...) | high | done |
| [0019](tasks/0019-filter-behaviour.md) | Filter behaviour divergences | medium | done |
| [0020](tasks/0020-python-methods-on-values.md) | Python methods on str, list and dict values | high | done |
| [0021](tasks/0021-loop-and-assignment-statements.md) | Loop controls, loop object, namespace, tuple assignment | high | done |
| [0022](tasks/0022-macro-call-semantics.md) | Macro call semantics | medium | done |
| [0023](tasks/0023-inheritance-and-import.md) | Template inheritance and import semantics | medium | done |
| [0024](tasks/0024-whitespace-and-newlines.md) | Trailing newline, `-` modifiers, newline normalisation | high | done |
| [0025](tasks/0025-autoescape.md) | Autoescape and Markup | medium | done |
| [0026](tasks/0026-undefined-semantics.md) | Undefined semantics and undefined policies | medium | 18 |
| [0027](tasks/0027-reject-invalid-templates.md) | Reject what Jinja2 rejects | medium | done |
| [0028](tasks/0028-delimiters-and-line-statements.md) | Custom delimiters, line statements | low | done |
| [0029](tasks/0029-i18n-extension.md) | i18n extension | low | 3 |
| [0030](tasks/0030-global-functions.md) | Global functions: `cycler`, `joiner`, `lipsum`, `range` | medium | done |
| [0031](tasks/0031-insertion-ordered-mappings.md) | Mappings keep insertion order | medium | 2 |
| [0032](tasks/0032-custom-filters-and-tests.md) | Register custom filters and tests | medium | done |
| [0033](tasks/0033-wide-string-parity.md) | Run the corpus through the wide-string API | low | API |
| [0034](tasks/0034-none-versus-undefined.md) | Tell `None` apart from undefined | high | done |
| [0036](tasks/0036-non-string-mapping-keys.md) | Mapping keys that are not strings | low | 2 |
| [0037](tasks/0037-sequence-protocol-follow-ups.md) | Sequence protocol follow-ups (non-ASCII sort, string self-subscript, `sum`, mapping `is sequence`, zero-width errors) | medium | 5 |
| [0038](tasks/0038-lexical-scoping-for-macros.md) | Lexical scoping for macros | medium | 2 |
| [0041](tasks/0041-string-literal-escapes.md) | String literal escape sequences (`\x`, `\u`, octal, `\N{}`, `\v`) | low | 3 |
| [0042](tasks/0042-loop-cycle-magic-number.md) | Global function follow-ups: `loop.cycle` is the integer 2, globals are maps | low | 6 |
| [0043](tasks/0043-ordered-valuesmap-2-0.md) | Insertion-ordered `ValuesMap` (2.0.0) | medium | 4 |
| [0044](tasks/0044-lstrip-blocks-leftovers.md) | `lstrip_blocks` and modifier leftovers | low | 1 |
| [0045](tasks/0045-ordering-none-and-undefined.md) | `sort`, `min` and `max` over `None`, undefined values or dicts | low | 0 |
| [0047](tasks/0047-none-leftovers.md) | None and undefined: JSON null, `Undefined` repr, string filters on None | medium | 1 |
| [0048](tasks/0048-filter-behaviour-leftovers.md) | Filter leftovers: Unicode case, HTML entities, big ints, unused JSON serializers | low | 11 |
| [0049](tasks/0049-aliasing-borrowed-containers.md) | Mutation follow-ups: aliases of context data, cycles, loops over changing lists | low | 1 |
| [0051](tasks/0051-markup-leftovers.md) | Markup leftovers: `~` under autoescape, Markup methods and repr, Markup from C++ | low | 6 |

Order: `python3 scripts/task_batches.py --area parity` groups the tasks into waves that
can run side by side (Oct 2026: 0012 0013 0016 0022 0033 → 0014 0018 0023 0024 0030 0031
→ 0015 0027 0034 → 0017 0019 0020 0028 → 0021 0025 0032 → 0026 0029). 0012 comes first
because it makes the remaining divergences readable: today a wrong filter and a wrong
repr look the same.

## Literals and keywords (`literals`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| Integers, floats, strings, `\n` `\r` `\t` escapes, unicode | ✅ | `int`, `float`, `string_*` | |
| `\xHH`, `\uHHHH`, octal, `\N{...}` and `\a` `\b` `\f` `\v` `\0` escapes | ❌ backslash dropped, rest kept | `string_escape_hex_octal`, `string_escape_control`, `string_escape_named` | 0041 |
| `true`/`false`/`True`/`False` | 🟡 parse; print as `true` | `bool_lower`, `bool_title` | 0012 |
| `none`/`None` | ✅ | `none_lower`, `none_title` | |
| `1_000`, `0x1F`, `0o17`, `0b101` | ✅ | `int_underscore`, `int_hex`, ... | |
| Exponent floats `1e3` | 🟡 prints `1000` | `float_exponent` | 0012 |
| Integers beyond 64 bits | 🟡 deliberate: a literal beyond int64 becomes a float, arithmetic overflow raises | `int_big` | 0015 |
| Adjacent strings `'a' 'b'` | ✅ | `string_adjacent_concat` | |
| List literals, trailing comma | 🟡 parse; print as empty | `list_trailing_comma` | 0012 |
| Tuple literals `(1, 2)`, `(1,)`, `()` | ✅ | `tuple`, `tuple_single`, `tuple_empty` | |
| Dict literals `{'a': 1}`, `{key_expr: v}` | 🟡 parse (`{'a'=1}` stays as a C++ extension); print as empty | `dict`, `dict_expression_key`, `dict_nested` | 0012 / 0028 |
| Non-string dict keys | 🟡 stored as strings (`1` → `'1'`) | `dict_int_key` | 0036 |

## Printing values (`output`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| Strings, integers | ✅ | `string_var`, `int_var` | |
| `None` | ✅ prints `None`; undefined prints empty | `none_var`, `none_in_concat` | |
| Booleans | ❌ `true`/`false` | `true_var`, `bool_expr` | 0012 |
| Whole floats `3.0` | ❌ `3` | `float_whole_var`, `float_division_whole` | 0012 |
| Float precision (`0.1 + 0.2`, `1/3`) | ❌ 8 significant digits | `float_precision`, `float_repr_third` | 0012 |
| Lists, tuples, dicts (`[1, 2]`, `{'a': 1}`) | ❌ print empty | `list_var`, `dict_var`, `nested_var` | 0012 |
| `range(3)` | ✅ `range(0, 3)` | `range_object` | |
| `~` with non-strings | 🟡 same str() gaps | `bool_in_concat`, `list_in_concat` | 0012 |

## Operators (`operators`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `+ - * / // % **` on integers | ✅ | `add`, `mul`, `floordiv`, `pow` | |
| `/` always returns float | ✅ | `div_exact` | |
| `//`, `%` with negatives floor (ints and floats) | ✅ | `floordiv_negative`, `mod_negative` | |
| `**` right-associative | ✅ | `pow_right_assoc` | |
| Division by zero raises | ✅ | `div_by_zero`, `floordiv_by_zero`, `mod_by_zero` | |
| Big integers | 🟡 deliberate: integers are int64 and overflow raises an error instead of growing | `int_overflow_mul`, `int_big_pow` | 0015 |
| `str * int`, `int * str`, `bool` as an int (`1 + true`) | ✅ | `string_times`, `int_times_string`, `int_plus_bool` | |
| `list + list`, `list * int` | 🟡 compute; print empty | `list_plus`, `list_times` | 0012 |
| Type errors raise (`str + int`, `1 < 'a'`, `x()` on a number) | ✅ | `string_plus_int`, `compare_mixed_types`, `errors.call_non_callable` | |
| `==`, `<` on numbers and strings | ✅ | `eq`, `lt_gt`, `compare_strings` | |
| `==`, `<` on lists, `==` on dicts | ✅ | `eq_list`, `compare_lists`, `eq_dict*` | |
| Chained comparison `a < b < c` | ✅ | `chained_compare*`, `chained_in` | |
| `in` on list/string | ✅ | `in_list`, `in_string` | |
| `in` on dict keys | ✅ | `in_dict` | |
| `not in` | ✅ | `not_in*` | |
| `and`/`or` return an operand | ✅ | `and_value`, `or_value`, `and_or_idiom` | |
| Short-circuit evaluation | ✅ | `and_short_circuit` | |
| Precedence: `not a == b`, `**` over unary minus and left-associative, `~` between `+` and `*` | ✅ | `not_precedence`, `pow_*`, `concat_precedence` | |
| Conditional expression, nested, no else | ✅ | `ternary*` | |
| Truthiness of `''`, `{}`, `None` | ✅ | `truthiness_*` | |
| Truthiness of floats (`0.0` falsy, `1.5` truthy) | ✅ | `truthiness_zero_float` | |

## Attributes and subscripts (`subscripts`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `a.b`, `a['b']`, nested, variable keys | ✅ | `dot_attr`, `item_attr`, `nested_*` | |
| Negative index on lists and strings | ✅ | `index_negative`, `index_string_negative` | |
| String index counts code points, not bytes | ✅ | `sequences.utf8_index` | |
| Slices `[a:b:c]` on lists, tuples and strings; step 0 and non-integer bounds raise | ✅ | `slice_*` | |
| `l.0` | ✅ | `dot_index*` | |
| Subscript after a literal or call (`'abc'[0]`, `range(5)[2]`) | ✅ | `string_literal_index`, `subscript_on_call` | |
| Missing attribute of undefined raises | ❌ renders empty | `missing_nested_attr` | 0026 |

## Strings as sequences (`sequences`)

A string is a sequence of characters wherever Python iterates one: `for`, indexing
(negative too), `length`, `first`, `last`, `reverse`, `min`, `max`, `unique`, `join`,
`list`, `sort`, `map`, `select`/`reject`, `batch`, `slice`, and the `iterable` and
`sequence` tests. A character is a Unicode code point, as in Python: narrow strings are
read as UTF-8 and wide ones as UTF-16 or UTF-32 by the size of `wchar_t`, so `'héllo'|length`
is 5. Malformed UTF-8 does not fail; a stray continuation byte stays with the character
before it. Still open (0037): `sort`, `min` and `max` order non-ASCII narrow characters by
signed bytes; `join(attribute=)` over characters sees the character as its own attribute;
`s|sum` does not raise; `mapping is sequence` is false.

## Methods on values (`methods`)

Python's builtin methods work on template values (task 0020, `src/value_methods.cpp`):
`str` (case, strip family, split/rsplit/splitlines, join, replace, startswith/endswith,
find/rfind/index/rindex, count, `format` with format specs, is* checks, zfill,
center/ljust/rjust, partition/rpartition, removeprefix/removesuffix) and `%` formatting,
`list` (index, count, append, extend, insert, pop, remove, reverse, clear, copy), `tuple` (index, count),
`dict` (keys, values, items, get, setdefault, update, pop, popitem, copy, clear),
`int.bit_length` and `float.is_integer`. `x.name` finds a method before a key and
`x['name']` a key before a method, as in Jinja2. Lists and dicts the template builds are
shared, so a mutation is seen through every name; a context list is copied on its first
mutation and stored back in its variable (the caller's data never changes).

Left: dict views print as lists and context mappings iterate in hash order (0043), an
alias of a context list taken before the list is changed keeps the old list (0049), and
storing a container in itself raises instead of printing `[...]` (deliberate, 0049).

## Tests (`tests`)

| Test | Status | Task |
|---|---|---|
| `defined`, `undefined`, `even`, `odd`, `lower`, `upper`, `mapping`, `number`, `string` | ✅ | |
| `divisibleby(n)`, `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `in`, `sameas` with parentheses | ✅ | |
| ... the same with a space-separated argument (`is eq 3`) | ✅ | |
| `is not test` | ✅ | |
| `none`, `true`, `false` | ❌ parse, but the tests are missing | 0017 |
| `boolean`, `callable`, `escaped`, `filter`, `test`, `float`, `integer`, `sameas`, `divisibleby` (in `select`) | ❌ missing | 0017 |
| `iterable`, `sequence` on strings | ✅ | |
| Unknown test is a compile error | ❌ silently false | 0017 |
| `x is odd and y` precedence | ✅ | |

## Filters (`filters`)

| Filter | Status | Notes | Task |
|---|---|---|---|
| `abs`, `capitalize`, `first`, `last`, `lower`, `upper`, `max`, `min`, `sum`, `wordcount`, `replace`, `map`, `select`, `reject`, `selectattr`, `rejectattr`, `unique`, `sort` (single attribute) | ✅ | | |
| `attr` | ✅ attributes only; a reflected object's fields are attributes | `attr*` | |
| `batch`, `slice` | ✅ | `batch*`, `slice*` | |
| `center` | ✅ | `center*` | |
| `count` | ✅ alias of `length` | `count*` | |
| `default`/`d` | ✅ | `default*` | |
| `dictsort` | ✅ | `dictsort*` | |
| `escape` | ✅ | `escape` | |
| `e` | ✅ alias of `escape`; both convert non-strings with `str()` | `escape_alias`, `escape_non_string` | |
| `filesizeformat`, `indent`, `items`, `string`, `urlize` | ✅ | `filesizeformat*`, `indent*`, `items*`, `string*`, `urlize*` | |
| `safe`, `forceescape` | ✅ mark the result as Markup | `safe*`, `forceescape`, `autoescape.*` | |
| `float`, `int` | ✅ within the int64 range | `float*`, `int*` | 0048 |
| `format` | ✅ printf-style; a string without `%` keeps the C++ `{}` syntax | `format_*` | |
| `groupby` | ✅ | `groupby*` | |
| `join` | 🟡 drops non-string items | `join_numbers` | 0012 |
| `length` | ✅ on strings (code points) and dicts | `length`, `sequences.utf8_length` | |
| `list` | ✅ on strings | `list_string` | |
| `pprint` | ✅ | `pprint`, `pprint_dict_literal` | |
| `random` | ➖ not compared (non-deterministic) | | |
| `reverse` | ✅ a string reverses into a string | `reverse_string`, `sequences.utf8_reverse` | |
| `round` | ✅ | `round*` | |
| `sort(attribute='a,b')`, dotted attributes | ✅ | `sort_multi_attribute`, `sort_dotted_attribute` | |
| `striptags` | 🟡 a few named entities only | `striptags*` | 0048 |
| `title`, `upper`, `lower`, `capitalize` | 🟡 ASCII letters only | `title*`, `case_non_ascii` | 0048 |
| `tojson` | ✅ | `tojson*` | |
| `trim(chars)` | ✅ | `trim*` | |
| `truncate` | ✅ | `truncate*` | |
| `urlencode` | ✅ | `urlencode*` | |
| `wordwrap` | ✅ port of `textwrap.wrap` (hyphens, em-dashes, `splitlines` boundaries, Unicode `\w`/`\d`/`strip()` classes); `width <= 0` returns the input instead of raising | `wordwrap*`, `sequences.wordwrap_*` | 0037 |
| `xmlattr` | ✅ | `xmlattr*` | |
| Unknown filter is an error; in a branch never taken it is not | ✅ | `unknown_filter*` | |

C++-only filters (`camelize`, `underscorize`, `escapecpp`, `toxml`, `toyaml`,
`applymacro`) and the `startsWith` test are deliberate extensions; Jinja2 rejects them.

## Statements (`statements`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `for` with `else`, filter, unpacking, nesting, `range`, strings | ✅ | `for*` | |
| Iterating a dict | 🟡 literals and kwargs in insertion order, context dicts in hash order | `for_dict_literal_order`, `macro_kwargs_order`, `for_dict_keys` | 0031 / 0043 |
| `for (a, b) in`, `for x in 1, 2`, nested targets `for (a, b), c in` | ✅ | `for_unpack_parens`, `for_implicit_tuple`, `for_nested_target*` | |
| `loop.index/index0/first/last/length/cycle/previtem/nextitem` | ✅ | `loop_*` | |
| `loop.revindex/revindex0`, `loop.changed`, `loop.depth` | ✅ | `loop_revindex*`, `loop_changed*`, `loop_depth*` | |
| Recursive loops | ✅ | `loop_recursive*` | |
| `break`/`continue` (`Settings::Extensions::LoopControls`) | ✅ | `break*`, `continue*` | |
| Loop scoping of `set` | ✅ | `loop_set_scope` | |
| `if`/`elif`/`else` | ✅ | `if_*` | |
| `set`, block `set`, `set` with filter | ✅ | `set*` | |
| `set a, b = ...`, `set (a, b), c = ...` | ✅ | `set_multiple`, `set_unpack_list`, `set_nested_target` | |
| `namespace()` and `set ns.attr` | ✅ | `namespace*` | |
| `namespace()` as an opaque object (not a mapping, not iterable, printed `<Namespace ...>`) | ❌ it is a mapping | `namespace_is_mapping`, `namespace_iterate`, `namespace_print` | 0042 |
| `with` | ✅ | `with*` | |
| Macros: defaults, keywords, `varargs`, `kwargs`, `caller`, recursion | ✅ | `macro*`, `caller*` | |
| Argument validation (too many, unknown keyword, unused `caller`) | ✅ | `macro_too_many_args`, `caller_not_used` | |
| Defaults that name an argument see it; others use the definition scope | ✅ | `macro_default_refers_arg`, `macro_default_lexical`, `import_macro_default` | |
| `macro.name`, `macro.arguments`, `catch_kwargs`, `catch_varargs`, `caller` | ✅ | `macro_name`, `macro_catch_flags`, `caller_attributes` | |
| Names in a macro resolve where it is defined; defaults see later reassignments | ❌ dynamic scoping | `macro_body_lexical_scope`, `macro_default_reassigned_global` | 0038 |
| `filter` blocks, `raw`, comments | ✅ | `filter_block*`, `raw` | |
| `do` | ✅ | `do` | |
| `autoescape` block | ✅ | `autoescape_block*`, `autoescape.block_*` | |

## Template composition (`loader`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `include`: context, `without context`, `ignore missing`, lists, variables | ✅ | `include*` | |
| `import ... as`, `from ... import [as]` | ✅ | `import_as`, `from_import*` | |
| Import context rules | ✅ | `import_no_context`, `import_with_context` | |
| Importing `_private` names is an error | ✅ | `from_import_private` | |
| `extends`, `super()`, two levels | ✅ | `extends*` | |
| `super()` across three levels | ✅ | `extends_super_chain` | |
| `extends` with a variable or inside `if`; output after `extends` dropped | ✅ | `extends_variable`, `extends_conditional*`, `extends_output_before_kept` | |
| `block ... scoped` | ✅ | `block_scoped*` | |
| Unscoped blocks do not see loop variables | ✅ | `block_unscoped_loop_var`, `block_sees_top_level_set` | |
| `required` blocks | ✅ | `block_required*`, `block_scoped_required` | |
| `self.blockname()` | ✅ | `block_self*` | |
| Invalid structure (`endblock b`, duplicate block, double `extends`, `extends` in a loop) | ✅ rejected | `block_end_name_mismatch`, `block_*duplicate`, `extends_twice*`, `extends_in_for` | |

## Global functions (`globals`)

| Function | Status | Task |
|---|---|---|
| `range(stop)`, `range(start, stop[, step])` | ✅ | |
| `range` with negative step | ❌ stops early | 0030 |
| `dict(...)` | ✅ | |
| `cycler`, `joiner`, `lipsum` | ❌ missing | 0030 |
| `range(stop)`, `range(start, stop[, step])`, negative steps | ✅ | |
| `cycler`, `joiner` | ✅ | |
| `lipsum` | ✅ same shape (the text is random in Jinja2 too) | |
| Calling an integer inside a loop | ❌ `2` acts as `loop.cycle` | 0042 |
| `cycler`/`joiner` objects, `range` argument types | 🟡 objects test as mappings; `range(1.5)` renders | 0042 |
| `namespace` | ✅ (a mapping, not an opaque object) | 0042 |

## Whitespace control (`whitespace`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `-` and `+` modifiers on tags, expressions, comments | ✅ | `*_minus`, `plus_*` | |
| `-` stripping across several newlines | ✅ | `minus_strips_newlines`, `expr_minus_strips_newlines` | |
| `trim_blocks`, `lstrip_blocks`, both (`trim_blocks` removes only a newline right after the tag) | ✅ | `trim_blocks*`, `lstrip_blocks*`, `both` | |
| Single trailing newline removed (`keep_trailing_newline=False`) | ✅ | `trailing_newline_*`, `only_newline` | |
| `keep_trailing_newline` option (`Settings::keepTrailingNewline`) | ✅ | `keep_trailing_newline*` | |
| `trim_blocks` inside `raw`, modifiers on `raw` | ✅ | `raw_trim_blocks`, `raw_minus_and_trim_blocks`, `raw_plus_lstrip`, `raw_body_starts_with_modifier` | |
| `lstrip_blocks` keeps trailing and mid-line whitespace, `{% raw +%}` rejected | ✅ | `lstrip_*`, `raw_plus_close_rejected` | |
| Unicode whitespace after `-` | ❌ | `minus_strips_unicode_space` | 0044 |
| `\r\n` and `\r` normalised to `newline_sequence` (`Settings::newlineSequence`), in text, string literals and the default `wordwrap` separator | ✅ | `crlf_*`, `cr_text`, `newline_sequence*` | |

## Autoescape (`autoescape`)

Off by default in both engines. Since 0025 Jinja2C++ has `Settings::autoescape`, the
`{% autoescape %}` block and Markup: a string value carries a markup flag that `safe`,
`escape`, `forceescape`, `tojson`, macros and block `set` set, that the `escaped` test reads,
and that `+`, `%`, `format`, `join`, `replace` and the case and padding filters follow as
Markup does. Blocks, includes and macro bodies escape as where they are defined. Left
for 0051: `~` on a Markup variable, `truncate`/`reverse`/`last`, string methods on Markup,
the `Markup('...')` repr and Markup values from C++.

## Undefined values (`undefined`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| Printing, iterating, concatenating, testing undefined | ✅ | `print`, `iterate_undefined`, `undefined_in_concat` | |
| Attribute/item/call/arithmetic on undefined raises | ❌ renders empty | `attr_of_undefined`, `call_undefined`, `undefined_plus` | 0026 |
| `undefined|length` is `0`, `undefined|list` is `[]` | ❌ | `length_undefined`, `undefined_list_filter` | 0026 |
| `StrictUndefined`, `ChainableUndefined`, `DebugUndefined` | ❌ no policy option | `strict_*`, `chainable_deep` | 0026 |

## Errors (`errors`)

Jinja2C++ rejects most malformed templates (23 of 42 match: missing operands, unclosed
subscripts and strings, stray end tags, unknown filters, invalid macro signatures). It
accepts what Jinja2 rejects in some cases, task 0027 unless noted: unclosed
blocks/expressions/comments, `else` after `else`, `set` without a value, double `extends` (0023),
unpacking count mismatches, invalid filter arguments, unknown tests (0017). Only the fact of an
error is compared, not the message or the line.

## Environment options and extensions (`options`)

| Option / extension | Status | Task |
|---|---|---|
| `trim_blocks`, `lstrip_blocks` | ✅ | |
| `keep_trailing_newline`, `newline_sequence` | ✅ | |
| `autoescape` | ✅ bool (`Settings::autoescape`); no `select_autoescape` callback | 0051 |
| `undefined` | ❌ | 0026 |
| `block_/variable_/comment_start_string` and `_end_string` | ✅ | |
| `line_statement_prefix`, `line_comment_prefix` | ✅ (`useLineStatements` means prefix `#`) | |
| `jinja2.ext.do` | ✅ | |
| `jinja2.ext.loopcontrols` | ✅ (`Settings::Extensions::LoopControls`) | |
| `jinja2.ext.i18n` (`trans`, `gettext`, `_`) | ❌ | 0029 |
| `jinja2.ext.debug` | ❌ (not in corpus: output is not deterministic) | |

## API level (not corpus-checked)

| Jinja2 feature | Jinja2C++ | Status | Task |
|---|---|---|---|
| `DictLoader`, `FileSystemLoader`, `PrefixLoader` | `MemoryFileSystem`, `RealFileSystem`, `AddFilesystemHandler(prefix, ...)` | ✅ | |
| `ChoiceLoader` | several handlers on one prefix | 🟡 not verified | |
| `FunctionLoader`, `PackageLoader` | custom `IFilesystemHandler` | ➖ | |
| `env.globals` | `AddGlobal`/`RemoveGlobal` | ✅ | |
| `env.filters[...]`, `env.tests[...]` | `AddFilter`/`AddTester` (bound when a template loads, replace builtins); corpus area `custom` | ✅ | |
| `finalize` | `Settings::finalize`; corpus area `custom` | ✅ | |
| Template cache, `auto_reload` | `cacheSize`, `autoReload` | ✅ | |
| `Template.generate`/`stream` | `Render(std::ostream&)` | ✅ | |
| `Template.module`, `make_module` | none | ❌ | |
| `jinja2.meta.find_undeclared_variables` | none (`GetMetadata` reads a C++-specific `meta` block) | ❌ | |
| Sandbox | ➖ no Python object access to sandbox | ➖ | |
| Async rendering | ➖ | ➖ | |
| Wide strings | `TemplateW`; the corpus runs every case through it too (`ParityWide`), with narrow/wide conversion still locale-dependent | 🟡 | 0035 |
| Error position (file, line, column) | `ErrorInfo` | ✅ messages differ from Jinja2 | |

## Keeping this map current

When a PR fixes a divergence, the parity suite fails until the case's line is removed
from `test/parity/divergences/<area>.txt`. Update the affected feature rows here in the
same PR. The summary table and the corpus size above are a snapshot that parity PRs leave
alone: the integration branch that lands a wave regenerates them with
`python3 test/parity/generate.py --report`, so parallel PRs do not conflict on them. New gaps found
later get a corpus case first, then a row here.
