---
status: in-progress
priority: medium
area: api
depends: [0357]
touches: [src/public_host_object.cpp, python/src/adapters.cpp, python/src/adapters.h]
shares: [include/jinja2cpp/host_object.h]
---
# A host sequence with its own iteration is iterated by index

**Problem.** The public host-object contract (`jinja2cpp/host_object.h`) iterates an
`IsSequence` object that has a `Len` by index, through `ItemAtTo`, so that a loop sees the
length change live as Python's list iterator does (`HostSequenceList`). A Python list subclass
that overrides `__iter__` is a sequence (sorting, comparisons, `tojson` read its items), but
Python iterates it through its `__iter__`. Since 0357 S3 the module goes through the public
contract, and the loop reads the subclass by index:

    class L(list):
        def __iter__(self): return iter(["x", "y"])
    Template("{% for i in l %}{{ i }}{% endfor %}").render(l=L([1, 2, 3]))

gives `xy` in Jinja2 and on master before S3, and `123` through the module now.

**Proposal.** Let the host say whether it may be iterated by index: a virtual on
`IHostObject` (for example `IteratesByIndex()`, true by default for a sequence) checked where
`HostBridge::Iterate` picks `HostSequenceList`. `PyHost` answers false for a list or tuple
subclass whose `tp_iter` is not the base type's. The change adds a vtable slot, so it lands
before the G1 freeze or goes through `IHostObject2`.

**Done when.** The probe gives `xy`, with a row in `python/tests` and one in
`test/public_host_object_test.cpp` for a C++ host that iterates by its own `Iterate`.

**Progress (2026-11-10).** `IHostObject::IteratesByIndex()` (true by default) is checked where
`HostBridge::Iterate` picks `HostSequenceList`; `PyHost` answers false for a list or tuple
subclass whose `tp_iter` slot is not the base type's (read with `PyType_GetSlot`, as the module
builds on the limited API). The probe gives `xy`, as Jinja2 does; items by index (`l[1]`), `len`
and `loop.length` stay the list's own, also as in Jinja2. Rows: `python/tests/test_values.py`
(list and tuple subclass, a plain subclass) and
`PublicHostObjectTest.SequenceThatIteratesByItsOwnIterate`. Decision record:
docs/api-2.0.md section 7 item 10.
