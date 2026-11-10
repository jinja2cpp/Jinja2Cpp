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
`HostBridge::Iterate` picks `HostSequenceList`. `PyHost` answers true only for an exact `list`
or `tuple`: a subclass is iterated through `iter()`, which is its own `__iter__` when it has one,
and otherwise the base type's iterator, which (unlike `ItemAtTo`, which calls `__getitem__`) reads
the items without a subclass's `__getitem__`, as Python does. The probe gives `xy`. Rows:
`python/tests/test_values.py::test_list_and_tuple_subclasses_iterate_as_python_does` (contexts
built afresh per engine, since a deep copy of a list subclass is rebuilt from its `__iter__`) and
`PublicHostObjectTest.SequenceThatIteratesByItsOwnIterate`. Decision record: docs/api-2.0.md
section 7 item 10. `loop.length` and operators over such a subclass still differ: task 0452.
