#!/usr/bin/env python3
"""Builds the seed corpus for the fuzz targets (fuzz/README.md).

Seeds are the templates the project already has: every case of the parity corpus
(test/parity/expected/*.json, valid and invalid templates alike, plus the templates they
include), the template literals of the unit tests (test/*.cpp), test/test_data and the
regression inputs in fuzz/regressions/. Files are named by content hash, as libFuzzer
names its own, so re-running the script into a grown corpus only adds what is missing.
"""
import argparse
import ast
import hashlib
import json
import pathlib
import re
import sys

ROOT = pathlib.Path(__file__).resolve().parent.parent
MAX_SIZE = 4096  # fuzz_common.h MaxInputSize

RAW_STRING = re.compile(r'R"([^()\s\\]{0,16})\((.*?)\)\1"', re.S)
PLAIN_STRING = re.compile(r'L?"((?:[^"\\\n]|\\.)*)"')


def parity_templates():
    for path in sorted((ROOT / "test/parity/expected").glob("*.json")):
        data = json.loads(path.read_text(encoding="utf-8"))
        for case in data.get("cases", []):
            if isinstance(case.get("template"), str):
                yield case["template"]
            for text in (case.get("templates") or {}).values():
                if isinstance(text, str):
                    yield text


def unit_test_templates():
    for path in sorted((ROOT / "test").glob("*.cpp")):
        source = path.read_text(encoding="utf-8", errors="replace")
        for match in RAW_STRING.finditer(source):
            yield match.group(2)
        for match in PLAIN_STRING.finditer(source):
            body = match.group(1)
            if "{{" not in body and "{%" not in body and "{#" not in body:
                continue
            try:
                # C and Python escapes agree on everything the tests use
                yield ast.literal_eval('"' + body + '"')
            except (ValueError, SyntaxError):
                yield body


def file_templates(directory):
    if directory.is_dir():
        for path in sorted(directory.iterdir()):
            if path.is_file() and not path.name.startswith("."):
                yield path.read_bytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("out", type=pathlib.Path, help="corpus directory (created if missing)")
    parser.add_argument("--parity-only", action="store_true",
                        help="only the parity corpus: differential.py --known skips what it already covers")
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)

    sources = [parity_templates()]
    if not args.parity_only:
        sources += [unit_test_templates(), file_templates(ROOT / "test/test_data"), file_templates(ROOT / "fuzz/regressions")]
    added = total = 0
    for source in sources:
        for item in source:
            data = item if isinstance(item, bytes) else item.encode("utf-8", errors="surrogatepass")
            if not data or len(data) > MAX_SIZE:
                continue
            total += 1
            target = args.out / hashlib.sha1(data).hexdigest()
            if not target.exists():
                target.write_bytes(data)
                added += 1
    print(f"{total} seeds, {added} new files in {args.out}", file=sys.stderr)


if __name__ == "__main__":
    main()
