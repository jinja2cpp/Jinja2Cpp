#!/usr/bin/env python3
"""Run the latest-LLVM lifetime, thread-safety and cross-TU analyses and report them (docs/tasks/0345).

    scripts/clang_analysis.py -p build --clang clang++ --out clang-analysis
    scripts/clang_analysis.py -p build --clang clang++ --out clang-analysis --ctu

The compile pass runs `--clang -fsyntax-only` over every translation unit of the compile
database that matches --files (default: src/ and test/), with the build's own flags minus
-Werror, plus ANALYSIS_FLAGS: Lifetime Safety (strict, invalidation, suggestions and
validations, with inference and the whole-TU mode), -Wthread-safety and
-Wthread-safety-beta. Any other warning the newer compiler gives under the project's flags
is reported too, as "other".

--ctu also runs the static analyzer with cross-translation-unit analysis over the units
that match --ctu-files (default: src/): clang-extdef-mapping indexes the definitions, and
each unit imports the ones it calls by parsing their unit on demand. The analyzer runs its
default checkers plus CTU_CHECKERS and writes SARIF into the same report. Use an unwrapped
clang (no wrapper script that adds include paths): the analyzer parses imported units
in-process with the arguments from the compile database.

Findings are deduplicated by location, check and message, so a header seen by many
translation units counts once. A finding that matches a line of --suppressions is listed
as suppressed, with its reason, and a suppression that matches nothing is listed as
stale. The report never fails the caller: the exit status is 0 unless the script itself
cannot run. Outputs in --out: report.md (also appended to $GITHUB_STEP_SUMMARY when set),
findings.json, compile.log and, with --ctu, ctu.log and ctu/ (index and SARIF).

Suppression lines (scripts/clang_analysis_suppressions.txt), globs as in fnmatch:

    <check glob> <path glob>[:<line>] ["<message glob>"]  # reason (required)
"""

import argparse
import concurrent.futures
import fnmatch
import json
import os
import re
import shlex
import shutil
import subprocess
import sys

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DEFAULT_SUPPRESSIONS = os.path.join(REPO, "scripts", "clang_analysis_suppressions.txt")
DIAG = re.compile(r"^(?P<path>[^\s:][^:]*):(?P<line>\d+):(?P<col>\d+): (?P<level>warning|error): "
                  r"(?P<msg>.*?)(?: \[(?P<flags>[^\]]+)\])?$")

ANALYSIS_FLAGS = [
    "-Wlifetime-safety-all",
    # Interprocedural inside a translation unit: infer lifetimebound for callees and analyse
    # in call-graph post-order, so the inferred annotations reach their callers.
    "-Xclang", "-flifetime-safety-inference",
    "-Xclang", "-fexperimental-lifetime-safety-tu-analysis",
    # Suggestions name the project's macro (docs/tasks/0343) instead of the attribute.
    "-Xclang", "-lifetime-safety-lifetimebound-macro=JINJA2CPP_LIFETIMEBOUND",
    "-Wthread-safety", "-Wthread-safety-beta",
]
CTU_CHECKERS = [
    "optin.cplusplus.UninitializedObject",
    "alpha.cplusplus.InvalidatedIterator",
    "alpha.cplusplus.MismatchedIterator",
    "alpha.cplusplus.UseAfterLifetimeEnd",
]
# Build flags that would turn a finding into a failed compile, or that each pass sets itself.
DROP = re.compile(r"^-Werror(=.*)?$|^-W(no-)?error=.*|^-c$|^-f(no-)?color-diagnostics$")
CATEGORIES = ("compile error", "lifetime", "thread safety", "static analyzer (CTU)", "other",
              "lifetime suggestions")


def category(check):
    if check.startswith("-Wlifetime-safety") and check.endswith("suggestions"):
        return "lifetime suggestions"
    if check.startswith(("-Wlifetime-safety", "-Wdangling", "-Wreturn-stack-address")):
        return "lifetime"
    if check.startswith("-Wthread-safety"):
        return "thread safety"
    if check.startswith("analyzer:"):
        return "static analyzer (CTU)"
    if check == "error":
        return "compile error"
    return "other"


def rel(path, directory):
    """Repository-relative path, or None outside the repository or in fetched dependencies."""
    path = os.path.realpath(os.path.join(directory, path))
    if os.path.commonpath([path, REPO]) != REPO:
        return None
    path = os.path.relpath(path, REPO)
    return None if "/_deps/" in "/" + path else path


