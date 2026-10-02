---
status: open
priority: low
area: parity
depends: [0025]
touches: [src/expression_parser.cpp#ParseStringConcat, src/value_methods.cpp, src/value_visitors.h#ValueRenderer]
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
