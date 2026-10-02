#!/usr/bin/env python3
"""Report comparisons with nullptr inside conditions (docs/tasks/0054, 0058).

    scripts/null_compare.py                      # whole tree
    scripts/null_compare.py --changed origin/master --github --fail

House style is `if (ptr)` / `if (!ptr)`. clang-tidy has no check that flags the explicit
form, so this runs scripts/null_compare.query with clang-query over every translation unit
of the compile database and prints one `file:line:col` per match in the repository
(vendored files from .clang-format-ignore excluded). --changed keeps only matches on lines
the diff against that ref adds; --github prints GitHub annotations; --fail exits 1 when
anything is reported.
"""

import argparse
import concurrent.futures
import fnmatch
import json
import os
import re
import shutil
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
QUERY = os.path.join(REPO, "scripts", "null_compare.query")
MATCH = re.compile(r"^(/[^:]+):(\d+):(\d+): note: \"root\" binds here")


def clang_query(override):
    if override:
        return override
    for name in ("clang-query", "clang-query-22", "clang-query-21", "clang-query-20", "clang-query-19", "clang-query-18"):
        if shutil.which(name):
            return name
    sys.exit("clang-query not found (apt install clang-tools-18)")


def vendored():
    path = os.path.join(REPO, ".clang-format-ignore")
    lines = open(path, encoding="utf-8").read().splitlines() if os.path.exists(path) else []
    return [l.strip() for l in lines if l.strip() and not l.startswith("#")]


def changed_lines(ref):
    diff = subprocess.run(["git", "diff", "-U0", "--no-color", ref, "--"], cwd=REPO, check=True,
                          stdout=subprocess.PIPE, text=True).stdout
    result, current = {}, None
    for line in diff.splitlines():
        if line.startswith("+++ "):
            current = None if line[4:] == "/dev/null" else line[6:]
        elif line.startswith("@@") and current:
            m = re.search(r"\+(\d+)(?:,(\d+))?", line)
            start, count = int(m.group(1)), int(m.group(2) or 1)
            result.setdefault(current, set()).update(range(start, start + count))
    return result


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("-p", "--build-dir", default=os.path.join(REPO, "build"))
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 2)
    parser.add_argument("--changed", metavar="REF")
    parser.add_argument("--github", action="store_true")
    parser.add_argument("--fail", action="store_true")
    parser.add_argument("--clang-query")
    args = parser.parse_args()

    tool = clang_query(args.clang_query)
    with open(os.path.join(args.build_dir, "compile_commands.json"), encoding="utf-8") as f:
        units = sorted({os.path.normpath(os.path.join(e["directory"], e["file"])) for e in json.load(f)})
    units = [u for u in units if os.path.relpath(u, REPO).split(os.sep)[0] in ("src", "test")]

    def run(unit):
        return subprocess.run([tool, "-p", args.build_dir, "--extra-arg=-Wno-unknown-warning-option", "-f", QUERY, unit],
                              stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True).stdout

    skip = vendored()
    found = set()
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for output in pool.map(run, units):
            for line in output.splitlines():
                m = MATCH.match(line)
                if not m:
                    continue
                path = os.path.normpath(m.group(1))
                rel = os.path.relpath(path, REPO)
                if rel.startswith("..") or rel.split(os.sep)[0] not in ("src", "include", "test"):
                    continue
                if any(fnmatch.fnmatch(rel, p) for p in skip):
                    continue
                found.add((rel, int(m.group(2)), int(m.group(3))))

    if args.changed:
        lines = changed_lines(args.changed)
        found = {f for f in found if f[1] in lines.get(f[0], ())}

    for rel, line, col in sorted(found):
        text = "compare with nullptr in a condition; use `if (p)` / `if (!p)` (docs/tasks/0058)"
        if args.github:
            print(f"::warning file={rel},line={line},col={col}::{text}")
        else:
            print(f"{rel}:{line}:{col}: {text}")
    print(f"{len(found)} comparison(s) with nullptr in conditions", file=sys.stderr)
    return 1 if args.fail and found else 0


if __name__ == "__main__":
    sys.exit(main())
