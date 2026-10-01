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
"""
import argparse
import pathlib
import re
import sys

HERE = pathlib.Path(__file__).resolve().parent
def area_of(case_id):
    """The area a case id belongs to: wide.literals.dict and literals.dict are both literals."""
    return case_id.removeprefix("wide.").split(".")[0]


LINE = re.compile(r"(?P<id>\S+)(?P<sp1>\s+)(?P<kind>\S+)(?P<rest>\s+.*)?$")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("results", help="file written by the suite via JINJA2CPP_PARITY_RESULTS")
    parser.add_argument("--divergences", default=str(HERE / "divergences"), help="directory of <area>.txt files")
    parser.add_argument("--dry-run", action="store_true", help="print the edits, change nothing")
    args = parser.parse_args()

    outcomes = {}
    for line in pathlib.Path(args.results).read_text(encoding="utf-8").splitlines():
        if line.strip():
            case_id, outcome = line.split()
            outcomes[case_id] = outcome
    if not outcomes:
        sys.exit("no results recorded: was JINJA2CPP_PARITY_RESULTS set for the run?")

    listed, removed, changed, rewrites = set(), [], [], {}
    for path in sorted(pathlib.Path(args.divergences).glob("*.txt")):
        out, edited = [], False
        for line in path.read_text(encoding="utf-8").splitlines(keepends=True):
            m = LINE.match(line.rstrip("\r\n"))
            if not m or line.startswith("#"):
                out.append(line)
                continue
            case_id, kind = m.group("id"), m.group("kind")
            listed.add(case_id)
            seen = outcomes.get(case_id)
            if seen is None or seen == kind or kind in ("unordered", "crash"):
                out.append(line)
                continue
            edited = True
            if seen == "match":
                removed.append(case_id)
                continue
            changed.append((case_id, kind, seen))
            # Keep the reason column where it was.
            rest = m.group("rest") or ""
            column = len(kind) + len(rest) - len(rest.lstrip())
            new_kind = seen.ljust(column) if column > len(seen) else seen + " "
            out.append(case_id + m.group("sp1") + new_kind + rest.lstrip() + line[len(line.rstrip("\r\n")):])
        if edited:
            rewrites[path] = "".join(out)

    new = sorted((i, o) for i, o in outcomes.items() if o != "match" and i not in listed)

    for case_id in removed:
        print(f"removed  {case_id}")
    for case_id, old, seen in changed:
        print(f"changed  {case_id}: {old} -> {seen} (check its task and reason)")
    for case_id, seen in new:
        print(f"NEW      {case_id}: {seen} (not listed; add a line naming the owning task to divergences/{area_of(case_id)}.txt)")
    if not args.dry_run:
        for path, text in rewrites.items():
            path.write_text(text, encoding="utf-8")
    print(f"{len(removed)} removed, {len(changed)} changed, {len(new)} new", file=sys.stderr)
    return 1 if new else 0


if __name__ == "__main__":
    sys.exit(main())
