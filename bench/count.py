#!/usr/bin/env python3
"""Count instructions per benchmark iteration with callgrind: a deterministic gate.

Wall-clock times on shared machines vary by 5-15% between runs, too much to catch a 5%
regression. The number of instructions a benchmark executes does not depend on the
machine's load: under callgrind the same binary gives the same count to the last
instruction. For each benchmark, this script runs `jinja2cpp_bench --count=<name>`
under callgrind, collecting only CountedRegion() (the timed loop, after a warm-up
iteration), and divides the total by the iteration count. The driver also counts the
heap allocations (operator new calls) and bytes per iteration, which are just as
deterministic, and the memory one more iteration holds: `Retained` is what is still
allocated when it ends (Load: the loaded template's footprint; Render: what a render leaves
behind once its output is freed, normally 0) and `Peak` the most held at once. These are
reported next to the instructions but do not gate.

With --cache-sim, callgrind also simulates the caches (`--cache-sim=yes`) and a second table
gives the misses per iteration: D1 read and write misses, last-level data read misses and
I1 (instruction fetch) misses. The cache geometry is fixed (CACHE_MODEL), not taken from the
host, so runs on different machines compare. The misses are as repeatable as the
instructions for one binary, but the model is idealised (no prefetcher, no L2), so read
their relative change; they are reported and do not gate.

Usage: count.py --bench build-rel/bench/jinja2cpp_bench [--cases-dir DIR] [--out counts.json]
                [--baseline old-counts.json] [--threshold 0.03] [--filter REGEX]
                [--iterations 5] [--jobs 4] [--data reflect] [--cache-sim]
With --baseline, prints the change per benchmark and exits with 2 when one got more
expensive than --threshold. Needs valgrind.
"""
import argparse
import concurrent.futures
import json
import os
import pathlib
import re
import shutil
import subprocess
import sys
import tempfile

# Typical x86-64 L1 (32 KB, 8-way) and an 8 MB last-level cache, 64-byte lines
CACHE_MODEL = ["--I1=32768,8,64", "--D1=32768,8,64", "--LL=8388608,16,64"]
# The callgrind events reported, in table order
CACHE_EVENTS = ("D1mr", "D1mw", "DLmr", "I1mr")


def list_benchmarks(bench, cases_dir):
    out = subprocess.run([bench, f"--cases-dir={cases_dir}", "--benchmark_list_tests=true"], check=True, stdout=subprocess.PIPE,
                         text=True).stdout
    return [line.strip() for line in out.splitlines() if line.strip()]


