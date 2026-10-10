---
status: open
priority: low
area: perf
depends: [0140]
touches: [src/internal_value.h, src/string_converter_filter.cpp, src/serialize_filters.cpp]
---
# Strings built then moved into a value allocate twice

**Problem.** After 0140 P2 a string longer than 14 characters lives in a refcounted
`StringObject`. Moving a finished `std::string` into one costs an allocation for the object
on top of the string's own buffer. Copies become cheap, but creation gets dearer, and the
suite moves about 3x more strings than it copies (0140 plan §1.1).

**Proposal.** Producers that build strings (concatenation, string filters, `tojson`, macro
bodies) write into an `fmt::basic_memory_buffer`, then allocate one `StringObject` holding
the characters inline.

**Done when.** P2's allocation counts per case are back at or below master on html_autoescape,
strings and the chat templates, or the task is dropped because P2 already is.
