---
status: open
priority: low
area: parity
depends: [0596]
touches: [src/statements.cpp#FindSelf, src/statements.cpp#SelfBlockCall, src/statements.cpp#SuperBlockCall]
shares: [test/parity/cases/statements.py, test/parity/divergences/statements.txt]
---
# A stored `self` or super() misses the scoped block's derived context in the shapes 0596 left

**Problem.** Found by the verifier of batch 15 A (0596), the same on master b3313a2. 0596 makes a `self` or super()
made directly in a scoped block's body keep its derived context (a snapshot of the frames around the block and of the
template's top level) once those frames end. These shapes still render with the live or ended frames instead:
- a macro defined in a scoped block in a loop, calling `self.y()`, stored and called after the loop: `[1][2]|[]`,
  Jinja2 3.1.6 `[1][2]|[2]` (corpus `statements.stored_macro_calling_self_block_of_scoped_block_after_loop`);
- a `self` stored in an unscoped block inside a scoped one in a loop: `|[]`, Jinja2 `|[2]`
  (`statements.stored_self_of_unscoped_block_in_scoped_block_after_loop`);
- a `self` stored in a scoped block at the template's top level, no loop, read after a later top-level `set`: the new
  value, Jinja2 the snapshot `[91]|[91]` (`statements.stored_self_of_top_level_scoped_block_top_level_set_later`);
  FindSelf takes its derived branch only for a block above the top level;
- a three-level `super` chain whose middle template stores its own `super`: Jinja2 `<b2>|b2`, the engine `<b2>|b`
  (it loses `{{ i }}`);
- a macro an included template defines, stored and called after the include, that calls `self.y()`: Jinja2 raises
  UndefinedError, the engine renders `[]`.

**Proposal.** Make every `self` and super() made in a call whose context is derived (the block call's context or one
made from it, a macro called there) take the capture its derived owner keeps (OwnsDerivedCapture), and give the top-level
scoped block the same snapshot. Check each shape against Jinja2 narrow and wide, under ASan.

**Done when.** The three corpus lines are gone from test/parity/divergences/statements.txt, the `super` chain and the
include shape have corpus cases that match, narrow and wide.
