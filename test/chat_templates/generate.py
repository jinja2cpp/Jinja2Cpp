#!/usr/bin/env python3
"""Render every template in templates/ with every conversation the way transformers does.

The oracle is transformers' own chat-template environment
(``chat_template_utils._compile_jinja_template``: ImmutableSandboxedEnvironment with
trim_blocks, lstrip_blocks, loop controls and ``{% generation %}``, its ``tojson`` filter and
the ``raise_exception`` and ``strftime_now`` globals), pinned in requirements.txt (transformers) and
test/parity/requirements.txt (Jinja2), under the Python of .python-version as test/parity. ``strftime_now`` reads a fixed clock, FIXED_NOW; the C++ suite
uses the same one.

Outputs, read by chat_template_test.cpp:
- ``expected/conversations.json``: the render context of each conversation, after the special
  tokens and the reasoning aliases transformers adds, so both sides render the same data;
- ``expected/<template>.json``: per conversation, the rendered text or the error; a
  conversation id ``<template>:<name>`` (conversations.TEMPLATE_CONVERSATIONS) belongs to that
  template only. A template with a ``{% generation %}`` block also stores ``generation``, the
  ``[start, end]`` code-point spans of its blocks in the output (the assistant token mask).

    python3 test/chat_templates/generate.py           # rewrite expected/
    python3 test/chat_templates/generate.py --check   # fail if expected/ or SOURCES.json is stale
"""
import argparse
import copy
import datetime
import hashlib
import importlib.util
import json
import logging
import os
import pathlib
import re
import sys

sys.dont_write_bytecode = True
os.environ.setdefault("TRANSFORMERS_VERBOSITY", "error")
logging.getLogger("transformers").setLevel(logging.ERROR)

import jinja2  # noqa: E402
import transformers  # noqa: E402
from transformers.utils import chat_template_utils  # noqa: E402

HERE = pathlib.Path(__file__).resolve().parent
# transformers' check that a template has a generation block (apply_chat_template, return_assistant_tokens_mask)
GENERATION_TAG = re.compile(r"\{\%-?\s*generation\s*-?\%\}")


def pinned_version(requirements, package):
    """The version requirements.txt pins for package; the one place each pin lives."""
    for line in requirements.read_text(encoding="utf-8").splitlines():
        name, _, version = line.split("#")[0].strip().partition("==")
        if name.lower() == package:
            return version
    sys.exit(f"{requirements} does not pin {package}==<version>")


TRANSFORMERS_VERSION = pinned_version(HERE / "requirements.txt", "transformers")
JINJA2_VERSION = pinned_version(HERE.parent / "parity" / "requirements.txt", "jinja2")
FIXED_NOW = datetime.datetime(2025, 3, 14, 9, 26, 53)
# transformers passes the tokenizer's special tokens; placeholders keep the suite tokenizer-free
SPECIAL_TOKENS = {"bos_token": "<|bos|>", "eos_token": "<|eos|>"}


class _FixedClock(datetime.datetime):
    @classmethod
    def now(cls, tz=None):
        return FIXED_NOW


def load_conversations():
    """Every conversation by id; a template's own ones have the id "<template>:<name>"."""
    spec = importlib.util.spec_from_file_location("conversations", HERE / "conversations.py")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    conversations = dict(module.CONVERSATIONS)
    for stem, own in module.TEMPLATE_CONVERSATIONS.items():
        conversations.update({f"{stem}:{name}": conv for name, conv in own.items()})
    return conversations


def applies(cid, stem):
    """Whether conversation cid is rendered with the template stem."""
    return ":" not in cid or cid.split(":", 1)[0] == stem


def build_contexts():
    contexts = {}
    for cid, conv in load_conversations().items():
        ctx = copy.deepcopy(conv)
        ctx["messages"] = chat_template_utils._alias_reasoning_fields(ctx["messages"])
        ctx.setdefault("tools", None)
        ctx.setdefault("documents", None)
        ctx.setdefault("add_generation_prompt", False)
        ctx.update(SPECIAL_TOKENS)
        contexts[cid] = ctx
    return contexts


