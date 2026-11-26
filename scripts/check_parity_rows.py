#!/usr/bin/env python3
"""Check the hand-written feature rows of docs/parity.md against the task files and the corpus.

A feature row whose status cell holds a red or yellow mark (the "missing or broken" and
"works with differences" marks of the legend) must say why it is not a match, in one
of two ways:

- it cites an open task (`docs/tasks/NNNN-*.md` with a status other than `done` or
  `dropped`), or
- its evidence names a case listed in `test/parity/divergences/` (a deliberate or tracked
  divergence; the allow-list line carries the task and the reason).

A row that cites only done tasks and no divergence line is stale: the task fixed the
gap and the row was not updated (docs/tasks/0235). A cited task that does not exist
fails too. Evidence ids are the backticked names in the row; a name without a dot
belongs to the area of the section heading (`## Title (`area`)`), and `*` is a glob.

Usage: scripts/check_parity_rows.py   (exit 1 and one line per bad row)
"""
import fnmatch
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
PAGE = ROOT / "docs" / "parity.md"
INACTIVE = {"done", "dropped"}
MARKS = ("❌", "\U0001f7e1")  # cross mark, yellow circle


def task_status():
    out = {}
    for path in (ROOT / "docs" / "tasks").glob("[0-9][0-9][0-9][0-9]-*.md"):
        m = re.search(r"^status:\s*(\S+)", path.read_text(encoding="utf-8"), re.M)
        out[path.name[:4]] = m.group(1) if m else ""
    return out


def divergence_ids():
    ids = set()
    for path in (ROOT / "test" / "parity" / "divergences").glob("*.txt"):
        for line in path.read_text(encoding="utf-8").splitlines():
            if line.strip() and not line.startswith("#"):
                name = line.split()[0]
                ids.add(name[len("wide."):] if name.startswith("wide.") else name)
    return ids


def main():
    status, diverging = task_status(), divergence_ids()
    bad, area, checked = [], None, 0
    for number, line in enumerate(PAGE.read_text(encoding="utf-8").splitlines(), 1):
        if line.startswith("## "):
            m = re.search(r"\(`(\w+)`\)\s*$", line)
            area = m.group(1) if m else None
        if not line.startswith("|") or not any(mark in line for mark in MARKS):
            continue
        checked += 1
        tasks = re.findall(r"\b0\d{3}\b", re.sub(r"`[^`]*`", "", line))
        for t in tasks:
            if t not in status:
                bad.append((number, f"cites task {t}, which does not exist"))
        names = [e if "." in e else f"{area}.{e}" for e in re.findall(r"`([^`]+)`", line) if area]
        diverges = any(fnmatch.filter(diverging, n) for n in names)
        if diverges:
            continue
        known = [t for t in tasks if t in status]
        if not tasks:
            bad.append((number, "marked as a gap but cites no task and no divergence line"))
        elif known and all(status[t] in INACTIVE for t in known):
            bad.append((number, f"cites only closed tasks {', '.join(known)} and no divergence line: "
                                "re-check the row and mark it as a match"))
    for number, why in bad:
        print(f"docs/parity.md:{number}: {why}")
    print(f"{checked} gap rows checked, {len(bad)} bad")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
