---
status: done
priority: low
area: api
touches: [include/jinja2cpp/template.h, src/template_impl.h, src/template.cpp, src/name_resolver.h, src/name_resolver.cpp, src/template_parser.cpp, test/undeclared_variables_test.cpp]
---
# Template introspection: `meta.find_undeclared_variables`, `Template.module`

**Problem.** `docs/parity.md` marks `jinja2.meta.find_undeclared_variables` and
`Template.module`/`make_module` as ❌ with no task. Hosts commonly use
`find_undeclared_variables` to learn which inputs a template needs (LLM chat-template
hosts check required variables this way). `GetMetadata` reads only the C++-specific
`{% meta %}` block. Raised by milestone 1.2's `meta` issue (#107), whose closure delivered
the `{% meta %}` block rather than introspection.

**Proposal.** A public API that returns the free names of a loaded template by walking
the parse tree (names read before any local binding, including in included templates
only when Jinja2's `meta` does); tests mirror `jinja2.meta`'s behaviour. Decide
`Template.module` (exported macros and top-level sets as a map) as do or won't do. An
additive API, so it can land in a 2.x minor (docs/release-2.0.md section 4).

**Done when.** The API exists with tests checked against `jinja2.meta`, and parity.md's two
rows name this task or a decision.

**Done.** `BasicTemplate::FindUndeclaredVariables()` returns
`Result<std::vector<std::string>, CharT>`: the names sorted, `TemplateNotParsed` before a
successful Load. It parses the kept source again into an arena of its own and asks that
parse's `NameResolver` (`UndeclaredNames`), whose frames already follow Jinja2's idtracking
(0117, 0168), so Load pays nothing. A name counts when it is read, or stored in a branch of
`if`, with no store before it in its frame, no store anywhere in a frame around it (blocks
see only their own), and no binder (loop targets and `loop`, macro and call-block arguments
with `caller`/`varargs`/`kwargs`, defaults seeing their macro's arguments, `self`, a block's
`super`). The built-in globals (with the i18n ones when the extension is on) and the
environment's globals are left out, as Jinja2 leaves out `env.globals`. Includes, imports and
the parent of `extends` are not looked into, as in Jinja2. Imported names other than their
alias now store with `StoreKind::Hidden` instead of `Branch` (same behaviour at render), and
the names a trans block's messages use are read where the block is (`AddUse`), so they also
resolve to loop and macro slots. Tests: test/undeclared_variables_test.cpp, 65 templates
checked against Jinja2 3.1 with the do, loopcontrols and i18n extensions and null
translations installed, narrow and wide.

**Known divergence:** without `install_gettext_callables`, Jinja2 reports `gettext` and
`ngettext` for a trans block (they are not globals then); Jinja2C++ has them as globals
whenever the i18n extension is on, so it never reports them.

Bare trans variables (`{% trans user %}`) are reads as well, and a set block in a loop's
`else` stores in the else's frame rather than in a branch, as a plain `set` there does.

**Other known divergences (verifier, 2026-10-18):** `{% autoescape e %}` reads `e` in the
frame around it, where Jinja2 reads it in a scope of its own, so a later `{% set e %}` in that
frame declares it for Jinja2 only; a trans block whose first variable is a call
(`{% trans x=f(y) %}`) makes Jinja2 also report its synthetic `_trans`. The check is
O(reads x stores of a frame): about 0.5 s in a Debug build for 5,000 top-level `set`s each
read once, which only an opt-in call on such a template pays.

**`Template.module`** is filed as 0284: it needs the macros of a template to be callable
from the host outside a render, which is new lifetime work rather than introspection.
