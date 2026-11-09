---
status: open
priority: low
area: perf
depends: [0357]
touches: [python/src/adapters.cpp, python/src/convert.cpp]
---
# Python module: one sandbox stub type per class, and the repr of a cycler or joiner

**Problem.** Two leftovers of 0357 S3, found by its verifier:
- With a custom `is_safe_attribute` (`SandboxedEnvironment` subclass), `PySandboxPolicy::NativeAttribute`
  asks it with a stand-in object for an engine value of a class (`LoopContext`, `Namespace`,
  `Macro`, ...). `Stub()` makes a new Python heap type for each call, so a loop that reads
  `loop.index` makes one type per step.
- A `cycler()` or `joiner()` passed to a Python callable reprs as
  `<Cycler {'next': Undefined, 'items': [1, 2], ...}>`; Jinja2 gives its default object repr
  (`<jinja2.utils.Cycler object at 0x...>`).

**Proposal.** Cache one stub type per class name in the module state. Print a cycler and a
joiner as `<jinja2.utils.Cycler object at 0x...>` and `<jinja2.utils.Joiner object at 0x...>`
with the `TemplateValue`'s address, so a test can compare the prefix.

**Done when.** A loop under a custom `is_safe_attribute` makes no type per step (checked by
counting types or by `bench/count.py`), and a row compares the cycler's repr prefix with Jinja2's.
