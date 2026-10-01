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
"""
import argparse
import difflib
import importlib.util
import json
import pathlib
import sys

import jinja2

sys.dont_write_bytecode = True  # keep cases/ free of __pycache__

HERE = pathlib.Path(__file__).resolve().parent
JINJA2_VERSION = "3.1.6"

# Options the C++ harness knows how to map (or knows it cannot map yet).
ENV_OPTIONS = {
    "trim_blocks", "lstrip_blocks", "keep_trailing_newline", "autoescape",
    "extensions", "undefined", "block_start_string", "block_end_string",
    "variable_start_string", "variable_end_string", "comment_start_string",
    "comment_end_string", "line_statement_prefix", "line_comment_prefix",
    "newline_sequence",
}
EXTENSIONS = {"do": "jinja2.ext.do", "loopcontrols": "jinja2.ext.loopcontrols",
              "i18n": "jinja2.ext.i18n", "debug": "jinja2.ext.debug"}
UNDEFINED = {"default": jinja2.Undefined, "strict": jinja2.StrictUndefined,
             "chainable": jinja2.ChainableUndefined, "debug": jinja2.DebugUndefined}


def make_env(options, templates):
    unknown = set(options) - ENV_OPTIONS
    if unknown:
        raise ValueError(f"unknown env options {sorted(unknown)}")
    kwargs = {k: v for k, v in options.items() if k not in ("extensions", "undefined")}
    kwargs["extensions"] = [EXTENSIONS[e] for e in options.get("extensions", [])]
    kwargs["undefined"] = UNDEFINED[options.get("undefined", "default")]
    env = jinja2.Environment(loader=jinja2.DictLoader(templates), **kwargs)
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
    return {"generator": f"jinja2 {jinja2.__version__}", "area": path.stem, "cases": cases}


def report():
    """Print the parity metric from expected/ and divergences.txt; needs no jinja2."""
    divergences = {}
    for line in (HERE / "divergences.txt").read_text(encoding="utf-8").splitlines():
        if line.strip() and not line.startswith("#"):
            case_id, kind, task = line.split()[:3]
            divergences[case_id] = (kind, task)
    areas = json.loads((HERE / "expected" / "index.json").read_text(encoding="utf-8"))["areas"]
    kinds = ["output", "rejects", "accepts", "unsupported", "unordered", "crash"]
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


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--check", action="store_true", help="fail if expected/ is stale")
    parser.add_argument("--report", action="store_true", help="print the parity table and exit")
    args = parser.parse_args()
    if args.report:
        return report()
    if jinja2.__version__ != JINJA2_VERSION:
        sys.exit(f"expected jinja2 {JINJA2_VERSION}, found {jinja2.__version__}; "
                 f"pip install jinja2=={JINJA2_VERSION}")

    out_dir = HERE / "expected"
    out_dir.mkdir(exist_ok=True)
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
    index_path = out_dir / "index.json"
    if not index_path.exists() or index_path.read_text(encoding="utf-8") != index:
        if args.check:
            stale.append("index.json")
        else:
            index_path.write_text(index, encoding="utf-8")
    for orphan in sorted(p.name for p in out_dir.glob("*.json") if p.name not in wanted):
        if args.check:
            stale.append(orphan)
        else:
            (out_dir / orphan).unlink()
    if stale:
        sys.exit(f"stale expectations: {', '.join(stale)}; run test/parity/generate.py")
    print(f"{total} cases, jinja2 {jinja2.__version__}")


if __name__ == "__main__":
    main()