def render(source, ctx):
    try:
        template = chat_template_utils._compile_jinja_template(source)
        result = {"output": template.render(**copy.deepcopy(ctx))}
        if GENERATION_TAG.search(source):
            # The assistant token mask, in code points (return_assistant_tokens_mask=True; docs/tasks/0377)
            _, indices = chat_template_utils._render_with_assistant_indices(template, **copy.deepcopy(ctx))
            result["generation"] = [[start, end] for start, end in indices]
        return result
    except Exception as e:  # noqa: BLE001 - every error is an expected result
        result = {"error": type(e).__name__}
        if isinstance(e, jinja2.TemplateError) and not isinstance(e, jinja2.TemplateSyntaxError):
            result["message"] = str(e)
        return result


def dump(obj):
    return json.dumps(obj, ensure_ascii=False, indent=1) + "\n"


def generate():
    files = {}
    contexts = build_contexts()
    files["conversations.json"] = dump(contexts)
    sources = json.loads((HERE / "SOURCES.json").read_text(encoding="utf-8"))
    chat_template_utils.datetime = _FixedClock
    for entry in sources["templates"]:
        source = (HERE / "templates" / entry["file"]).read_text(encoding="utf-8")
        stem = pathlib.Path(entry["file"]).stem
        renders = {cid: render(source, ctx) for cid, ctx in contexts.items() if applies(cid, stem)}
        files[stem + ".json"] = dump({"template": entry["file"], "renders": renders})
    return files


def check_sources():
    problems = []
    sources = json.loads((HERE / "SOURCES.json").read_text(encoding="utf-8"))
    listed = {e["file"] for e in sources["templates"]}
    for entry in sources["templates"]:
        path = HERE / "templates" / entry["file"]
        if not path.exists():
            problems.append(f"{entry['file']}: listed in SOURCES.json, missing")
        elif hashlib.sha256(path.read_bytes()).hexdigest() != entry["sha256"]:
            problems.append(f"{entry['file']}: sha256 differs from SOURCES.json")
    for path in (HERE / "templates").glob("*.jinja"):
        if path.name not in listed:
            problems.append(f"{path.name}: not listed in SOURCES.json")
    return problems


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="fail if expected/ is stale")
    args = parser.parse_args()
    if transformers.__version__ != TRANSFORMERS_VERSION or jinja2.__version__ != JINJA2_VERSION:
        sys.exit(f"need transformers {TRANSFORMERS_VERSION} and jinja2 {JINJA2_VERSION} "
                 f"(pip install -r test/parity/requirements.txt -r test/chat_templates/requirements.txt), found {transformers.__version__} and {jinja2.__version__}")
    problems = check_sources()
    files = generate()
    out = HERE / "expected"
    if args.check:
        on_disk = {p.name for p in out.glob("*.json")}
        for name, text in files.items():
            path = out / name
            if not path.exists() or path.read_text(encoding="utf-8") != text:
                problems.append(f"expected/{name} is stale; run test/chat_templates/generate.py")
        problems += [f"expected/{name} has no template" for name in sorted(on_disk - files.keys())]
        for p in problems:
            print(p, file=sys.stderr)
        sys.exit(1 if problems else 0)
    for p in problems:
        print("warning:", p, file=sys.stderr)
    out.mkdir(exist_ok=True)
    for stale in out.glob("*.json"):
        if stale.name not in files:
            stale.unlink()
    for name, text in files.items():
        (out / name).write_text(text, encoding="utf-8")
    print(f"{len(files) - 1} templates, {sum(text.count('"output"') + text.count('"error"') for name, text in files.items() if name != 'conversations.json')} renders")


if __name__ == "__main__":
    main()
