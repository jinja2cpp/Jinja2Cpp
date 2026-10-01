#!/usr/bin/env python3
"""Group the active tasks in docs/tasks/ into waves that can run in parallel.

Two tasks conflict when the paths in their `touches:` front matter overlap: run in
parallel (two project threads, two PRs) they would edit the same files and one of
them would have to rebase onto the other. Tasks in the same wave have disjoint
`touches` and no `depends` between them, so each can get its own thread, branch and
PR. A task without `touches` is treated as touching everything and runs alone.

Usage: scripts/task_batches.py [--all] [--json]
  --all   include tasks whose status is done or dropped
  --json  machine-readable output
"""
import json
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TASKS = ROOT / "docs" / "tasks"
INACTIVE = {"done", "dropped"}
PRIORITY = {"high": 0, "medium": 1, "low": 2}
WILDCARDS = re.compile(r"[*?\[]")


def front_matter(path):
    """Parse the `key: value` / `key: [a, b]` / `- item` subset used in task files."""
    lines = path.read_text(encoding="utf-8").splitlines()
    if not lines or lines[0].strip() != "---":
        return {}
    meta, key = {}, None
    for line in lines[1:]:
        if line.strip() == "---":
            break
        item = re.match(r"\s+-\s+(.*)", line) or re.match(r"-\s+(.*)", line)
        if item and key:
            meta.setdefault(key, []).append(unquote(item.group(1)))
            continue
        m = re.match(r"([\w-]+):\s*(.*)", line)
        if not m:
            continue
        key, value = m.group(1), m.group(2).strip()
        if value.startswith("[") and value.endswith("]"):
            meta[key] = [unquote(v) for v in value[1:-1].split(",") if v.strip()]
        elif value:
            meta[key] = unquote(value)
    return meta


def unquote(value):
    value = value.strip()
    return value[1:-1] if len(value) > 1 and value[0] == value[-1] and value[0] in "'\"" else value


def as_list(value):
    if value is None:
        return []
    return value if isinstance(value, list) else [value]


def glob_regex(pattern):
    """`**` spans directories, `*` and `?` stay within one; a trailing `/` means the whole directory."""
    if pattern.endswith("/"):
        pattern += "**"
    out, i = "", 0
    while i < len(pattern):
        if pattern.startswith("**", i):
            out, i = out + ".*", i + 2
        elif pattern[i] == "*":
            out, i = out + "[^/]*", i + 1
        elif pattern[i] == "?":
            out, i = out + "[^/]", i + 1
        else:
            out, i = out + re.escape(pattern[i]), i + 1
    return re.compile(out + r"\Z")


def static_prefix(pattern):
    m = WILDCARDS.search(pattern)
    return pattern if not m else pattern[: m.start()]


def is_glob(pattern):
    return pattern.endswith("/") or WILDCARDS.search(pattern) is not None


def under(path, directory):
    directory = directory.rstrip("/") + "/"
    return path.startswith(directory)


def overlap(a, b, files):
    """Shared paths of two patterns, or a reason string when they overlap only by prefix."""
    ra, rb = glob_regex(a), glob_regex(b)
    shared = sorted(f for f in files if ra.match(f) and rb.match(f))
    if shared:
        return shared
    if not is_glob(a) and not is_glob(b):
        return [a] if a == b else []
    if not is_glob(b):
        a, b, ra, rb = b, a, rb, ra
    if not is_glob(a):  # a literal, b glob: a may be a file not created yet
        return [a] if rb.match(a) or under(static_prefix(b), a) else []
    pa, pb = static_prefix(a), static_prefix(b)  # both globs: conservative
    if pa.startswith(pb) or pb.startswith(pa):
        return [f"{a} ~ {b}"]
    return []


def load(include_all):
    tasks = []
    for path in sorted(TASKS.glob("[0-9][0-9][0-9][0-9]-*.md")):
        meta = front_matter(path)
        status = meta.get("status", "open")
        if status in INACTIVE and not include_all:
            continue
        tasks.append({
            "id": path.name[:4],
            "file": str(path.relative_to(ROOT)),
            "status": status,
            "priority": meta.get("priority", "medium"),
            "area": meta.get("area", ""),
            "depends": [str(d).zfill(4) for d in as_list(meta.get("depends"))],
            "touches": as_list(meta.get("touches")),
        })
    return tasks


def conflicts(tasks, files):
    result = {}
    for i, t in enumerate(tasks):
        for u in tasks[i + 1:]:
            if not t["touches"] or not u["touches"]:
                continue
            shared = sorted({p for a in t["touches"] for b in u["touches"] for p in overlap(a, b, files)})
            if shared:
                result[(t["id"], u["id"])] = shared
    return result


def waves(tasks, clash):
    """Greedy colouring by priority: a task joins the earliest wave after its dependencies
    that holds nothing it conflicts with. Tasks without `touches` get a wave of their own."""
    order = sorted(tasks, key=lambda t: (PRIORITY.get(t["priority"], 1), t["id"]))
    wave_of, result, pending = {}, [], list(order)
    while pending:
        progressed = False
        for t in list(pending):
            deps = [d for d in t["depends"] if any(x["id"] == d for x in tasks)]
            if any(d not in wave_of for d in deps):
                continue
            start = max((wave_of[d] + 1 for d in deps), default=0)
            w = start
            while w < len(result) and not fits(t, result[w], clash):
                w += 1
            if w == len(result):
                result.append([])
            result[w].append(t)
            wave_of[t["id"]] = w
            pending.remove(t)
            progressed = True
        if not progressed:  # dependency cycle: report the rest as one final wave
            result.append(pending)
            break
    return result


def fits(task, wave, clash):
    if not task["touches"]:
        return not wave
    return all(other["touches"] and tuple(sorted((task["id"], other["id"]))) not in clash for other in wave)


def main(argv):
    include_all, as_json = "--all" in argv, "--json" in argv
    files = subprocess.run(["git", "-C", str(ROOT), "ls-files"], capture_output=True,
                           text=True, check=True).stdout.splitlines()
    tasks = load(include_all)
    clash = conflicts(tasks, files)
    plan = waves(tasks, clash)
    if as_json:
        json.dump({
            "waves": [[t["id"] for t in w] for w in plan],
            "conflicts": [{"tasks": list(k), "paths": v} for k, v in sorted(clash.items())],
            "without_touches": [t["id"] for t in tasks if not t["touches"]],
        }, sys.stdout, indent=2)
        print()
        return
    for n, wave in enumerate(plan, 1):
        print(f"Wave {n}: " + ", ".join(f"{t['id']} ({t['area']}, {t['priority']}, {t['status']})" for t in wave))
    if clash:
        print("\nConflicts (same files; run one after the other, or let one thread own them):")
        for (a, b), paths in sorted(clash.items()):
            shown = ", ".join(paths[:4]) + (f", +{len(paths) - 4} more" if len(paths) > 4 else "")
            print(f"  {a} x {b}: {shown}")
    missing = [t["id"] for t in tasks if not t["touches"]]
    if missing:
        print("\nNo `touches` (treated as touching everything, so each runs alone): " + ", ".join(missing))


if __name__ == "__main__":
    main(sys.argv[1:])
