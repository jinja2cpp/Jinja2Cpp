#!/usr/bin/env python3
"""Run the pallets/jinja test suite against jinja2cpp (docs/tasks/0355, 0248 plan D10.1).

The suite is the ``tests/`` directory of the Jinja2 sdist the oracle pins
(test/parity/requirements.txt). It is fetched from PyPI and checked against SHA256, or read
from ``--sdist``. Its ``jinja2`` imports are rewritten to reach jinja2cpp (``upstream_map``),
and pytest runs it with ``upstream_plugin.py``, which holds each failure to ``expected-failures.txt``
and prints the pass rate:

    passed / (collected - internals)

with the ``deliberate`` and ``divergence`` counts beside it. The run fails on a test that
fails without a line, and on a listed test that now passes (delete its line).

Usage, with the module's build on PYTHONPATH (as for python/tests):

    PYTHONPATH=<build>/python python python/tests/upstream/run.py [--sdist FILE] [-- pytest args]
"""

import argparse
import ast
import hashlib
import io
import os
import pathlib
import subprocess
import sys
import tarfile
import tempfile
import urllib.request

HERE = pathlib.Path(__file__).resolve().parent
REPO = HERE.parents[2]

# The sdist of the Jinja2 the oracle pins; its hash is PyPI's, so a changed file is refused.
# A bump of test/parity/requirements.txt changes both lines (docs/parity.md, "Oracle pin")
JINJA2_SDIST_VERSION = "3.1.6"
JINJA2_SDIST_SHA256 = "0137fb05990d35f1275a587e9aee6d56da821fc83491a0fb838183be43f66d6d"
JINJA2_SDIST_URL = f"https://files.pythonhosted.org/packages/source/j/jinja2/jinja2-{JINJA2_SDIST_VERSION}.tar.gz"


def pinned_version():
    for line in (REPO / "test/parity/requirements.txt").read_text(encoding="utf-8").splitlines():
        name, sep, version = line.split("#")[0].strip().partition("==")
        if sep and name.strip().lower() == "jinja2":
            return version.strip()
    sys.exit("test/parity/requirements.txt does not pin jinja2")


def fetch_sdist(path):
    if path:
        data = pathlib.Path(path).read_bytes()
    else:
        with urllib.request.urlopen(JINJA2_SDIST_URL, timeout=60) as response:  # noqa: S310 - fixed https URL
            data = response.read()
    digest = hashlib.sha256(data).hexdigest()
    if digest != JINJA2_SDIST_SHA256:
        sys.exit(f"jinja2-{JINJA2_SDIST_VERSION}.tar.gz has SHA256 {digest}, expected {JINJA2_SDIST_SHA256}")
    return data


def _alias(alias):
    return alias.asname or alias.name


def rewrite(source):
    """`source` with each import of jinja2 replaced by an upstream_map call, line numbers kept."""
    lines = source.splitlines(keepends=True)
    edits = []
    for node in ast.walk(ast.parse(source)):
        if isinstance(node, ast.Import) and any(a.name.split(".")[0] == "jinja2" for a in node.names):
            if any(a.name.split(".")[0] != "jinja2" for a in node.names):
                raise ValueError(f"line {node.lineno}: an import of jinja2 mixed with other modules")
            stmts = []
            for a in node.names:
                if a.asname:
                    stmts.append(f"{a.asname} = _upstream_map.module({a.name!r})")
                else:  # `import jinja2.ext` binds `jinja2`
                    stmts.append("jinja2 = _upstream_map.module('jinja2')")
            edits.append((node, "; ".join(stmts)))
        elif isinstance(node, ast.ImportFrom) and node.level == 0 and node.module and node.module.split(".")[0] == "jinja2":
            if any(a.name == "*" for a in node.names):
                raise ValueError(f"line {node.lineno}: star import of {node.module}")
            targets = ", ".join(_alias(a) for a in node.names)
            attrs = ", ".join(repr(a.name) for a in node.names)
            edits.append((node, f"({targets},) = _upstream_map.names({node.module!r}, {attrs})"))
    for node, text in sorted(edits, key=lambda e: e[0].lineno, reverse=True):
        first, last = node.lineno - 1, node.end_lineno - 1
        indent = lines[first][: node.col_offset]
        if lines[first][: node.col_offset].strip() or lines[last][node.end_col_offset:].strip():
            raise ValueError(f"line {node.lineno}: the import shares its line with other code")
        lines[first : last + 1] = [f"{indent}import upstream_map as _upstream_map; {text}\n"] + ["\n"] * (last - first)
    return "".join(lines)


def extract(data, work):
    """The sdist's tests/ in `work`/tests, rewritten."""
    prefix = f"jinja2-{JINJA2_SDIST_VERSION}/tests/"
    tests = work / "tests"
    with tarfile.open(fileobj=io.BytesIO(data), mode="r:gz") as archive:
        for member in archive.getmembers():
            if not member.name.startswith(prefix) or not member.isfile():
                continue
            relative = pathlib.PurePosixPath(member.name[len(prefix):])
            if relative.is_absolute() or ".." in relative.parts:
                sys.exit(f"unexpected path in the sdist: {member.name}")
            target = tests.joinpath(*relative.parts)
            target.parent.mkdir(parents=True, exist_ok=True)
            content = archive.extractfile(member).read()
            if target.suffix == ".py":
                content = rewrite(content.decode("utf-8")).encode("utf-8")
            target.write_bytes(content)
    return tests


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--sdist", help=f"jinja2-{JINJA2_SDIST_VERSION}.tar.gz to read instead of fetching it")
    parser.add_argument("--work", help="directory for the rewritten suite (default: a temporary one)")
    parser.add_argument("--results", help="write each test's outcome to this file (one line per test)")
    parser.add_argument("pytest_args", nargs="*", help="passed to pytest (after --)")
    args = parser.parse_args()
    if pinned_version() != JINJA2_SDIST_VERSION:
        sys.exit(f"the oracle pins jinja2 {pinned_version()}; update JINJA2_SDIST_VERSION and _SHA256 in {__file__}")
    data = fetch_sdist(args.sdist)
    with tempfile.TemporaryDirectory(prefix="jinja2-upstream-") as tmp:
        work = pathlib.Path(args.work or tmp)
        tests = extract(data, work)
        env = dict(os.environ)
        env["PYTHONPATH"] = os.pathsep.join(p for p in (str(HERE), env.get("PYTHONPATH", "")) if p)
        env["JINJA2CPP_FALLBACK"] = "never"  # a test passes on jinja2cpp alone, never on the real Jinja2
        env["JINJA2CPP_UPSTREAM_EXPECTED"] = str(HERE / "expected-failures.txt")
        env["JINJA2CPP_UPSTREAM_TASKS"] = str(REPO / "docs" / "tasks")
        if args.results:
            env["JINJA2CPP_UPSTREAM_RESULTS"] = str(pathlib.Path(args.results).resolve())
        command = [sys.executable, "-m", "pytest", "-p", "upstream_plugin", "-p", "no:cacheprovider",
                   "--rootdir", str(tests), "-q", "--no-header", "--tb=no", "-rN", "--continue-on-collection-errors", str(tests), *args.pytest_args]
        return subprocess.call(command, cwd=tests, env=env)


if __name__ == "__main__":
    sys.exit(main())
