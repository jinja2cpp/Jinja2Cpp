---
status: open
priority: low
area: parity
depends: [0567]
touches: [src/statements.cpp]
shares: [test/template_module_test.cpp]
---
# A closure defined in a `with` at a module's top level reads the module's names as undefined on later calls

**Problem.** Found by the verifier of batch 13 D (0567), identical on master 1b69673. A `Template::MakeModule` macro
`keep()` imports `lib` with context, `use()` calls `ns.f()`. With `lib` =
`{% set v = 5 %}{% with q = 1 %}{% macro f() %}{{ v }}{% endmacro %}{% set ns.f = f %}{% endwith %}`, `use()` prints
nothing where Jinja2 3.1.6 prints `5`; with `{{ x.am() }}` (`x` imported at `lib`'s top level) the call fails with
`TemplateExpired`, and so does a `{% call st() %}` inside the `with` that `st` stores. The same code in a `for` or an
`if`, and in a plain render, matches Jinja2. The `with` body's frame (FrameEnd::Soft) ends before the module's top level
does, so the closure's capture of the module's top level is not kept with the module walk (`ImportedModule::MarkReached`)
or the copy has no names of the top level.

**Done when** the three rows print as Jinja2 does (`5`, `AM`, `CAM`), narrow and wide.
