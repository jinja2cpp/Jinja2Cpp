#!/usr/bin/env python3
"""Refresh templates/ from a llama.cpp checkout and record the hashes in SOURCES.json.

The Hugging Face Hub is not reachable from every runner, so the templates come from
llama.cpp's models/templates/, which its scripts/get_chat_template.py fetches from the Hub.

    git clone https://github.com/ggml-org/llama.cpp /tmp/llama.cpp
    python3 test/chat_templates/update_templates.py /tmp/llama.cpp            # refresh listed files
    python3 test/chat_templates/update_templates.py /tmp/llama.cpp --add X.jinja

Fill in hf_repo and license of a new entry from the model card, then run generate.py and
update divergences.txt.
"""
import argparse
import hashlib
import json
import pathlib
import shutil
import subprocess

HERE = pathlib.Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("llama_cpp", type=pathlib.Path, help="llama.cpp checkout")
    parser.add_argument("--add", action="append", default=[], help="template file to add")
    args = parser.parse_args()

    sources_path = HERE / "SOURCES.json"
    sources = json.loads(sources_path.read_text(encoding="utf-8"))
    entries = {e["file"]: e for e in sources["templates"]}
    for name in args.add:
        entries.setdefault(name, {"file": name, "hf_repo": None, "license": None, "sha256": ""})

    src_dir = args.llama_cpp / "models" / "templates"
    for name, entry in entries.items():
        shutil.copyfile(src_dir / name, HERE / "templates" / name)
        entry["sha256"] = hashlib.sha256((HERE / "templates" / name).read_bytes()).hexdigest()

    commit = subprocess.run(["git", "-C", str(args.llama_cpp), "rev-parse", "HEAD"], check=True,
                            capture_output=True, text=True).stdout.strip()
    sources["llama_cpp_commit"] = commit
    sources["origin"] = f"https://github.com/ggml-org/llama.cpp/tree/{commit}/models/templates"
    sources["templates"] = sorted(entries.values(), key=lambda e: e["file"].lower())
    sources_path.write_text(json.dumps(sources, indent=1) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
