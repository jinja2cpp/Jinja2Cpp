#!/usr/bin/env python3
"""Bring test/parity/divergences/ in line with what the parity suite observed.

Run the suite with JINJA2CPP_PARITY_RESULTS pointing at a file, then this script:

    rm -f /tmp/parity.txt
    JINJA2CPP_PARITY_RESULTS=/tmp/parity.txt build/jinja2cpp_tests --gtest_filter='Parity/*'
    python3 test/parity/update_divergences.py /tmp/parity.txt

It deletes the lines of cases that now match and rewrites the kind of cases whose
divergence changed, keeping each line's task and reason (check those: a case that
stops being rejected by the parser may now be blocked by another task). New
divergences are printed with the file they belong in, not added: they need an owning task. The exit status is 1
while any new divergence is left, so the script can gate a merge of master into a
parity branch.

A kind may carry a build condition, `rejects@nobigint` (see README.md). The suite writes
the build's features in a `# features:` header; the script edits only lines whose
condition holds in that build, and reports a case it lists under a condition that does
not hold but that diverged anyway. Results without the header (an older test binary)
leave every conditional line alone. `--self-test` runs the script on built-in fixtures.
"""
import argparse
import io
import pathlib
import re
import sys
import tempfile

HERE = pathlib.Path(__file__).resolve().parent
def area_of(case_id):
    """The area a case id belongs to: wide.literals.dict and literals.dict are both literals."""
    return case_id.removeprefix("wide.").split(".")[0]


LINE = re.compile(r"(?P<id>\S+)(?P<sp1>\s+)(?P<kind>\S+)(?P<rest>\s+.*)?$")

# Build features a kind's "@<feature>" or "@no<feature>" condition may name; keep in step
# with KnownFeatures() in parity_test.cpp.
KNOWN_FEATURES = {"bigint"}


def condition_holds(condition, features):
    """Whether a kind's condition ("@nobigint", "" for none) holds in a build with features.

    None for an unknown feature; features is None when the results carry no header."""
    if not condition:
        return True
    name = condition[1:]
    negated = name.startswith("no") and name[2:] in KNOWN_FEATURES
    feature = name[2:] if negated else name
    if feature not in KNOWN_FEATURES:
        return None
    if features is None:
        return False  # unknown build: leave the line alone
    return (feature in features) != negated


def update(results_text, divergences, dry_run, report):
    """Apply results_text to the <area>.txt files in divergences; returns the exit status."""
    outcomes, features = {}, None
    for line in results_text.splitlines():
        if line.startswith("# features:"):
            seen_features = set(line.split(":", 1)[1].split())
            if features is not None and seen_features != features:
                print(f"results mix builds: features {sorted(features)} and {sorted(seen_features)}", file=report)
                return 1
            features = seen_features
        elif line.strip() and not line.startswith("#"):
            case_id, outcome = line.split()
            outcomes[case_id] = outcome
    if not outcomes:
        print("no results recorded: was JINJA2CPP_PARITY_RESULTS set for the run?", file=report)
        return 1
    if features is None:
        print("note: the results have no '# features:' header; lines with a build condition are left alone", file=report)
    unknown = sorted(features - KNOWN_FEATURES) if features else []
    if unknown:
        print(f"unknown build features {unknown}: add them to KNOWN_FEATURES", file=report)
        return 1

    listed, removed, changed, rewrites, bad, off = set(), [], [], {}, [], []
    for path in sorted(pathlib.Path(divergences).glob("*.txt")):
        out, edited = [], False
        for line in path.read_text(encoding="utf-8").splitlines(keepends=True):
            m = LINE.match(line.rstrip("\r\n"))
            if not m or line.startswith("#"):
                out.append(line)
                continue
            case_id, kind = m.group("id"), m.group("kind")
            kind, at, condition = kind.partition("@")
            condition = at + condition
            listed.add(case_id)
            seen = outcomes.get(case_id)
            holds = condition_holds(condition, features)
            if holds is None:
                bad.append((case_id, condition))
            if not holds:
                # Not this build's line: the case must match here.
                if seen not in (None, "match", "crash") and features is not None:
                    off.append((case_id, kind + condition, seen))
                out.append(line)
                continue
            if seen is None or seen == kind or kind in ("platform", "crash"):
                out.append(line)
                continue
            edited = True
            if seen == "match":
                removed.append(case_id)
                continue
            changed.append((case_id, kind + condition, seen + condition))
            # Keep the reason column where it was.
            rest = m.group("rest") or ""
            column = len(kind + condition) + len(rest) - len(rest.lstrip())
            seen += condition
            new_kind = seen.ljust(column) if column > len(seen) else seen + " "
            out.append(case_id + m.group("sp1") + new_kind + rest.lstrip() + line[len(line.rstrip("\r\n")):])
        if edited:
            rewrites[path] = "".join(out)

    new = sorted((i, o) for i, o in outcomes.items() if o != "match" and i not in listed)

    for case_id, condition in bad:
        print(f"BAD      {case_id}: unknown condition '{condition}' (known features: {', '.join(sorted(KNOWN_FEATURES))})", file=report)
    for case_id in removed:
        print(f"removed  {case_id}", file=report)
    for case_id, old, seen in changed:
        print(f"changed  {case_id}: {old} -> {seen} (check its task and reason)", file=report)
    for case_id, seen in new:
        print(f"NEW      {case_id}: {seen} (not listed; add a line naming the owning task to divergences/{area_of(case_id)}.txt)", file=report)
    for case_id, kind, seen in off:
        print(f"NEW      {case_id}: {seen} (listed as {kind}, which does not hold in this build, so it must match)", file=report)
    if bad:
        return 1
    if not dry_run:
        for path, text in rewrites.items():
            path.write_text(text, encoding="utf-8")
    print(f"{len(removed)} removed, {len(changed)} changed, {len(new) + len(off)} new", file=report)
    return 1 if new or off else 0


