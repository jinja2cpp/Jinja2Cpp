---
status: open
priority: medium
area: parity
depends: [0022]
touches: [src/render_context.h, src/statements.cpp#macro]
shares: [src/statements.h, src/template_impl.h]
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

**Done when.** No line of `test/parity/divergences/` names task 0038, and
`ctest -R parity` passes.

**Next.** `{% set %}` inside loops and blocks has its own scoping rules that the same
closure model will expose.
