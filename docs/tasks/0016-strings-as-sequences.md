---
status: done
priority: high
area: parity
depends: [0001]
touches: [src/internal_value.cpp#Subscript, src/internal_value.cpp#ConvertToList]
shares: [src/internal_value.cpp, src/internal_value.h, src/filters.cpp, src/testers.cpp]
---
# Strings behave as sequences

**Problem.** In Jinja2 a string is a sequence of characters. In Jinja2C++ `s|length`,
`s|first`, `s|reverse`, `s|wordwrap`, `s[-1]` and the `iterable`/`sequence` tests on
strings give empty or false, although iterating a string in `for` works (9 cases).

**Proposal.** Give string values the list protocol used by filters (size, index with
negative indices, reverse iteration) in one place in the value model rather than per
filter. Decide and document what a "character" is: Python counts code points, so UTF-8
strings need code-point iteration, not bytes.

**Done when.** No line of `test/parity/divergences.txt` names task 0016, and `ctest -R parity` passes.
