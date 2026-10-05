---
status: open
priority: low
area: perf
touches: [src/template_impl.h, src/template_parser.cpp, src/template_parser.h, src/lexer.cpp]
---
# Load has a fixed cost that inja does not

**Problem.** 0120's comparison (bench/README.md "Other C++ engines", master 698f881)
shows inja parsing small templates two to four times faster than Jinja2C++, while
Jinja2C++ is ahead on every render:

| Benchmark | Jinja2C++ | inja |
|---|---:|---:|
| `Load/plain_text` | 942 ns | 249 ns |
| `Load/substitute` | 2.32 µs | 907 ns |
| `Load/mitsuhiko_table` | 14.4 µs | 8.97 µs |
| `Load/large_static` | 67.1 µs | 147 µs |

The gap shrinks as templates grow and reverses on `large_static`, so most of it is a
fixed cost per `Load`: `Load/plain_text` takes 9,600 instructions and 18 allocations for a
template with no tags (`count.py`). Candidates from a first look: `MakeDelimiters` and the
lexer's settings rebuilt per Load, the template name and source copied into several
owners, error-reporting tables.

**Proposal.** Profile `Load/plain_text` and `Load/substitute` with callgrind
(bench/README.md "Profiling"), move per-environment work (delimiter regexes, keyword
tables) to the environment or to statics, and drop the copies.

**Done when.** `Load/plain_text` takes under half today's instructions and allocations,
with no regression on the other `Load/*` cases.
