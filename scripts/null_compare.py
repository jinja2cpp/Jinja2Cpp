#!/usr/bin/env python3
"""Report comparisons with nullptr inside conditions (docs/tasks/0054, 0058).

    scripts/null_compare.py                      # whole tree
    scripts/null_compare.py --changed origin/master --github --fail

House style is `if (ptr)` / `if (!ptr)`. clang-tidy has no check that flags the explicit
form, so this runs scripts/null_compare.query with clang-query over every translation unit
of the compile database and prints one `file:line:col` per match in the repository
(vendored files from .clang-format-ignore excluded). --changed keeps only matches on lines
the diff against that ref adds; --github prints GitHub annotations; --fail exits 1 when
anything is reported. --fix rewrites the matches in place: `X != nullptr` becomes `X`
and `X == nullptr` becomes `!X` (parenthesised unless `X` is a postfix expression);
matches it cannot rewrite (a range spanning lines) are listed for hand editing.
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


POSTFIX = re.compile(r"[A-Za-z_~]\w*(?:(?:::|\.)[A-Za-z_~]\w*|<>|\(\)|\[\])*")
COMPARISON = re.compile(r"^(?:nullptr\s*(==|!=)\s*(?P<rhs>.+)|(?P<lhs>.+?)\s*(==|!=)\s*nullptr)$", re.S)


def is_postfix(expr):
    """True for `p`, `a.b`, `p->q()`, `GetIf<T>(&v)`: needs no parentheses after `!`."""
    flat, depth = [], 0
    # `->` is member access; template arguments nest like calls and subscripts.
    for ch in expr.replace("->", "."):
        if ch in "([<":
            if depth == 0:
                flat.append(ch)
            depth += 1
        elif ch in ")]>":
            depth -= 1
            if depth == 0:
                flat.append(ch)
        elif depth == 0:
            flat.append(ch)
    return bool(POSTFIX.fullmatch("".join(flat)))


def rewrite(text):
    """`X != nullptr` -> `X`, `X == nullptr` -> `!X` (parenthesised unless postfix)."""
    m = COMPARISON.match(text)
    if not m:
        return None
    operand = (m.group("rhs") or m.group("lhs")).strip()
    op = m.group(1) or m.group(4)
    if op == "!=":
        return operand
    return "!" + (operand if is_postfix(operand) else f"({operand})")


def underline_length(caret_line):
    """Length of the `^~~~` underline clang prints below a single-line range, else None."""
    bar = caret_line.find("|")
    if bar < 0:
        return None
    m = re.search(r"\^~*", caret_line[bar + 1:])
    return len(m.group(0)) if m else None


def apply_fixes(found_ranges):
    """found_ranges: {(rel, line, col): length or None}. Rewrites files in place."""
    by_file, manual = {}, []
    for (rel, line, col), length in found_ranges.items():
        by_file.setdefault(rel, []).append((line, col, length))
    fixed = 0
    for rel, sites in by_file.items():
        path = os.path.join(REPO, rel)
        lines = open(path, encoding="utf-8").read().split("\n")
        # Right to left, bottom to top, so earlier offsets stay valid.
        for line, col, length in sorted(sites, reverse=True):
            src = lines[line - 1]
            start = col - 1
            new = rewrite(src[start:start + length]) if length else None
            if new is None:
                manual.append(f"{rel}:{line}:{col}")
                continue
            lines[line - 1] = src[:start] + new + src[start + length:]
            fixed += 1
        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))
    return fixed, manual


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
    parser.add_argument("--fix", action="store_true", help="rewrite the matches in place")
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
    found = {}
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        for output in pool.map(run, units):
            lines = output.splitlines()
            for i, line in enumerate(lines):
                m = MATCH.match(line)
                if not m:
                    continue
                path = os.path.normpath(m.group(1))
                rel = os.path.relpath(path, REPO)
                if rel.startswith("..") or rel.split(os.sep)[0] not in ("src", "include", "test"):
                    continue
                if any(fnmatch.fnmatch(rel, p) for p in skip):
                    continue
                # clang prints the source line and a  underline below the location.
                length = underline_length(lines[i + 2]) if i + 2 < len(lines) else None
                found[(rel, int(m.group(2)), int(m.group(3)))] = length

    if args.changed:
        lines = changed_lines(args.changed)
        found = {f: n for f, n in found.items() if f[1] in lines.get(f[0], ())}

    if args.fix:
        fixed, manual = apply_fixes(found)
        print(f"{fixed} comparison(s) rewritten", file=sys.stderr)
        for site in sorted(manual):
            print(f"{site}: rewrite by hand", file=sys.stderr)
        return 1 if manual else 0

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
