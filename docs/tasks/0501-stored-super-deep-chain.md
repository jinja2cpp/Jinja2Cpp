---
status: open
priority: low
area: parity
depends: [0482]
touches: [src/statements.cpp]
shares: [test/template_module_test.cpp]
---
# A stored `super` called past its block renders without the block's frame

**Problem.** Found by the verifier of 0480/0482. The callable `super` gives (src/statements.cpp,
the lambda near `RenderBlockAt`, ~2757) captures the block name as a `std::string_view` and runs
`RenderBlockAt` without setting up the frame of the block chain it came from. Stored in a module's
namespace and called by a later call, deeper in an `extends` chain
(`{% extends 'mid' %}{% block b %}{% set ns.f = super %}CH{% endblock %}`, `mid` extending `base`,
each with block `b`, `mid`'s calling `super()`), its output differs from Jinja2 3.1.6, which
renders the parent's block as at the point `super` was taken. The view is safe today only
because a tree with blocks always holds a slot (0482 never frees it).

**Proposal.** Capture what `SelfBlockCall` captures (an owned block name and the chain's
`SelfBlocks` handle plus level), so the stored callable renders the same parent block as Python.

**Done when** a `TemplateModuleChangingSourceTest` row with the template above (stored by call 1,
called by call 2 and 3) prints what Jinja2 3.1.6's `make_module()` prints, narrow and wide, ASan
clean.
