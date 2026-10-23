---
status: done
priority: high
area: robustness
depends: [0253]
touches: [src/renderer.h, src/render_failure.h, src/render_failure.cpp, src/template_parser.h, src/template_parser.cpp, src/internal_value.cpp, src/error_boundary.h, src/engine_errors.h, src/node_arena.h, src/node_arena.cpp, src/template_impl.h, src/statements.cpp, test/errors_test.cpp, test/user_callable_test.cpp, test/includes_test.cpp, test/recursion_limits_test.cpp, test/limits_test.cpp, test/filters_test.cpp, REVIEW.md]
shares: [src/node_arena.h, src/node_arena.cpp, src/template_impl.h, src/statements.cpp, test/includes_test.cpp]
---
# Error contract P2: render errors carry their template location

**Problem.** Every render error reports `file:1:1`, and an exception from a host callable is
indistinguishable from the engine's own errors. Contract: docs/api-2.0.md §2.1 rule 8; plan:
docs/design/0250-error-contract-plan.md §3c, §3d.

**Scope.**
- Record each statement's source offset when the parser closes a body (in the arena block after
  the children; fallback a sorted table in `TemplateImpl`); the sealed arena keeps a pointer to
  its template's source and name.
- One `try` around the child loop in `ComposedRenderer::Render`: rethrow `bad_alloc` and
  `RenderFailure` (innermost location wins); everything else goes to
  `detail::ThrowRenderFailure`, which classifies it (`jinja2::Exception` keeps its code, host
  callable failure 15, `TemplateRuntimeError` 17, undefined 11, recursion 12, other 1 with the
  exception kept) and attaches the location.
- `UserCallableBody::operator()` wraps only the host call: engine-origin types pass through,
  anything else becomes `HostCallableFailure` (code 15) with the exception kept.
- Narrow the include `FileNotFound` catch (src/statements.cpp) to the template lookup: Python's
  `ignore missing` does not cover a missing template nested inside the included one.
- Review rule from now on: no exceptions as control flow across statement bodies.

**Tests.** errors_test.cpp `RenderErrorLocationTest` {template, context, code, line, col, has
exception}: a throwing callable at 2:1 gives 15; `i.y.z` inside a loop at 2:3 gives 11; a
reflected `uint64_t` field holding `UINT64_MAX` gives 16; a throwing `GenericList` accessor gives
1; errors inside included, imported and parent templates name that template; recursion gives 12
with a location; a throwing `AddFilter` filter gives 15. Update the rows that encode `1:1`.
user_callable_test.cpp: `std::rethrow_exception(GetException())` yields the original type.
includes_test.cpp: `ignore missing` with a missing nested include fails with `TemplateNotFound`.
Sanitizer build and fuzz smoke (new unwinding paths).

**Perf gate.** Render Ir within ±0.1%, I1 misses unchanged; Load allocation count unchanged with
arena offsets (+1 per template with the fallback).

**Done when.** The rows pass and the bench gate holds.

**Result (PR #PRNUM).** As planned, with these choices:
- Offsets are stored after the children, last child first, so `ComposedRenderer::Render` finds
  them from the end pointer the loop already holds: the handler adds no live register, and
  Render instruction counts stay at the base (-0.3% to +0.1%). The origin is a pointer slot
  after the sealed tree's cleanup table, written by `Seal`.
- Load costs +0.6% to +2.5% instructions (the offset per child in the parse list and the
  tree, +4 B per statement); allocations are unchanged.
- One classification, `detail::CurrentExceptionToData` (src/error_boundary.h), serves the body
  catch and the public boundaries. A thrown `ErrorInfo` with a caret line (a parse error in an
  included template) keeps its location; one without gets the statement's, and keeps its own
  file name as a related location when it names another template.
- A parent template renders after the body of the template that extends it, outside every
  statement: its errors are located in the parent's own bodies, and recursion through
  `extends` reports the child's start.
- A callable's `std::length_error` is the callable's failure (15); host budgets throw
  `jinja2::Exception{ResourceLimitExceeded}`.
