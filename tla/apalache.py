#!/usr/bin/env python3
"""Checks tla/CellProtocol.tla with Apalache: bounded runs and the inductive step of IndInv.

Usage: tla/apalache.py [--apalache DIR] [--scope NAME] [--set NAME=VALUE ...] MODE
  MODE: typecheck | bounded LENGTH | inductive | ...

Apalache cannot read three things TLC needs, so each run copies the spec into a work directory
and replaces them there, checking that each text occurs exactly once:
  - InBound reads TLC's BFS level; Apalache bounds the length itself, so InBound is TRUE;
  - Close is RECURSIVE; Apalache folds Succ over the nodes a fixed number of times instead,
    one more than the longest path the scope's objects allow;
  - EXTENDS gains Apalache, for that fold.
The constants come from tla/check.py's scopes, so the two tools check the same models. See
tla/README.md, "Inductive invariants".
"""

import argparse
import os
import re
import shutil
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import check  # noqa: E402

APALACHE_URL = "https://github.com/apalache-mc/apalache/releases/download/v0.51.1/apalache-0.51.1.tgz"

SUBST = [
    ('InBound == TLCGet("level") < MaxLevel\n', "InBound == TRUE\n"),
    ("RECURSIVE Close(_)\n"
     "Close(S) == LET T == S \\cup UNION {Succ(n) : n \\in S} IN IF T = S THEN S ELSE Close(T)\n",
     # A path visits each adapter, cell, accessor and storage at most once. The reached set is
     # a flag per object: Apalache encodes a set of tuples grown by UNION far less compactly
     'Nodes == ({"a"} \\X AdIds) \\cup ({"c"} \\X CellIds) \\cup ({"x"} \\X GenIds) \\cup ({"g"} \\X GenIds)\n'
     "\\* @type: (<<Str, Int>> -> Bool, Int) => <<Str, Int>> -> Bool;\n"
     "CloseStep(R, i) == [n \\in Nodes |-> R[n] \\/ \\E m \\in Nodes : R[m] /\\ n \\in Succ(m)]\n"
     "\\* @type: (Int) => Int;\n"
     "CloseIndex(i) == i\n"
     "\\* @type: (Set(<<Str, Int>>)) => Set(<<Str, Int>>);\n"
     "Close(S) == LET R == ApaFoldSeqLeft(CloseStep, [n \\in Nodes |-> n \\in S],\n"
     "                                    MkSeq(MaxAd + MaxCell + 2 * MaxGen, CloseIndex))\n"
     "            IN {n \\in Nodes : R[n]}\n"),
    ("EXTENDS Integers, Sequences, FiniteSets, TLC\n",
     "EXTENDS Integers, Sequences, FiniteSets, TLC, Apalache\n"),
]


def prepare(workdir):
    with open(os.path.join(HERE, "CellProtocol.tla"), encoding="utf-8") as src:
        text = src.read()
    for old, new in SUBST:
        if text.count(old) != 1:
            sys.exit(f"apalache.py: expected one occurrence in CellProtocol.tla of:\n{old}")
        text = text.replace(old, new)
    with open(os.path.join(workdir, "CellProtocol.tla"), "w", encoding="utf-8") as dst:
        dst.write(text)
    for name in ("CellApalache.tla", "CellInductive.tla"):
        shutil.copy(os.path.join(HERE, name), workdir)


def constants(scope, overrides):
    """The constants check.py would give TLC, as a CInit predicate"""
    cfg = check.config(scope, overrides, [])
    conj = []
    for line in cfg.splitlines():
        m = re.match(r"    (\w+) = (.*)$", line)
        if not m:
            continue
        key, value = m.groups()
        # check.py may write the threads as model values; Apalache's are strings
        value = re.sub(r'(?<!")\b(t\d+)\b(?!")', r'"\1"', value)
        conj.append(f"    /\\ {key} = {value}")
    sizes = sizes_of(cfg)
    gens = "IndInit == IndInitOf(" + ", ".join(str(v) for v in sizes.values()) + ")\n"
    return "CInit ==\n" + "\n".join(conj) + "\n\n" + gens


