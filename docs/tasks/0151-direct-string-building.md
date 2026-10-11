---
status: done
priority: low
area: perf
depends: [0140]
touches: [src/internal_value.h, src/internal_value.cpp, src/value_visitors.h, src/string_builder.h, src/string_converter_filter.cpp, src/serialize_filters.cpp, src/filters.cpp, src/markup.h, src/expression_evaluator.cpp, src/template_parser.h, src/template_parser.cpp]
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

**Targets** (Render allocations per render, CI count job on #462 at 60b4b7e vs ea60b00; the perf
track accepted P2's rise as owed here). Before P2 → after P2, target ≤ the first number:

| case | before | after P2 |
|---|--:|--:|
| strings | 975 | 1,285 |
| config_file | 1,120 | 1,281 |
| html_autoescape | 932 | 1,094 |
| chat_mistral | 416 | 491 |
| chat_llama | 350 | 421 |
| chat_qwen | 318 | 395 |
| filters | 35 | 51 |

Load allocations rose too (chat_llama 59 → 76, chat_qwen 45 → 57, mitsuhiko_table_wide 45 → 51),
with bytes, retained and peak all lower; bring them back where the same fix reaches them.

## Result

A long string's characters now follow its `StringObject` in the same allocation
(`StringObject::Make`), and producers write them there once: `InternalValue::MakeText(size,
writer)` when the size is known, `InternalValue::FromText(parts)` for a copy of views, and
`StringBuilder` (`src/string_builder.h`, an `fmt::basic_memory_buffer` with 500 characters on
the stack) otherwise. Visitors see the text of a heap string as a `std::basic_string_view` of
the object; `IntValue2Value` copies it into the `Value`. Converted producers: `join` (it also
stops rebuilding the result per item, which was quadratic), `escape`/`forceescape`, `~` on two
strings, `+` and `*` on strings, slices and indexing, `trim`, `title`, `upper`/`lower`/
`capitalize`, `truncate`, `tojson`, `Value` strings converted by value, and string literals at
load (escaped ones on the stack, in `template_parser.cpp`). `join` of an empty sequence is now
`''` as in Python (it was undefined; parity case `filters.join_empty`).

Allocations per render or load, local count job (ce547f1 → this change):

| case | target | before | after |
|---|--:|--:|--:|
| Render/strings | 975 | 1,285 | 449 |
| Render/config_file | 1,120 | 1,281 | 1,071 |
| Render/html_autoescape | 932 | 1,094 | 748 |
| Render/chat_mistral | 416 | 491 | 360 |
| Render/chat_llama | 350 | 421 | 262 |
| Render/chat_qwen | 318 | 395 | 249 |
| Render/filters | 35 | 51 | 29 |
| Load/chat_llama | 59 | 76 | 59 |
| Load/chat_qwen | 45 | 57 | 45 |
| Load/mitsuhiko_table_wide | 45 | 51 | 45 |

Left as they were (two allocations for a long result): `indent`, `urlize`, `center`,
`wordwrap`, `striptags`, `replace`, `format`, `~` with a non-string operand and str() of a
non-string, which still build a `std::basic_string`. None shows in the suite's counts.

Traps: `GetAsTargetString`-style code paths that take a visited view and make a value of it
must copy (`InternalValue::FromText`): `InternalValue(std::string_view)` borrows. Inlining in
`template.cpp`, which compiles parser and renderer together, moved with parser edits:
`ApplyNewlineSequence` is now out of line and `~WorkspaceMaps` forced inline to hold Load and
Render/plain_text at base.
