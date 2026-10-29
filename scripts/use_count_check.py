#!/usr/bin/env python3
"""Report shared_ptr/weak_ptr use_count() calls (docs/tasks/0339, 0343).

    scripts/use_count_check.py                      # whole tree
    scripts/use_count_check.py --changed origin/master --github --fail

use_count() is a relaxed load, so "the count is 1, nobody else holds it, write in place" races
with the release of the last other holder on another thread (0339). No Clang analysis sees
that: thread safety analysis knows nothing about atomics, and TSan sees it only when the timing
happens. This runs scripts/use_count.query with clang-query over every translation unit of the
compile database and prints one `file:line:col` per call in src/ and include/ (tests count
references on one thread to check ownership). A call that is right (an assertion on the owning
thread, say) carries `use_count: <reason>` in a comment on its line. --changed keeps only calls on lines the diff against that ref adds; --github prints
GitHub annotations; --fail exits 1 when anything is reported.
"""

import argparse
import concurrent.futures
import fnmatch
import json
import os
import subprocess
import sys

from null_compare import MATCH, REPO, changed_lines, clang_query, vendored

QUERY = os.path.join(REPO, "scripts", "use_count.query")
ALLOW = "use_count: "


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
                rel = os.path.relpath(os.path.normpath(m.group(1)), REPO)
                if rel.startswith("..") or rel.split(os.sep)[0] not in ("src", "include"):
                    continue
                if any(fnmatch.fnmatch(rel, p) for p in skip):
                    continue
                found.add((rel, int(m.group(2)), int(m.group(3))))

    sources = {}
    for rel, line, _ in found:
        sources.setdefault(rel, open(os.path.join(REPO, rel), encoding="utf-8").read().split("\n"))
    found = {f for f in found if ALLOW not in sources[f[0]][f[1] - 1]}

    if args.changed:
        lines = changed_lines(args.changed)
        found = {f for f in found if f[1] in lines.get(f[0], ())}

    for rel, line, col in sorted(found):
        text = "use_count() is a relaxed load: it does not order the other holder's release before what follows (docs/tasks/0339)"
        if args.github:
            print(f"::warning file={rel},line={line},col={col}::{text}")
        else:
            print(f"{rel}:{line}:{col}: {text}")
    print(f"{len(found)} use_count() call(s)", file=sys.stderr)
    return 1 if args.fail and found else 0


if __name__ == "__main__":
    sys.exit(main())
