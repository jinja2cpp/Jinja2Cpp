#!/usr/bin/env python3
"""Stack final PR heads on current master for a merge train (docs/steward-checklist.md).

Does the mechanical part of building a train: fetch master, reset the steward branch to it,
fetch each owner branch by name and merge it, flip the finished tasks to `status: done` with
their PR link, regenerate the task index and check it. It never pushes, never opens or merges a
PR and never resolves a conflict: the steward judges conflicts, perf exceptions and the merge.

Usage:
  scripts/train.py --branch STEWARD_BRANCH --pr 977:claude/project-thread-x [--pr N:BRANCH ...]
                   [--done 0312=977 ...] [--remote origin] [--resume] [--dry-run]

  --pr N:BRANCH   PR number and owner branch; merged in the order given, as "Merge #N (BRANCH)"
  --done T=N      task number T becomes done and gets `pr:` of PR N (leave multi-phase tasks out)
  --resume        continue on an existing train branch (after resolving a conflict): keep its
                  merges and merge current master into it if master moved. Without it the
                  branch is always reset to current master
  --dry-run       print the plan and change nothing

Exit status: 0 train built; 2 a merge conflicted (tree left conflicted: resolve, `git commit`,
rerun with the same arguments plus --resume; PRs already in the branch are skipped); 1 any other error.
After 0: push the branch, open the draft train PR, subscribe, and wait for CI.
"""
import argparse
import pathlib
import re
import subprocess
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
TASKS = ROOT / "docs" / "tasks"
TRAILERS = ("Co-Authored-By: Claude Sonnet 5.5 <noreply@anthropic.com>\n"
            "Claude-Session: {session}")


def git(*args, check=True):
    return subprocess.run(["git", *args], cwd=ROOT, text=True, capture_output=True, check=check)


def fail(msg, code=1):
    print(f"train.py: {msg}", file=sys.stderr)
    sys.exit(code)


def parse_prs(values):
    prs = []
    for value in values:
        number, sep, branch = value.partition(":")
        if not sep or not number.isdigit() or not branch:
            fail(f"--pr wants N:BRANCH, got {value!r}")
        prs.append((int(number), branch))
    return prs


def parse_done(values):
    done = {}
    for value in values:
        task, sep, number = value.partition("=")
        if not sep or not re.fullmatch(r"\d{4}", task) or not number.isdigit():
            fail(f"--done wants TTTT=N, got {value!r}")
        done[task] = int(number)
    return done


def mark_done(task, number, repo_url):
    matches = sorted(TASKS.glob(f"{task}-*.md"))
    if len(matches) != 1:
        fail(f"task {task}: expected one file in docs/tasks, found {len(matches)}")
    path = matches[0]
    text = path.read_text(encoding="utf-8")
    head, sep, rest = text.partition("\n---\n")
    if not text.startswith("---\n") or not sep:
        fail(f"{path.name}: no front matter")
    head = re.sub(r"(?m)^status:.*$", "status: done", head, count=1)
    head = re.sub(r"(?m)^pr:.*\n?", "", head)
    head = head.rstrip("\n") + f"\npr: {repo_url}/pull/{number}"
    path.write_text(head + sep + rest, encoding="utf-8")
    return path


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--branch", required=True)
    ap.add_argument("--pr", action="append", default=[], required=True)
    ap.add_argument("--done", action="append", default=[])
    ap.add_argument("--remote", default="origin")
    ap.add_argument("--session", default="<session>", help="Claude-Session URL for the trailers")
    ap.add_argument("--resume", action="store_true")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    prs, done = parse_prs(args.pr), parse_done(args.done)
    for task, number in done.items():
        if number not in [n for n, _ in prs]:
            fail(f"--done {task}={number}: PR {number} is not in --pr")
    url = git("remote", "get-url", args.remote).stdout.strip()
    url = re.sub(r"\.git$", "", re.sub(r"^git@github\.com:", "https://github.com/", url))
    url = re.sub(r"^https://[^@/]*@", "https://", url)
    master = f"{args.remote}/master"
    if args.dry_run:
        print(f"would reset {args.branch} to {master}")
        for number, branch in prs:
            print(f"would merge #{number} from {branch}")
        for task, number in done.items():
            print(f"would set {task} done, pr {url}/pull/{number}")
        print("would run scripts/task_index.py and --check")
        return
    if git("status", "--porcelain", "--untracked-files=no").stdout.strip():
        fail("tracked files or the index have changes; commit or stash them first")
    git("fetch", args.remote, "master")
    current = git("rev-parse", "--abbrev-ref", "HEAD").stdout.strip()
    if args.resume:
        if current != args.branch:
            fail(f"--resume: check out {args.branch} first (on {current})")
        if git("merge-base", "--is-ancestor", master, "HEAD", check=False).returncode != 0:
            merged = git("merge", "--no-ff", "-m", "Merge current master into the train\n\n"
                         + TRAILERS.format(session=args.session), master, check=False)
            if merged.returncode != 0:
                print(merged.stdout + merged.stderr)
                fail("master conflicts with the train; resolve, `git commit`, rerun with --resume", 2)
    else:
        git("checkout", "-B", args.branch, master)
    for number, branch in prs:
        git("fetch", "-f", args.remote, branch)
        sha = git("rev-parse", "FETCH_HEAD").stdout.strip()
        if git("merge-base", "--is-ancestor", sha, "HEAD", check=False).returncode == 0:
            print(f"#{number}: {sha[:9]} already in {args.branch}")
            continue
        msg = f"Merge #{number} ({branch})\n\n" + TRAILERS.format(session=args.session)
        merged = git("merge", "--no-ff", "-m", msg, sha, check=False)
        if merged.returncode != 0:
            conflicts = git("diff", "--name-only", "--diff-filter=U").stdout.split()
            print(merged.stdout + merged.stderr)
            fail(f"#{number} conflicts in: {', '.join(conflicts) or '(no files; see git status)'}\n"
                 "Resolve, `git commit`, rerun with the same arguments plus --resume.", 2)
        print(f"#{number}: merged {sha[:9]}")
    if done:
        for task, number in done.items():
            print(f"{mark_done(task, number, url).name}: status done")
    subprocess.run([sys.executable, str(ROOT / "scripts" / "task_index.py")], cwd=ROOT, check=True)
    subprocess.run([sys.executable, str(ROOT / "scripts" / "task_index.py"), "--check"], cwd=ROOT, check=True)
    changed = git("status", "--porcelain", "--untracked-files=no", "--", "docs/tasks").stdout.split("\n")
    paths = [line[3:] for line in changed if line.strip()]
    if paths:
        msg = "Train: mark tasks done, refresh task index\n\n" + TRAILERS.format(session=args.session)
        git("add", "--", *paths)
        git("commit", "-m", msg, "--", *paths)
        print("committed task status and index")
    print(f"train branch {args.branch} at {git('rev-parse', '--short', 'HEAD').stdout.strip()}; "
          "next: push, open the draft train PR, subscribe, arm a send_later wake")


if __name__ == "__main__":
    main()
