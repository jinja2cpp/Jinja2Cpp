---
status: open
priority: low
area: parity
depends: [0326, 0323]
touches: [src/statements.cpp, src/render_context.h, src/template_module.cpp, test/parity/divergences/loader.txt]
---
# Closures kept past a module call or a scoped frame of an include lose their names

**Problem.** Found by the verifier of parity batch 2 group C (0326, 0323). A closure that keeps
the names of the frame it was defined in (`ClosureCapture`) loses them in two cases.

1. *A host's module call.* The copies made during a call the host makes into a template module
   (`TemplateModuleState::Call`) are owned by that call's cycle collector, which releases them
   when the call ends. A closure that a module macro stores in module state then reads the
   macro's names as undefined. Module:

   ```jinja
   {% set ns = namespace(f=none) %}{% macro keep(n) %}{% macro i() %}I{{ n }}{% endmacro %}{% set ns.f = i %}{% endmacro %}{% macro later() %}{{ ns.f() }}{% endmacro %}{{ keep('top') }}
   ```

   `later()` from the host gives `I`, and after the host calls `keep('host')` still `I`.
   Python Jinja2 3.1.6 gives `Itop`, then `Ihost`. No corpus case: the corpus renders
   templates, not host calls into a module; pin it in `test/template_module_test.cpp`.
2. *A `with` or `block` frame inside an included template.* The include captures its context
   when it ends (`RenderIncludedTemplate`), but a closure defined in a `with` or `block` body
   there registered with the frame that ended first, so nothing is copied for it: the block
   case reads `a` and `q` as undefined (`|`, Python `|AQ`), the `with` case reads `q` as
   undefined too (`|missing`, Python `|missingQ`). Corpus rows
   `loader.include_block_macro_kept` and `loader.include_with_macro_kept`.

**Proposal.** (1) Own the copies of a module call by the module's state (or the render that
holds it) rather than by the call's collector, so they live as long as the closure can be
reached. (2) Capture a closure's frame at the end of the scoped frame it was defined in (the
`with`, block or scoped block), as `InvokeMacroRenderer` does for a macro call, down to the
include's top level, and let the include's own capture add its top-level names.

**Done when.** Both corpus rows match and their divergence lines are deleted, a module-state
test pins case 1, and the memory bound of 0326 (copies freed once no closure outside them
holds the capture) still holds.
