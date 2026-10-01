---
status: open
priority: medium
area: parity
depends: [0001]
touches: [include/jinja2cpp/value.h, src/internal_value.cpp, src/internal_value.h, src/serialize_filters.cpp, test/parity/]
---
# Mappings keep insertion order

**Problem.** Python dicts keep insertion order, and templates rely on it when iterating
a mapping or serialising it (`tojson`, `pprint`). `jinja2::ValuesMap` derives from
`std::unordered_map`, so the order is a hash order that can change between standard
libraries and versions: output is not reproducible across platforms (3 cases, more
hidden behind 0012 and 0013).

**Proposal.** Public API change: an architect plan should weigh replacing the base of
`ValuesMap` with an insertion-ordered map (ABI and source break for users who use
`unordered_map` members) against keeping `ValuesMap` and ordering only dict literals and
internal maps. Python's `tojson` sorts keys, so `tojson` can be fixed independently by
sorting.

**Done when.** No line of `test/parity/divergences.txt` names task 0031, and `ctest -R parity` passes.
