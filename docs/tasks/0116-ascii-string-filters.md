---
status: open
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
