---
status: open
priority: low
area: style
depends: []
touches: [src/value.cpp, src/value_helpers.h, src/binding/, src/template_impl.h, test/binding/, CMakeLists.txt]
---
# Delete the dead `src/value.cpp` and `src/value_helpers.h` bodies

**Problem.** `src/value.cpp` and most of `src/value_helpers.h` have been wrapped in `#if 0`
since 1.x. They hold an old `GenericListIterator` and `Value` helpers that the
public `generic_list_iterator.h` and `value.h` replaced. `value.cpp` still compiles
into the library (as an empty translation unit); nothing includes `value_helpers.h` since
0064. The `#if 0` lines carry a `readability-avoid-unconditional-preprocessor-if` NOLINT
pointing here (0081).

Also unreferenced: the serializer half of `src/binding/` (`*_json_serializer.{h,cpp}`,
`ToJson`, `DocumentWrapper`). `tojson` has had its own writer since 0019; only
`template_impl.h` includes the boost header and only `test/binding/rapid_json_serializer_test.cpp`
calls one (found in 0083).

**Proposal.** Delete `src/value.cpp` and `src/value_helpers.h`, and drop `value.cpp`
from `CMakeLists.txt`. A build plus the full test suite prove nothing referenced the dead
code. Removing whole files needs the owner's go-ahead.

**Done when** no `#if 0` block remains in `src/` and the NOLINTs are gone.
