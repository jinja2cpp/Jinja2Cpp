#!/usr/bin/env python3
"""Run the benchmark suite with both engines and print a comparison table.

  1. Checks that jinja2cpp_bench and Python Jinja2 render every case to the same text
     (a benchmark that does different work on the two sides compares nothing).
  2. Runs jinja2cpp_bench (median of --repetitions) and bench/python_bench.py.
  3. Prints a Markdown table: Jinja2C++ time, Python Jinja2 time, and the speedup
     (Python / C++; above 1 means Jinja2C++ is faster). With --baseline, adds the
     change against an earlier results file and exits with 2 when a benchmark got
     slower than --threshold.

Usage: run.py --bench build-rel/bench/jinja2cpp_bench [--out results.json]
              [--baseline old.json] [--threshold 0.10] [--repetitions 5] [--no-python]
"""
import argparse
import json
import pathlib
import subprocess
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent


def check_outputs(bench, cases_dir):
    with tempfile.TemporaryDirectory() as tmp:
        cpp_dir, py_dir = pathlib.Path(tmp, "cpp"), pathlib.Path(tmp, "py")
        subprocess.run([bench, f"--cases-dir={cases_dir}", f"--dump-dir={cpp_dir}"], check=True)
        subprocess.run([sys.executable, HERE / "python_bench.py", "--cases-dir", cases_dir,
                        "--dump-dir", py_dir], check=True)
        bad = [p.stem for p in sorted(py_dir.glob("*.txt"))
               if not (cpp_dir / p.name).exists() or (cpp_dir / p.name).read_bytes() != p.read_bytes()]
    return bad


def run_cpp(bench, cases_dir, repetitions, min_time, bench_filter):
    with tempfile.NamedTemporaryFile(suffix=".json") as out:
        cmd = [bench, f"--cases-dir={cases_dir}", f"--benchmark_out={out.name}",
               "--benchmark_out_format=json", f"--benchmark_min_time={min_time}s",
               f"--benchmark_repetitions={repetitions}", "--benchmark_report_aggregates_only=true"]
        if bench_filter:
            cmd.append(f"--benchmark_filter={bench_filter}")
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
        data = json.load(open(out.name))
    times, cv = {}, {}
    for b in data["benchmarks"]:
        name = b.get("run_name", b["name"])
        if b.get("aggregate_name") == "median" or (repetitions == 1 and b.get("run_type") == "iteration"):
            times[name] = b["cpu_time"]
        elif b.get("aggregate_name") == "cv":
            cv[name] = b["cpu_time"]
    return data["context"], times, cv


def run_python(cases_dir, min_time, bench_filter):
    cmd = [sys.executable, HERE / "python_bench.py", "--cases-dir", cases_dir, "--min-time", str(min_time)]
    if bench_filter:
        cmd += ["--filter", bench_filter]
    data = json.loads(subprocess.run(cmd, check=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                                     text=True).stdout)
    return data["context"], {b["name"]: b["cpu_time"] for b in data["benchmarks"]}


def fmt_ns(ns):
    for unit, scale in (("s", 1e9), ("ms", 1e6), ("µs", 1e3)):
        if ns >= scale:
            return f"{ns / scale:.3g} {unit}"
    return f"{ns:.3g} ns"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--bench", required=True, type=pathlib.Path)
    ap.add_argument("--cases-dir", type=pathlib.Path, default=HERE / "cases")
    ap.add_argument("--out", type=pathlib.Path)
    ap.add_argument("--baseline", type=pathlib.Path)
    ap.add_argument("--threshold", type=float, default=0.10)
    ap.add_argument("--repetitions", type=int, default=5)
    ap.add_argument("--min-time", type=float, default=0.2)
    ap.add_argument("--filter", default="")
    ap.add_argument("--no-python", action="store_true")
    args = ap.parse_args()

    bad = [] if args.no_python else check_outputs(args.bench, args.cases_dir)
    if bad:
        print(f"output differs between the engines for: {', '.join(bad)}", file=sys.stderr)
        return 1

    cpp_ctx, cpp, cv = run_cpp(args.bench, args.cases_dir, args.repetitions, args.min_time, args.filter)
    py_ctx, py = ({}, {}) if args.no_python else run_python(args.cases_dir, args.min_time, args.filter)
    base = json.load(open(args.baseline))["cpp"] if args.baseline else {}

    header = "| Benchmark | Jinja2C++ | CV | Python Jinja2 | Speedup |"
    rule = "|---|---:|---:|---:|---:|"
    if base:
        header += " vs baseline |"
        rule += "---:|"
    print(header)
    print(rule)
    regressions = []
    for name in sorted(cpp, key=lambda n: (n.split("/", 1)[1], n)):
        row = f"| {name} | {fmt_ns(cpp[name])} | {cv.get(name, 0) * 100:.1f}% |"
        row += f" {fmt_ns(py[name])} | {py[name] / cpp[name]:.2f}x |" if name in py else " | |"
        if base:
            if name in base:
                change = cpp[name] / base[name] - 1
                row += f" {change * 100:+.1f}% |"
                if change > args.threshold:
                    regressions.append((name, change))
            else:
                row += " new |"
        print(row)

    if args.out:
        json.dump({"cpp_context": cpp_ctx, "python_context": py_ctx, "cpp": cpp, "cpp_cv": cv, "python": py},
                  open(args.out, "w"), indent=1)
    if regressions:
        print(f"\nslower than baseline by more than {args.threshold * 100:.0f}%:", file=sys.stderr)
        for name, change in regressions:
            print(f"  {name}: {change * 100:+.1f}%", file=sys.stderr)
        return 2
    return 0


if __name__ == "__main__":
    sys.exit(main())