DIVERGENCES = """\
operators.plain        rejects          0015 always diverges
operators.fixed        rejects          0015 now matches everywhere
operators.overflow     rejects@nobigint 0178 needs a backend
operators.overflow_cr  crash@nobigint   0178 traps without a backend
operators.renamed      output@nobigint  0178 kind changes
"""

# (name, results, exit status, operators.txt afterwards, text the report must contain[, operators.txt before])
SELF_TESTS = [
    ("default build", "# features:\noperators.plain rejects\noperators.fixed match\noperators.overflow rejects\n"
     "operators.overflow_cr crash\noperators.renamed rejects\n", 0,
     DIVERGENCES.replace("operators.fixed        rejects          0015 now matches everywhere\n", "")
     .replace("output@nobigint  0178", "rejects@nobigint 0178"), "changed  operators.renamed: output@nobigint -> rejects@nobigint"),
    ("feature build", "# features: bigint\noperators.plain rejects\noperators.fixed rejects\noperators.overflow match\n"
     "operators.overflow_cr match\noperators.renamed match\n", 0, DIVERGENCES, "0 removed, 0 changed, 0 new"),
    ("feature build, conditional case diverges", "# features: bigint\noperators.plain rejects\noperators.overflow output\n", 1,
     DIVERGENCES, "operators.overflow: output (listed as rejects@nobigint, which does not hold in this build"),
    ("no header", "operators.plain rejects\noperators.fixed match\noperators.overflow match\noperators.renamed rejects\n", 0,
     DIVERGENCES.replace("operators.fixed        rejects          0015 now matches everywhere\n", ""), "no '# features:' header"),
    ("mixed builds", "# features:\noperators.plain rejects\n# features: bigint\n", 1, DIVERGENCES, "results mix builds"),
    ("unknown feature", "# features: float128\noperators.plain rejects\n", 1, DIVERGENCES, "unknown build features ['float128']"),
    ("unknown condition", "# features:\noperators.fixed match\n", 1, DIVERGENCES + "operators.odd rejects@nofloat128 0178 x\n",
     "unknown condition '@nofloat128'", DIVERGENCES + "operators.odd rejects@nofloat128 0178 x\n"),
]


def self_test():
    failures = 0
    for name, results, status, expected, message, *before in SELF_TESTS:
        with tempfile.TemporaryDirectory() as tmp:
            path = pathlib.Path(tmp) / "operators.txt"
            path.write_text(before[0] if before else DIVERGENCES, encoding="utf-8")
            out = io.StringIO()
            got = update(results, tmp, False, out)
            text = path.read_text(encoding="utf-8")
            problems = []
            if got != status:
                problems.append(f"exit status {got}, expected {status}")
            if text != expected:
                problems.append(f"operators.txt is\n{text}expected\n{expected}")
            if message not in out.getvalue():
                problems.append(f"report lacks {message!r}")
            for problem in problems:
                failures += 1
                print(f"FAIL {name}: {problem}\n--- report:\n{out.getvalue()}")
    print(f"self-test: {len(SELF_TESTS)} cases, {failures} failures")
    return 1 if failures else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("results", nargs="?", help="file written by the suite via JINJA2CPP_PARITY_RESULTS")
    parser.add_argument("--divergences", default=str(HERE / "divergences"), help="directory of <area>.txt files")
    parser.add_argument("--dry-run", action="store_true", help="print the edits, change nothing")
    parser.add_argument("--self-test", action="store_true", help="check the script on built-in fixtures and exit")
    args = parser.parse_args()
    if args.self_test:
        return self_test()
    if not args.results:
        parser.error("the results file is required")
    return update(pathlib.Path(args.results).read_text(encoding="utf-8"), args.divergences, args.dry_run, sys.stdout)


if __name__ == "__main__":
    sys.exit(main())
