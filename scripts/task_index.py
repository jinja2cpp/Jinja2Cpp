#!/usr/bin/env python3
"""Regenerate the index table at the end of docs/tasks/README.md from the task files.

Each row comes from one docs/tasks/NNNN-*.md file: the number and file name, the
title from its `# ` heading, and `area`, `priority` and `status` from its front
matter. The task file is the only place a status or title is edited; the index
follows it. Task PRs change only their own task file and leave the table alone, so
two of them never conflict in README.md; the merge steward runs this script after a
merge batch and lands the result as one docs-only commit (docs/tasks/0157).

Usage: scripts/task_index.py [--check [--strict]]
  (no flag)  rewrite the table in place
  --check    print the rows that would change, as a GitHub warning; exit 0
  --strict   with --check, exit 1 when the table is out of date
"""
import difflib
import pathlib
import sys

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))
from task_batches import TASKS, front_matter  # noqa: E402

README = TASKS / "README.md"
HEADER = "| # | Task | Area | Priority | Status |"


def title(path):
    for line in path.read_text(encoding="utf-8").splitlines():
        if line.startswith("# "):
            return line[2:].strip()
    return path.stem[5:].replace("-", " ")


def rows():
    out = [HEADER, "|---|---|---|---|---|"]
    for path in sorted(TASKS.glob("[0-9][0-9][0-9][0-9]-*.md")):
        meta = front_matter(path)
        cells = [meta.get(key, "") for key in ("area", "priority", "status")]
        out.append(f"| [{path.name[:4]}]({path.name}) | {title(path)} | " + " | ".join(cells) + " |")
    return out


def main():
    lines = README.read_text(encoding="utf-8").splitlines()
    try:
        start = lines.index(HEADER)
    except ValueError:
        sys.exit(f"{README}: no index table header {HEADER!r}")
    end = start
    while end < len(lines) and lines[end].startswith("|"):
        end += 1
    new = lines[:start] + rows() + lines[end:]
    if new == lines:
        return 0
    if "--check" in sys.argv[1:]:
        sys.stdout.writelines(difflib.unified_diff(
            [l + "\n" for l in lines[start:end]], [l + "\n" for l in rows()],
            "README.md (index)", "task files"))
        level = "error" if "--strict" in sys.argv[1:] else "warning"
        print(f"::{level}::docs/tasks/README.md index differs from the task files. "
              "The merge steward runs: python3 scripts/task_index.py")
        return 1 if level == "error" else 0
    README.write_text("\n".join(new) + "\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    sys.exit(main())
