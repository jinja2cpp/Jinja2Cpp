---
status: open
priority: medium
area: perf
touches: [src/serialize_filters.cpp#PythonJsonWriter, src/markup.h#EscapeHtml, src/string_converter_filter.cpp#PythonStrip]
---
# `tojson`, HTML escaping and `trim` walk strings one character at a time

**Problem.** The realistic workloads of 0119 spend most of their render in three string
scanners, none of which shows in the synthetic cases (callgrind, Release, master cf927ef
plus 0119's cases):
- `tojson` (`PythonJsonWriter`): 45% of `Render/chat_llama`, 51% of `Render/chat_qwen`,
  27% of `Render/chat_mistral`. `WriteString` splits the string into a vector of code
  points (`SplitCodePoints`, one allocation that grows with the string), decodes each one
  and appends it with `push_back`; plain ASCII text, nearly all of a chat message, pays
  the full path per byte.
- `trim` (`PythonStrip`): 23% of `Render/chat_llama`, 11% of `Render/chat_mistral`. It
  splits the whole string into code points to look at its two ends, then copies the
  middle back piece by piece. Chat templates trim every message.
- autoescape (`EscapeHtml` under `MarkupEscape`): 18% of `Render/html_autoescape`, a
  `switch` and `push_back` per character.
Python does each of these in C over the whole string (`json.encoder`'s
`ESCAPE_ASCII.sub`, `str.strip`, markupsafe's `_speedups`), which is why the chat cases
are among the closest to Python (1.1-3.3x over three noisy runs, against 2-7x for most synthetic
cases).

**Proposal.** Scan for the next byte that needs work (`find_first_of`, or a 256-entry
table) and append the run before it with one `append`; decode code points only around
non-ASCII bytes (`tojson` escapes those as `\uXXXX`; `trim` only needs them at the ends,
where Unicode whitespace can be). `trim` returns `str.substr(first, last - first)` once it
has found both ends. Wide strings get the same treatment for code units below 0x80.
Check the `tojson`, `trim` and escaping corpus cases stay unchanged.

**Done when.** `Render/chat_llama` and `Render/chat_qwen` instructions drop by at least
30% and `Render/html_autoescape` by 10% (`bench/count.py --baseline`), no other case
slower.
