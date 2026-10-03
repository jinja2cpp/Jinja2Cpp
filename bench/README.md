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

## Profiling

The benchmark binary is a convenient profiling harness, since a filter isolates one
workload:

```bash
valgrind --tool=callgrind build-rel/bench/jinja2cpp_bench \
  --benchmark_filter='^Render/mitsuhiko_table$' --benchmark_min_time=30x
callgrind_annotate --inclusive=yes callgrind.out.<pid> | less
```

## Noise

Timings on shared machines (cloud containers, GitHub runners) vary by 5-15% between
runs; check the CV column before reading a small difference as a change, or compare
instruction counts instead.
