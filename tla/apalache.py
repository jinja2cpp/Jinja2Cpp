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
import concurrent.futures
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time

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


def constants(scope, overrides, hyp="IndInv"):
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
    gens = ("IndInit == IndInitOf(" + ", ".join(str(v) for v in sizes.values()) + ")"
            + f" /\\ ({hyp})\n")
    return "CInit ==\n" + "\n".join(conj) + "\n\n" + gens


def hosts():
    """The spec's Hosts, read from it so that a new host's key cannot leave Gen too small"""
    with open(os.path.join(HERE, "CellProtocol.tla"), encoding="utf-8") as src:
        m = re.search(r"^Hosts == \{([^}]*)\}", src.read(), re.M)
    return re.findall(r'"[^"]*"', m.group(1))


def sizes_of(cfg):
    """The domain sizes, as literals for Apalache's Gen"""
    val = {m.group(1): m.group(2) for m in re.finditer(r"    (\w+) = (.*)", cfg)}
    count = lambda key: len(re.findall(r"t\d+|\"[^\"]*\"", val[key]))
    num = lambda key: int(val[key])
    threads = count("Threads")
    ad, cells, gens, depth = num("MaxAd"), num("MaxCell"), num("MaxGen"), num("MaxDepth")
    keys = len(hosts()) + count("OwnedAddrs")
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


def definitions():
    """name -> body of each top-level definition of the spec and CellInductive.tla"""
    defs = {}
    for name in ("CellProtocol.tla", "CellInductive.tla"):
        with open(os.path.join(HERE, name), encoding="utf-8") as src:
            text = src.read()
        for m in re.finditer(r"^(\w+) ==(.*?)(?=^\S|\Z)", text, re.M | re.S):
            defs[m.group(1)] = re.sub(r"\\\*[^\n]*", "", m.group(2))
    return defs


def leaves(name, defs):
    """The conjuncts of a definition that is a conjunction of names, recursively: the parts of
    IndInv that a sweep checks one by one"""
    body = defs.get(name, "")
    names = [part.strip() for part in body.replace("\n", " ").split("/\\") if part.strip()]
    if not names or not all(re.fullmatch(r"\w+", n) and n in defs for n in names):
        return [name]
    return [leaf for n in names for leaf in leaves(n, defs)]


def actions(defs):
    """CellInductive's NextXxx: one action of Next each"""
    return [n for n in defs if re.fullmatch(r"Next[A-Z]\w*", n)]


def query(args, mode, inv=None, hyp="IndInv", nxt="Next", timeout=None, keep=False):
    """One Apalache run: (verdict, seconds, work directory). verdict: ok, BAD or timeout"""
    tool = os.path.join(args.apalache, "bin", "apalache-mc")
    kind, scope, overrides, _invariants, _note = check.RUNS[args.scope]
    overrides = dict(overrides, **dict(item.split("=", 1) for item in args.set))
    work = tempfile.mkdtemp(prefix="apalache-cells-")
    prepare(work)
    with open(os.path.join(work, "MCApalache.tla"), "w", encoding="utf-8") as mc:
        # An action invariant: Apalache checks it on the step only, not on IndInit's state
        goal = f"Goal == ({inv or 'IndInv'})'\n"
        mc.write("---- MODULE MCApalache ----\nEXTENDS CellInductive\n\n"
                 + constants(scope, overrides, hyp) + goal + "====\n")
    cmd = [tool, mode if mode == "typecheck" else "check"]
    if mode != "typecheck":
        init, minv, length = MODES[mode]
        minv = minv if mode == "inductive" else inv or minv
        cmd += ["--cinit=CInit", f"--init={init}", f"--next={nxt}", f"--inv={minv}",
                f"--length={args.length if length is None else length}", "--no-deadlock"]
    cmd += [f"--out-dir={os.path.join(work, 'out')}", "MCApalache.tla"]
    env = dict(os.environ, JVM_ARGS=args.java_opts)
    start = time.time()
    try:
        proc = subprocess.run(cmd, cwd=work, env=env, capture_output=True, text=True,
                              timeout=timeout, check=False)
        out = proc.stdout + proc.stderr
    except subprocess.TimeoutExpired:
        return "timeout", time.time() - start, work
    secs = time.time() - start
    with open(os.path.join(work, "apalache.out"), "w", encoding="utf-8") as log:
        log.write(out)
    ok = "EXITCODE: OK" in out
    if mode == "sat":
        ok = "EXITCODE: ERROR (12)" in out  # the violation of NoState: IndInit has a state
    if ok and not keep:
        shutil.rmtree(work, ignore_errors=True)
    return ("ok" if ok else "BAD"), secs, work


