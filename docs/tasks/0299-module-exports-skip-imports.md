---
status: open
priority: low
area: parity
touches: [src/statements.cpp, src/statements.h]
---
# A module exports the names its top level imports, and keeps a re-set import hidden

**Problem.** Jinja2 exports from a template's module only the names its top level sets
(`context.exported_vars`): `{% import %}` and `{% from ... import %}` discard their targets
from it, and a later `{% set %}` of the same name adds it back. Two gaps remain:

1. `{% import %}` of a module (statements.cpp, `ImportStatement::ImportNames`) copies every
   macro and non-callable of the module's top scope, the ones that module imported included.
   With `a` = `{% macro am() %}AM{% endmacro %}` and `m2` =
   `{% from "a" import am %}{% import "a" as aa %}{% macro own() %}O{% endmacro %}`,
   `{% import "m2" as x %}[{{ x.am is defined }}|{{ x.aa is defined }}|{{ x.own() }}]` gives
   `[True|True|O]`; Jinja2 3.1.6 gives `[False|False|O]`. `{% from "m2" import am %}` binds
   `am` (Jinja2: undefined, and an error when it is used).
2. `Template::MakeModule` (0284) leaves out a name its top level imported while the name
   still holds the imported object, which `IRendererCallback::ImportBound` records. A
   `{% set am = am %}` after `{% from "a" import am %}` stores the same object, so `am` stays
   hidden; Jinja2 exports it. Setting it to another value exports it.

**Proposal.** Record imports per module in `ImportedModule` (the `ImportBound` hook already
reports them, at the module's top level only) and filter them in `ImportNames`. For the
re-set case, the top-level `set` statements of a module render would have to clear the
record, which costs a check on every top-level assignment: only if it can be kept off the
render path of templates that are not modules.

**Done when** both examples match Jinja2, pinned by corpus cases (loader area).
