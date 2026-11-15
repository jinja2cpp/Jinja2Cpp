---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/894
priority: low
area: robustness
depends: [0418]
touches: [include/jinja2cpp/value.h, src/value.cpp, test/value_deep_test.cpp]
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

**Decision (Ruslan 2026-11-15).** Take the additive helper (`jinja2::DestroyDeep(Value&&)` and a depth-checked `DeepCopy`) and leave `Value`'s destructor as it is; it may land after the G1 freeze.

**Implementation.** `include/jinja2cpp/value.h` declares, after `ValuesMap`, two exported free
functions and a constant; nothing in `Value` changes, so the change is additive to the ABI and
costs the render path nothing (no inline code calls them):

- `jinja2::DestroyDeep(Value&&) noexcept` takes the value apart depth-first on a heap worklist:
  each list or map is destroyed after the items that hold items of their own were moved out of
  it, so no destructor recurses more than one level. The value is left `none`. If the worklist
  cannot grow (`std::bad_alloc`), the rest is destroyed by `Value`'s destructor, as without it.
- `jinja2::DeepCopy(const Value&, std::size_t maxDepth = ValueMaxSafeDepth)` returns
  `std::optional<Value>`: the same value the copy constructor makes (views stay views,
  `GenericList`/`GenericMap`/`UserCallable` copied as the copy constructor copies them), built
  top-down from a worklist, or `std::nullopt` when list and map levels nest deeper than
  `maxDepth` (a scalar is 0 deep, `[1]` 1). The partial copy of a refusal or an exception is
  released with `DestroyDeep`.
- `ValueMaxSafeDepth = 512`, the default bound: a copy that deep is destroyed by `~Value` well
  inside a 1 MB stack, as `JsonMaxCopyDepth` (also 512) bounds JSON documents.

The JSON bindings already cap their own depth (`JsonCopyLimits::maxDepth`, 128 by default and at
most `JsonMaxCopyDepth`), so a `ToValue` copy never builds a value that needs `DestroyDeep`; only
a host's own reader without a depth limit does. `test/value_deep_test.cpp` builds lists and
mixed list/map values 200000 deep, destroys them with `DestroyDeep`, copies them with an
unlimited `DeepCopy`, and checks the refusal at the bound, the depth count, key order and that a
string view stays a view.
