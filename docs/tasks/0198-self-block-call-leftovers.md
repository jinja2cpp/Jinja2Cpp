---
status: done
priority: low
area: parity
depends: [0194]
touches: [src/statements.cpp#MakeTemplateSelf]
shares: [test/parity/divergences/loader.txt]
pr: TBD
---
# Calls through `self` that still differ from Jinja2

**Problem.** Found by the 0194/0195 verifier; all differ on master before 0194 too.
- `self.b(1)` and `self.b(k=1)` render the block; Jinja2 raises TypeError
  (`loader.self_block_call_with_args`).
- A `self` that leaves its template renders nothing when called from a template whose
  frames do not lead back to it: a module's `self` stored by one of its macros and called
  by the importer (`loader.module_self_stashed_called_by_importer`, Jinja2 `MB[mv]`), or a
  `self` stored by an include that has ended. `MakeTemplateSelf` looks for the owning frame
  only through `outer`; when the caller also has a block of that name, it renders the
  caller's block instead.
- `self.b()` inside a scoped block in a loop shows the top-level value of the loop
  variable; Jinja2 shows the loop's (a scoped block's context includes the loop locals).
- `{% for self in ... %}` followed by `self.b()` differs.

**Proposal.** Check the call's arguments as Jinja2's `BlockReference.__call__` does. For the
frame, let `self` keep its own (the module's `TemplateFrame` lives as long as the render;
an ended include's blocks would need the stack kept alive), using the `ClosureSite` 0194
added to know whether the home is still running.

**Done when** no line of `test/parity/divergences/` names 0198.

**Outcome.** A call through `self` checks its arguments as `BlockReference.__call__`
(keyword first, as Python reports). A `self` whose template is not on the caller's frame
chain renders in the frame its home context runs, when that frame is the template's: a
module's `self` stored by its macro renders the module's block. `{% for self in ... %}`
matches since 0143. An ended include's `self` and a scoped block's loop locals seen through
`self` are left in 0206.
