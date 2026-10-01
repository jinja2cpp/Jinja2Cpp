---
status: done
priority: high
area: parity
depends: [0001, 0012]
touches: [src/internal_value.h#variant, src/value_visitors.h#ValueRenderer, src/internal_value.cpp#Subscript, src/render_context.h]
shares: [src/internal_value.h, src/internal_value.cpp, src/value_visitors.h, src/expression_evaluator.cpp, src/testers.cpp, src/value.cpp]
---
# Tell `None` apart from undefined

**Problem.** Jinja2 has two different "nothing" values: `None`, a real value that prints
as `None`, is `defined` and can be passed around, and `Undefined`, what a missing
variable or attribute evaluates to, which prints as empty and fails on most uses.
Jinja2C++ represents both as `EmptyValue`, so `{{ none_var }}` cannot print `None`
without also printing `None` for every typo, `n is defined` is false for a context
value that is `None`, and `default` cannot tell them apart. Four tasks need the
distinction (0012 printing, 0017 `none` test, 0019 `default`, 0026 undefined policies);
done separately they would each invent it in the value model at once.

**Proposal.** Keep the public `jinja2::Value` unchanged: `EmptyValue` there is `None`
(what a JSON `null` or a default-constructed `Value` means to the user). Inside, add an
undefined alternative to `InternalValueData` that only lookups produce (a missing name in
`RenderContext`, a missing attribute or item in `Subscript`), carrying the name for
later error messages. Then: undefined prints empty and `None` prints `None`; `is
defined`/`is undefined` test for the undefined kind; `none` literals (0013) and `None`
from the context are the None value; conversion back to `Value` maps undefined to
`EmptyValue`. Visitors that today special-case `EmptyValue` get the same case for
undefined unless Python treats them differently. The policies (strict, chainable) stay
in 0026. Start with an architect plan: the variant is used by every visitor in
`value_visitors.h`.

Cases: `output.none_var`, `output.none_in_concat`, the `tests.none*`/`defined` cases.

**Scheduling.** Runs after 0012, which rewrites the value printer this task extends.

**Done when.** No line of `test/parity/divergences/` names task 0034, and `ctest -R parity` passes.

**Next.** 0026 adds the undefined policies on top of the undefined value.

## Plan (architect review, 2026-10-01)

**Decision.** Add an internal `struct UndefinedValue {}` as index 0 of `InternalValueData`
and keep `EmptyValue` as `None`. A default-constructed `InternalValue` is then undefined,
so the ~80 `return InternalValue()` error and miss paths and every `BaseVisitor` `R()`
fallback keep printing empty, and `None` appears only where it is produced on purpose.
Rejected: making the default `None` and producing undefined only on lookups (every failing
filter or operator would print `None`), and keeping `EmptyValue` as undefined with a new
internal `NoneValue` (`EmptyValue` would mean None in `include/` and undefined in `src/`,
and every `Value` to `InternalValue` path would need remapping). The public API does not
change: `InternalValueData` is not exposed, and a `jinja2::Value` holding `EmptyValue`
already converts to the `EmptyValue` alternative, i.e. `None`.

`UndefinedValue` carries no name yet: the error paths have none to give. 0026 can add a
shared hint pointer; everything constructs it through `InternalValue()`, so that stays
source-compatible.

**Sites.**
- `internal_value.h`: member `IsEmpty()` becomes `IsUndefined()` plus `IsNone()`; the free
  `IsEmpty(val)` stays and means "undefined or None" (its callers check "argument not
  given", whose Python default is None).
- `value_visitors.h`: the renderer prints `None` for `EmptyValue` and nothing for
  undefined (repr keeps `None` for undefined inside containers until 0026);
  `UnaryOperation`, `BinaryMathOperation` (undefined == undefined, undefined != None),
  `BooleanEvaluator` and the `StringJoiner` seed get explicit undefined cases.
- `internal_value.cpp`: `OutputValueConvertor` maps undefined to `Value()`.
  `GenericMapAdapter` keeps mapping an empty reflected field to undefined (absent), which
  `map(attribute=..., default=...)` relies on; JSON `null` inside an object therefore
  still reads as undefined (filed as a new task).
- `testers.cpp`: `ValueKind::Undefined` first; `defined`/`undefined` test it.
- `filters.cpp`: `attr` and `items` check `IsUndefined()`; `default` keeps `IsEmpty` (0019).
- `serialize_filters.cpp`: `pprint`/`format` print undefined as `none` as before.
- Producers of None: the `none`/`None` literal (`expression_parser.cpp`) and
  `cycler.reset()` (`global_functions.cpp`).

**Left to other tasks.** `default` on None (0019), the `none` test (0017), `dict.get` and
`list.append` (0020), strict/chainable policies, `tojson` of undefined and the
`Undefined` repr (0026), string filters on None (`none|upper`).

**Tests.** The nine allow-list lines naming 0034 go; new corpus cases pin `defined` on
None, undefined vs None equality, `not`, `in`, macro defaults, `set x = none`, None in a
list and in `join`.
