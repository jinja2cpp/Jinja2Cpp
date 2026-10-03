#!/usr/bin/env python3
"""Differential check of a fuzz corpus against Python Jinja2 (fuzz/README.md).

Renders every input of a corpus directory with Jinja2C++ (jinja2cpp_fuzz_replay --json, the
same environment and context as the fuzz_render target) and with Python Jinja2, and reports
where they disagree: different output, or one side rendering what the other rejects.
Crashes are the fuzzers' job; this finds behaviour gaps the parity corpus does not cover yet.

    python3 fuzz/differential.py build/jinja2cpp_fuzz_replay CORPUS_DIR [--out report.md]
"""
import argparse
import collections
import copy
import json
import pathlib
import signal
import subprocess
import sys

import jinja2
import jinja2.ext

# Mirrors fuzz/fuzz_common.h: AddSupportTemplates, ConfigureEnv and MakeContext. `d` is in
# key order: ValuesMap sorts its keys (docs/tasks/0043), which is not what this check is after.
SUPPORT_TEMPLATES = {
    "base": "<{% block content %}base{% endblock %}|{% block other %}{% endblock %}>",
    "header": "[{{ foo }}|{{ bar }}]",
    "macros": "{% macro m(a, b=2) %}{{ a }}-{{ b }}-{{ varargs }}-{{ kwargs }}{{ caller() if caller }}{% endmacro %}{% set v = 42 %}",
}
CONTEXT = {
    "x": 3, "neg": -3, "y": 2.567, "s": "Hello World", "l": [3, 1, 2], "dup": [3, 1, 3, 2, 1],
    "words": ["b", "A", "c"], "d": {"C": 3, "a": 1, "b": 2}, "e": [], "n": None, "t": True,
    "html": "<b>Tom & \"Jerry\"</b>",
    "users": [{"name": "bob", "age": 30}, {"name": "alice", "age": 25, "city": "Rome"}],
    "foo": 42, "ws": "wide é",
}
MAX_INPUT_SIZE = 4096

# Any exception counts as Jinja2 rejecting the template: Jinja2 also lets Python's own
# errors through (SyntaxError for `break` outside a loop, RecursionError, ...)
REJECTIONS = (Exception,)


class Timeout(BaseException):
    pass


def on_alarm(signum, frame):
    raise Timeout()


def render_python(source):
    signal.signal(signal.SIGALRM, on_alarm)
    signal.alarm(5)
    try:
        return render_python_unguarded(source)
    except Timeout:
        return "timeout", ""
    finally:
        signal.alarm(0)


def render_python_unguarded(source):
    env = jinja2.Environment(loader=jinja2.DictLoader(dict(SUPPORT_TEMPLATES, self=source)),
                             extensions=["jinja2.ext.do", "jinja2.ext.loopcontrols", "jinja2.ext.i18n"])
    env.install_null_translations(newstyle=True)
    env.globals["bar"] = 23
    try:
        template = env.from_string(source)
    except REJECTIONS as error:
        return "parse_error", type(error).__name__ + ": " + str(error)[:200]
    try:
        # a deep copy: `{% do l.append(1) %}` must not leak into the next input
        return "rendered", template.render(copy.deepcopy(CONTEXT))
    except REJECTIONS as error:
        return "render_error", type(error).__name__ + ": " + str(error)[:200]


def classify(source, cpp_kind, cpp_output, py_kind, py_output):
    if py_kind == "timeout":
        return "py_timeout"
    # Random (lipsum, random) and address-dependent (object reprs) output cannot match
    if py_kind == "rendered" and (" object at 0x" in py_output or "lipsum" in source or "random" in source):
        return "nondeterministic"
    cpp_ok = cpp_kind == "rendered"
    py_ok = py_kind == "rendered"
    if cpp_ok and py_ok:
        return "match" if cpp_output == py_output else "output"
    if not cpp_ok and not py_ok:
        return "match"
    return "rejects" if py_ok else "accepts"


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("replay", help="path to jinja2cpp_fuzz_replay")
    parser.add_argument("corpus", type=pathlib.Path)
    parser.add_argument("--out", type=pathlib.Path, help="write the divergent inputs to this markdown report")
    parser.add_argument("--timeout", type=int, default=600)
    parser.add_argument("--known", type=pathlib.Path, action="append", default=[],
                        help="corpus whose inputs are not reported, e.g. the seeds: parity cases already list theirs")
    args = parser.parse_args()

    known = {p.name for directory in args.known for p in directory.iterdir()}
    files = {}
    for path in args.corpus.iterdir():
        if not path.is_file() or path.stat().st_size > MAX_INPUT_SIZE or path.name in known:
            continue
        try:
            # Python sees text, Jinja2C++ bytes: invalid UTF-8 only shows that difference
            path.read_bytes().decode("utf-8")
        except UnicodeDecodeError:
            continue
        files[path.name] = path
    cpp = {}
    names = sorted(files)
    # Batches keep one crash or hang from losing the whole corpus
    for start in range(0, len(names), 200):
        batch = [str(files[n]) for n in names[start:start + 200]]
        try:
            proc = subprocess.run([args.replay, "--json", *batch], capture_output=True, timeout=args.timeout)
        except subprocess.TimeoutExpired:
            print(f"replay timed out on a batch starting at {names[start]}", file=sys.stderr)
            continue
        for line in proc.stdout.decode().splitlines():
            row = json.loads(line)
            cpp[row["file"]] = (row["kind"], bytes.fromhex(row["output"]).decode("utf-8", "replace"))

    stats = collections.Counter()
    divergent = collections.defaultdict(list)
    for name in names:
        if name not in cpp:
            stats["cpp_missing"] += 1
            continue
        source = files[name].read_bytes().decode("utf-8")
        py_kind, py_output = render_python(source)
        kind = classify(source, cpp[name][0], cpp[name][1], py_kind, py_output)
        stats[kind] += 1
        if kind not in ("match", "nondeterministic"):
            divergent[kind].append((name, source, cpp[name], (py_kind, py_output)))

    print(" ".join(f"{k}={v}" for k, v in sorted(stats.items())))
    if args.out:
        with args.out.open("w", encoding="utf-8") as report:
            report.write("# Differential fuzzing report\n\n" + ", ".join(f"{k}: {v}" for k, v in sorted(stats.items())) + "\n")
            for kind, rows in sorted(divergent.items()):
                report.write(f"\n## {kind} ({len(rows)})\n")
                for name, source, (ck, co), (pk, po) in sorted(rows, key=lambda r: len(r[1])):
                    report.write(f"\n### {name}\n\n```jinja\n{source}\n```\n\n- C++ {ck}: `{co[:300]!r}`\n- Python {pk}: `{po[:300]!r}`\n")


if __name__ == "__main__":
    main()
