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
