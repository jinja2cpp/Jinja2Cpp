#!/usr/bin/env python3
"""Runs a libFuzzer target once over every input of a corpus, without generating new ones.

This is the pull-request mode of .github/workflows/fuzz.yml (docs/tasks/0185): the same
corpus gives the same verdict on every run, so the job fails only when the change under
test makes a known input crash. Finding new inputs is the nightly job's work.

libFuzzer stops at the first input that crashes, times out or runs out of memory; this
script records that input and carries on with the next one. Crashes and leaks fail the run
and their inputs are copied to the artifacts directory. Timeouts and out-of-memory inputs
are reported only: Jinja2C++ has no sandbox limits yet (docs/tasks/0097).
"""
import argparse
import pathlib
import shutil
import subprocess
import sys

BATCH = 256
FLAGS = ["-timeout=25", "-rss_limit_mb=2560", "-runs=1"]


def classify(log):
    if "ERROR: libFuzzer: timeout" in log:
        return "timeout"
    if "ERROR: libFuzzer: out-of-memory" in log:
        return "oom"
    return "crash"


def replay(binary, files, artifacts, log_file):
    outcomes = {"crash": [], "timeout": [], "oom": []}
    pos = 0
    while pos < len(files):
        batch = files[pos:pos + BATCH]
        proc = subprocess.run([binary, *FLAGS, *map(str, batch)], stdout=subprocess.PIPE,
                              stderr=subprocess.STDOUT, text=True, errors="replace")
        log_file.write(proc.stdout)
        if proc.returncode == 0:
            pos += len(batch)
            continue
        # libFuzzer prints "Running: <file>" before each input, so the last one is the culprit
        running = [line[len("Running: "):] for line in proc.stdout.splitlines() if line.startswith("Running: ")]
        if not running:
            sys.exit(f"{binary} exited with status {proc.returncode} before running any input:\n"
                     + proc.stdout[-4000:])
        culprit = pathlib.Path(running[-1])
        index = pos + [str(f) for f in batch].index(str(culprit))
        kind = classify(proc.stdout[proc.stdout.rfind("Running: " + str(culprit)):])
        outcomes[kind].append(culprit)
        if kind == "crash":
            shutil.copyfile(culprit, artifacts / f"crash-{culprit.name}")
            print(proc.stdout[proc.stdout.rfind("Running: " + str(culprit)):][-6000:])
        pos = index + 1
    return outcomes


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("binary")
    parser.add_argument("corpus", nargs="+", type=pathlib.Path, help="corpus directories or files")
    parser.add_argument("--artifacts", type=pathlib.Path, default=pathlib.Path("artifacts"))
    parser.add_argument("--log", type=pathlib.Path, default=pathlib.Path("fuzz.log"))
    parser.add_argument("--summary", type=pathlib.Path, help="append a Markdown summary here")
    args = parser.parse_args()

    files = sorted(f for c in args.corpus for f in (c.iterdir() if c.is_dir() else [c]) if f.is_file())
    args.artifacts.mkdir(parents=True, exist_ok=True)
    with args.log.open("w") as log_file:
        outcomes = replay(args.binary, files, args.artifacts, log_file)

    name = pathlib.Path(args.binary).name
    lines = [f"### replay {name}", "",
             f"{len(files)} inputs; crashes: {len(outcomes['crash'])}, timeouts: {len(outcomes['timeout'])}, "
             f"out of memory: {len(outcomes['oom'])}"]
    for kind in ("crash", "timeout", "oom"):
        lines += [f"- {kind}: `{f.name}`" for f in outcomes[kind]]
    print("\n".join(lines))
    if args.summary:
        with args.summary.open("a") as summary:
            summary.write("\n".join(lines) + "\n")
    if outcomes["crash"]:
        print(f"::error::{name} crashed or leaked on {len(outcomes['crash'])} corpus input(s)")
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
