---
status: done
priority: medium
area: perf
depends: [0135]
touches: [docs/tasks/, bench/README.md]
---
# Where MiniJinja and Tera are faster

**Problem.** The Rust engines added to the comparison in 0135 are the first ones that beat
Jinja2C++ on some shared cases (bench/README.md "Rust engines", median of three runs on
master faea865; the engine's time over Jinja2C++'s):

- `Load`: MiniJinja parses in 0.41-0.68x of our time on every case but `large_static`
  (`chat_llama` 67 µs against 142 µs, `many_tags` 1.64 ms against 3.37 ms). Its lexer and
  parser emit flat bytecode; we build a node tree with many small allocations (0130, 0131,
  0118).
- `Render`, MiniJinja: chat templates 0.75-0.94x (`chat_qwen` 57 µs against 76 µs),
  `config_file` 0.89x, `for_loop_vars` 0.75x. The chat templates spend most of their time in
  `tojson`, `trim`, slicing and string concatenation (0124).
- `Render`, Tera 2: `for_loop_vars` 0.42x, `inheritance` 0.62x, `for_filter_if` 0.67x,
  `plain_text` 0.53x (136 ns), `substitute` 0.45x (262 ns). Small loop bodies and the fixed
  cost of a render (context and scope setup, output stream, globals) cost us more than
  Tera's VM. Its `.tera` translations differ slightly (`loop.cycle` becomes an `if`,
  `for ... if` an `if` in the body), so profile ours before reading the whole gap as ours.

We stay ahead on expression- and output-heavy work (`mitsuhiko_table` 1.5-3.1x,
`expressions`, `for_range`, `macros`).

**Proposal.** Profile each gap with `bench/count.py` and callgrind (bench/README.md
"Profiling") and split what it finds into tasks, or fold it into the plans it belongs to:
the fixed render cost and small loop bodies into 0117 (name slots), Load into 0118 (arena)
and 0131, the chat templates into 0124's follow-ups.

**Done when.** Each gap above is either closed (the README table shows Jinja2C++ at or
below the faster engine) or explained in this file with the task that owns it.

**Result (re-measured on master 50dec81, after 0117 P1/P2, 0118 P3-P5b, 0124, 0131, 0139).**
Every gap is closed or explained with the task that owns it; bench/README.md "Where the
Rust engines stand" has the new table (median of three `run.py` runs; one other set of
three agreed within the 10-30% noise). Profiles: callgrind on `jinja2cpp_bench --count`,
instruction counts from `bench/count.py`.

| Gap on faea865 | Now (engine / Jinja2C++) | Verdict |
|---|---|---|
| Render `chat_llama` 0.89x, `chat_qwen` 0.75x (MiniJinja) | 1.0x, 0.91x (0.98-0.99x in the other set) | Closed within noise (0124, 0117 P1) |
| Render `chat_mistral` 0.94x | 0.99x (0.89x in the other set) | Explained: slicing a string splits it into a vector of code points, 8.6% of the render (`out[:-1]` on each tool call): 0153 |
| Render `config_file` 0.89x | 1.3x | Closed (0117 P2 macros in slots, 0124) |
| Render `for_loop_vars` MiniJinja 0.75x, Tera 0.42x | 2.1x, 1.1x | Closed (0117 P1-i `loop` attributes: 502 allocations to 2) |
| Render `plain_text` Tera 0.53x | 1.2x (94 ns against 115 ns) | Closed (0139) |
| Render `substitute` Tera 0.45x | 0.71x (270 ns against 192 ns) | Explained: of 2,672 instructions, about 1,000 build the parameter map (robin_hood inserts 598, its destructor 354, two `malloc`s); 0139 c) left this to 0117 P4 (N5c), which removes it and should bring the render to about 1,650 instructions, Tera's level. The name lookups that follow (166 instructions each) are 0148 |
| Render `inheritance` Tera 0.62x | 0.74x | Explained: `FindValueWithViews` 12.6% (an include in a loop walks the loop's view; phase 6 of 0118/0117 must bring the case to 202.7k instructions from 214.5k); attribute reads 23.1% and per-`include` setup 10.0%: 0154 |
| Render `for_filter_if` Tera 0.67x | 0.71x | Explained: attribute reads on user maps 31.3% (about 320 instructions each), the filtered-loop adapter 12.9%, an allocated adapter per map item (one per `users` item): 0154. Tera's translation filters with an `if` in the body, so part of the adapter cost is Jinja2's `loop.length` semantics |
| Load, MiniJinja 0.41-0.68x | 0.58-0.73x (`plain_text` 0.92x) | Explained, not closed: `chat_llama` (554,741 instructions) spends 18.4% in the rough tag split and 18.1% in the lexer (two scans: 0142), 6.7% relocating nodes at `Seal` (`many_tags` 10.8%; 0118 P5 makes nodes trivially copyable so Seal can copy), and 10.4% descending all eleven precedence levels for each operand (`many_tags` 11.1%, `substitute` 11.5%: 0152). Together about -25..-30%, which would put `chat_llama` near 0.8x of MiniJinja's time; re-measure after them |

Found on the way and filed:
- 0152: the expression parser's per-level descent and its large `ParseResult` (Load ~10%).
- 0153: string slicing and indexing build a vector of code points (`chat_mistral` 8.6%).
- 0154: per-item costs of loops over user data (attribute reads, map item adapters, the
  filtered-loop adapter, per-`include` lookup by name), and the bench driver's allocation
  and memory columns miss robin_hood's `std::malloc` blocks (`Render/substitute` reports 1
  allocation per render and makes 3).
