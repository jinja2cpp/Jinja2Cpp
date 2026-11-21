---
status: open
priority: low
area: robustness
depends: [0545]
touches: [src/value_methods.cpp#SetMethods, src/statements.cpp, src/generic_adapters.h]
shares: [test/parity/cases/methods.py, test/parity/divergences/methods.txt]
---
# The engine's set has no hash index, and a loop over a set changed in its body goes on

**Problem.** 0545 gave the engine's set (`d.keys() - []`) Python's methods. The set is a list adapter
with no index, so `add`, `remove`, `discard` and `in` scan it (`ItemEquals`, O(n) each). A loop over
a set that its body grows, `{% for x in s %}{% do s.add(x + 1) %}{% endfor %}`, reads the set by
index and goes on, where Python raises `Set changed size during iteration` at the next step
(`methods.set_add_during_loop`, `methods.set_remove_during_loop`, deliberate lines of 0545). With
the scan, such a loop slows down quadratically until `maxSequenceSize` or `maxRenderSteps` stops it;
neither is set by default, so an untrusted template can spend O(n^2) time on it.

**Proposal.** Keep a hash index beside the set's items (the token hashing that dicts use), and give
the loop over a set the size check a dict walk has (0538): a size change between steps raises
Python's error.

**Done when** the two divergence lines are gone and a bench or limits test shows `add` in a loop
is linear.
