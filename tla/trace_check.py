#!/usr/bin/env python3
"""Checks logged C++ traces of the borrowed-cell protocol against tla/CellTrace.tla with TLC.

Usage: tla/check.py --trace --tests BUILD/jinja2cpp_tests [--filter PATTERN] [--jobs N]
       tla/check.py --trace --logs FILE.ndjson ...

A build configured with -DJINJA2CPP_CELL_TRACE=ON writes the protocol's events to the file the
environment variable JINJA2CPP_CELL_TRACE names (src/cell_trace.h). With --tests this script
runs each test the filter selects in its own process, so that each test is one trace, turns the
trace into the module TraceLog and runs TLC on CellTrace.tla. A trace passes when TLC reads it to
the end with no invariant violated. It fails when an invariant is violated (the code's decision
or state differs from the protocol's rules) or when no step of the spec takes an event (TLC's
deadlock: a code path the spec has no transition for). See tla/README.md, "Trace validation".
"""

import argparse
import concurrent.futures
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_FILTER = "BorrowedCells*:TemplateModuleTest*:ModuleCallTest*"
INVARIANTS = ["NoForeignCell", "AliasesAgree", "NoUseAfterFree", "NoDanglingLink", "SnapshotStable",
              "FrozenUnwritten", "NoCrossThreadWrite", "MarkRule", "DropRule", "LiveCount",
              "LinksAgree", "TableAgrees", "CallsAgree", "StorageAgrees", "EpochAgrees",
              "FrozenAgrees"]
UINT32_MAX = 2 ** 32 - 1


def tla_value(value):
    if isinstance(value, bool):
        return "TRUE" if value else "FALSE"
    if isinstance(value, int):
        # CellLink::Attached and BorrowedCell::SharedExport: the spec's ATT and SHARED
        return str(-1 if value == UINT32_MAX else value)
    if isinstance(value, list):
        return "<<" + ", ".join(tla_value(v) for v in value) + ">>"
    if isinstance(value, str):
        return json.dumps(value)
    raise ValueError(f"no TLA+ value for {value!r}")


def read_log(path):
    events = []
    with open(path, encoding="utf-8") as log:
        for line in log:
            line = line.strip()
            if line:
                events.append(json.loads(line))
    return events


def write_trace_module(events, path):
    threads = {e["t"] for e in events}
    with open(path, "w", encoding="utf-8") as out:
        out.write("---- MODULE TraceLog ----\nEXTENDS Integers, Sequences\n")
        out.write(f"OneThread == {'TRUE' if len(threads) <= 1 else 'FALSE'}\n")
        out.write("Log == <<\n")
        out.write(",\n".join("[" + ", ".join(f"{k} |-> {tla_value(v)}" for k, v in e.items()) + "]"
                             for e in events))
        out.write("\n>>\n====\n")


def check_trace(jar, log_path, workdir, fix_nested=True):
    """TLC on one trace: (verdict, ok, the event TLC stopped at or None, seconds)."""
    start = time.time()
    events = read_log(log_path)
    if trivial(events):
        return "no protocol events", True, None, 0, len(events)
    os.makedirs(workdir, exist_ok=True)
    for spec in ("CellTrace.tla", "CellRules.tla"):
        shutil.copy(os.path.join(HERE, spec), workdir)
    write_trace_module(events, os.path.join(workdir, "TraceLog.tla"))
    with open(os.path.join(workdir, "MC.cfg"), "w", encoding="utf-8") as cfg:
        cfg.write("SPECIFICATION Spec\nCONSTANTS\n")
        cfg.write(f"    FixNestedExport = {'TRUE' if fix_nested else 'FALSE'}\n")
        cfg.write("INVARIANTS\n" + "".join(f"    {inv}\n" for inv in INVARIANTS))
        cfg.write("ALIAS Alias\n")
    cmd = ["java", "-XX:+UseParallelGC", "-Xss16m", "-cp", jar, "tlc2.TLC", "-workers", "1",
           "-config", "MC.cfg", "-metadir", os.path.join(workdir, "states"), "CellTrace.tla"]
    out = subprocess.run(cmd, cwd=workdir, capture_output=True, text=True, check=False).stdout
    with open(os.path.join(workdir, "tlc.out"), "w", encoding="utf-8") as tlc_log:
        tlc_log.write(out)
    shutil.rmtree(os.path.join(workdir, "states"), ignore_errors=True)
    secs = int(time.time() - start)
    # The last state TLC printed: the event read last (ev) and the next one (l)
    positions = [int(n) for n in re.findall(r"/\\ l = (\d+)", out)]
    at = positions[-1] if positions else None
    violated = re.search(r"Invariant (\w+) is violated", out)
    if violated:
        # The violating state was reached by reading event l - 1
        index = at - 1 if at else None
        return f"violated: {violated.group(1)}", False, (index, events[index - 1] if index else None), secs, len(events)
    if "Deadlock reached" in out:
        index = at
        event = events[index - 1] if index and index <= len(events) else None
        return "no step takes this event", False, (index, event), secs, len(events)
    if "Model checking completed. No error has been found." in out:
        return "ok", True, None, secs, len(events)
    return f"TLC failed (see {os.path.join(workdir, 'tlc.out')})", False, None, secs, len(events)


