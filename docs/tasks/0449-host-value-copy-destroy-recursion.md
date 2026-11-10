---
status: open
priority: low
area: robustness
depends: [0418]
touches: [include/jinja2cpp/value.h, src/value.cpp]
shares: [test/template_module_test.cpp]
---
# A host Value nested deeper than the stack allows cannot be copied or destroyed

**Problem.** `Value`'s copy constructor and destructor are the defaults: each recurses once per
level of its `ValuesList` and `ValuesMap` (about six frames a level), so a host value nested
around 100000 deep overflows the stack when the host copies or destroys it. The engine no longer
copies one without a stack check (0418) and its own values are destroyed iteratively (0093), so
only the host's own code is exposed, as with any recursive structure in C++. But a host that
builds such data from untrusted input (a JSON reader without a depth limit) crashes on its
destruction, and the library offers no way out. 0418 tried an iterative destructor:

- inline in value.h, scanning for nested containers: +5.9% instructions on `Module/keep`, and
  a worklist allocation in a noexcept destructor;
- an inline index compare calling an exported `Value::DestroyContainer` (thread-local depth,
  recursive to 64 levels, then a worklist, falling back on any failure): +2.2% on `Module/keep`
  (a call, the TLS access and a move per destroyed container), a new exported symbol the
  inline `~Value` calls (a binary built on the new header needs the new library), and
  `__tls_get_addr` in -fPIC builds unless the variable takes `JINJA2CPP_HOT_TLS`.

**Proposal.** Decide whether host-side deep values are the library's concern. If they are,
either take the second design with the cost recorded as an exception (or measured cheaper with
a hot/cold split and an early return for a moved-from container), or offer an explicit
`jinja2::DestroyDeep(Value&&)`/depth-checked `DeepCopy` helper and leave `Value` as it is, and
document the limit for the JSON bindings (which may need their own depth cap).

**Done when** the decision is recorded, and under the chosen design a `Value` nested 200000 deep
is copied (or refused) and destroyed under ASan without a crash, with the instruction gate within
its bar.
