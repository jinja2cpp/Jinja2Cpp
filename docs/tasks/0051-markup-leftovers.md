---
status: done
priority: low
area: parity
depends: [0025]
touches: [src/expression_parser.cpp#ParseBinary, src/value_methods.cpp, src/value_visitors.h#ValueRenderer]
shares: [src/expression_evaluator.cpp, src/expression_evaluator.h, src/string_converter_filter.cpp, src/filters.cpp, src/markup.h, include/jinja2cpp/value.h, include/jinja2cpp/template_env.h]
---
# Markup: what 0025 left behind

**Problem.** Task 0025 added autoescape and a markup flag on values. Some Markup rules
are still missing, each pinned by a corpus case in `test/parity/divergences/autoescape.txt`:

- **`~` under autoescape** (`env_on_concat_markup_var`). Jinja2 joins with `markup_join`
  when autoescape is on, so `{% set a = '<i>'|safe %}{{ a ~ '<u>' }}` gives `<i>&lt;u&gt;`.
  Jinja2C++ always returns a plain str. The catch: Jinja2 constant-folds a `~` chain whose
  operands are all constant (`'<i>'|safe ~ '<u>'`) with `str()`, which drops Markup, and
  the corpus case `env_on_concat_safe` depends on that. A fix needs an `IsConstant()` on
  expressions (constant literal, or a pure filter over constants) and one fold decision for
  the whole n-ary chain in `ParseStringConcat`.
- **Filters that keep Markup** (`markup_kept_by_filters`): `truncate`, `reverse` and
  `last` return Markup for a Markup input in Python. `truncate`'s `end` must then be
  escaped.
- **String methods on Markup** (`markup_method_upper`): `.upper()`, `.replace()`,
  `.format()` and the rest return Markup and escape their string arguments.
- **`caller()` result** (`env_on_caller_markup_call_site`): Python makes it Markup by the
  autoescape setting at the `{% call %}` site; Jinja2C++ uses the setting inside the macro
  where `caller()` runs. Differs only with an `{% autoescape %}` block around the call.
- **Indexing and slicing** (`markup_slice`): `(s|safe)[0]` and `[1:3]` are Markup in Python.
- **Markup repr** (`markup_repr_in_list`): a Markup item in a list prints as
  `Markup('<i>')`, in output and in `pprint`.
- **Markup from C++**: `jinja2::Value` has no markup flag, so context data and user
  callables cannot pass safe HTML; the flag is lost in `IntValue2Value`. This is a public
  API change (a `Value` flag or alternative); it needs an architect plan.
- **`select_autoescape`**: `Settings::autoescape` is a bool; Jinja2 also takes a callback
  by template name. A `std::function` in `Settings` breaks `operator==`, and the value has
  to flow per template from `TemplateImpl` into the renderer.

**Found while verifying 0025 (not Markup).** `{% block a %}{{ self.a() }}{% endblock %}`
segfaults from unbounded recursion (master 9f695fd too); Python raises RecursionError.
Rendering needs a recursion depth limit for blocks, macros and includes. No corpus case
pins it, because a crashing case would take the suite down.

**Done when.** No line of `test/parity/divergences/` names task 0051, and the two API
items and the recursion limit are either done or split into their own tasks.

**Done (this PR).**
- `~` under autoescape is Markup-joined when either side is Markup. The parser marks a `~`
  chain (the `~` operators of one `ParseBinary` call, Jinja2's n-ary `Concat`) as folded when
  every operand is foldable (`Expression::IsFoldable`: literals, operators, subscripts,
  slices, list/dict literals, comparisons, built-in tests, `and`/`or` (stopping at a
  deciding literal), conditional expressions (the picked branch for a literal test), and
  filters other than `map`, `random`, `select`, `reject`, `selectattr`, `rejectattr` and
  environment filters, on foldable arguments). A folded chain joins with `str()`, as
  Jinja2's optimizer does. Not modelled: a conditional without `else` whose test is
  foldable but not a literal (never folded here; Jinja2 folds it when the test is true),
  and the volatile `{% autoescape expr %}` that turns folding off.
- `truncate` (escaping `end`), `reverse`, `last` and `random` keep Markup; `first` does
  not, as in Python (it iterates).
- Markup has its own method table (`MarkupMethods` in `src/value_methods.cpp`): results are
  Markup (lists and tuples of Markup for `split`, `partition`, ...), `replace`'s new text,
  fill characters and `join`'s items are escaped, and `format` escapes each field after
  formatting it (markupsafe's `EscapeFormatter`).
- An item or slice of Markup is Markup.
- `repr` (container items, `%r`, `{!r}`) and `pprint` print `Markup('...')`. Markup dict
  keys stay plain: map keys are `std::string`.
- The recursion item is fixed on master: `{% block a %}{{ self.a() }}{% endblock %}` now
  fails with "Maximum recursion depth exceeded" (0003's render depth limit).

Split out: `caller()` Markup by the runtime autoescape to [0179](0179-caller-markup-eval-context.md)
(needs 0165's files), Markup from C++ to [0180](0180-markup-from-cpp.md), `select_autoescape`
to [0181](0181-select-autoescape.md). Found by the verifier: `%r` and containers in Markup's `%`
print escaped Markup reprs, filed as [0182](0182-markup-percent-repr.md).
