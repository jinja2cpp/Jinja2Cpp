---
status: in-progress
priority: low
area: parity
depends: [0420]
touches: [src/statements.cpp#LoopAccessor, src/loop_attr.h]
---
# `loop.__call__` prints nothing

**Problem.** Item 5 of 0420: `{% for i in [1] recursive %}{{ loop.__call__ }}{% endfor %}` prints
nothing; Jinja2 3.1.6 prints `<bound method LoopContext.__call__ of <jinja2.runtime.LoopContext
object at 0x...>>`. The address keeps it out of the parity corpus as an exact case; `(loop.__call__
|string).split(' at ')[0]` pins it as `globals.cycler_method_prints` does for a cycler.

**Proposal.** Give `loop`'s `__call__` the bound-method repr the cycler's and joiner's methods
have (`MakeUtilsMethod`, global_functions.cpp), keeping `loop(...)` on the call slot.

**Done when** the split form above matches Jinja2 in the narrow and wide paths.

**Resolution.** Jinja2 3.1.6 prints `<bound method LoopContext.__call__ of <LoopContext 1/2>>`: the
receiver's repr is the loop's own, with no address, so the corpus holds the exact output
(`statements.loop_dunder_call_*`). `loop.__call__` is looked up by name (LoopAttr::DunderCall) and
made by `MakeLoopDunderCall`: the call slot's callable, bound to the loop's cycle cell under the name
`__call__`, so its repr follows the loop after it ends, as `loop.cycle`'s does. The call slot itself
stays unbound, so a recursive loop makes no cell unless the template reads `loop.__call__`.
Found on the way (verifier): `loop(none)` rendered empty where Python raises `'NoneType' object is not
iterable`; it raises now, and `loop(undefined)` stays empty as in Python (`statements.loop_call_none*`).
