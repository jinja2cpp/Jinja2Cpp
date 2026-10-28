#!/usr/bin/env python3
"""Model-checks tla/CellProtocol.tla with TLC: the fixed protocol, then each #673 fix taken out.

Usage: tla/check.py [--jar tla2tools.jar] [--only NAME ...] [--list]

Every run is a TLC model: a set of constants plus the invariants it checks. A "pass" run must
find no violation; a "mutant" run puts back the code as it was before one fix and must find a
counterexample for the named invariant; an "open" run checks an invariant of an open task and
is expected to fail too. See tla/README.md.
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
TLA_URL = "https://github.com/tlaplus/tlaplus/releases/download/v1.8.0/tla2tools.jar"

FIXES = {
    "FixLinkEpoch": True, "FixEnumRefetch": True, "FixSortRefetch": True,
    "FixCallRestore": True, "FixNestedGlobal": True, "FixMarkOrder": True,
    "FixSortMutableFirst": True, "FixMarkerAfterCopy": True, "FixReadOnlyExport": True,
    "FixFreeze": True, "FixSourceKeepsId": True,
    # Proposed by this model, not in #673 yet (README, finding F1)
    "FixNestedExport": True,
}
SAFETY = ["SnapshotStable", "NoCrossThreadWrite", "NoUseAfterFree", "NoForeignCell",
          "RegistryBounded", "AliasesAgree", "NoUndefItems", "LiveCount"]

# Scopes. The deep counterexamples need about 14 steps, which an all-actions model of two
# threads cannot reach exhaustively; each scope enables the parts a group of fixes needs.
RENDER1 = dict(Threads='{"t1"}', ModuleThreads='{}', RenderThreads='{"t1"}', MaxDepth=2,
               MaxLevel=12, MaxAd=9, MaxCell=4, MaxGen=5, MaxEx=2)
MODULE1 = dict(Threads='{"t1"}', ModuleThreads='{"t1"}', RenderThreads='{}', MaxDepth=2,
               MaxLevel=12, MaxAd=10, MaxCell=3, MaxGen=5, MaxEx=2)
MIXED1 = dict(Threads='{"t1"}', ModuleThreads='{"t1"}', RenderThreads='{"t1"}', MaxDepth=2,
              MaxLevel=11, MaxAd=9, MaxCell=4, MaxGen=5, MaxEx=2)
TWO = dict(Threads='{"t1", "t2"}', ModuleThreads='{"t1", "t2"}', RenderThreads='{"t1", "t2"}',
           MaxDepth=1, MaxLevel=11, MaxAd=9, MaxCell=3, MaxGen=4, MaxEx=2)
# Two threads' renders that may read what the host kept: the shape of 0331's b2.cpp 1
SHARED_KEPT = dict(Threads='{"t1", "t2"}', ModuleThreads='{}', RenderThreads='{"t1", "t2"}',
                   MaxDepth=1, MaxLevel=13, MaxAd=9, MaxCell=2, MaxGen=2, MaxEx=1,
                   Actions='{"read", "export"}')
# Module calls on one thread with only what a borrower across a callback needs
DEEP = dict(Threads='{"t1"}', ModuleThreads='{"t1"}', RenderThreads='{}', MaxDepth=2,
            MaxLevel=34, MaxAd=10, MaxCell=1, MaxGen=4, MaxEx=2,
            Actions='{"export", "release", "enum", "sort"}')
ALL = '{"touch", "read", "export", "keeplist", "ns", "enum", "sort", "release"}'


# name: (kind, scope, overrides, invariants, note)
RUNS = {
    # The protocol as #673 has it at 459d8f7
    "render": ("pass", RENDER1, {}, SAFETY, "renders, nested through a callback"),
    "module": ("pass", MODULE1, {}, SAFETY, "module calls, nested through a callback"),
    "mixed": ("pass", MIXED1, {}, SAFETY, "renders and module calls on one thread"),
    "threads": ("pass", TWO, {}, SAFETY, "two threads, renders and the locked module"),
    # Finding F1: #673 as it is at 459d8f7, without the proposed FixNestedExport
    "f1-nested-export": ("finding", MODULE1, {"FixNestedExport": False, "MaxFail": 0},
                         ["SnapshotStable"],
                         "F1: a nested call's kept value changes when the outer call writes"),
    # Each fix taken out
    "no-link-epoch": ("mutant", RENDER1, {"FixLinkEpoch": False}, ["AliasesAgree"],
                      "c06d969, Codex P2: alias detached after a failed Link"),
    "no-enum-refetch": ("mutant", DEEP, {"FixEnumRefetch": False}, ["NoUseAfterFree"],
                        "f83b89f, Codex P1: loop over storage two copies freed"),
    "no-sort-refetch": ("mutant", DEEP, {"FixSortRefetch": False},
                        ["NoUseAfterFree", "SnapshotStable"],
                        "1e15c2d/89df914, Codex P0: sort writes back into freed storage"),
    "no-call-restore": ("mutant", MODULE1, {"FixCallRestore": False}, ["SnapshotStable"],
                        "1e15c2d, Codex P1: outer call writes a nested call's snapshot"),
    "no-nested-global": ("mutant", RENDER1, {"FixNestedGlobal": False}, ["NoForeignCell"],
                         "9c91ffc, Codex P2: nested render reads the outer render's cell"),
    "no-mark-order": ("mutant", RENDER1, {"FixMarkOrder": False}, ["NoForeignCell"],
                      "5e37c24, verifier r11: failed conversion loses the attached mark"),
    "no-sort-mutable-first": ("mutant", MODULE1, {"FixSortMutableFirst": False},
                              ["NoUndefItems"],
                              "9c91ffc, Codex P2: sort restores moved-from items"),
    "no-marker-after-copy": ("mutant", MODULE1, {"FixMarkerAfterCopy": False},
                             ["SnapshotStable"],
                             "b3993a5, Ruslan P2: failed copy clears the export mark"),
    "no-read-only-export": ("mutant", SHARED_KEPT, {"FixReadOnlyExport": False},
                            ["NoCrossThreadWrite"],
                            "0331 A1 (b2.cpp 1): an export links a shared adapter"),
    "no-freeze": ("mutant", RENDER1, {"FixFreeze": False}, ["SnapshotStable"],
                  "0331 A2, 4588321, Ruslan P2: kept nested value reverts at Finish"),
    "no-drop-unheld": ("mutant", MODULE1, {"DropRule": '"none"'}, ["RegistryBounded"],
                       "459d8f7, Codex P2: module registry grows per call"),
    # Owned data has one adapter per call in the model (LifetimeAnchor is not modelled), so
    # IsUnheld's links test can never decide: this run shows it, rather than a counterexample
    "drop-without-link-check": ("pass", MODULE1, {"DropRule": '"nolinks"'}, SAFETY,
                                "IsUnheld without its links test: never decides here"),
    "drop-without-source-check": ("mutant", MODULE1, {"DropRule": '"nosource"'},
                                  ["NoUseAfterFree", "NoForeignCell", "AliasesAgree"],
                                  "IsUnheld without its source-owner test"),
    "no-source-keeps-id": ("mutant", MODULE1, {"FixSourceKeepsId": False, "DropRule": '"none"'},
                           ["NoForeignCell"],
                           "m_source: identity reused while the registry lists the cell"),
    # Open tasks
    "open-0335-late-export": ("open", MODULE1, {}, ["ExportKeepsCallState"],
                              "0335: an export before the call's first write"),
    "open-0332-kept-list": ("open", MODULE1, {"AllowModuleKeepList": True}, ["SnapshotStable"],
                            "0332: a module's live cell read through a kept list"),
    "open-0332-nested": ("open", MODULE1, {"AllowModuleNested": True}, ["SnapshotStable"],
                         "0332: a nested item of a module's export, written later"),
}


def config(scope, overrides, invariants):
    overrides = {k: v for k, v in overrides.items() if not k.startswith("_")}
    consts = dict(OwnedAddrs='{"O1", "O2"}', MaxVal=3, MaxFail=1, DropRule='"full"',
                  AllowModuleKeepList=False, AllowModuleNested=False)
    consts["Actions"] = ALL
    consts.update(FIXES)
    consts.update(scope)
    consts.update(overrides)
    lines = ["SPECIFICATION Spec", "CONSTANTS"]
    for key, value in consts.items():
        if isinstance(value, bool):
            value = "TRUE" if value else "FALSE"
        lines.append(f"    {key} = {value}")
    lines.append("CONSTRAINT LevelBound")
    lines.append("INVARIANTS")
    lines += [f"    {inv}" for inv in invariants]
    return "\n".join(lines) + "\n"


def run(jar, name, spec, workdir, extra=None, timeout=None):
    kind, scope, overrides, invariants, note = spec
    overrides = dict(overrides, **(extra or {}))
    os.makedirs(workdir, exist_ok=True)
    shutil.copy(os.path.join(HERE, "CellProtocol.tla"), workdir)
    with open(os.path.join(workdir, "MC.cfg"), "w", encoding="utf-8") as cfg:
        cfg.write(config(scope, overrides, invariants))
    start = time.time()
    mode = []
    if "_sim" in overrides:
        # Random behaviours: for counterexamples deeper than an exhaustive search reaches
        traces, depth = overrides["_sim"]
        mode = ["-simulate", f"num={traces}", "-depth", str(depth), "-seed", "1"]
    cmd = ["java", "-XX:+UseParallelGC", "-cp", jar, "tlc2.TLC", "-workers", "auto", "-deadlock",
           "-config", "MC.cfg"] + mode + ["CellProtocol.tla"]
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
    # TLC's disk queue and fingerprints: gigabytes for the larger scopes
    shutil.rmtree(os.path.join(workdir, "states"), ignore_errors=True)
    violated = re.search(r"Invariant (\w+) is violated", out)
    states = (re.findall(r"([\d,]+) distinct states found", out)[-1:]
              or re.findall(r"([\d,]+) states generated", out)[-1:])
    steps = [re.sub(r" line \d+, col.*>", ">", line) for line in out.splitlines()
             if re.match(r"State \d+: <", line)]
    if violated:
        verdict = "counterexample: " + violated.group(1)
        ok = kind != "pass" and violated.group(1) in invariants
    elif ("Model checking completed. No error has been found." in out
          or (mode and "Finished in" in out and "Error:" not in out)):
        verdict = "no error"
        ok = kind == "pass"
    elif timed_out:
        level = re.findall(r"Progress\((\d+)\)", out)
        verdict = f"no error to level {level[-1] if level else '?'} (time limit)"
        ok = kind == "pass"
    else:
        verdict = "TLC failed (see " + os.path.join(workdir, "tlc.out") + ")"
        ok = False
    return dict(name=name, kind=kind, note=note, verdict=verdict, ok=ok,
                states=states[0] if states else "?", secs=int(time.time() - start),
                trace=[s for s in steps if not s.endswith("<GC>")])


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--jar", default=os.environ.get("TLA2TOOLS", os.path.join(HERE, "tla2tools.jar")))
    parser.add_argument("--set", nargs="*", default=[], help="override a constant: NAME=VALUE")
    parser.add_argument("--only", nargs="*")
    parser.add_argument("--list", action="store_true")
    parser.add_argument("--timeout", type=int, help="seconds per run; a pass run that reaches it reports the BFS level it completed")
    parser.add_argument("--traces", action="store_true", help="print each counterexample's steps")
    args = parser.parse_args()
    args.jar = os.path.abspath(args.jar)
    if args.list:
        for name, spec in RUNS.items():
            print(f"{name:28} {spec[0]:7} {spec[4]}")
        return 0
    if not os.path.exists(args.jar):
        sys.exit(f"tla2tools.jar not found at {args.jar}: download {TLA_URL} or pass --jar")
    names = args.only or list(RUNS)
    base = tempfile.mkdtemp(prefix="tlc-cells-")
    failed = 0
    for name in names:
        extra = dict(item.split("=", 1) for item in args.set)
        res = run(args.jar, name, RUNS[name], os.path.join(base, name), extra, args.timeout)
        failed += not res["ok"]
        mark = "ok " if res["ok"] else "BAD"
        print(f"{mark} {name:28} {res['kind']:7} {res['verdict']:36} {res['states']:>12} states "
              f"{res['secs']:>5}s  {res['note']}", flush=True)
        if args.traces and res["trace"]:
            for step in res["trace"]:
                print("      " + step)
    print(f"logs in {base}")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
