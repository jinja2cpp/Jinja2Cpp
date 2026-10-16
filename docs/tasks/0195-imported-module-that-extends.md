---
status: open
priority: low
area: parity
depends: [0167]
touches: [src/statements.cpp#ImportedModule]
shares: [test/parity/divergences/loader.txt]
---
# An imported module that extends a parent

**Problem.** A module that `extends` another template gets only its own blocks on its
`self`: Jinja2C++'s `ImportedModule::Render` pushes the module's blocks but never the
parent's, so in a macro of the module
- `self.x()` for a block only the parent defines raises `'dict object' has no attribute 'x'`;
  Jinja2 renders the parent's block (`loader.extending_module_self_parent_block`, `FBX`);
- `self.y()` for a child block that calls `super()` raises `there is no parent block called 'y'`;
  Jinja2 renders `BYC` (`loader.extending_module_self_super_block`).
Pre-existing (the old import path had the same blocks); found by the 0167 verifier.

**Proposal.** When the module's renderer extends, push the parent chain's blocks into the
module's `BlocksStack` as the extends statement does for a rendered template, without
rendering the parent's output (an import drops output anyway, but the parent's top-level
`set`s should not reach the module's names: check Jinja2 on that before choosing).

**Done when** both corpus cases match and their divergence lines are gone.
