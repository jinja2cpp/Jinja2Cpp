#!/usr/bin/env python3
"""Render the parity corpus with Python Jinja2 and store the results as expectations.

The corpus lives in ``cases/<area>.py``. Each module defines ``CASES``, a list of
``(name, template)`` or ``(name, template, options)`` tuples, and may define
``CONTEXT``, the default render context for its cases. Options:

- ``ctx``: render context for this case (replaces the module ``CONTEXT``);
  must survive a JSON round trip, because the C++ side reads it from JSON.
- ``env``: Environment options, see ``ENV_OPTIONS``.
- ``templates``: extra templates for include/import/extends, by name.

``expected/<area>.json`` is generated from that and is what the C++ suite
(``parity_test.cpp``) reads, so Python is needed only to regenerate.

    python3 test/parity/generate.py           # rewrite expected/*.json
    python3 test/parity/generate.py --check   # fail if expected/*.json is stale
    python3 test/parity/generate.py --report  # parity table by area and task (Markdown)
    python3 test/parity/generate.py --out DIR --any-version  # another oracle (docs/tasks/0186)

``--out DIR`` writes the expectations to DIR instead of ``expected/``, with ``oracle.json``
naming the Python, Jinja2 and MarkupSafe that produced them. ``--any-version`` accepts a
Jinja2 other than the pinned one (only with ``--out``, so ``expected/`` keeps the pin);
``parity_test.cpp`` compares against DIR when ``JINJA2CPP_TEST_ORACLE_DIR`` names it.
"""
import argparse
import difflib
import importlib.metadata
import importlib.util
import json
import pathlib
import platform
import re
import sys

import jinja2

sys.dont_write_bytecode = True  # keep cases/ free of __pycache__

HERE = pathlib.Path(__file__).resolve().parent


def pinned_jinja2_version():
    """The one pin of the oracle's Jinja2: requirements.txt (docs/parity.md, "Oracle pin")."""
    for line in (HERE / "requirements.txt").read_text(encoding="utf-8").splitlines():
        name, sep, version = line.split("#")[0].strip().partition("==")
        if sep and name.strip().lower() == "jinja2":
            return version.strip()
    sys.exit("requirements.txt does not pin jinja2==<version>")


JINJA2_VERSION = pinned_jinja2_version()

# The oldest oracle generate.py supports with --any-version (test/parity/README.md, "Other oracles")
MIN_PYTHON = (3, 10)
MIN_JINJA2 = (3, 0)


def package_version(name, module=None):
    """A distribution's version, from its metadata: jinja2 and markupsafe deprecate __version__."""
    try:
        return importlib.metadata.version(name)
    except importlib.metadata.PackageNotFoundError:
        return getattr(module, "__version__", "unknown")


def release_tuple(version):
    """(3, 1) for "3.1.6", "3.2.0.dev0" or "3.0.0rc1"."""
    match = re.match(r"(\d+)\.(\d+)", version)
    return (int(match[1]), int(match[2])) if match else (0, 0)


def oracle_versions():
    import markupsafe  # a dependency of jinja2, so always present
    return {"python": platform.python_version(), "jinja2": package_version("jinja2", jinja2),
            "markupsafe": package_version("markupsafe", markupsafe)}


JINJA2_FOUND = package_version("jinja2", jinja2)

# Options the C++ harness knows how to map (or knows it cannot map yet).
ENV_OPTIONS = {
    "trim_blocks", "lstrip_blocks", "keep_trailing_newline", "autoescape",
    "extensions", "undefined", "block_start_string", "block_end_string",
    "variable_start_string", "variable_end_string", "comment_start_string",
    "comment_end_string", "line_statement_prefix", "line_comment_prefix",
    "newline_sequence", "filters", "tests", "finalize",
}
EXTENSIONS = {"do": "jinja2.ext.do", "loopcontrols": "jinja2.ext.loopcontrols",
              "i18n": "jinja2.ext.i18n", "debug": "jinja2.ext.debug"}
# Functions a case can register by name with the "filters", "tests" and "finalize" options.
# parity_test.cpp registers a C++ counterpart under each name; they take ints and strings.
FILTERS = {
    "double": lambda v, n=2: v * n,
    "wrap": lambda v, left="[", right="]": f"{left}{v}{right}",
    "upper": lambda v: "U:" + v,  # replaces the builtin
}
TESTS = {
    "big": lambda v, limit=10: v > limit,
    "even": lambda v: v % 3 == 0,  # replaces the builtin
}
FINALIZE = {
    "none_to_empty": lambda v: "" if v is None else v,
    "brackets": lambda v: f"[{v}]",
    "none_marker": lambda v: "N" if v is None else v,
    "plus_one": lambda v: v + 1,  # raises unless v is a number
}
UNDEFINED = {"default": jinja2.Undefined, "strict": jinja2.StrictUndefined,
             "chainable": jinja2.ChainableUndefined, "debug": jinja2.DebugUndefined}


