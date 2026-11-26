---
status: in-progress
priority: low
area: parity
depends: [0312]
touches: [src/statements.cpp, src/template_impl.h, test/template_module_test.cpp]
shares: [src/render_context.h, src/statements.h]
---
# A stored closure that reaches a call's module only through its capture keeps the module

**Problem.** Found by the 0312 verifier. Since 0312 a module a host's call of a `Template::MakeModule` export
imports with context stays while a closure found from the template module's values runs in it
(`ImportedModule::MarkReached`). The walk follows `ValueObject::Traverse`, which does not show what a closure keeps
of its ended frames (`ClosureCapture`, behind the callable's keep-alive). A closure of the template module itself that
names the imported module in such a frame is not seen, so the module ends with the call and a later call fails with
`TemplateExpired`; Jinja2 3.1.6 prints `X`:

```
lib:  {% macro x() %}X{% endmacro %}
main: {% set ns = namespace() %}
      {% macro keep() %}{% import 'lib' as l with context %}{% macro inn() %}{{ l.x() }}{% endmacro %}{% set ns.f = inn %}{% endmacro %}
      {% macro use() %}{{ ns.f() }}{% endmacro %}
keep(), then use()
```

The same holds for a `caller` made at a module's top level and stored by a macro it calls, which
`ImportedModule::MayBeReached` does not count as a closure that may escape.

**Proposal.** Let the walk visit the names of a closure's capture (the copy `KeptObjects` holds with its names), from
the closure only, never from `KeptObjects` as a root, so that a copy only a dying module holds cannot keep it; and let
`MayBeReached` count a top-level `caller`.

**Done when** the example prints `X` on `use()` as in Jinja2, `ModuleCallTest.StoredMacrosStayFlat` still holds with a
row storing such a closure on every call, and the 0312 rows stay as they are.

**As built.** `ImportedModule::MarkReached` also walks, from each closure it reaches, the names its capture kept of
the closure's ended frames (and their `endedSelf`), along `ClosureCapture::outer` (`AppendCapturedObjects`); never from
`KeptObjects` as a root. `MacroCallStatement::Render` notes a `caller` made in a module that ends with the call
(`RenderContext::NoteOtherClosure`), so that `MayBeReached` looks. Rows:
`TemplateModuleTest.ClosureStoredAcrossCallsKeepsTheModuleItsCaptureNames` (the example, a top-level `caller` naming an
imported module and one reading a later top-level name) and a `ModuleCallTest.StoredMacrosStayFlat` row storing such a
closure on every call.
