---
status: open
priority: low
area: parity
depends: [0357]
touches: [python/src/template_module.cpp, src/template.cpp]
shares: [include/jinja2cpp/template.h]
---
# Python module: Ctrl-C does not stop `Template.module` or a call of a module's macro

**Problem.** Before 0357 S3 the module installed the private `t_hostPoll` around
`Template.make_module` and around calls of a module's macros, so `KeyboardInterrupt` stopped a
long loop in them as it stops a render. The public `RenderOptions::poll` is taken only by
`Template::RenderTo`; `Template::MakeModule` and the calls of the module it returns take no
options, so since S3 a Ctrl-C during them is seen only after they return. A
`BaseException` a Python callable raises inside them is still raised after the call
(`RenderState::abort`).

**Proposal.** Give `Template::MakeModule` an overload with `RenderOptions` (globals and
poll), and let the module keep the options for the calls of its exports, as a render
keeps them for its callables. This is a public-API addition, so it needs a decision before
the G1 freeze.

**Done when.** A test interrupts a long loop in a module's macro from Python (as
`python/tests` does for a render) and gets `KeyboardInterrupt` during the call.
