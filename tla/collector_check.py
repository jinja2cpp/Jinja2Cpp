#!/usr/bin/env python3
"""Model-checks tla/CycleCollector.tla with TLC: the collector as built, and each part of it and of
its contract taken out (docs/tasks/0338 step E).

Usage: tla/collector_check.py [--jar tla2tools.jar] [--only NAME ...] [--except NAME ...] [--list]

The run kinds are those of tla/check.py: a "pass" run must explore its whole bounded space with
no violation, a "mutant" run takes one part of the collector out and must find a counterexample
for the named invariant, an "open" run shows a limit the design accepts. See tla/README.md,
"Cycle collector".
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

HERE = os.path.dirname(os.path.abspath(__file__))
TLA_URL = "https://github.com/tlaplus/tlaplus/releases/download/v1.7.4/tla2tools.jar"

FIXES = {"FixNoteStore": True, "FixCountCands": True, "FixPropagate": True,
         "FixHoldGarbage": True, "FixPruneUnique": True}
SAFETY = ["TypeOK", "CountsSound", "NoUseAfterFree", "NoClearLive"]
ALL_KINDS = '{"list", "cell", "adapter", "opaque"}'

# Scopes. A cycle needs one list and one store; a kept value adds a cell and an adapter
LISTS = dict(Objs="{1, 2, 3, 4}", Kinds='{"list"}', MaxRef=2, MaxSteps=8, MaxHost=0,
             Prior=False, HostInCollect=False, HostReadsShare=False)
KEPT = dict(Objs="{1, 2, 3, 4}", Kinds='{"list", "cell", "adapter"}', MaxRef=1, MaxSteps=7,
            MaxHost=2, Prior=False, HostInCollect=False, HostReadsShare=False)
OPAQUE = dict(LISTS, Kinds='{"list", "opaque"}', MaxSteps=7)
# The shortest shapes of a reference moved past the walk: `h.append(h); keep(h)` with the
# host's data h, and an earlier render's kept value held in a cycle of this one
KEPT3 = dict(KEPT, Objs="{1, 2, 3}", MaxSteps=6, MaxHost=2)
# The same, deeper: four objects, three host steps
KEPT4 = dict(KEPT, MaxHost=3)
PRIOR = dict(KEPT, Objs="{1, 2, 3, 4}", MaxSteps=6, MaxHost=2, Prior=True)

# name: (kind, scope, overrides, invariants, note)
RUNS = {
    "lists": ("pass", LISTS, {}, SAFETY + ["Complete"],
              "lists and namespaces of one render, the host still"),
    "kept": ("pass", KEPT, {}, SAFETY + ["Complete"],
             "kept values of the host's data (frozen cells), host steps before the walk"),
    "opaque": ("pass", OPAQUE, {}, SAFETY,
               "captures Traverse does not show: may leak, never clears a live list"),
    "opaque-leaks": ("open", OPAQUE, {}, ["Complete"],
                     "a cycle through a capture is missed (by design, 0110)"),
    # Each part taken out
    "store-without-note": ("mutant", LISTS, {"FixNoteStore": False}, ["Complete"],
                           "a store that does not note its container: the cycle leaks"),
    "candidates-not-counted": ("mutant", LISTS, {"FixCountCands": False}, ["Complete"],
                               "the walk misses the collector's own reference: nothing is collected"),
    "no-propagate": ("mutant", LISTS, {"FixPropagate": False, "MaxHost": 1}, ["NoClearLive"],
                     "only the outside-held nodes kept: their items are emptied"),
    "no-garbage-hold": ("mutant", LISTS, {"FixHoldGarbage": False}, ["NoUseAfterFree"],
                        "emptying one garbage list frees the next before its Clear"),
    "prune-shared": ("mutant", LISTS, {"FixPruneUnique": False}, ["Complete"],
                     "the prune drops a candidate another reference holds: its cycle leaks"),
    # Other threads copy and drop their references while the walk reads the counts one by one
    "host-during-walk": ("pass", KEPT4, {"HostInCollect": True}, SAFETY,
                         "the host copies and drops a kept value during the walk"),
    "prior-during-walk": ("pass", PRIOR, {"HostInCollect": True}, SAFETY,
                          "an earlier render's kept value, held by the host, in a cycle of this one"),
    # Contract H6 taken out: a host read that kept a reference into the heap would let the host
    # move its reference past the walk's reads, and the walk would empty a list it still reaches
    "host-reads-share": ("mutant", KEPT3, {"HostInCollect": True, "HostReadsShare": True},
                         ["NoClearLive"], "H6 out: a copy of a kept value takes the cell's link"),
    "host-reads-share-prior": ("mutant", PRIOR, {"HostInCollect": True, "HostReadsShare": True},
                               ["NoClearLive"], "H6 out: a read of an earlier render's item keeps it"),
}


def config(scope, overrides, invariants):
    consts = dict(FIXES)
    consts.update(scope)
    consts.update(overrides)
    lines = ["SPECIFICATION Spec", "CONSTANTS"]
    for key, value in consts.items():
        if isinstance(value, bool):
            value = "TRUE" if value else "FALSE"
        lines.append(f"    {key} = {value}")
    lines.append("INVARIANTS")
    lines += [f"    {inv}" for inv in invariants]
    return "\n".join(lines) + "\n"


def run(jar, name, spec, workdir, extra=None, timeout=None, java_opts=()):
    kind, scope, overrides, invariants, note = spec
    overrides = dict(overrides, **(extra or {}))
    os.makedirs(workdir, exist_ok=True)
    shutil.copy(os.path.join(HERE, "CycleCollector.tla"), workdir)
    with open(os.path.join(workdir, "MC.cfg"), "w", encoding="utf-8") as cfg:
        cfg.write(config(scope, overrides, invariants))
    start = time.time()
    cmd = ["java", "-XX:+UseParallelGC", *java_opts, "-cp", jar, "tlc2.TLC", "-workers", "auto",
           "-deadlock", "-config", "MC.cfg", "CycleCollector.tla"]
    timed_out = False
    try:
        out = subprocess.run(cmd, cwd=workdir, capture_output=True, text=True, check=False,
                             timeout=timeout).stdout
    except subprocess.TimeoutExpired as expired:
        timed_out = True
        out = expired.stdout or ""
        out = out.decode() if isinstance(out, bytes) else out
    with open(os.path.join(workdir, "tlc.out"), "w", encoding="utf-8") as log:
        log.write(out)
    shutil.rmtree(os.path.join(workdir, "states"), ignore_errors=True)
    violated = re.search(r"Invariant (\w+) is violated", out)
    states = re.findall(r"([\d,]+) distinct states found", out)[-1:]
    steps = [re.sub(r" line \d+, col.*>", ">", line) for line in out.splitlines()
             if re.match(r"State \d+: <", line)]
    if violated:
        verdict = "counterexample: " + violated.group(1)
        ok = kind != "pass" and violated.group(1) in invariants
    elif "Model checking completed. No error has been found." in out:
        verdict = "no error"
        ok = kind == "pass"
    elif timed_out:
        verdict = "inconclusive: time limit"
        ok = False
    else:
        verdict = "TLC failed (see " + os.path.join(workdir, "tlc.out") + ")"
        ok = False
    return dict(name=name, kind=kind, note=note, verdict=verdict, ok=ok,
                states=states[0] if states else "?", secs=int(time.time() - start), trace=steps)


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--jar", default=os.environ.get("TLA2TOOLS", os.path.join(HERE, "tla2tools.jar")))
    parser.add_argument("--set", nargs="*", default=[], help="override a constant: NAME=VALUE")
    parser.add_argument("--only", nargs="*")
    parser.add_argument("--except", nargs="*", dest="skip", default=[], help="every run but these")
    parser.add_argument("--java-opts", default=os.environ.get("TLC_JAVA_OPTS", ""),
                        help="extra JVM options for TLC, for example -Xmx10g")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--timeout", type=int, help="seconds per run; a run that reaches it fails")
    parser.add_argument("--traces", action="store_true", help="print each counterexample's steps")
    args = parser.parse_args()
    args.jar = os.path.abspath(args.jar)
    if args.list:
        for name, spec in RUNS.items():
            print(f"{name:24} {spec[0]:7} {spec[4]}")
        return 0
    if not os.path.exists(args.jar):
        sys.exit(f"tla2tools.jar not found at {args.jar}: download {TLA_URL} or pass --jar")
    names = [name for name in args.only or list(RUNS) if name not in args.skip]
    unknown = [name for name in names + args.skip if name not in RUNS]
    if unknown:
        parser.error("unknown runs: " + " ".join(unknown))
    base = tempfile.mkdtemp(prefix="tlc-collector-")
    failed = 0
    extra = dict(item.split("=", 1) for item in args.set)
    for name in names:
        res = run(args.jar, name, RUNS[name], os.path.join(base, name), extra, args.timeout,
                  args.java_opts.split())
        failed += not res["ok"]
        mark = "ok " if res["ok"] else "BAD"
        print(f"{mark} {name:24} {res['kind']:7} {res['verdict']:30} {res['states']:>10} states "
              f"{res['secs']:>5}s  {res['note']}", flush=True)
        summary = os.environ.get("GITHUB_STEP_SUMMARY")
        if summary:
            with open(summary, "a", encoding="utf-8") as out:
                out.write(f"| {'ok' if res['ok'] else '**BAD**'} | `{name}` | {res['kind']} | "
                          f"{res['verdict']} | {res['states']} | {res['secs']} s |\n")
        if args.traces and res["trace"]:
            for step in res["trace"]:
                print("      " + step)
    print(f"logs in {base}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
