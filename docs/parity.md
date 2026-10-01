# Jinja2C++ feature parity map

How far Jinja2C++ is from Python [Jinja2](https://jinja.palletsprojects.com/) 3.1, area by
area, and which task in `docs/tasks/` closes each gap.

Every statement marked with a case id is backed by the differential corpus in
`test/parity/` (631 templates rendered by both engines, see
[test/parity/README.md](../test/parity/README.md)); `ctest -R parity` re-checks all of
them. Statements in the last section (API level) are read from the headers and are not
corpus-checked yet.

Legend: ✅ matches Jinja2 · 🟡 works with differences · ❌ missing or broken ·
➖ not applicable to C++.

## Summary

Snapshot of `python3 test/parity/generate.py --report` (Jinja2 3.1.6, Oct 2026):

| area | cases | match | output | rejects | accepts | unsupported | unordered | crash | tasks |
|---|---|---|---|---|---|---|---|---|---|
| autoescape | 28 | 1 | 0 | 5 | 0 | 22 | 0 | 0 | 0017, 0018, 0025 |
| errors | 44 | 22 | 0 | 0 | 22 | 0 | 0 | 0 | 0015, 0017, 0022, 0023, 0027, 0036 |
| filters | 118 | 58 | 46 | 13 | 0 | 0 | 1 | 0 | 0012, 0016, 0017, 0018, 0019, 0031 |
| globals | 17 | 5 | 10 | 1 | 1 | 0 | 0 | 0 | 0012, 0014, 0021, 0026, 0030 |
| literals | 52 | 31 | 20 | 1 | 0 | 0 | 0 | 0 | 0012, 0015, 0028, 0034, 0036 |
| loader | 35 | 24 | 6 | 2 | 3 | 0 | 0 | 0 | 0023 |
| methods | 41 | 0 | 29 | 11 | 1 | 0 | 0 | 0 | 0020 |
| operators | 71 | 36 | 24 | 5 | 5 | 0 | 0 | 1 | 0012, 0014, 0015, 0034 |
| options | 10 | 1 | 0 | 0 | 0 | 9 | 0 | 0 | 0028, 0029 |
| output | 27 | 6 | 18 | 3 | 0 | 0 | 0 | 0 | 0012, 0018, 0030, 0034 |
| statements | 71 | 48 | 11 | 9 | 2 | 0 | 1 | 0 | 0012, 0014, 0021, 0022, 0025, 0031 |
| subscripts | 29 | 15 | 3 | 10 | 1 | 0 | 0 | 0 | 0014, 0016, 0020, 0026 |
| tests | 34 | 10 | 12 | 11 | 1 | 0 | 0 | 0 | 0012, 0014, 0016, 0017 |
| undefined | 26 | 7 | 4 | 1 | 7 | 7 | 0 | 0 | 0012, 0018, 0026, 0034 |
| whitespace | 28 | 20 | 6 | 0 | 0 | 2 | 0 | 0 | 0024 |
| **total** | **631** | **284** | **189** | **72** | **43** | **40** | **2** | **1** | |

*output*: both render, text differs. *rejects*: C++ errors on a valid template.
*accepts*: C++ renders a template Jinja2 rejects. *unsupported*: needs an Environment
option C++ lacks. *unordered*: depends on hash order, so it matches on some standard
libraries and not others. *crash*: skipped because it hits undefined behaviour (one
case: 64-bit signed overflow in `*`, task 0015).

Two gaps account for most of the visible damage, because nearly every template prints
values or calls methods:

1. **Printing values** (0012): `None`, `True`/`False`, `2.0`, lists and dicts print as
   `""`, `true`, `2`, `""`. Any template that prints a list or a boolean differs.
2. **Expression grammar** (0014): `x[1:]`, `a < b < c`, `not in`, `is not`,
   `is divisibleby 3` fail to parse, so the template does not load at all. Literals
   (`none`, `{'a': 1}`, `(1,)`, `0x1F`) parse since 0013.

Next come Python methods on values (0020, `s.strip()`, `d.items()`, used heavily by LLM
chat templates), arithmetic semantics (0015) and the missing filters and tests (0017,
0018).

The earlier audit probe (56/159 matching) understated parity: many of its empty outputs
came from printing lists and from `join` over numbers, which this corpus isolates.

## Tasks

| # | Gap | Priority | Cases |
|---|---|---|---|
| [0012](tasks/0012-python-value-stringification.md) | Print values the way Python `str()` does | high | 40 |
| [0013](tasks/0013-literal-syntax.md) | Literal syntax: `none`, numeric forms, dict and tuple literals | high | done |
| [0014](tasks/0014-operator-and-postfix-grammar.md) | Operator and postfix grammar: chained compare, `not in`, `is not`, slices | high | 30 |
| [0015](tasks/0015-arithmetic-and-logic-semantics.md) | Python arithmetic, comparison and `and`/`or` semantics | high | 31 |
| [0016](tasks/0016-strings-as-sequences.md) | Strings behave as sequences | high | 9 |
| [0017](tasks/0017-builtin-tests.md) | Complete the builtin tests | medium | 12 |
| [0018](tasks/0018-missing-builtin-filters.md) | Missing builtin filters (`string`, `safe`, `indent`, ...) | high | 20 |
| [0019](tasks/0019-filter-behaviour.md) | Filter behaviour divergences | medium | 32 |
| [0020](tasks/0020-python-methods-on-values.md) | Python methods on str, list and dict values | high | 42 |
| [0021](tasks/0021-loop-and-assignment-statements.md) | Loop controls, loop object, namespace, tuple assignment | high | 11 |
| [0022](tasks/0022-macro-call-semantics.md) | Macro call semantics | medium | 6 |
| [0023](tasks/0023-inheritance-and-import.md) | Template inheritance and import semantics | medium | 12 |
| [0024](tasks/0024-whitespace-and-newlines.md) | Trailing newline, `-` modifiers, newline normalisation | high | 8 |
| [0025](tasks/0025-autoescape.md) | Autoescape and Markup | medium | 26 |
| [0026](tasks/0026-undefined-semantics.md) | Undefined semantics and undefined policies | medium | 18 |
| [0027](tasks/0027-reject-invalid-templates.md) | Reject what Jinja2 rejects | medium | 13 |
| [0028](tasks/0028-delimiters-and-line-statements.md) | Custom delimiters, line statements | low | 6 |
| [0029](tasks/0029-i18n-extension.md) | i18n extension | low | 3 |
| [0030](tasks/0030-global-functions.md) | Global functions: `cycler`, `joiner`, `lipsum`, `range` | medium | 7 |
| [0031](tasks/0031-insertion-ordered-mappings.md) | Mappings keep insertion order | medium | 3 |
| [0032](tasks/0032-custom-filters-and-tests.md) | Register custom filters and tests | medium | API |
| [0033](tasks/0033-wide-string-parity.md) | Run the corpus through the wide-string API | low | API |
| [0034](tasks/0034-none-versus-undefined.md) | Tell `None` apart from undefined | high | 2 |
| [0036](tasks/0036-non-string-mapping-keys.md) | Mapping keys that are not strings | low | 2 |

Order: `python3 scripts/task_batches.py --area parity` groups the tasks into waves that
can run side by side (Oct 2026: 0012 0013 0016 0022 0033 → 0014 0018 0023 0024 0030 0031
→ 0015 0027 0034 → 0017 0019 0020 0028 → 0021 0025 0032 → 0026 0029). 0012 comes first
because it makes the remaining divergences readable: today a wrong filter and a wrong
repr look the same.

## Literals and keywords (`literals`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| Integers, floats, strings, escapes, unicode | ✅ | `int`, `float`, `string_*` | |
| `true`/`false`/`True`/`False` | 🟡 parse; print as `true` | `bool_lower`, `bool_title` | 0012 |
| `none`/`None` | 🟡 parse; print as empty | `none_lower`, `none_title` | 0034 |
| `1_000`, `0x1F`, `0o17`, `0b101` | ✅ | `int_underscore`, `int_hex`, ... | |
| Exponent floats `1e3` | 🟡 prints `1000` | `float_exponent` | 0012 |
| Integers beyond 64 bits | ❌ become floats | `int_big` | 0015 |
| Adjacent strings `'a' 'b'` | ✅ | `string_adjacent_concat` | |
| List literals, trailing comma | 🟡 parse; print as empty | `list_trailing_comma` | 0012 |
| Tuple literals `(1,)`, `()` | 🟡 parse; print as empty | `tuple_single`, `tuple_empty` | 0012 |
| Dict literals `{'a': 1}`, `{key_expr: v}` | 🟡 parse (`{'a'=1}` stays as a C++ extension); print as empty; `}}` inside a tag ends it | `dict`, `dict_expression_key`, `dict_nested` | 0012 / 0028 |
| Non-string dict keys | 🟡 stored as strings (`1` → `'1'`) | `dict_int_key` | 0036 |

## Printing values (`output`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| Strings, integers | ✅ | `string_var`, `int_var` | |
| `None` | ❌ prints empty | `none_var`, `none_in_concat` | 0034 |
| Booleans | ❌ `true`/`false` | `true_var`, `bool_expr` | 0012 |
| Whole floats `3.0` | ❌ `3` | `float_whole_var`, `float_division_whole` | 0012 |
| Float precision (`0.1 + 0.2`, `1/3`) | ❌ 8 significant digits | `float_precision`, `float_repr_third` | 0012 |
| Lists, tuples, dicts (`[1, 2]`, `{'a': 1}`) | ❌ print empty | `list_var`, `dict_var`, `nested_var` | 0012 |
| `range(3)` | ❌ | `range_object` | 0030 |
| `~` with non-strings | 🟡 same str() gaps | `bool_in_concat`, `list_in_concat` | 0012 |

## Operators (`operators`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `+ - * / // % **` on integers | ✅ | `add`, `mul`, `floordiv`, `pow` | |
| `/` always returns float | ❌ `10/5` prints `2` | `div_exact` | 0015 / 0012 |
| `//`, `%` with negatives floor | ❌ truncate | `floordiv_negative`, `mod_negative` | 0015 |
| `**` right-associative | ❌ | `pow_right_assoc` | 0015 |
| Division by zero raises | ❌ renders `inf`/`nan` | `div_by_zero` | 0015 |
| 64-bit overflow / big ints | ❌ overflow is undefined behaviour | `int_overflow_mul`, `int_big_pow` | 0015 |
| `str * int`, `int * str` | 🟡 only `str * int` | `string_times`, `int_times_string` | 0015 |
| `list + list`, `list * int` | 🟡 compute; print empty | `list_plus`, `list_times` | 0012 |
| `str + int` raises | ❌ renders empty | `string_plus_int` | 0015 |
| `==`, `<` on numbers and strings | ✅ | `eq`, `lt_gt`, `compare_strings` | |
| `==`, `<` on lists | ❌ | `eq_list`, `compare_lists` | 0015 |
| Chained comparison `a < b < c` | ❌ parse error | `chained_compare` | 0014 |
| `in` on list/string | ✅ | `in_list`, `in_string` | |
| `in` on dict keys | ❌ | `in_dict` | 0015 |
| `not in` | ❌ parse error | `not_in` | 0014 |
| `and`/`or` return an operand | ❌ return bool | `and_value`, `or_value`, `and_or_idiom` | 0015 |
| Short-circuit evaluation | ✅ | `and_short_circuit` | |
| `not a == b` precedence | ❌ | `not_precedence` | 0014 |
| Conditional expression, nested, no else | ✅ | `ternary*` | |
| Truthiness of `''`, `{}`, `None` | ✅ | `truthiness_*` | |
| Truthiness of `0.0` | ❌ truthy | `truthiness_zero_float` | 0015 |

## Attributes and subscripts (`subscripts`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `a.b`, `a['b']`, nested, variable keys | ✅ | `dot_attr`, `item_attr`, `nested_*` | |
| Negative index on lists and strings | ❌ empty | `index_negative`, `index_string_negative` | 0016 |
| Slices `[a:b:c]` | ❌ parse error | `slice_*` | 0014 |
| `l.0` | ❌ | `dot_index` | 0014 |
| Subscript after a literal or call (`'abc'[0]`, `range(5)[2]`) | ❌ | `string_literal_index`, `subscript_on_call` | 0014 |
| Missing attribute of undefined raises | ❌ renders empty | `missing_nested_attr` | 0026 |

## Methods on values (`methods`)

No Python method works: `str.upper/strip/split/replace/startswith/format`, `%`
formatting, `list.index/count/append/pop`, `dict.items/keys/values/get/update`. Method
calls on context variables render empty, on literals they fail to parse (41 cases, all
diverge). Task 0020. Mutating methods additionally need mutable containers (`do
l.append(4)` leaves `l` unchanged, `statements.do`).

## Tests (`tests`)

| Test | Status | Task |
|---|---|---|
| `defined`, `undefined`, `even`, `odd`, `lower`, `upper`, `mapping`, `number`, `string` | ✅ | |
| `divisibleby(n)`, `eq`, `ne`, `lt`, `le`, `gt`, `ge`, `in`, `sameas` with parentheses | ✅ | |
| ... the same with a space-separated argument (`is divisibleby 3`) | ❌ parse error | 0014 |
| `is not test` | ❌ parse error | 0014 |
| `none`, `true`, `false` | ❌ keyword names do not parse | 0014 / 0017 |
| `boolean`, `callable`, `escaped`, `filter`, `test`, `float`, `integer`, `sameas`, `divisibleby` (in `select`) | ❌ missing | 0017 |
| `iterable`, `sequence` on strings | ❌ false | 0016 |
| Unknown test is a compile error | ❌ silently false | 0017 |
| `x is odd and y` precedence | ❌ | 0014 |

## Filters (`filters`)

| Filter | Status | Notes | Task |
|---|---|---|---|
| `abs`, `capitalize`, `first`, `last`, `lower`, `upper`, `max`, `min`, `sum`, `wordcount`, `replace`, `map`, `select`, `reject`, `selectattr`, `rejectattr`, `unique`, `sort` (single attribute) | ✅ | | |
| `attr` | 🟡 falls back to item lookup | `attr` | 0019 |
| `batch` | 🟡 pads without `fill_with` | `batch` | 0019 |
| `center` | 🟡 odd padding on the wrong side | `center_default` | 0019 |
| `count` | ❌ missing | | 0018 |
| `default`/`d` | 🟡 replaces `None` too | `default_defined_none` | 0019 |
| `dictsort` | ❌ yields nothing | `dictsort*` | 0019 |
| `escape` | ✅ | `escape` | |
| `e` | ❌ alias missing | `escape_alias`, `escape_single_quote` | 0018 |
| `filesizeformat`, `forceescape`, `indent`, `items`, `safe`, `string`, `urlize` | ❌ missing | | 0018 |
| `float`, `int` | 🟡 no `0` fallback, `'3.9'|int` | `float`, `int` | 0019 |
| `format` | ❌ ignores `%`-placeholders | `format_*` | 0019 |
| `groupby` | 🟡 not sorted, no `default`, groups do not unpack | `groupby*` | 0019 |
| `join` | 🟡 drops non-string items | `join_numbers` | 0012 |
| `length` | 🟡 strings and dicts give empty | `length` | 0016 |
| `list` | ✅ on strings | `list_string` | |
| `pprint` | 🟡 dict order | `pprint` | 0031 |
| `random` | ➖ not compared (non-deterministic) | | |
| `reverse` | 🟡 strings give empty | `reverse_string` | 0016 |
| `round` | 🟡 returns int, rounds half away from zero | `round*` | 0019 |
| `slice` | ❌ behaves like `batch` | `slice*` | 0019 |
| `sort(attribute='a,b')` | ❌ | `sort_multi_attribute` | 0019 |
| `striptags` | 🟡 keeps newlines | `striptags` | 0019 |
| `title` | 🟡 keeps upper case inside words | `title` | 0019 |
| `tojson` | 🟡 compact separators, key order | `tojson_list`, `tojson_dict` | 0019 / 0031 |
| `trim(chars)` | 🟡 ignores `chars` | `trim_chars` | 0019 |
| `truncate` | ❌ different length rule, `leeway` | `truncate*` | 0019 |
| `urlencode` | 🟡 `+` for spaces, quotes `/` | `urlencode` | 0019 |
| `wordwrap` | ❌ empty | `wordwrap*` | 0016 |
| `xmlattr` | 🟡 no leading space, key order | `xmlattr*` | 0019 |
| Unknown filter is an error; in a branch never taken it is not | ✅ | `unknown_filter*` | |

C++-only filters (`camelize`, `underscorize`, `escapecpp`, `toxml`, `toyaml`,
`applymacro`) and the `startsWith` test are deliberate extensions; Jinja2 rejects them.

## Statements (`statements`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `for` with `else`, filter, unpacking, nesting, `range`, strings | ✅ | `for*` | |
| `for (a, b) in` | ❌ | `for_unpack_parens` | 0014 |
| Iterating a dict | 🟡 hash order | `for_dict_keys` | 0031 |
| `loop.index/index0/first/last/length/cycle/previtem/nextitem` | ✅ | `loop_*` | |
| `loop.revindex/revindex0`, `loop.changed`, `loop.depth` | ❌ | `loop_revindex`, `loop_changed`, `loop_depth` | 0021 |
| Recursive loops | ✅ | `loop_recursive*` | |
| `break`/`continue` | ❌ | `break`, `continue` | 0021 |
| Loop scoping of `set` | ✅ | `loop_set_scope` | |
| `if`/`elif`/`else` | ✅ | `if_*` | |
| `set`, block `set`, `set` with filter | ✅ | `set*` | |
| `set a, b = ...` | ❌ | `set_multiple`, `set_unpack_list` | 0014 / 0021 |
| `namespace()` and `set ns.attr` | ❌ | `namespace*` | 0021 |
| `with` | 🟡 `{% with %}` without targets fails | `with_set_inside` | 0014 |
| Macros: defaults, keywords, `varargs`, `kwargs`, `caller`, recursion | ✅ | `macro*`, `caller*` | |
| Argument validation (too many, unknown keyword) | ❌ accepted | `macro_too_many_args` | 0022 |
| Default referring to an earlier argument | ❌ | `macro_default_refers_arg` | 0022 |
| `macro.name`, `macro.arguments` | ❌ | `macro_name` | 0022 |
| `filter` blocks, `raw`, comments | ✅ | `filter_block*`, `raw` | |
| `do` | 🟡 parses; cannot mutate | `do` | 0021 |
| `autoescape` block | ❌ | `autoescape_block` | 0025 |

## Template composition (`loader`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `include`: context, `without context`, `ignore missing`, lists, variables | ✅ | `include*` | |
| `import ... as`, `from ... import [as]` | ✅ | `import_as`, `from_import*` | |
| Import context rules | 🟡 inside a macro `name` resolves to the macro name | `import_no_context`, `import_with_context` | 0023 |
| Importing `_private` names is an error | ❌ | `from_import_private` | 0023 |
| `extends`, `super()`, two levels | ✅ | `extends*` | |
| `super()` across three levels | ❌ skips a level | `extends_super_chain` | 0023 |
| `extends` with a variable or inside `if` | ❌ | `extends_variable`, `extends_conditional` | 0023 |
| `block ... scoped` | ✅ | `block_scoped*` | |
| Unscoped blocks do not see loop variables | ❌ | `block_unscoped_loop_var` | 0023 |
| `required` blocks | ❌ | `block_required_given` | 0023 |
| `self.blockname()` | ❌ | `block_self_call` | 0023 |
| Invalid structure (`endblock b`, duplicate block, double `extends`) | ❌ accepted | `block_end_name_mismatch`, `block_duplicate` | 0023 |

## Global functions (`globals`)

| Function | Status | Task |
|---|---|---|
| `range(stop)`, `range(start, stop[, step])` | ✅ | |
| `range` with negative step | ❌ stops early | 0030 |
| `dict(...)` | 🟡 works; printing and subscripting the call fail | 0012 / 0014 |
| `cycler`, `joiner`, `lipsum` | ❌ missing | 0030 |
| `namespace` | ❌ | 0021 |

## Whitespace control (`whitespace`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| `-` and `+` modifiers on tags, expressions, comments | ✅ | `*_minus`, `plus_*` | |
| `-` stripping across several newlines | ❌ | `minus_strips_newlines` | 0024 |
| `trim_blocks`, `lstrip_blocks`, both | ✅ | `trim_blocks*`, `lstrip_blocks*`, `both` | |
| Single trailing newline removed (`keep_trailing_newline=False`) | ❌ kept | `trailing_newline_*` | 0024 |
| `keep_trailing_newline` option | ❌ | `keep_trailing_newline` | 0024 |
| `trim_blocks` inside `raw` | ❌ | `raw_trim_blocks` | 0024 |
| `\r\n` normalised to `newline_sequence` | ❌ | `crlf_text`, `newline_sequence` | 0024 |

## Autoescape (`autoescape`)

Off by default in both engines (✅). Everything else is missing (task 0025): the
`autoescape` Environment option, the `{% autoescape %}` block, `Markup` semantics (safe
strings surviving concatenation, `join`, `replace`, `format`, macros and block `set`),
the `escaped` test and the `safe`/`forceescape`/`e` filters.

## Undefined values (`undefined`)

| Feature | Status | Evidence | Task |
|---|---|---|---|
| Printing, iterating, concatenating, testing undefined | ✅ | `print`, `iterate_undefined`, `undefined_in_concat` | |
| Attribute/item/call/arithmetic on undefined raises | ❌ renders empty | `attr_of_undefined`, `call_undefined`, `undefined_plus` | 0026 |
| `undefined|length` is `0`, `undefined|list` is `[]` | ❌ | `length_undefined`, `undefined_list_filter` | 0026 |
| `StrictUndefined`, `ChainableUndefined`, `DebugUndefined` | ❌ no policy option | `strict_*`, `chainable_deep` | 0026 |

## Errors (`errors`)

Jinja2C++ rejects most malformed templates (18 of 39 match: missing operands, unclosed
subscripts and strings, stray end tags, unknown filters). It accepts what Jinja2 rejects
in 21 cases, task 0027 unless noted: unclosed blocks/expressions/comments, `else` after
`else`, `set` without a value, invalid macro signatures (0022), double `extends` (0023),
type errors such as `'a' + 1` and `1 + [1]` (0015), calling a non-callable, unpacking
count mismatches, invalid filter arguments, unknown tests (0017). Only the fact of an
error is compared, not the message or the line.

## Environment options and extensions (`options`)

| Option / extension | Status | Task |
|---|---|---|
| `trim_blocks`, `lstrip_blocks` | ✅ | |
| `keep_trailing_newline`, `newline_sequence` | ❌ | 0024 |
| `autoescape` | ❌ | 0025 |
| `undefined` | ❌ | 0026 |
| `block_/variable_/comment_start_string` and `_end_string` | ❌ | 0028 |
| `line_statement_prefix`, `line_comment_prefix` | ❌ (`useLineStatements` exists, unimplemented) | 0028 |
| `jinja2.ext.do` | 🟡 parses; no mutation | 0021 |
| `jinja2.ext.loopcontrols` | ❌ | 0021 |
| `jinja2.ext.i18n` (`trans`, `gettext`, `_`) | ❌ | 0029 |
| `jinja2.ext.debug` | ❌ (not in corpus: output is not deterministic) | |

## API level (not corpus-checked)

| Jinja2 feature | Jinja2C++ | Status | Task |
|---|---|---|---|
| `DictLoader`, `FileSystemLoader`, `PrefixLoader` | `MemoryFileSystem`, `RealFileSystem`, `AddFilesystemHandler(prefix, ...)` | ✅ | |
| `ChoiceLoader` | several handlers on one prefix | 🟡 not verified | |
| `FunctionLoader`, `PackageLoader` | custom `IFilesystemHandler` | ➖ | |
| `env.globals` | `AddGlobal`/`RemoveGlobal` | ✅ | |
| `env.filters[...]`, `env.tests[...]` | none; user callables can only be globals (`applymacro` as a workaround) | ❌ | 0032 |
| `finalize` | none | ❌ | 0032 |
| Template cache, `auto_reload` | `cacheSize`, `autoReload` | ✅ | |
| `Template.generate`/`stream` | `Render(std::ostream&)` | ✅ | |
| `Template.module`, `make_module` | none | ❌ | |
| `jinja2.meta.find_undeclared_variables` | none (`GetMetadata` reads a C++-specific `meta` block) | ❌ | |
| Sandbox | ➖ no Python object access to sandbox | ➖ | |
| Async rendering | ➖ | ➖ | |
| Wide strings | `TemplateW`; the corpus only runs narrow templates | 🟡 | 0033 |
| Error position (file, line, column) | `ErrorInfo` | ✅ messages differ from Jinja2 | |

## Keeping this map current

When a PR fixes a divergence, the parity suite fails until the case's line is removed
from `test/parity/divergences.txt`. Update the affected feature rows here in the same
PR. The summary table is a snapshot: refresh it with
`python3 test/parity/generate.py --report` when convenient rather than in every PR, so
that parallel parity PRs do not conflict on it. New gaps found
later get a corpus case first, then a row here.
