"""pytest plugin of run.py: the upstream suite's outcomes held to expected-failures.txt (docs/tasks/0355).

run.py sets ``JINJA2CPP_FALLBACK=never`` and the plugin checks that it took, so a test passes
only when jinja2cpp itself does what Jinja2 does, never by handing the template to the real Jinja2.
"""

import os
import pathlib
import re

import jinja2cpp
import pytest

CLASSES = {
    "internals": "Jinja2's lexer, parser nodes, compiler or other internals: not in the denominator",
    "async": "async rendering, outside the supported subset",
    "extension": "a custom Extension or a builtin one jinja2cpp lacks, outside the supported subset",
    "unsupported": "other API outside the supported subset (docs/python.md): NativeEnvironment, compiled modules, ...",
    "deliberate": "a deliberate divergence (Jinja2C++ keeps different behaviour)",
    "divergence": "a parity gap, owned by a task",
}
# Classes whose lines need a task number after the class
NEEDS_TASK = {"deliberate", "divergence"}

_LINE = re.compile(r"^(?P<id>\"[^\"]+\"|\S+)\s+(?P<cls>[a-z]+)(?:\s+(?P<task>\d{4}))?\s+(?P<reason>\S.*)$")


def parse_expected(path, tasks=None):
    """{test id or prefix: (class, task, reason)}; raises ValueError on a malformed line, or one
    naming a task that has no file in `tasks` (docs/tasks)."""
    entries = {}
    for number, raw in enumerate(pathlib.Path(path).read_text(encoding="utf-8").splitlines(), 1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue
        match = _LINE.match(line)
        if not match:
            raise ValueError(f"expected-failures.txt:{number}: want '<test id> <class> [task] <reason>': {raw}")
        cls, task, test_id = match["cls"], match["task"], match["id"].strip('"')
        if cls not in CLASSES:
            raise ValueError(f"expected-failures.txt:{number}: unknown class {cls!r}")
        if cls in NEEDS_TASK and not task:
            raise ValueError(f"expected-failures.txt:{number}: class {cls} needs a task number")
        if task and tasks and not any(pathlib.Path(tasks).glob(f"{task}-*.md")):
            raise ValueError(f"expected-failures.txt:{number}: no task {task} in {tasks}")
        if test_id in entries:
            raise ValueError(f"expected-failures.txt:{number}: {test_id} is listed twice")
        entries[test_id] = (cls, task, match["reason"])
    return entries


def lookup(entries, nodeid):
    """The entry for `nodeid`: its own line, else the longest listed prefix (a file, class or test
    whose parameters are all covered)."""
    if nodeid in entries:
        return nodeid, entries[nodeid]
    best = None
    for key in entries:
        if nodeid.startswith((key + "::", key + "[")) and (best is None or len(key) > len(best)):
            best = key
    return (best, entries[best]) if best else (None, None)


class Upstream:
    def __init__(self, config):
        self.config = config
        self.entries = parse_expected(os.environ["JINJA2CPP_UPSTREAM_EXPECTED"], os.environ.get("JINJA2CPP_UPSTREAM_TASKS"))
        self.outcomes = {}  # nodeid -> "passed" | "failed" | "skipped"
        self.messages = {}

    def pytest_runtest_logreport(self, report):
        if report.when == "call" or report.outcome != "passed":
            previous = self.outcomes.get(report.nodeid)
            outcome = "failed" if report.outcome == "failed" else report.outcome
            if previous != "failed":
                self.outcomes[report.nodeid] = outcome
            if report.outcome == "failed" and report.nodeid not in self.messages:
                self.messages[report.nodeid] = _first_line(report)

    def pytest_collectreport(self, report):
        if report.failed:
            self.outcomes[report.nodeid] = "failed"
            self.messages[report.nodeid] = _first_line(report)

    def summarize(self):
        counts = {"passed": 0, "skipped": 0, "internals": 0}
        failed = {cls: 0 for cls in CLASSES}
        unexpected, fixed, used = [], [], set()
        for nodeid, outcome in sorted(self.outcomes.items()):
            key, entry = lookup(self.entries, nodeid)
            if key:
                used.add(key)
            cls = entry[0] if entry else None
            if cls == "internals":
                counts["internals"] += 1
                continue
            if outcome == "skipped":
                counts["skipped"] += 1
            elif outcome == "passed":
                counts["passed"] += 1
                # A file-level line may cover passing tests too; only a test's own line must fail
                if entry and key == nodeid:
                    fixed.append((nodeid, cls))
            elif entry:
                failed[cls] += 1
            else:
                unexpected.append((nodeid, self.messages.get(nodeid, "")))
        collected = len(self.outcomes)
        denominator = collected - counts["internals"]
        stale = sorted(set(self.entries) - used)
        return counts, failed, unexpected, fixed, stale, collected, denominator

    def pytest_sessionfinish(self, session, exitstatus):
        counts, failed, unexpected, fixed, stale, collected, denominator = self.summarize()
        rate = counts["passed"] / denominator if denominator else 0.0
        lines = [
            "",
            "Jinja2 upstream suite through jinja2cpp (docs/tasks/0355)",
            f"  pass rate: {counts['passed']} / {denominator} = {rate:.1%}"
            f"  (collected {collected}, internals {counts['internals']} not counted, skipped {counts['skipped']})",
            "  listed failures: " + ", ".join(f"{cls} {n}" for cls, n in failed.items() if cls != "internals"),
        ]
        subset = counts["passed"] + failed["deliberate"] + failed["divergence"] + len(unexpected)
        if subset:
            lines.insert(3, f"  within the supported subset: {counts['passed']} / {subset} = {counts['passed'] / subset:.1%}"
                            "  (without async, extension and unsupported)")
        for nodeid, message in unexpected:
            lines.append(f"  UNEXPECTED FAILURE {nodeid}: {message}")
        for nodeid, cls in fixed:
            lines.append(f"  NOW PASSES (delete its line, class {cls}): {nodeid}")
        for key in stale:
            lines.append(f"  STALE LINE (no such test): {key}")
        self.report = "\n".join(lines)
        path = os.environ.get("JINJA2CPP_UPSTREAM_RESULTS")
        if path:
            with open(path, "w", encoding="utf-8") as out:
                for nodeid, outcome in sorted(self.outcomes.items()):
                    out.write(f"{nodeid}\t{outcome}\t{self.messages.get(nodeid, '')}\n")
        # The suite's own failures are expected; the run fails only on what the list does not say
        session.exitstatus = 1 if unexpected or fixed or stale else 0

    def pytest_terminal_summary(self, terminalreporter):
        terminalreporter.write_line(self.report)


def _first_line(report):
    text = str(report.longrepr) if report.longrepr else ""
    # The last "E   " line of the traceback names the exception
    errors = [line[1:].strip() for line in text.splitlines() if line.startswith("E ")]
    message = errors[-1] if errors else (text.strip().splitlines() or [""])[-1]
    return message[:200]


def pytest_configure(config):
    if jinja2cpp.get_fallback() != "never":
        raise pytest.UsageError("run the upstream suite through run.py, which sets JINJA2CPP_FALLBACK=never")
    config.pluginmanager.register(Upstream(config), "jinja2cpp_upstream")
