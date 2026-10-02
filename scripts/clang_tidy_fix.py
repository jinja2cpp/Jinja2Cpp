#!/usr/bin/env python3
"""Apply clang-tidy fix-its to the tree (docs/tasks/0054).

    scripts/clang_tidy_fix.py --checks 'modernize-use-override,readability-qualified-auto'
    scripts/clang_tidy_fix.py --checks readability-qualified-auto --files src/filters.cpp
    scripts/clang_tidy_fix.py --checks modernize-use-override --dry-run

Runs clang-tidy on every translation unit of the compile database with --export-fixes,
then applies the fixes once with clang-apply-replacements. Two things make it safer than
`run-clang-tidy -fix` on this repository:

- every FilePath in the exported YAML is normalised. Headers reached as
  `src/binding/../internal_value.h` or `test/../src/helpers.h` are the same file as
  `src/internal_value.h`, but clang-apply-replacements deduplicates by path string and
  would apply the same edit twice (`override override`);
- fixes outside the repository, or in vendored files (.clang-format-ignore), are dropped,
  whatever the header filter says.

The configuration (checks, header filter, options) comes from .clang-tidy; --checks only
narrows it. Applied fixes are formatted with .clang-format.
"""

import argparse
import concurrent.futures
import fnmatch
import json
import os
import shutil
import subprocess
import sys
import tempfile

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def find_tool(name, override):
    if override:
        return override
    for candidate in (name, name + "-22", name + "-21", name + "-20", name + "-19", name + "-18"):
        path = shutil.which(candidate)
        if path:
            return path
    # The PyPI clang-tidy wheel ships clang-apply-replacements next to its data files.
    try:
        import clang_tidy  # type: ignore

        path = os.path.join(os.path.dirname(clang_tidy.__file__), "data", "bin", name)
        if os.path.exists(path):
            return path
    except ImportError:
        pass
    sys.exit(f"{name} not found; install it (pip install clang-tidy==22.1.8) or pass --{name}")


def vendored_patterns():
    patterns = []
    path = os.path.join(REPO, ".clang-format-ignore")
    if os.path.exists(path):
        for line in open(path, encoding="utf-8"):
            line = line.strip()
            if line and not line.startswith("#"):
                patterns.append(line)
    return patterns


def keep_file(path, vendored):
    path = os.path.normpath(path)
    if not path.startswith(REPO + os.sep):
        return False
    rel = os.path.relpath(path, REPO)
    return not any(fnmatch.fnmatch(rel, p) for p in vendored)


def translation_units(build_dir, files):
    with open(os.path.join(build_dir, "compile_commands.json"), encoding="utf-8") as f:
        db = json.load(f)
    units = sorted({os.path.normpath(os.path.join(e["directory"], e["file"])) for e in db})
    units = [u for u in units if u.startswith(REPO + os.sep) and not os.path.relpath(u, REPO).startswith(("build", "."))]
    if files:
        wanted = {os.path.normpath(os.path.join(REPO, f)) for f in files}
        units = [u for u in units if u in wanted]
    return units


def run_tidy(clang_tidy, build_dir, checks, unit, out_dir, index):
    out = os.path.join(out_dir, f"{index:04d}.yaml")
    cmd = [clang_tidy, "-p", build_dir, "--quiet", f"--export-fixes={out}",
           # The GCC compile database carries GCC-only warning flags.
           "--extra-arg=-Wno-unknown-warning-option", unit]
    if checks:
        cmd.insert(1, f"--checks=-*,{checks}")
    proc = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    return unit, proc.returncode, proc.stdout


def normalise(out_dir, vendored):
    """Rewrite file paths to their canonical form and drop out-of-tree edits."""
    import yaml  # PyYAML: preinstalled on GitHub runners, `pip install pyyaml` elsewhere

    def fix_message(msg):
        if msg.get("FilePath"):
            msg["FilePath"] = os.path.normpath(msg["FilePath"])
        reps = msg.get("Replacements") or []
        kept = [dict(r, FilePath=os.path.normpath(r["FilePath"])) for r in reps if keep_file(r["FilePath"], vendored)]
        if reps:
            msg["Replacements"] = kept
        return len(reps), len(kept)

    total = 0
    for name in sorted(os.listdir(out_dir)):
        path = os.path.join(out_dir, name)
        with open(path, encoding="utf-8") as f:
            doc = yaml.safe_load(f)
        if not doc:
            os.remove(path)
            continue
        diagnostics = []
        for diag in doc.get("Diagnostics") or []:
            had, kept = fix_message(diag["DiagnosticMessage"])
            for note in diag.get("Notes") or []:
                fix_message(note)
            # A fix that lost part of its edits would leave the code half changed.
            if had and kept != had:
                continue
            diagnostics.append(diag)
            total += kept
        doc["Diagnostics"] = diagnostics
        doc["MainSourceFile"] = os.path.normpath(doc.get("MainSourceFile", ""))
        with open(path, "w", encoding="utf-8") as f:
            yaml.safe_dump(doc, f, sort_keys=False, explicit_start=True, explicit_end=True, width=1 << 20)
    return total


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--checks", help="comma-separated checks to apply (default: all enabled in .clang-tidy)")
    parser.add_argument("--files", nargs="*", help="limit to these translation units (repo-relative)")
    parser.add_argument("-p", "--build-dir", default=os.path.join(REPO, "build"))
    parser.add_argument("-j", "--jobs", type=int, default=os.cpu_count() or 2)
    parser.add_argument("--dry-run", action="store_true", help="export and normalise fixes, do not apply")
    parser.add_argument("--keep-fixes", help="directory to keep the exported YAML in")
    parser.add_argument("--clang-tidy")
    parser.add_argument("--clang-apply-replacements")
    args = parser.parse_args()

    clang_tidy = find_tool("clang-tidy", args.clang_tidy)
    apply = find_tool("clang-apply-replacements", args.clang_apply_replacements)
    units = translation_units(args.build_dir, args.files)
    if not units:
        sys.exit("no translation units matched")

    out_dir = args.keep_fixes or tempfile.mkdtemp(prefix="tidy-fixes-")
    os.makedirs(out_dir, exist_ok=True)
    failed = []
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as pool:
        futures = [pool.submit(run_tidy, clang_tidy, args.build_dir, args.checks, u, out_dir, i) for i, u in enumerate(units)]
        for future in concurrent.futures.as_completed(futures):
            unit, code, output = future.result()
            print(f"[{'ok' if code == 0 else 'warn'}] {os.path.relpath(unit, REPO)}", flush=True)
            if "clang-diagnostic-error" in output:
                failed.append(unit)
    if failed:
        print("compiler errors in: " + ", ".join(os.path.relpath(u, REPO) for u in failed), file=sys.stderr)

    edits = normalise(out_dir, vendored_patterns())
    print(f"{edits} replacements kept in {out_dir}")
    if args.dry_run:
        return 0
    subprocess.run([apply, "-format", "-style=file", out_dir], check=True, cwd=REPO)
    if not args.keep_fixes:
        shutil.rmtree(out_dir, ignore_errors=True)
    print("applied; now build, run ctest and `git clang-format --diff origin/master`")
    return 0


if __name__ == "__main__":
    sys.exit(main())
