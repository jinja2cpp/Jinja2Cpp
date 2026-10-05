#!/usr/bin/env python3
"""Count instructions per benchmark iteration with callgrind: a deterministic gate.

Wall-clock times on shared machines vary by 5-15% between runs, too much to catch a 5%
regression. The number of instructions a benchmark executes does not depend on the
machine's load: under callgrind the same binary gives the same count to the last
instruction. For each benchmark, this script runs `jinja2cpp_bench --count=<name>`
under callgrind, collecting only CountedRegion() (the timed loop, after a warm-up
iteration), and divides the total by the iteration count. The driver also counts the
heap allocations (operator new calls) and bytes per iteration, which are just as
deterministic; they are reported next to the instructions but do not gate.

Usage: count.py --bench build-rel/bench/jinja2cpp_bench [--cases-dir DIR] [--out counts.json]
                [--baseline old-counts.json] [--threshold 0.03] [--filter REGEX]
                [--iterations 5] [--jobs 4] [--data reflect]
With --baseline, prints the change per benchmark and exits with 2 when one got more
expensive than --threshold. Needs valgrind.
"""
import argparse
import concurrent.futures
import json
import os
import pathlib
import re
import subprocess
import sys
import tempfile


def list_benchmarks(bench, cases_dir):
    out = subprocess.run([bench, f"--cases-dir={cases_dir}", "--benchmark_list_tests=true"], check=True, stdout=subprocess.PIPE,
                         text=True).stdout
    return [line.strip() for line in out.splitlines() if line.strip()]


def count(bench, cases_dir, name, iterations, data="convert"):
    with tempfile.TemporaryDirectory() as tmp:
        out_file = pathlib.Path(tmp, "callgrind.out")
        proc = subprocess.run(["valgrind", "--tool=callgrind", "--collect-atstart=no",
                               "--toggle-collect=*CountedRegion*", f"--callgrind-out-file={out_file}",
                               bench, f"--cases-dir={cases_dir}", f"--count={name}", f"--count-iters={iterations}", f"--data={data}"],
                              check=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
        m = re.search(r"^summary:\s+(\d+)", out_file.read_text(), re.M)
    allocs = re.search(r"^allocations (\d+) bytes (\d+)", proc.stdout, re.M)
    return int(m.group(1)) / iterations, (int(allocs.group(1)), int(allocs.group(2))) if allocs else None


def format_allocations(current, base):
    if not current:
        return " | |"
    cells = []
    for key in ("count", "bytes"):
        cell = f"{current[key]:,}"
        if base and base.get(key) != current[key]:
            cell += f" ({current[key] - base[key]:+,})"
        cells.append(cell)
    return f" {cells[0]} | {cells[1]} |"


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
    args = ap.parse_args()

    names = [n for n in list_benchmarks(args.bench, args.cases_dir) if re.search(args.filter, n)]
    with concurrent.futures.ThreadPoolExecutor(args.jobs) as pool:
        results = dict(zip(names, pool.map(lambda n: count(args.bench, args.cases_dir, n, args.iterations, args.data), names)))
    counts = {name: r[0] for name, r in results.items()}
    allocations = {name: {"count": r[1][0], "bytes": r[1][1]} for name, r in results.items() if r[1]}
    baseline = json.load(open(args.baseline)) if args.baseline else {}
    base = baseline.get("instructions", {})
    base_allocations = baseline.get("allocations", {})

    header, rule = "| Benchmark | Instructions |", "|---|---:|"
    if args.baseline:
        header += " Baseline | Change |"
        rule += "---:|---:|"
    if allocations:
        header += " Allocations | Bytes |"
        rule += "---:|---:|"
    print(header)
    print(rule)
    regressions = []
    for name in sorted(counts, key=lambda n: (n.split("/", 1)[1], n)):
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
            row += format_allocations(allocations.get(name), base_allocations.get(name))
        print(row)

    if args.out:
        json.dump({"instructions": counts, "allocations": allocations}, open(args.out, "w"), indent=1)
    if regressions:
        print(f"\nmore instructions than baseline by over {args.threshold * 100:.0f}%:", file=sys.stderr)
        for name, change in regressions:
            print(f"  {name}: {change * 100:+.2f}%", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
