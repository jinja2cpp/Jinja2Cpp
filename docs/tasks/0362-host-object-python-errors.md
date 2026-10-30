---
status: open
priority: low
area: parity
depends: [0352]
touches: [src/filters.cpp#SequenceAccessor, src/testers.cpp, src/string_converter_filter.cpp#ApplyUrlEncode, src/expression_evaluator.cpp#SubscriptExpression]
shares: [src/host_object.h, test/host_object_test.cpp]
---
# Host objects: Python's errors where generic filters guess

**Problem.** The 0352 verifier compared about 560 templates on a toy host object
(`test/host_object_test.cpp`) with Python Jinja2 3.1.6 and an equivalent Python class
(`/mnt/project-files/0352/oracle_toy.py`). The seam itself agreed; these generic paths
answer where Python raises or asks the object:

- `x[1:2]` on a host object renders empty. Python passes a `slice` to `__getitem__`
  (KeyError or TypeError for most objects, a sublist for a sequence).
- `it|last` on an iterable host returns the last item. Python's `last` calls
  `reversed()`, which needs `__reversed__` or a sequence: TypeError for a plain iterator.
- `x is odd`, `x is even` on a host are False. Python computes `x % 2` and raises
  TypeError for an object without `__mod__`.
- `x|urlencode` on a host encodes `str(x)`. Python's `do_urlencode` iterates a mapping or
  an iterable and raises TypeError otherwise.
- `{% call j() %}` on an engine object with a call operator (`joiner()`) fails with
  "'joiner' object is not callable"; Python calls it with `caller=`. (Host objects take
  the call since 0352.)

**Proposal.** Give `IHostObject` a slice entry (or a `GetItem` with a slice value) and
route the other cases through the object's operations, raising Python's TypeError where
the object lacks one. Add rows to the host-object tests and to the parity corpus where an
engine object shows the same gap.

**Done when.** Each bullet renders or fails as Python does on the toy object, narrow and
wide.
