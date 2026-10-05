# Benchmarks

A Google Benchmark suite that runs the same workloads on Jinja2C++ and on Python Jinja2,
so every number has a reference point (docs/tasks/0011).

## Cases

Each directory in `cases/` is one workload:

- `main.j2` is the template;
- `data.json` (optional) holds the render parameters;
- any other `*.j2` file is available to `main.j2` through `include`, `import` and
  `extends` (an in-memory filesystem in C++, a `DictLoader` in Python).
- `settings.json` (optional) sets environment options: `trim_blocks`, `lstrip_blocks`
  and `autoescape`, as in Python's `Environment`, and `"wide": true`, which runs the case
  with `TemplateW` and `std::wstring` data in C++ (Python renders it as usual; the output
  is compared as UTF-8).

The cases fall into two groups. The synthetic ones isolate one engine feature each
(`substitute`, `for_range`, `filters`, `macros`, ...). The realistic ones are what people
render (docs/tasks/0119):

| Case | What it is |
|---|---|
| `chat_llama`, `chat_qwen`, `chat_mistral` | LLM chat templates in the style of Llama 3.1, Qwen 2.5 (ChatML) and Mistral v3, with tool definitions and tool calls (`tojson`, `trim`, slicing, `namespace`, `loop.index0` lookbehind), over a 23-message conversation; `trim_blocks` and `lstrip_blocks` as Hugging Face sets them |
| `html_autoescape` | a shop page with `autoescape` on: escaped user text, `safe`, `e`, imported macros for form fields |
| `config_file` | an nginx-style config built from many small `include`s and imported macros, `dictsort`, `trim_blocks` |
| `mitsuhiko_table_wide` | `mitsuhiko_table` with `TemplateW`, so the two compare narrow and wide rendering |

Real Hugging Face templates run in an environment whose `tojson` neither sorts keys nor
escapes HTML characters; these cases use Jinja2's own `tojson`, which both engines share.

Each case gives two benchmarks: `Load/<case>` parses `main.j2`, `Render/<case>` renders
the parsed template to a string. To add a workload, add a directory; both drivers pick it
up. A case must render the same text with both engines, which `run.py` checks before it
measures anything.

## Running

```bash
cmake -S . -B build-rel -G Ninja -DCMAKE_BUILD_TYPE=Release -DJINJA2CPP_BUILD_BENCHMARKS=ON
cmake --build build-rel --target jinja2cpp_bench
python3 bench/run.py --bench build-rel/bench/jinja2cpp_bench --out results.json
```

`run.py` prints a Markdown table: the Jinja2C++ median over `--repetitions` runs, its
coefficient of variation, the Python Jinja2 time and the speedup (Python time / C++
time; above 1 means Jinja2C++ is faster). With `--baseline old-results.json` it adds the
change against an earlier run and exits with status 2 when a benchmark got slower than
`--threshold` (10% by default). `--filter REGEX` limits the run, for example
`--filter 'Render/mitsuhiko'`.

The drivers can also run alone: `jinja2cpp_bench` takes the usual Google Benchmark flags
plus `--cases-dir` and `--dump-dir` (write every rendered output and exit), and
`python_bench.py` prints Google Benchmark style JSON.

### Where the realistic cases stand

Render only, on master cf927ef plus 0119, in a 4-core cloud container (one `run.py` run;
wall-clock numbers there move by 10-30% between runs, see Noise below):

| Case | Jinja2C++ | Python Jinja2 | Speedup | Instructions | Allocations |
|---|---:|---:|---:|---:|---:|
| `chat_llama` | 125 µs | 238 µs | 1.9x | 980 k | 980 |
| `chat_mistral` | 136 µs | 365 µs | 2.7x | 1.21 M | 1,046 |
| `chat_qwen` | 66.5 µs | 222 µs | 3.3x | 720 k | 848 |
| `html_autoescape` | 397 µs | 1.62 ms | 4.1x | 3.26 M | 2,998 |
| `config_file` | 321 µs | 1.84 ms | 5.7x | 2.80 M | 2,713 |
| `mitsuhiko_table` | 790 µs | 1.66 ms | 2.1x | 9.63 M | 3,030 |
| `mitsuhiko_table_wide` | 989 µs | 1.83 ms | 1.9x | 10.6 M | 3,051 |