def make_env(options, templates):
    unknown = set(options) - ENV_OPTIONS
    if unknown:
        raise ValueError(f"unknown env options {sorted(unknown)}")
    kwargs = {k: v for k, v in options.items() if k not in ("extensions", "undefined", "filters", "tests", "finalize")}
    kwargs["extensions"] = [EXTENSIONS[e] for e in options.get("extensions", [])]
    kwargs["undefined"] = UNDEFINED[options.get("undefined", "default")]
    if "finalize" in options:
        kwargs["finalize"] = FINALIZE[options["finalize"]]
    if isinstance(options.get("autoescape"), list):
        kwargs["autoescape"] = jinja2.select_autoescape(options["autoescape"])
    env = jinja2.Environment(loader=jinja2.DictLoader(templates), **kwargs)
    env.filters.update({name: FILTERS[name] for name in options.get("filters", [])})
    env.tests.update({name: TESTS[name] for name in options.get("tests", [])})
    if "i18n" in options.get("extensions", []):
        env.install_null_translations(newstyle=True)
    return env


def render(case):
    env = make_env(case["env"], case["templates"])
    try:
        tpl = env.from_string(case["template"])
    except Exception as ex:  # noqa: BLE001 - jinja2 can leak a Python SyntaxError here
        return {"error": {"phase": "compile", "type": type(ex).__name__, "message": str(ex)}}
    try:
        # Render from the JSON form, exactly what the C++ side will see.
        return {"output": tpl.render(**json.loads(json.dumps(case["context"])))}
    except Exception as ex:  # noqa: BLE001 - any render failure is an expected error
        return {"error": {"phase": "render", "type": type(ex).__name__, "message": str(ex)}}


def load_area(path):
    spec = importlib.util.spec_from_file_location(f"parity_cases_{path.stem}", path)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    default_ctx = getattr(module, "CONTEXT", {})
    cases, seen = [], set()
    for entry in module.CASES:
        name, template, options = (entry + ({},))[:3]
        if name in seen:
            raise ValueError(f"{path.name}: duplicate case name {name!r}")
        seen.add(name)
        case = {
            "id": f"{path.stem}.{name}",
            "template": template,
            "context": options.get("ctx", default_ctx),
            "env": options.get("env", {}),
            "templates": options.get("templates", {}),
        }
        result = render(case)
        if render(case) != result:
            raise ValueError(f"{case['id']}: output is not deterministic")
        case.update(result)
        cases.append(case)
    return {"generator": f"jinja2 {JINJA2_FOUND}", "area": path.stem, "cases": cases}


