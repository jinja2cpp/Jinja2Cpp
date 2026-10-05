---
status: done
priority: low
area: perf
touches: [src/string_converter_filter.cpp]
---
# Case-mapping filters go through the Unicode path for ASCII text

**Problem.** `upper`, `lower`, `capitalize`, `title` go through
`StringConverter::MapChars` for every string: `Render/many_tags` 10%
(`StringConverter::Filter` 14% incl.), `Render/strings` 8%. Most template text is
ASCII, where a byte loop is enough; Python's `str.upper` has the same fast path.

**Proposal.** Check once whether the input is ASCII (word-at-a-time) and map bytes in
place in the result buffer; keep the current path otherwise. Wide strings likewise for
code units below 0x80.

**Done when.** `Render/many_tags` -5% instructions, the string filter parity cases
unchanged (0107 lists the known divergences).

**Done** in #399: `Render/many_tags` -9.0%, `Render/filters` -16.4%,
`Render/html_autoescape` -8.3%, `Render/strings` -3.9% instructions, no case slower.
UTF-8 strings need no ASCII check: a byte loop that maps only `A-Z`/`a-z` leaves the bytes
of multi-byte characters as they are. `title` takes the byte loop when the string is ASCII
and the code point path otherwise. Non-ASCII letters still keep their case (0048).