def sweep(args):
    r"""IndInv /\ A => P' for every action A of Next and every part P of IndInv, one query each:
    the conjunction in one query is far slower than its parts (README, "Inductive invariants")"""
    defs = definitions()
    acts = args.actions or actions(defs)
    parts = args.parts or leaves(args.inv or "IndInv", defs)
    print(f"{len(acts)} actions x {len(parts)} parts, scope {args.scope} {' '.join(args.set)}", flush=True)
    bad = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        # An action no IndInv state takes (outside the scope) has no step to check. Finding a
        # state costs about as much as a query, so the check is only worth it for many parts
        enabled = dict(zip(acts, pool.map(
            lambda a: query(args, "inductive", "FALSE", args.hyp, a, args.timeout)[0] != "ok", acts))) \
            if args.skip_disabled else {a: True for a in acts}
        for a in acts:
            if not enabled[a]:
                print(f"{a}: no step in this scope", flush=True)
        todo = [(a, p) for a in acts if enabled[a] for p in parts]
        futures = {pool.submit(query, args, "inductive", p, args.hyp, a, args.timeout, True): (a, p)
                   for a, p in todo}
        for fut in concurrent.futures.as_completed(futures):
            a, p = futures[fut]
            verdict, secs, work = fut.result()
            if verdict == "ok":
                shutil.rmtree(work, ignore_errors=True)
            else:
                bad.append((a, p, verdict, work))
            print(f"{verdict:7} {secs:6.0f}s {a} => {p}'" + ("" if verdict == "ok" else f"  {work}"),
                  flush=True)
    print(f"{len(todo) - len(bad)} of {len(todo)} steps hold")
    return 0 if not bad else 1


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("mode", choices=["typecheck", "sweep", *MODES])
    parser.add_argument("--length", type=int, default=4, help="steps of a bounded run")
    parser.add_argument("--apalache", default=os.environ.get("APALACHE_HOME", os.path.join(HERE, "apalache")),
                        help=f"an unpacked {APALACHE_URL}")
    parser.add_argument("--scope", default="module", help="a run of tla/check.py whose constants to use")
    parser.add_argument("--set", nargs="*", default=[], help="override a constant: NAME=VALUE")
    parser.add_argument("--inv", help="check this invariant instead of the mode's (sweep: split it)")
    parser.add_argument("--hyp", default="IndInv",
                        help="what IndInit assumes besides TypeOK (default IndInv); a part of it is cheaper")
    parser.add_argument("--next", default="Next", help="the next-state action")
    parser.add_argument("--actions", nargs="*", help="sweep: these NextXxx (default: all)")
    parser.add_argument("--parts", nargs="*", help="sweep: these parts (default: the conjuncts of --inv)")
    parser.add_argument("--skip-disabled", action="store_true",
                        help="sweep: first drop the actions that take no step in the scope")
    parser.add_argument("--jobs", type=int, default=2, help="sweep: queries at once (each JVM + z3)")
    parser.add_argument("--keep", action="store_true", help="keep the work directory")
    parser.add_argument("--timeout", type=int, help="seconds (sweep: per query)")
    parser.add_argument("--java-opts", default=os.environ.get("JVM_ARGS", "-Xmx8g"))
    args = parser.parse_args()

    tool = os.path.join(args.apalache, "bin", "apalache-mc")
    if not os.path.exists(tool):
        sys.exit(f"apalache-mc not found under {args.apalache}: unpack {APALACHE_URL} there or pass --apalache")
    if args.mode == "sweep":
        return sweep(args)
    verdict, secs, work = query(args, args.mode, args.inv, args.hyp, args.next, args.timeout, args.keep)
    log = os.path.join(work, "apalache.out")
    if os.path.exists(log):
        with open(log, encoding="utf-8") as out:
            for line in out:
                if re.search(r"E@|EXITCODE|Checker reports|State \d+:|It took me|counterexample", line):
                    print(line, end="")
    if verdict == "timeout":
        print(f"timeout after {args.timeout}s; work directory {work}")
        return 2
    print(f"{verdict} {args.mode} {args.scope}; work directory {work}")
    return 0 if verdict == "ok" else 1


if __name__ == "__main__":
    sys.exit(main())
