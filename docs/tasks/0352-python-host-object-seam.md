---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/706
priority: medium
area: api
depends: [0248]
touches: [src/host_object.h, src/internal_value.h#IMapAccessor, src/internal_value.h#MapAttrPolicy, test/host_object_test.cpp]
shares: [src/internal_value.cpp, src/value_visitors.h, src/value_methods.cpp, src/expression_evaluator.cpp, src/template_dict.cpp, test/CMakeLists.txt]
---
# Python module P1a: a private host-object seam in the engine

**Problem.** A Python object needs operations the engine's values do not have. It prints
through `str()`, its attributes come from Python's `getattr`, it may be called, its dict keys
need not be strings, it is compared with `==`, and it must come back out of a user callable as
the same object. The public `Value` has none of this, and the plan for 0248 (D3, approved by
Ruslan 2026-10-30) keeps the public headers unchanged until 0357.

**Proposal.** Add a private header `src/host_object.h` with an internal interface,
`IHostObject`. Its operations are `Str`, `Repr`, `GetAttr`, `GetItem(InternalValue)`,
`SetItem`, `Call(CallParams)`, `Iterate`, `Len`, `Truth`, `Equals`, `Hash`, `Identity` and
`IsMarkupLike`. The engine reaches it from a map or list accessor, so `ValueStorage` gets no
new alternative. A new `MapAttrPolicy::Host` sends `x.name` to the host first. The print,
attribute, subscript, call, iteration, equality and `in` paths dispatch to the interface. Dict
keys use `ObjectKey` from 0049 P4 (0049 plan, section 9), with the address taken from
`Identity()`; if this task lands first, it adds `ObjectKey` with the address-only scheme. A
C++ toy host object in `test/` covers each operation. Plan: section 7 and D3 of
`/mnt/project-files/release-2.0/0248-plan.md`.

**Done when.** Every operation is tested on the toy host object, in both narrow and wide
templates where text is involved. The `bench/count.py` gate shows no change on the native
render path. ASan+UBSan is clean.
