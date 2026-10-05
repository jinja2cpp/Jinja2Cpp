#!/usr/bin/env python3
"""Run the bench/cases workloads with Python Jinja2, the reference engine.

Mirrors bench/jinja2cpp_bench.cpp: for every case it times Load (compile main.j2) and
Render (render the compiled template), and prints Google Benchmark style JSON so
bench/run.py can put both engines side by side.

Usage: python_bench.py [--cases-dir DIR] [--min-time SECONDS] [--dump-dir DIR] [--filter REGEX]
"""
import argparse
import json
import pathlib
import platform
import re
import sys
import time

import jinja2


# settings.json keys that are Environment options; "wide" only switches the C++ driver to TemplateW
ENV_OPTIONS = ("trim_blocks", "lstrip_blocks", "autoescape")


def load_case(case_dir):
    templates = {p.name: p.read_text(encoding="utf-8") for p in case_dir.glob("*.j2") if p.name != "main.j2"}
    settings_file = case_dir / "settings.json"
    settings = json.loads(settings_file.read_text(encoding="utf-8")) if settings_file.exists() else {}
    options = {k: v for k, v in settings.items() if k in ENV_OPTIONS}
    env = jinja2.Environment(loader=jinja2.DictLoader(templates), **options)
    data_file = case_dir / "data.json"
    params = json.loads(data_file.read_text(encoding="utf-8")) if data_file.exists() else {}
    return env, (case_dir / "main.j2").read_text(encoding="utf-8"), params


def measure(fn, min_time):
    """Mean ns per call: double the batch until it runs for min_time, like Google Benchmark."""
    batch = 1
    while True:
        start = time.perf_counter_ns()
        for _ in range(batch):
            fn()
        elapsed = time.perf_counter_ns() - start
        if elapsed >= min_time * 1e9 or batch >= 1 << 30:
            return elapsed / batch, batch
        batch *= 2 if elapsed < min_time * 1e8 else max(2, int(min_time * 1e9 * 1.2 / max(elapsed, 1)))


def main():
    here = pathlib.Path(__file__).resolve().parent
    ap = argparse.ArgumentParser()
    ap.add_argument("--cases-dir", type=pathlib.Path, default=here / "cases")
    ap.add_argument("--min-time", type=float, default=0.5)
    ap.add_argument("--dump-dir", type=pathlib.Path)
    ap.add_argument("--filter", default="")
    args = ap.parse_args()

    cases = sorted(d for d in args.cases_dir.iterdir() if (d / "main.j2").exists())
    if args.dump_dir:
        args.dump_dir.mkdir(parents=True, exist_ok=True)
        for d in cases:
            env, source, params = load_case(d)
            (args.dump_dir / f"{d.name}.txt").write_text(env.from_string(source).render(params), encoding="utf-8")
        return 0

    benchmarks = []
    for d in cases:
        env, source, params = load_case(d)
        tpl = env.from_string(source)
        for kind, fn in (("Load", lambda: env.from_string(source)), ("Render", lambda: tpl.render(params))):
            name = f"{kind}/{d.name}"
            if args.filter and not re.search(args.filter, name):
                continue
            ns, iterations = measure(fn, args.min_time)
            benchmarks.append({"name": name, "run_type": "iteration", "iterations": iterations,
                               "real_time": ns, "cpu_time": ns, "time_unit": "ns"})
            print(f"{name:40} {ns:14.0f} ns", file=sys.stderr)

    context = {"engine": "python-jinja2", "jinja2_version": jinja2.__version__,
               "python_version": platform.python_version(), "host_name": platform.node()}
    json.dump({"context": context, "benchmarks": benchmarks}, sys.stdout, indent=1)
    print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
