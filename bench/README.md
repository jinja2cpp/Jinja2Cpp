# Benchmarks

A Google Benchmark suite that runs the same workloads on Jinja2C++ and on Python Jinja2,
so every number has a reference point (docs/tasks/0011).

## Cases

Each directory in `cases/` is one workload:

- `main.j2` is the template;
- `data.json` (optional) holds the render parameters;
- any other `*.j2` file is available to `main.j2` through `include`, `import` and
  `extends` (an in-memory filesystem in C++, a `DictLoader` in Python).

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

## Trend

The `trend` job of `.github/workflows/benchmark.yml` runs `count.py` on every push to
master that touches the engine and appends the counts to `history.jsonl` on the
[`bench-data`](https://github.com/jinja2cpp/Jinja2Cpp/tree/bench-data) branch, one record
per commit. `bench/trend.py` then rewrites that branch's README (latest counts against
the previous and the first record) and one SVG chart per benchmark with instructions and
allocations per iteration, so a merged change shows up as a step. To draw it locally:

```bash
git fetch origin bench-data && git show origin/bench-data:history.jsonl > history.jsonl
python3 bench/trend.py render --history history.jsonl --out trend
```

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
