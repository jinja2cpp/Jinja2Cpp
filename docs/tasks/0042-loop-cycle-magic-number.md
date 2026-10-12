---
status: open
priority: low
area: parity
depends: [0030]
touches: [src/global_functions.cpp]
---
# Global function follow-ups: globals are maps

**Problem.** `loop.cycle` used to be the integer 2 with integer dispatch in
`CallExpression::Evaluate` (as `range` was until 0030 made it a `Callable`). That half of
this task moved to 0144 (2026-10-12), which owns `LoopCycleFn`, `globals.int_called_in_loop`
and the cycle bench check. What is left here are the global objects that are maps.

**Gaps found by the 0030 review** (fix together or split out):
- `cycler(...)` and `joiner(...)` return `MapAdapter`s of callables, so `c is mapping` is
  true (`globals.cycler_not_mapping`), `c|length` is 5 and `{{ c }}` prints their internals,
  where Jinja2 has opaque objects (`<jinja2.utils.Cycler object at ...>`). An object kind
  that is subscriptable but neither a mapping nor iterable would fix all three.
  `namespace()` (0021) is the same: it is a `MapAdapter` marked `IsNamespace()`, so it
  tests as a mapping, iterates its keys and prints as a dict, where Jinja2 prints
  `<Namespace {'a': 1}>` (`statements.namespace_is_mapping`, `namespace_iterate`,
  `namespace_print`).
- `range()` converts its arguments with `ConvertToInt`, so `range(1.5)` and `range('3')`
  render where Python raises `TypeError` (`globals.range_float_argument`); `dict(none)`
  renders `{}` (that one also needs 0034 to tell `None` from undefined).

**Done when.** No line of `test/parity/divergences/` names 0042 (`globals.range_float_argument`,
`globals.cycler_not_mapping` and the `statements.namespace_*` lines).
