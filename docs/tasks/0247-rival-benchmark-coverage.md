---
status: open
priority: high
area: perf
touches: [bench/cases/, bench/rust/, bench/engines_bench.cpp]
---
# Cover the rival engines' own benchmark cases, then close the gaps

**Problem.** `bench/cases/` (20 cases) was written by this project. Rivals ship their own
suites: MiniJinja's `benches/` and `benchmarks/`, Tera's `benches/`, inja's and minja's
benchmarks and their chat-template tests, and Jinja2's historical `examples/bench.py`
(our `mitsuhiko_table`). A reader of the comparison report (0246) will look for the cases
those projects publish. We do not know if we cover them, and perf work after 0154/0158 has
no defined end (0164, 0117 P4).

**Proposal.**
1. Inventory each rival's benchmark suite at its current release. Add every case that
   Jinja2 can render to `bench/cases/` with its data (licenses permitting; record origin),
   plus Python and engine drivers. Note cases that only one engine can express.
2. Add verbatim, licensed LLM chat templates from model repositories next to the style-based
   chat cases, with the host globals they expect (0227 asks for this distinction).
3. **Perf round before 2.0, with an exit criterion:** for every case, Jinja2C++ Render is at
   least on par with the fastest rival within noise, and Load within 1.1x; or the task
   records why not (a design limit, or a rival trading correctness). Gaps become phases
   here or in 0164/0117. Perf beyond this criterion is not a 2.0 condition
   (docs/release-2.0.md section 5).

**Done when.** The inventory is in `bench/README.md` with origins, the cases run in
`bench/run.py`, and the exit criterion holds or each exception is recorded.
