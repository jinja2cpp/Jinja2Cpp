---
status: open
priority: low
area: style
depends: []
touches: [src/value.cpp, src/value_helpers.h, src/filters.cpp, src/serialize_filters.cpp, src/string_converter_filter.cpp, CMakeLists.txt]
---
# Delete the dead `src/value.cpp` and `src/value_helpers.h` bodies

**Problem.** `src/value.cpp` and most of `src/value_helpers.h` have been wrapped in `#if 0`
since 1.x. They hold an old `GenericListIterator` and `Value` helpers that the
public `generic_list_iterator.h` and `value.h` replaced. They still compile into the
library, three filter sources include `value_helpers.h`, and the `#if 0` lines carry a
`readability-avoid-unconditional-preprocessor-if` NOLINT pointing here (0081).

**Proposal.** Delete `src/value.cpp`, remove it from `CMakeLists.txt`, and either delete
`value_helpers.h` or keep only what its three includers use. A build plus the full test
suite prove nothing referenced the dead code. Removing whole files needs the owner's go-ahead.

**Done when** no `#if 0` block remains in `src/` and the NOLINTs are gone.