def entries(cdb, files):
    with open(os.path.join(cdb, "compile_commands.json"), encoding="utf-8") as f:
        db = json.load(f)
    pattern = re.compile(files)
    for e in db:
        path = rel(e["file"], e["directory"])
        if path and pattern.search(path):
            yield e, path, e.get("arguments") or shlex.split(e["command"])


def strip_args(clang, args):
    out, skip = [clang], False
    for a in args[1:]:
        if skip:
            skip = False
        elif a == "-o":
            skip = True
        elif not DROP.match(a):
            out.append(a)
    return out


def run(cmd, cwd, timeout):
    try:
        proc = subprocess.run(cmd, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True,
                              errors="replace", check=False, timeout=timeout)
        return proc.returncode, proc.stdout
    except subprocess.TimeoutExpired as exc:
        out = exc.stdout.decode(errors="replace") if isinstance(exc.stdout, bytes) else (exc.stdout or "")
        return -1, f"{out}\ntimed out after {timeout} s\n"


def parse_compiler(output, directory):
    for line in output.splitlines():
        m = DIAG.match(line)
        if not m:
            continue
        path = rel(m["path"], directory)
        if not path:
            continue
        flag = (m["flags"] or "").split(",")[0]
        check = flag if flag.startswith("-W") else ("error" if m["level"] == "error" else "-W(unnamed)")
        yield {"check": check, "path": path, "line": int(m["line"]), "col": int(m["col"]), "message": m["msg"]}


def parse_sarif(directory):
    for root, _, names in os.walk(directory):
        for name in sorted(names):
            if not name.endswith(".sarif"):
                continue
            try:
                with open(os.path.join(root, name), encoding="utf-8") as f:
                    doc = json.load(f)
            except (OSError, json.JSONDecodeError):
                continue
            for run_ in doc.get("runs", []):
                for res in run_.get("results", []):
                    phys = (res.get("locations") or [{}])[0].get("physicalLocation", {})
                    uri = phys.get("artifactLocation", {}).get("uri", "")
                    path = rel(re.sub(r"^file://", "", uri), REPO) if uri else None
                    if path:
                        region = phys.get("region", {})
                        yield {"check": "analyzer:" + res.get("ruleId", "?"), "path": path,
                               "line": region.get("startLine", 0), "col": region.get("startColumn", 0),
                               "message": res.get("message", {}).get("text", "")}


def run_ctu(opts, log):
    """Index the definitions, then analyse each unit with on-demand import; return failed units."""
    ctu_dir = os.path.abspath(os.path.join(opts.out, "ctu"))
    sarif_dir = os.path.join(ctu_dir, "sarif")
    shutil.rmtree(ctu_dir, ignore_errors=True)
    os.makedirs(sarif_dir)
    units = [(e, path, strip_args(opts.ctu_clang, args) + opts.ctu_arg)
             for e, path, args in entries(opts.cdb, opts.ctu_files)]
    with open(os.path.join(ctu_dir, "compile_commands.json"), "w", encoding="utf-8") as f:
        json.dump([{"directory": e["directory"], "file": e["file"], "arguments": argv + ["-c"]}
                   for e, _, argv in units], f, indent=1)
    # JSON is valid YAML, which is what ctu-invocation-list reads.
    with open(os.path.join(ctu_dir, "invocations.yaml"), "w", encoding="utf-8") as f:
        json.dump({os.path.realpath(os.path.join(e["directory"], e["file"])): argv for e, _, argv in units}, f,
                  indent=1)
    sources = [os.path.realpath(os.path.join(e["directory"], e["file"])) for e, _, _ in units]
    status, mapping = 0, []
    for i in range(0, len(sources), 16):
        # stdout only: the progress lines on stderr would interleave with the index.
        proc = subprocess.run([opts.extdef_mapping, "-p", ctu_dir] + sources[i:i + 16], cwd=REPO,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, errors="replace",
                              check=False, timeout=opts.timeout)
        status |= proc.returncode
        mapping += [line for line in proc.stdout.splitlines() if re.match(r"^\d+:", line)]
    with open(os.path.join(ctu_dir, "externalDefMap.txt"), "w", encoding="utf-8") as f:
        f.write("\n".join(sorted(set(mapping))) + "\n")
    log.write(f"==> clang-extdef-mapping: {len(mapping)} definitions (exit {status})\n")

    config = ["experimental-enable-naive-ctu-analysis=true", f"ctu-dir={ctu_dir}",
              f"ctu-invocation-list={os.path.join(ctu_dir, 'invocations.yaml')}",
              # The iterator checkers require it.
              "aggressive-binary-operation-simplification=true"]
    extra = []
    for c in config:
        extra += ["-Xclang", "-analyzer-config", "-Xclang", c]
    # A checker this clang lacks would fail every unit.
    known = run([opts.ctu_clang, "-cc1", "-analyzer-checker-help-alpha", "-analyzer-checker-help"], REPO, 60)[1]
    for c in CTU_CHECKERS:
        if re.search(rf"^\s*{re.escape(c)}\b", known, re.M):
            extra += ["-Xclang", f"-analyzer-checker={c}"]
        else:
            log.write(f"==> checker {c} is unknown to {opts.ctu_clang}, skipped\n")

    def analyse(n, unit):
        e, path, argv = unit
        out = os.path.join(sarif_dir, f"{n:03d}-{os.path.basename(path)}.sarif")
        cmd = argv + ["--analyze", "--analyzer-output", "sarif", "-o", out] + extra
        return path, *run(cmd, e["directory"], opts.timeout)

    failed = 0
    with concurrent.futures.ThreadPoolExecutor(opts.jobs) as pool:
        for path, code, out in pool.map(lambda a: analyse(*a), enumerate(units)):
            log.write(f"==> analyzer {path} (exit {code})\n{out}")
            failed += code != 0
    return units, failed, sarif_dir


