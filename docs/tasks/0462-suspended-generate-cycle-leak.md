---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/936
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


**Resolution (batch 9 D).** `_LazyRender` takes part in the garbage collector: its traverse reports the template,
the arguments and, while the render runs or is suspended, each object a PyHost of the render's params and
globals holds on the fiber (`PieceOptions::held`, cleared before those values go). The render takes its
arguments by reference so no other reference to them sits on the fiber. Its clear closes the render (on the
owner thread it unwinds on the fiber; elsewhere the fiber is freed, as close() does). In practice the Python
generator's finalizer closes it first. Pinned by `test_a_cycle_through_a_suspended_generator_is_collected`
(keyword, positional, global, closure: a weakref callback fires) and
`test_a_cycle_collected_on_another_thread_does_not_resume_there`. Left: a cycle through a value the template
made while it runs (`{% set y = x.y %}` holding the generator) is still collected only when the generator is
closed (docs/python.md).
