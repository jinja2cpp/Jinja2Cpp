---
status: open
priority: low
area: parity
depends: [0420]
touches: [src/statements.cpp#LoopContext]
---
# `loop.__call__` prints nothing

**Problem.** Item 5 of 0420: `{% for i in [1] recursive %}{{ loop.__call__ }}{% endfor %}` prints
nothing; Jinja2 3.1.6 prints `<bound method LoopContext.__call__ of <jinja2.runtime.LoopContext
object at 0x...>>`. The address keeps it out of the parity corpus as an exact case; `(loop.__call__
|string).split(' at ')[0]` pins it as `globals.cycler_method_prints` does for a cycler.

**Proposal.** Give `loop`'s `__call__` the bound-method repr the cycler's and joiner's methods
have (`MakeUtilsMethod`, global_functions.cpp), keeping `loop(...)` on the call slot.

**Done when** the split form above matches Jinja2 in the narrow and wide paths.
