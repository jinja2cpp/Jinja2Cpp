---
status: done
priority: low
area: perf
touches: [src/template_impl.h, src/template_parser.h, src/load_settings.h, src/template_env_impl.h, src/template_env.cpp, src/lexertk.h, test/basic_tests.cpp]
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

**Done** (PR #PRNUM, phase P1 of the 0118 arena plan). Measured with `bench/count.py`
against master faea865:

| Benchmark | Instructions before | after | Allocations before | after |
|---|---:|---:|---:|---:|
| `Load/plain_text` | 9,006 | 4,093 (-54.6%) | 16 | 8 |
| `Load/substitute` | 17,109 | 11,391 (-33.4%) | 27 | 15 |
| `Load/for_range` | 28,098 | 20,709 (-26.3%) | 37 | 20 |
| `Load/mitsuhiko_table` | 98,403 | 87,934 (-10.6%) | 95 | 72 |
| `Load/large_static` | 300,649 | 294,601 (-2.0%) | 193 | 177 |
| `Load/many_tags` | 16,485,462 | 16,587,852 (+0.6%) | 9,659 | 9,637 |

Every other `Load/*` case is 1-17% faster. What changed:
- The settings and the delimiters built from them (`MakeDelimiters`, 30% of plain_text) live in one
  immutable `detail::LoadSettings` (src/load_settings.h) that the environment shares with every template
  it makes while its settings stay the same (`TemplateEnvImpl::GetLoadSettings`, a compare under a mutex;
  the snapshot is rebuilt when the settings were changed, also through the `GetSettings()` reference).
  An environment with a `finalize` callable is the exception: a callable edited in place keeps its
  identity and would compare equal, so each of its templates gets settings of its own (the old cost).
  Templates no longer copy `Settings` (23% of plain_text, two allocations and an atomic increment for the
  `finalize` callable).
- A template keeps its environment handle inline instead of on the heap, and a handle's destructor no longer
  takes the environment's exclusive lock (`owner` is atomic; only the owner drops the caches).
- The parser refers to the template name instead of copying it; its line table keeps 8 lines inline,
  the block list reserves 16 entries, and the lexer buffers it reuses across tags start with room for 16
  tokens (`lexertk::generator::reserve`).

`Load/many_tags` +0.6% is glibc's heap layout, not the parser: with fewer small allocations the
destructor's frees now coalesce with the top chunk, and `malloc_consolidate` runs from `free` (11 calls
in 5 iterations, none before), +0.7% together with `unlink_chunk`. The parser's own functions are 0.1%
lower; with jemalloc preloaded the case is within ±0.1% of master. Keyword tables were already static
(`ParserTraitsBase::s_keywordsInfo`), so `src/lexer.cpp` needed no change. The source keeps one heap box
(`unique_ptr<basic_string>`) so that the tree's views survive moving it in; dropping that allocation needs
the arena (0118 P4).