def load_suppressions(path):
    rules = []
    if not path or not os.path.exists(path):
        return rules
    with open(path, encoding="utf-8") as f:
        for n, raw in enumerate(f, 1):
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            body, sep, reason = line.partition("  #")
            fields = shlex.split(body)
            if not sep or not reason.strip() or len(fields) not in (2, 3):
                sys.exit(f'{path}:{n}: expected \'<check> <path>[:line] ["<message>"]  # reason\'')
            where, _, at = fields[1].partition(":")
            rules.append({"line": n, "check": fields[0], "path": where, "at": int(at) if at else None,
                          "message": fields[2] if len(fields) == 3 else "*", "reason": reason.strip(), "hits": 0})
    return rules


def suppress(finding, rules):
    for r in rules:
        if fnmatch.fnmatchcase(finding["check"], r["check"]) and fnmatch.fnmatchcase(finding["path"], r["path"]) \
                and r["at"] in (None, finding["line"]) and fnmatch.fnmatchcase(finding["message"], r["message"]):
            r["hits"] += 1
            return r["reason"]
    return None


def report(findings, rules, stats, limit):
    lines = [f"## Clang analysis ({stats['clang']})", "",
             f"Compile pass: {stats['units']} translation units, {stats['failed']} failed. "
             + (f"Cross-TU analyzer ({stats['ctu_clang']}): {stats['ctu_units']} units, "
                f"{stats['ctu_failed']} failed. "
                if "ctu_units" in stats else "")
             + "Report only, nothing here fails the job (docs/tasks/0345). Triage: a real defect "
             "becomes a task; a false positive gets a line with its reason in "
             "`scripts/clang_analysis_suppressions.txt`.", "",
             "| category | check | open | suppressed |", "|---|---|---:|---:|"]
    checks = sorted({f["check"] for f in findings}, key=lambda c: (CATEGORIES.index(category(c)), c))
    for c in checks:
        mine = [f for f in findings if f["check"] == c]
        s = sum(1 for f in mine if "suppressed" in f)
        lines.append(f"| {category(c)} | `{c}` | {len(mine) - s} | {s} |")
    if not checks:
        lines.append("| - | - | 0 | 0 |")
    for cat in CATEGORIES:
        group = [f for f in findings if category(f["check"]) == cat and "suppressed" not in f]
        if not group:
            continue
        lines += ["", f"### {cat} ({len(group)})", ""]
        for f in group[:limit]:
            lines.append(f"- `{f['path']}:{f['line']}:{f['col']}` `{f['check']}`: {f['message']}")
        if len(group) > limit:
            lines.append(f"- ... {len(group) - limit} more in findings.json")
    stale = [r for r in rules if not r["hits"]]
    if stale:
        lines += ["", f"### stale suppressions ({len(stale)})", ""]
        lines += [f"- line {r['line']}: `{r['check']} {r['path']}` ({r['reason']})" for r in stale]
    return "\n".join(lines) + "\n"


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("-p", dest="cdb", default="build", help="build directory with compile_commands.json")
    ap.add_argument("--clang", required=True, help="clang++ of the LLVM to analyse with")
    ap.add_argument("--files", default=r"^(src|test)/.*\.cpp$", help="compile pass: regex over repository paths")
    ap.add_argument("--ctu", action="store_true", help="also run the cross-TU static analyzer")
    ap.add_argument("--ctu-files", default=r"^src/.*\.cpp$", help="cross-TU analyzer: regex over repository paths")
    ap.add_argument("--ctu-clang", help="clang++ for the cross-TU analyzer (default: --clang)")
    ap.add_argument("--ctu-arg", action="append", default=[], help="extra argument for the cross-TU analyzer")
    ap.add_argument("--extdef-mapping", help="clang-extdef-mapping (default: next to --ctu-clang)")
    ap.add_argument("--out", default="clang-analysis", help="output directory")
    ap.add_argument("--suppressions", default=DEFAULT_SUPPRESSIONS)
    ap.add_argument("--limit", type=int, default=100, help="findings listed per category in report.md")
    ap.add_argument("--timeout", type=int, default=1800, help="seconds per translation unit")
    ap.add_argument("-j", "--jobs", type=int, default=os.cpu_count())
    opts = ap.parse_args()
    opts.clang = shutil.which(opts.clang) or opts.clang
    opts.ctu_clang = shutil.which(opts.ctu_clang or opts.clang) or opts.ctu_clang
    opts.extdef_mapping = opts.extdef_mapping or os.path.join(os.path.dirname(opts.ctu_clang), "clang-extdef-mapping")

    os.makedirs(opts.out, exist_ok=True)
    def version(clang):
        return subprocess.run([clang, "--version"], stdout=subprocess.PIPE, text=True,
                              check=True).stdout.splitlines()[0].strip()

    stats = {"clang": version(opts.clang)}
    if opts.ctu:
        stats["ctu_clang"] = version(opts.ctu_clang)
    rules = load_suppressions(opts.suppressions)
    work = list(entries(opts.cdb, opts.files))
    found = {}

    def add(f):
        found.setdefault((f["path"], f["line"], f["col"], f["check"], f["message"]), f)

    def compile_one(unit):
        e, _, args = unit
        cmd = strip_args(opts.clang, args) + ["-fsyntax-only", "-fno-color-diagnostics",
                                              "-Wno-unknown-warning-option"] + ANALYSIS_FLAGS
        return e, *run(cmd, e["directory"], opts.timeout)

    stats["units"], stats["failed"] = len(work), 0
    with open(os.path.join(opts.out, "compile.log"), "w", encoding="utf-8") as log:
        with concurrent.futures.ThreadPoolExecutor(opts.jobs) as pool:
            for e, code, out in pool.map(compile_one, work):
                log.write(f"==> {e['file']} (exit {code})\n{out}")
                stats["failed"] += code != 0
                for f in parse_compiler(out, e["directory"]):
                    add(f)
    if opts.ctu:
        with open(os.path.join(opts.out, "ctu.log"), "w", encoding="utf-8") as log:
            units, stats["ctu_failed"], sarif_dir = run_ctu(opts, log)
        stats["ctu_units"] = len(units)
        for f in parse_sarif(sarif_dir):
            add(f)

    findings = sorted(found.values(),
                      key=lambda f: (CATEGORIES.index(category(f["check"])), f["check"], f["path"], f["line"]))
    for f in findings:
        reason = suppress(f, rules)
        if reason:
            f["suppressed"] = reason
    with open(os.path.join(opts.out, "findings.json"), "w", encoding="utf-8") as f:
        json.dump(dict(stats, findings=findings), f, indent=1)
    text = report(findings, rules, stats, opts.limit)
    with open(os.path.join(opts.out, "report.md"), "w", encoding="utf-8") as f:
        f.write(text)
    summary = os.environ.get("GITHUB_STEP_SUMMARY")
    if summary:
        with open(summary, "a", encoding="utf-8") as f:
            f.write(text)
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
