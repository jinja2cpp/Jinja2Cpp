---
status: in-progress
priority: medium
area: parity
depends: [0165, 0166, 0167]
touches: [docs/design/0038-lexical-scoping-plan.md]
---
# Lexical scoping for macros

**Problem.** A Jinja2 macro is a closure: names in its body and defaults resolve where
the macro is defined, at call time. Jinja2C++ resolves names through the render context
of whoever calls the macro (dynamic scoping). So a macro called from another macro sees
the caller's locals instead of the globals (`macro_body_lexical_scope`), and macro
defaults had to keep definition-time evaluation in 0022 to stay right for imported
macros and nested calls, which makes a default miss a global reassigned after the
definition (`macro_default_reassigned_global`). Imported macros (`import ... as`) also
lose their module's variables inside the body.

**Proposal.** Give a macro a reference to its defining scope chain (the module's top
level for imported macros) and run the body and per-call defaults in a context made of
that chain plus the argument scope, instead of stacking on the caller's context. Then
evaluate every default per call, dropping the definition-time snapshot from 0022.
Watch lifetimes: the defining scopes must outlive the callable (shared ownership rather
than pointers into `RenderContext`'s deque).

**Done when.** 0165, 0166 and 0167 are done: no line of `test/parity/divergences/` names
them except `deliberate:` lines, and `ctest -R parity` passes.

**Next.** `{% set %}` inside loops and blocks has its own scoping rules that the same
closure model will expose.

**Plan (approved by Ruslan 2026-10-12, after an advisor review).**
docs/design/0038-lexical-scoping-plan.md, review in docs/design/0038-advisor-review.md.
Recommendation C: a closure records its defining site (weak anchor to the defining context,
scope depth and serial) and its body runs there with a static link that skips the caller's
scopes; names stay late-bound. Only a context owns its anchor and only the render owns
imported modules; everything else is weak. Decisions: closures that outlive their frame read
its names as undefined (deliberate lines); Jinja2's frame-entry binding is not reproduced
here (0168); `with context` snapshots the importer's names at import; no opt-out, changelog
entry only; "Done when" also accepts `deliberate:` lines owned by the phase tasks.

**Phases.** 0165 bodies run where defined (5 of the 6 lines), 0166 defaults per call (the
last line), 0167 modules own their context, in that order. Found on the way: 0168
(idtracking frame-entry binding), 0169 (`caller()` without a call block). This task closes
when 0165-0167 are done; the divergence lines it owned now name those tasks.
