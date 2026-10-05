---
status: open
priority: medium
area: perf
touches: [src/python_format.cpp, src/python_format.h, src/serialize_filters.cpp#StringFormat, src/value_visitors.h#FormatPythonFloat]
---
# Lean on fmt for string building: `format`, `%`, numbers, concatenation

**Problem.** `{fmt}` is already a dependency, but most string building goes around it,
through temporary `std::string`s. On `Render/strings` (2.54x Python) the `format`
filter is 42% of instructions (master cf927ef):
- `StringFormat::Filter` copies the format string (`AsString(InternalValue(
  GetAsTargetString(...)))`), evaluates the arguments into a vector, wraps the vector in
  a `ListAdapter` tuple, and `PythonPercentFormat` then copies it back out with
  `ToValueList()` (17% of the case, plus `malloc`/`free` around it);
- the `%` spec is re-parsed on every call although the format string is almost always
  a literal; each conversion builds its own `std::string` (`FormatInteger` via
  `fmt::format("{}", ...)`) and appends it;
- `FormatPythonFloat` returns a new string per float; `~` concatenation, `join`,
  `trim`, `upper` each allocate their result.

The contradiction: Python's `%` and `str()` semantics are implemented by hand on top of
`std::string`, while the library that already does fast, allocation-free formatting
into a caller's buffer is only used for leaf calls. (`std::format` is not an option
while the floor is C++17; `{fmt}` is its superset and works on every standard we
build.)

**Proposal.**
1. Pass `%` arguments as a span of `InternalValue` (no adapter, no `ToValueList()`
   copy); keep the mapping path for keyword arguments.
2. When the format string is a constant, parse the `%` spec once at Load into a list
   of literal pieces and conversions; render by walking it.
3. Format into one `fmt::memory_buffer` (inline storage, so short results do not
   allocate) with `fmt::format_to` for numbers, then move it into the result once.
4. Same treatment for `FormatPythonFloat` and `~`: build into a buffer reserved from
   the operands' sizes.
5. Check each step against the parity corpus: `%` behaviour is pinned by 0106.

**Done when.** `Render/strings` instructions drop by at least 20%
(`bench/count.py --baseline`), parity corpus unchanged.

**Next.** A shared "format into the output buffer" path (0113) lets `{{ '%s' % x }}`
write straight into the render output with no intermediate string at all.
