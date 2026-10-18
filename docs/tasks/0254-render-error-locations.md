---
status: open
priority: high
area: robustness
depends: [0253]
touches: [src/renderer.h, src/render_failure.h, src/render_failure.cpp, src/template_parser.h, src/template_parser.cpp, src/internal_value.cpp, test/errors_test.cpp, test/user_callable_test.cpp]
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