def report():
    """Print the parity metric from expected/ and divergences/; needs no jinja2."""
    divergences, conditional = {}, {}
    for path in sorted((HERE / "divergences").glob("*.txt")):
        for line in path.read_text(encoding="utf-8").splitlines():
            if line.strip() and not line.startswith("#"):
                case_id, kind, task = line.split()[:3]
                kind, _, condition = kind.partition("@")
                # The table is the default build's, which has no optional features: a
                # "@<feature>" line does not apply to it, a "@no<feature>" line does.
                if not condition or condition.startswith("no"):
                    divergences[case_id] = (kind, task)
                if condition:
                    conditional[case_id] = (kind, condition, task)
    areas = json.loads((HERE / "expected" / "index.json").read_text(encoding="utf-8"))["areas"]
    kinds = ["output", "rejects", "accepts", "unsupported", "platform", "crash"]
    print("| area | cases | match | " + " | ".join(kinds) + " | tasks |")
    print("|---|---|---|" + "---|" * len(kinds) + "---|")
    totals = [0] * (len(kinds) + 2)
    for area in areas:
        ids = [c["id"] for c in json.loads((HERE / "expected" / f"{area}.json").read_text(encoding="utf-8"))["cases"]]
        known = [divergences[i] for i in ids if i in divergences]
        row = [len(ids), len(ids) - len(known)] + [sum(k == kind for k, _ in known) for kind in kinds]
        totals = [a + b for a, b in zip(totals, row)]
        tasks = ", ".join(sorted({t for _, t in known}))
        print(f"| {area} | " + " | ".join(map(str, row)) + f" | {tasks} |")
    print("| **total** | " + " | ".join(f"**{n}**" for n in totals) + " | |")
    wide = sorted(i for i in divergences if i.startswith("wide."))
    print(f"\nWide-only divergences (TemplateW, listed as wide.<id>): {len(wide)}")
    for case_id in wide:
        print(f"- {case_id}: {divergences[case_id][0]}, task {divergences[case_id][1]}")
    print(f"\nBuild-conditional divergences (`@no<feature>` lines are counted above and match in builds with the feature;"
          f" `@<feature>` lines apply only to such builds): {len(conditional)}")
    for case_id, (kind, condition, task) in sorted(conditional.items()):
        print(f"- {case_id}: {kind}@{condition}, task {task}")


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--check", action="store_true", help="fail if expected/ is stale")
    parser.add_argument("--report", action="store_true", help="print the parity table and exit")
    parser.add_argument("--out", type=pathlib.Path, help="write the expectations to this directory, not expected/")
    parser.add_argument("--any-version", action="store_true",
                        help=f"accept a Jinja2 other than the pinned {JINJA2_VERSION} (needs --out)")
    args = parser.parse_args()
    if args.report:
        return report()
    if args.any_version and not args.out:
        sys.exit("--any-version needs --out: expected/ holds the pinned oracle's results")
    if args.check and args.out:
        sys.exit("--check compares expected/; it does not take --out")
    versions = oracle_versions()
    if args.any_version:
        if sys.version_info[:2] < MIN_PYTHON or release_tuple(JINJA2_FOUND) < MIN_JINJA2:
            sys.exit(f"unsupported oracle Python {versions['python']} with Jinja2 {JINJA2_FOUND}; "
                     f"the parity oracle needs Python >= {'.'.join(map(str, MIN_PYTHON))} "
                     f"and Jinja2 >= {'.'.join(map(str, MIN_JINJA2))}")
    elif JINJA2_FOUND != JINJA2_VERSION:
        sys.exit(f"expected jinja2 {JINJA2_VERSION}, found {JINJA2_FOUND}; "
                 f"pip install jinja2=={JINJA2_VERSION}")

    out_dir = args.out.resolve() if args.out else HERE / "expected"
    if args.out and out_dir == (HERE / "expected").resolve():
        sys.exit("--out names expected/; run generate.py without --out to regenerate it")
    out_dir.mkdir(parents=True, exist_ok=True)
    stale, total = [], 0
    wanted = set()
    for path in sorted((HERE / "cases").glob("*.py")):
        data = load_area(path)
        total += len(data["cases"])
        text = json.dumps(data, indent=1, ensure_ascii=False) + "\n"
        target = out_dir / f"{path.stem}.json"
        wanted.add(target.name)
        current = target.read_text(encoding="utf-8") if target.exists() else ""
        if current == text:
            continue
        if args.check:
            stale.append(target.name)
            sys.stdout.writelines(difflib.unified_diff(
                current.splitlines(True), text.splitlines(True), str(target), "regenerated", n=1))
        else:
            target.write_text(text, encoding="utf-8")
    index = json.dumps({"areas": sorted(n[:-5] for n in wanted)}, indent=1) + "\n"
    wanted.add("index.json")
    if args.out:
        # Which oracle produced the directory, for the delta report of parity_test.cpp
        (out_dir / "oracle.json").write_text(json.dumps(versions, indent=1) + "\n", encoding="utf-8")
        wanted.add("oracle.json")
    index_path = out_dir / "index.json"
    if not index_path.exists() or index_path.read_text(encoding="utf-8") != index:
        if args.check:
            stale.append("index.json")
        else:
            index_path.write_text(index, encoding="utf-8")
    # Only expected/ is swept: a --out directory may hold other files, and index.json lists the areas
    for orphan in [] if args.out else sorted(p.name for p in out_dir.glob("*.json") if p.name not in wanted):
        if args.check:
            stale.append(orphan)
        else:
            (out_dir / orphan).unlink()
    if stale:
        sys.exit(f"stale expectations: {', '.join(stale)}; run test/parity/generate.py")
    print(f"{total} cases, Python {versions['python']}, jinja2 {versions['jinja2']}, "
          f"markupsafe {versions['markupsafe']}" + (f", in {out_dir}" if args.out else ""))


if __name__ == "__main__":
    main()
