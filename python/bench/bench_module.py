#!/usr/bin/env python3
"""Time the bench/cases workloads through the Python module and with Python Jinja2.

Mirrors bench/python_bench.py (docs/tasks/0353): for every case it times Load (compile main.j2)
and Render (render the compiled template with the case's data.json as Python objects) with
both engines, checks that they render the same text, and prints a table. The module's
performance gate (docs/tasks/0353): jinja2cpp is faster on Load and on Render for every case.

Run it on a Release build of the module:
    PYTHONPATH=build-py-rel/python python3 python/bench/bench_module.py [--min-time S] [--filter RE] [--json FILE]

A case whose settings.json asks for transformers' environment (hf_*) runs in the environment
jinja2cpp.integrations.transformers.patch() uses (chat_template_environment: the sandbox, and
tojson, raise_exception and strftime_now in the engine); Python Jinja2 runs as
bench/python_bench.py runs it.
"""
import argparse
import datetime
import importlib.util
import json
import pathlib
import platform
import re
import sys

import jinja2
import jinja2cpp
from jinja2cpp.integrations.transformers import chat_template_environment

HERE = pathlib.Path(__file__).resolve().parent
# bench/python_bench.py's clock for strftime_now
FIXED_NOW = datetime.datetime(2025, 3, 14, 9, 26, 53).timestamp()
ROOT = HERE.parent.parent


def load_reference():
    """bench/python_bench.py as a module: its case loader and timer are the reference's"""
    spec = importlib.util.spec_from_file_location("python_bench", ROOT / "bench" / "python_bench.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def module_environment(case_dir, settings):
    templates = {p.name: p.read_text(encoding="utf-8") for p in case_dir.glob("*.j2") if p.name != "main.j2"}
    if settings.get("transformers"):
        # What jinja2cpp.integrations.transformers.patch() compiles chat templates in
        return chat_template_environment(lambda: FIXED_NOW)
    options = {k: v for k, v in settings.items() if k in ("trim_blocks", "lstrip_blocks", "autoescape")}
    return jinja2cpp.Environment(loader=jinja2cpp.DictLoader(templates), **options)


def bench_json(args):
    """The module's Load and Render times as bench/python_bench.py prints Jinja2's, for the
    `jinja2cpp (Python)` column of bench/run.py (docs/tasks/0355, 0246). A case the module renders
    differently from Jinja2 is left out and named on stderr, so the column compares equal work."""
    reference = load_reference()
    benchmarks, differs = [], []
    for d in sorted(d for d in args.cases_dir.iterdir() if (d / "main.j2").exists() and not reference.is_cpp_only(d)):
        settings_file = d / "settings.json"
        settings = json.loads(settings_file.read_text(encoding="utf-8")) if settings_file.exists() else {}
        py_env, source, params = reference.load_case(d)
        env = module_environment(d, settings)
        try:
            tpl = env.from_string(source)
            same = tpl.render(params) == py_env.from_string(source).render(params)
        except Exception as error:  # noqa: BLE001 - reported per case
            print(f"{d.name}: jinja2cpp failed: {type(error).__name__}: {error}", file=sys.stderr)
            same = False
        if not same:
            differs.append(d.name)
            continue
        for kind, fn in (("Load", lambda: env.from_string(source)), ("Render", lambda: tpl.render(params))):
            name = f"{kind}/{d.name}"
            if args.filter and not re.search(args.filter, name):
                continue
            ns, iterations = reference.measure(fn, args.min_time)
            benchmarks.append({"name": name, "run_type": "iteration", "iterations": iterations,
                               "real_time": ns, "cpu_time": ns, "time_unit": "ns"})
    if differs:
        print("the module renders differently from Jinja2, left out: " + ", ".join(differs), file=sys.stderr)
    context = {"engine": "jinja2cpp-python", "jinja2cpp_version": jinja2cpp.__version__,
               "python_version": platform.python_version(), "host_name": platform.node(), "differs": differs}
    json.dump({"context": context, "benchmarks": benchmarks}, sys.stdout, indent=1)
    print()
    return 0


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--cases-dir", type=pathlib.Path, default=ROOT / "bench" / "cases")
    ap.add_argument("--min-time", type=float, default=0.5)
    ap.add_argument("--filter", default="")
    ap.add_argument("--json", type=pathlib.Path, help="also write the results as JSON")
    ap.add_argument("--bench-json", action="store_true",
                    help="time the module alone and print bench/python_bench.py's JSON (bench/run.py --python-module)")
    args = ap.parse_args()
    if args.bench_json:
        return bench_json(args)

    reference = load_reference()
    cases = sorted(d for d in args.cases_dir.iterdir() if (d / "main.j2").exists())
    rows = []
    failed = []
    for d in cases:
        if args.filter and not re.search(args.filter, d.name):
            continue
        settings_file = d / "settings.json"
        settings = json.loads(settings_file.read_text(encoding="utf-8")) if settings_file.exists() else {}
        py_env, source, params = reference.load_case(d)
        cpp_env = module_environment(d, settings)
        py_tpl = py_env.from_string(source)
        try:
            cpp_tpl = cpp_env.from_string(source)
            same = cpp_tpl.render(params) == py_tpl.render(params)
        except Exception as error:  # noqa: BLE001 - reported per case
            print(f"{d.name}: jinja2cpp failed: {type(error).__name__}: {error}", file=sys.stderr)
            failed.append(d.name)
            continue
        if not same:
            print(f"{d.name}: the engines render different text", file=sys.stderr)
            failed.append(d.name)
        row = {"case": d.name, "same_output": same}
        for kind, py_fn, cpp_fn in (
            ("Load", lambda: py_env.from_string(source), lambda: cpp_env.from_string(source)),
            ("Render", lambda: py_tpl.render(params), lambda: cpp_tpl.render(params)),
        ):
            py_ns, _ = reference.measure(py_fn, args.min_time)
            cpp_ns, _ = reference.measure(cpp_fn, args.min_time)
            row[kind] = {"jinja2": py_ns, "jinja2cpp": cpp_ns}
        rows.append(row)
        print(f"{d.name:22} Load {row['Load']['jinja2']:12.0f} {row['Load']['jinja2cpp']:12.0f}"
              f"   Render {row['Render']['jinja2']:12.0f} {row['Render']['jinja2cpp']:12.0f}", file=sys.stderr)

    print("| case | Load Jinja2 (ns) | Load jinja2cpp (ns) | x | Render Jinja2 (ns) | Render jinja2cpp (ns) | x |")
    print("|---|---:|---:|---:|---:|---:|---:|")
    losing = []
    for row in rows:
        cells = [row["case"] + ("" if row["same_output"] else " (differs)")]
        for kind in ("Load", "Render"):
            py_ns, cpp_ns = row[kind]["jinja2"], row[kind]["jinja2cpp"]
            cells += [f"{py_ns:.0f}", f"{cpp_ns:.0f}", f"{py_ns / cpp_ns:.1f}"]
            if cpp_ns >= py_ns:
                losing.append(f"{kind}/{row['case']}")
        print("| " + " | ".join(cells) + " |")
    print()
    print("losing: " + (", ".join(losing) if losing else "none"))
    if failed:
        print("failed or different: " + ", ".join(failed))
    if args.json:
        context = {"jinja2_version": jinja2.__version__, "python_version": platform.python_version(), "host_name": platform.node()}
        args.json.write_text(json.dumps({"context": context, "cases": rows, "losing": losing, "failed": failed}, indent=1), encoding="utf-8")
    return 1 if losing or failed else 0


if __name__ == "__main__":
    sys.exit(main())