def list_tests(binary, pattern, per_suite):
    out = subprocess.run([binary, "--gtest_list_tests", f"--gtest_filter={pattern}"],
                         capture_output=True, text=True, check=True).stdout
    tests = []
    suite = None
    for line in out.splitlines():
        if not line.strip():
            continue
        if not line.startswith(" "):
            suite = line.split()[0]
            if per_suite:
                tests.append(suite + "*")
        elif not per_suite:
            tests.append(suite + line.split()[0])
    return tests


def trivial(events):
    """Nothing but registries made and gone and reads before any promotion: nothing to check."""
    return all(e["ev"] in ("regnew", "regfree") or (e["ev"] == "skip" and e["e"] == 0) for e in events)


def run_test(binary, test, log_path):
    env = dict(os.environ, JINJA2CPP_CELL_TRACE=log_path)
    # In the build directory, as ctest runs them: the tests read their data relative to it
    binary = os.path.abspath(binary)
    res = subprocess.run([binary, f"--gtest_filter={test}"], capture_output=True, text=True,
                         check=False, env=env, cwd=os.path.dirname(binary))
    return res.returncode == 0


def main(argv=None):
    parser = argparse.ArgumentParser(prog="check.py --trace", description=__doc__.splitlines()[0])
    parser.add_argument("--jar", default=os.environ.get("TLA2TOOLS", os.path.join(HERE, "tla2tools.jar")))
    parser.add_argument("--tests", help="a jinja2cpp_tests built with JINJA2CPP_CELL_TRACE=ON")
    parser.add_argument("--filter", default=DEFAULT_FILTER, help=f"gtest filter (default {DEFAULT_FILTER})")
    parser.add_argument("--logs", nargs="*", default=[], help="check these logs instead of running tests")
    parser.add_argument("--per-suite", action="store_true",
                        help="one trace for each test suite rather than each test (for wide filters)")
    parser.add_argument("--jobs", type=int, default=2, help="TLC runs at a time")
    parser.add_argument("--no-fix-nested-export", action="store_true",
                        help="check the mark rule without #689's shared mark")
    args = parser.parse_args(argv)
    args.jar = os.path.abspath(args.jar)
    if not os.path.exists(args.jar):
        sys.exit(f"tla2tools.jar not found at {args.jar}: see tla/README.md")
    base = tempfile.mkdtemp(prefix="tlc-trace-")
    traces = []
    failed = 0
    if args.tests:
        for test in list_tests(args.tests, args.filter, args.per_suite):
            log_path = os.path.join(base, re.sub(r"[^\w.-]", "_", test) + ".ndjson")
            if not run_test(args.tests, test, log_path):
                print(f"BAD {test:60} the test failed", flush=True)
                failed += 1
            traces.append((test, log_path))
    traces += [(os.path.basename(p), os.path.abspath(p)) for p in args.logs]
    with concurrent.futures.ThreadPoolExecutor(max_workers=max(1, args.jobs)) as pool:
        futures = {pool.submit(check_trace, args.jar, path, os.path.join(base, f"tlc-{i}"),
                               not args.no_fix_nested_export): (name, path)
                   for i, (name, path) in enumerate(traces)}
        for future in concurrent.futures.as_completed(futures):
            name, path = futures[future]
            verdict, ok, where, secs, count = future.result()
            failed += not ok
            print(f"{'ok ' if ok else 'BAD'} {name:60} {verdict:28} {count:>6} events {secs:>4}s", flush=True)
            if where:
                index, event = where
                print(f"      event {index} of {path}: {json.dumps(event)}")
    print(f"{len(traces)} traces, {failed} failed; logs in {base}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