The chat templates spend 27-51% of their render in `tojson` and up to 23% in `trim`, and
`html_autoescape` 18% in escaping: string scanners that the synthetic cases barely touch
(docs/tasks/0124). The wide table costs 10% more instructions and twice the bytes of the
narrow one.

## Other C++ engines

`engines_bench` runs the same cases on [inja](https://github.com/pantor/inja) (v3.5.0) and
[minja](https://github.com/ochafik/minja) (the Jinja subset written for LLM chat templates,
used by llama.cpp; the author's maintained fork), the engines an embedder would weigh
against Jinja2C++ (docs/tasks/0120). It is off by default; enabling it fetches both
(header-only, pinned commits):

```bash
cmake -S . -B build-rel -G Ninja -DCMAKE_BUILD_TYPE=Release -DJINJA2CPP_BUILD_BENCHMARKS=ON \
  -DJINJA2CPP_BENCH_WITH_OTHER_ENGINES=ON
cmake --build build-rel --target jinja2cpp_bench engines_bench
python3 bench/run.py --bench build-rel/bench/jinja2cpp_bench --engines build-rel/bench/engines_bench
```

Each engine gets the data in its own form, built once outside the timed loop, as
`jinja2cpp_bench` builds its `ValuesMap` once: inja a `nlohmann::json`, minja a
`minja::Value` (with keys sorted, so its `tojson` orders keys as Jinja2's does). `run.py`
then keeps only the cases where an engine renders exactly what Python Jinja2 renders, with
one allowance: Jinja2's `tojson` escapes `<`, `>`, `&` and `'` for HTML and minja's does not
(neither does Hugging Face's), so those four escapes are undone before comparing. A case a
driver cannot run (an error, or a setting the engine lacks) is skipped and the reason goes to
`<dump-dir>/<engine>/<case>.err` with `engines_bench --dump-dir`. inja's dialect differs
from Jinja's (functions instead of filters, double-quoted strings, no tuple unpacking), so a
case may carry `main.inja`, the same template written for inja; `mitsuhiko_table` does.

### Where the engines stand

Median of three runs on master 698f881, Release, GCC 13, in a 4-core cloud container (expect
10-30% noise). In parentheses: how many times faster Jinja2C++ is.

| Benchmark | Jinja2C++ | Python Jinja2 | inja | minja |
|---|---:|---:|---:|---:|
| `Render/chat_llama` | 63.3 µs | 246 µs (3.9x) | | 174 µs (2.7x) |
| `Render/chat_mistral` | 102 µs | 339 µs (3.3x) | | 310 µs (3.0x) |
| `Render/chat_qwen` | 52.8 µs | 187 µs (3.5x) | | 156 µs (3.0x) |
| `Render/dict_ops` | 40.1 µs | 76.6 µs (1.9x) | | 356 µs (8.9x) |
| `Render/for_filter_if` | 51 µs | 262 µs (5.1x) | | 251 µs (4.9x) |
| `Render/for_loop_vars` | 74.2 µs | 250 µs (3.4x) | | 348 µs (4.7x) |
| `Render/for_range` | 4.47 µs | 28.3 µs (6.3x) | 23.8 µs (5.3x) | 131 µs (29x) |
| `Render/large_static` | 3.26 µs | 13.7 µs (4.2x) | 9.93 µs (3.0x) | 14.3 µs (4.4x) |
| `Render/macros` | 117 µs | 796 µs (6.8x) | | 770 µs (6.6x) |
| `Render/many_tags` | 167 µs | 642 µs (3.8x) | | 5.19 ms (31x) |
| `Render/mitsuhiko_table` | 715 µs | 1.37 ms (1.9x) | 5.2 ms (7.3x) | 18.5 ms (26x) |
| `Render/plain_text` | 224 ns | 7.97 µs (36x) | 411 ns (1.8x) | 256 ns (1.1x) |
| `Render/substitute` | 418 ns | 8.71 µs (21x) | 762 ns (1.8x) | 812 ns (1.9x) |
| `Load/chat_llama` | 140 µs | 10.9 ms (78x) | | 477 µs (3.4x) |
| `Load/many_tags` | 2.69 ms | 286 ms (106x) | | 9.38 ms (3.5x) |
| `Load/mitsuhiko_table` | 14.4 µs | 1.71 ms (119x) | 8.97 µs (0.62x) | 88.7 µs (6.2x) |
| `Load/large_static` | 67.1 µs | 6.91 ms (103x) | 147 µs (2.2x) | 1.3 ms (19x) |
| `Load/plain_text` | 942 ns | 191 µs (203x) | 249 ns (0.26x) | 1.34 µs (1.4x) |
| `Load/substitute` | 2.32 µs | 447 µs (193x) | 907 ns (0.39x) | 8.24 µs (3.6x) |

Jinja2C++ renders every shared case fastest, by 1.1x to 31x. inja parses small templates
two to four times faster than Jinja2C++ (a fixed cost per `Load`, docs/tasks/0131) and
renders only five cases, none with filters: its dialect is too far from Jinja's for the
rest, and with `nlohmann::ordered_json` as its data type v3.5.0 crashes in `range`.
minja renders 13 of the 20 cases, including all three chat templates; it has no `include`,
`import` or `extends`, no `is even` test and no `title` or `format` filter. It falls far
behind on loops that write many small values (26-31x on `mitsuhiko_table`, `many_tags` and
`for_range`), so it is close to Jinja2C++ only on the chat templates it was written for.

## Instruction counts

Timings on shared machines are noisy; instruction counts are not. `count.py` runs each
benchmark under callgrind, collecting only the measured loop, and reports instructions
per iteration, repeatable to about 0.05% for the same binary:

```bash
python3 bench/count.py --bench build-rel/bench/jinja2cpp_bench --out before.json
# ... change the code, rebuild ...
python3 bench/count.py --bench build-rel/bench/jinja2cpp_bench --baseline before.json
```

With `--baseline` it exits with status 2 when a benchmark costs more than `--threshold`
(3% by default) over the baseline. The whole suite takes about 10 seconds on 4 cores.
The `instructions` job of `.github/workflows/benchmark.yml` does this on every pull
request that touches the engine, building the base commit and the PR head on the same
runner. Instructions are not time (cache misses and branch mispredictions do not show), so
confirm a real improvement with `run.py` as well.

The driver also replaces the global `operator new` and reports heap allocations and
bytes per iteration; `count.py` prints them next to the instructions (with the change
against a baseline that has them) but does not gate on them. One benchmark by hand:

```bash
build-rel/bench/jinja2cpp_bench --count=Render/mitsuhiko_table --count-iters=5
# allocations 4053 bytes 1196331
```

### Memory

After the counted iterations, `--count` runs one more iteration with the allocator
following how many heap bytes are held (malloc's usable size of each block, so what a block
really occupies) and prints `memory retained <n> peak <n>`; `count.py` shows them as the
`Retained` and `Peak` columns (docs/tasks/0121):

- `Load/<case>`: `Retained` is what the loaded template keeps, the figure that matters
  to a cache of many templates; `Peak` adds the parser's transient memory.
- `Render/<case>`: `Peak` is the most a render holds at once, including the output string
  as it grows; `Retained` is what a render leaves behind after its output is freed, and
  anything above 0 is a leak or a cache that grows per render.

Like the counts, both are repeatable for the same binary. They are glibc's figures on
Linux (`_msize` on Windows, `malloc_size` on macOS), so compare runs on one platform only.
The trend charts show `Retained` for `Load/*` and `Peak` for `Render/*`.

On master 698f881, a tag-heavy template keeps 11 to 30 times its source once loaded (static
text costs little: `large_static` keeps 1.4 times): 39 KB of `many_tags`
(2,400 tags) becomes 1.14 MB, about 470 bytes per tag, and the 3.5 KB `chat_llama` 38 KB
(docs/tasks/0130). Render peaks follow the output: `mitsuhiko_table` peaks at 347 KB for a
344 KB output and its wide twin at 1.38 MB (four-byte `wchar_t`, plus the old buffer while the
string grows), while `dict_ops` peaks at 35 KB for 1.8 KB of output.

The bookkeeping costs a few instructions per allocation in every `--count` run: introducing
it moved the counts by +0.0% to +1.1% (`Load/plain_text`) at once, one step in the trend.

`--data=reflect` (on both `jinja2cpp_bench` and `count.py`) passes each case's `data.json`
through the nlohmann JSON binding (`jinja2::Reflect`) instead of converting it to a
`ValuesMap`, so every lookup goes through a user `IMapItemAccessor`. Wide cases still
convert. Compare a reflect run only with another reflect run.

## Trend

The `trend` job of `.github/workflows/benchmark.yml` runs `count.py` on every push to
master that touches the engine and appends the counts to `history.jsonl` on the
[`bench-data`](https://github.com/jinja2cpp/Jinja2Cpp/tree/bench-data) branch, one record
per commit. `bench/trend.py` then rewrites that branch's README (latest counts against
the previous and the first record) and one SVG chart per benchmark with instructions,
allocations and memory per iteration, so a merged change shows up as a step. To draw it locally:

```bash
git fetch origin bench-data && git show origin/bench-data:history.jsonl > history.jsonl
python3 bench/trend.py render --history history.jsonl --out trend
```

The PR gate compares a PR with its base only, so several merges that each add 2% all
pass it. After each trend update, `trend.py drift` compares the latest record with the
one ten records back. When a count grew by more than 3% (allocations: also by more than
two), the job comments on the open issue "Benchmark drift on master", or opens it.
A case reported once is not reported again on the next merges while it stays above the
threshold. Close the issue once the drift is fixed or accepted. Locally:
`python3 bench/trend.py drift --history history.jsonl --window 10 --threshold 0.03`.

## Profiling

The benchmark binary is a convenient profiling harness, since a filter isolates one
workload:

```bash
valgrind --tool=callgrind build-rel/bench/jinja2cpp_bench \
  --benchmark_filter='^Render/mitsuhiko_table$' --benchmark_min_time=30x
callgrind_annotate --inclusive=yes callgrind.out.<pid> | less
```

### gperftools

Sampling CPU profiles and heap profiles come from
[gperftools](https://github.com/gperftools/gperftools) (`apt install libgoogle-perftools-dev`,
which brings `google-pprof`). Build the driver against it in a build directory of its
own: tcmalloc replaces `operator new`, so this build has no allocation counts.

```bash
cmake -S . -B build-gperf -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_FLAGS='-g -fno-omit-frame-pointer' \
  -DJINJA2CPP_BUILD_BENCHMARKS=ON -DJINJA2CPP_BENCH_WITH_GPERFTOOLS=ON
cmake --build build-gperf --target jinja2cpp_bench
B=build-gperf/bench/jinja2cpp_bench

# CPU: samples only the measured loop (100 Hz; CPUPROFILE_FREQUENCY=1000 for short runs)
$B --count=Render/mitsuhiko_table --count-iters=2000 --cpu-profile=cpu.prof
google-pprof --text $B cpu.prof | head -30

# Heap: every allocation in the measured loop, by call site
$B --count=Load/many_tags --count-iters=20 --heap-profile=heap
google-pprof --text --alloc_objects --lines $B heap.0001.heap | head -30
google-pprof --text --alloc_objects --cum --focus=realloc_insert $B heap.0001.heap
```

`--alloc_space` ranks by bytes instead of calls, `--web`/`--svg` draw the call graph.

### Threads

`--threads` adds `MT/Render/<case>` benchmarks that render one shared, loaded template
from 1, 2, 4, ... threads (up to the core count) and report renders per second
(`items_per_second`). Linear scaling doubles it with each step; a flat line means the
threads contend on something shared. Run them on an otherwise idle machine.

```bash
build-rel/bench/jinja2cpp_bench --threads --benchmark_filter='^MT/'
```

## Noise

Timings on shared machines (cloud containers, GitHub runners) vary by 5-15% between
runs; check the CV column before reading a small difference as a change, or compare
instruction counts instead.