def count(bench, cases_dir, name, iterations, data="convert", cache_sim=False):
    with tempfile.TemporaryDirectory() as tmp:
        out_file = pathlib.Path(tmp, "callgrind.out")
        cache_args = ["--cache-sim=yes", *CACHE_MODEL] if cache_sim else []
        # The arguments and the environment sit at the top of the stack, so their length shifts
        # every stack address, and with them the cache sets: running the same binary from a
        # longer path moved Render/macros' D1 misses by 80%. Short relative links in the
        # working directory and an empty environment make the layout the same for any build
        # directory and any machine.
        pathlib.Path(tmp, "b").symlink_to(pathlib.Path(bench).resolve())
        pathlib.Path(tmp, "c").symlink_to(pathlib.Path(cases_dir).resolve())
        proc = subprocess.run([shutil.which("valgrind") or "valgrind", "--tool=callgrind", "--collect-atstart=no", *cache_args,
                               "--toggle-collect=*CountedRegion*", f"--callgrind-out-file={out_file}",
                               "./b", "--cases-dir=c", f"--count={name}", f"--count-iters={iterations}", f"--data={data}"],
                              cwd=tmp, env={}, check=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
        text = out_file.read_text()
        m = re.search(r"^summary:\s+(\d+)", text, re.M)
        cache = None
        if cache_sim:
            events = re.search(r"^events:(.*)$", text, re.M).group(1).split()
            totals = dict(zip(events, map(int, re.search(r"^summary:(.*)$", text, re.M).group(1).split())))
            # The summary leaves out trailing events that are zero
            cache = {event: round(totals.get(event, 0) / iterations) for event in CACHE_EVENTS}
    allocs = re.search(r"^allocations (\d+) bytes (\d+)", proc.stdout, re.M)
    memory = re.search(r"^memory retained (-?\d+) peak (\d+)", proc.stdout, re.M)
    return (int(m.group(1)) / iterations, (int(allocs.group(1)), int(allocs.group(2))) if allocs else None,
            {"retained": int(memory.group(1)), "peak": int(memory.group(2))} if memory else None, cache)


def format_cells(current, base, keys=("count", "bytes")):
    if not current:
        return " | |"
    cells = []
    for key in keys:
        cell = f"{current[key]:,}"
        if base and base.get(key) != current[key]:
            cell += f" ({current[key] - base[key]:+,})"
        cells.append(cell)
    return f" {cells[0]} | {cells[1]} |"


def print_cache_table(names, cache, base_cache):
    """The misses per iteration, with the relative change against a baseline that has them."""
    print("\nCache misses per iteration (callgrind's cache model; reported, not gated):\n")
    print("| Benchmark | D1 read misses | D1 write misses | LL data read misses | I1 misses |")
    print("|---|---:|---:|---:|---:|")
    for name in names:
        current, base = cache.get(name), base_cache.get(name)
        if not current:
            continue
        cells = []
        for event in CACHE_EVENTS:
            cell = f"{current[event]:,}"
            before = (base or {}).get(event)
            if before is not None and before != current[event]:
                cell += f" ({current[event] / before - 1:+.1%})" if before else f" ({current[event] - before:+,})"
            cells.append(cell)
        print(f"| {name} | {' | '.join(cells)} |")


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bench", required=True, type=pathlib.Path)
    ap.add_argument("--cases-dir", type=pathlib.Path, default=pathlib.Path(__file__).resolve().parent / "cases")
    ap.add_argument("--out", type=pathlib.Path)
    ap.add_argument("--baseline", type=pathlib.Path)
    ap.add_argument("--threshold", type=float, default=0.03)
    ap.add_argument("--filter", default="")
    ap.add_argument("--iterations", type=int, default=5)
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    ap.add_argument("--data", choices=("convert", "reflect"), default="convert",
                    help="reflect: pass data.json through the nlohmann binding (jinja2cpp_bench --data)")
    ap.add_argument("--cache-sim", action="store_true",
                    help="also simulate the caches and report D1, LL and I1 misses per iteration (slower)")
    args = ap.parse_args()

    names = [n for n in list_benchmarks(args.bench, args.cases_dir) if re.search(args.filter, n)]
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        results = dict(zip(names, pool.map(
            lambda n: count(args.bench, args.cases_dir, n, args.iterations, args.data, args.cache_sim), names)))
    counts = {name: r[0] for name, r in results.items()}
    allocations = {name: {"count": r[1][0], "bytes": r[1][1]} for name, r in results.items() if r[1]}
    memory = {name: r[2] for name, r in results.items() if r[2]}
    cache = {name: r[3] for name, r in results.items() if r[3]}
    baseline = json.load(open(args.baseline)) if args.baseline else {}
    base = baseline.get("instructions", {})
    base_allocations = baseline.get("allocations", {})
    base_memory = baseline.get("memory", {})

    header, rule = "| Benchmark | Instructions |", "|---|---:|"
    if args.baseline:
        header += " Baseline | Change |"
        rule += "---:|---:|"
    if allocations:
        header += " Allocations | Bytes |"
        rule += "---:|---:|"
    if memory:
        header += " Retained | Peak |"
        rule += "---:|---:|"
    print(header)
    print(rule)
    regressions = []
    names = sorted(counts, key=lambda n: (n.split("/", 1)[1], n))
    for name in names:
        row = f"| {name} | {counts[name]:,.0f} |"
        if args.baseline:
            if name in base:
                change = counts[name] / base[name] - 1
                mark = " ⚠" if change > args.threshold else ""
                row += f" {base[name]:,.0f} | {change * 100:+.2f}%{mark} |"
                if change > args.threshold:
                    regressions.append((name, change))
            else:
                row += " | new |"
        if allocations:
            row += format_cells(allocations.get(name), base_allocations.get(name))
        if memory:
            row += format_cells(memory.get(name), base_memory.get(name), ("retained", "peak"))
        print(row)
    if cache:
        print_cache_table(names, cache, baseline.get("cache", {}))

    if args.out:
        result = {"instructions": counts, "allocations": allocations, "memory": memory}
        if cache:
            result["cache"] = cache
        json.dump(result, open(args.out, "w"), indent=1)
    if regressions:
        print(f"\nmore instructions than baseline by over {args.threshold * 100:.0f}%:", file=sys.stderr)
        for name, change in regressions:
            print(f"  {name}: {change * 100:+.2f}%", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
