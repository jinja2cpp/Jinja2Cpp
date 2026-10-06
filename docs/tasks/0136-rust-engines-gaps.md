---
status: open
priority: medium
area: perf
depends: [0135]
touches: [src/]
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
