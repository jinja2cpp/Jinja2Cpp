---
status: open
priority: low
area: robustness
depends: [0457]
touches: [python/src/environment.cpp]
shares: [python/tests/test_generate.py]
---
# A reference cycle through a suspended generator is not collected

**Problem.** Python Jinja2's generator is an ordinary Python generator, so a cycle through it (the
generator stored in a context value the template holds, or a callable closing over it) is collected by
the cycle GC. After 0457 a suspended on-demand render holds its parameters and host objects from C++ on
the fiber's stack, which `tp_traverse` cannot see, so such a cycle leaks until the generator is closed:
the objects, plus the fiber stack (8 MiB reserved, a few KiB resident). Accepted for 0457 (decision D4).

**Proposal.** Keep the render's Python parameters as borrowed `PyHost` references into a private dict
owned by the generator object, which `tp_traverse` visits, and on `tp_clear` close the render (cancel the
fiber at its suspended write so destructors run on the owner thread). Check that a generator collected
from a GC pass on another thread does not resume there (close it eagerly or defer to its owner).

**Done when.** A cycle through a suspended generator is collected by `gc.collect()` (a weakref callback
fires) with destructors run, under the ASan module job.