def sizes_of(cfg):
    """The domain sizes, as literals for Apalache's Gen"""
    val = {m.group(1): m.group(2) for m in re.finditer(r"    (\w+) = (.*)", cfg)}
    count = lambda key: len(re.findall(r"t\d+|\"[^\"]*\"", val[key]))
    num = lambda key: int(val[key])
    threads = count("Threads")
    ad, cells, gens, depth = num("MaxAd"), num("MaxCell"), num("MaxGen"), num("MaxDepth")
    keys = 3 + count("OwnedAddrs")
    # Per variable: the largest set or function inside its value (a cell's links are adapters)
    return dict(NAd=ad, NCell=max(cells, ad), NGen=gens, NReg=max(1 + threads * depth, keys),
                NFr=max(threads, depth, ad), NEx=max(num("MaxEx"), 2), NThreads=threads,
                NOwned=count("OwnedAddrs"), NWr=max(ad + cells + gens, 2 * threads))


MODES = {
    # name: (init, invariant, length)
    "bounded": ("Init", "Safety", None),
    "init": ("Init", "IndInv", 0),          # Init => IndInv
    "inductive": ("IndInit", "Goal", 1),    # IndInv /\ Next => Goal' (Goal: IndInv or --inv)
    "safety": ("IndInit", "Safety", 0),     # IndInv => the invariants of check.py
    "sat": ("IndInit", "NoState", 0),       # IndInv has a state: must report a violation
}


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("mode", choices=["typecheck", *MODES])
    parser.add_argument("--length", type=int, default=4, help="steps of a bounded run")
    parser.add_argument("--apalache", default=os.environ.get("APALACHE_HOME", os.path.join(HERE, "apalache")),
                        help=f"an unpacked {APALACHE_URL}")
    parser.add_argument("--scope", default="module", help="a run of tla/check.py whose constants to use")
    parser.add_argument("--set", nargs="*", default=[], help="override a constant: NAME=VALUE")
    parser.add_argument("--inv", help="check this invariant instead of the mode's")
    parser.add_argument("--next", default="Next", help="the next-state action")
    parser.add_argument("--keep", action="store_true", help="keep the work directory")
    parser.add_argument("--timeout", type=int, help="seconds")
    parser.add_argument("--java-opts", default=os.environ.get("JVM_ARGS", "-Xmx8g"))
    args = parser.parse_args()

    tool = os.path.join(args.apalache, "bin", "apalache-mc")
    if not os.path.exists(tool):
        sys.exit(f"apalache-mc not found under {args.apalache}: unpack {APALACHE_URL} there or pass --apalache")
    kind, scope, overrides, _invariants, _note = check.RUNS[args.scope]
    overrides = dict(overrides, **dict(item.split("=", 1) for item in args.set))
    work = tempfile.mkdtemp(prefix="apalache-cells-")
    prepare(work)
    with open(os.path.join(work, "MCApalache.tla"), "w", encoding="utf-8") as mc:
        # An action invariant: Apalache checks it on the step only, not on IndInit's state
        goal = f"Goal == ({args.inv or 'IndInv'})'\n"
        mc.write("---- MODULE MCApalache ----\nEXTENDS CellInductive\n\n"
                 + constants(scope, overrides) + goal + "====\n")
    cmd = [tool, args.mode if args.mode == "typecheck" else "check"]
    if args.mode != "typecheck":
        init, inv, length = MODES[args.mode]
        inv = inv if args.mode == "inductive" else args.inv or inv
        cmd += ["--cinit=CInit", f"--init={init}", f"--next={args.next}", f"--inv={inv}",
                f"--length={args.length if length is None else length}", "--no-deadlock"]
    cmd += [f"--out-dir={os.path.join(work, 'out')}", "MCApalache.tla"]
    env = dict(os.environ, JVM_ARGS=args.java_opts)
    try:
        proc = subprocess.run(cmd, cwd=work, env=env, capture_output=True, text=True,
                              timeout=args.timeout, check=False)
        out = proc.stdout + proc.stderr
    except subprocess.TimeoutExpired:
        print(f"timeout after {args.timeout}s; work directory {work}")
        return 2
    with open(os.path.join(work, "apalache.out"), "w", encoding="utf-8") as log:
        log.write(out)
    for line in out.splitlines():
        if re.search(r"E@|EXITCODE|Checker reports|State \d+:|It took me|counterexample", line):
            print(line)
    ok = "EXITCODE: OK" in out
    if args.mode == "sat":
        ok = "EXITCODE: ERROR (12)" in out  # the violation of NoState: IndInit has a state
    print(("ok " if ok else "BAD ") + f"{args.mode} {args.scope}; work directory {work}")
    if ok and not args.keep:
        shutil.rmtree(work, ignore_errors=True)
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main())
