---
status: open
priority: medium
area: perf
touches: [src/internal_value.cpp#SliceVisitor, src/internal_value.cpp#ListConverter, src/internal_value.h#SplitCodePoints, src/filters.cpp#Reverse]
---
# Slicing a string builds a vector of every code point

**Problem.** Found re-profiling the MiniJinja gap on `Render/chat_mistral` (0136,
master 50dec81). `str[a:b]` (and string indexing, `reverse`, and iterating a string as a
list) first calls `SplitCodePoints`, which allocates a `std::vector<std::string_view>`
with one entry per character, then appends the chosen characters one at a time.
`chat_mistral` cuts the last character off each tool call's `tojson` output
(`out[:-1]`, about 150 characters): 6 slices per render cost 3.04M of 35.4M instructions
over 52 renders, **8.6%** of the render, about 9,700 instructions per slice. Python slices
by code point too, so the semantics stay; the work per character is the problem.

**Proposal.**
- When the string has no non-ASCII unit (narrow: no byte ≥ 0x80; wide: no surrogate on
  16-bit `wchar_t`, nothing to check on 32-bit) code points are units: compute the
  indices on `size()` and, for step 1, return one `substr`; other steps index directly.
  The check is one pass over the bytes, far cheaper than the split.
- Otherwise walk the string once to find the start and stop offsets for step 1, and keep
  the split only for other steps.
- Apply the same to string indexing (`str[i]`), `reverse` and `ListConverter::FromString`
  where they split.

**Done when.** `Render/chat_mistral` -6% instructions or better (`bench/count.py
--baseline`), with new rows in the string slice tests for ASCII, multi-byte UTF-8,
negative and out-of-range bounds, steps other than 1 and wide strings, all matching
Python.
