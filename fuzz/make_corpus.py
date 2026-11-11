#!/usr/bin/env python3
"""Builds the seed corpus for the fuzz targets (fuzz/README.md).

Seeds are the templates the project already has: every case of the parity corpus
(test/parity/expected/*.json, valid and invalid templates alike, plus the templates they
include), the template literals of the unit tests (test/*.cpp), test/test_data and the
regression inputs in fuzz/regressions/. Files are named by content hash, as libFuzzer
names its own, so re-running the script into a grown corpus only adds what is missing.

The untrusted, host, loader and alloc_failure targets read a few bytes before the template
(fuzz_common.h, docs/tasks/0267); --prefix TARGET writes each seed with the default bytes and
once more with bytes picked from its hash, its delimiters rewritten to the ones they select.
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


# fuzz_common.h DelimiterSet: variable start/end, block start/end, comment start/end
DELIMITERS = [
    ("{{", "}}", "{%", "%}", "{#", "#}"),
    ("${", "}", "<%", "%>", "<#", "#>"),
    ("\\VAR{", "}", "\\BLOCK{", "}", "\\#{", "}"),
    ("[[", "]]", "[%", "%]", "[#", "#]"),
]
DELIMITER_SHIFT = 4  # fuzz_common.h settings_byte
WIDE = 0x80
# The bytes before the template, and whether the target reads the wide bit
PREFIXES = {"untrusted": (1, True), "host": (1, True), "loader": (2, False), "alloc_failure": (3, False)}


def with_delimiters(data, index):
    pattern = re.compile(b"{{|}}|{%|%}|{#|#}")
    table = dict(zip(DELIMITERS[0], DELIMITERS[index]))
    return pattern.sub(lambda m: table[m.group(0).decode()].encode(), data)


def prefixed(data, target):
    """The seed with the default prefix, and with one picked from its hash"""
    length, wide = PREFIXES[target]
    yield bytes(length) + data
    digest = hashlib.sha1(data).digest()
    settings = digest[0] if wide else digest[0] & ~WIDE
    body = with_delimiters(data, (settings >> DELIMITER_SHIFT) & 3)
    if target == "loader":
        yield bytes([settings, digest[1] & 1]) + body
    elif target == "alloc_failure":
        # Small templates make a few hundred allocations; the fuzzer grows the count from there
        yield bytes([digest[1], 0, settings]) + body
    else:
        yield bytes([settings]) + body


def file_templates(directory):
    if directory.is_dir():
        for path in sorted(directory.iterdir()):
            if path.is_file() and not path.name.startswith("."):
                yield path.read_bytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("out", type=pathlib.Path, help="corpus directory (created if missing)")
    parser.add_argument("--prefix", choices=sorted(PREFIXES),
                        help="write the seeds for this target, with its prefix bytes")
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
            if not data:
                continue
            for seed in prefixed(data, args.prefix) if args.prefix else [data]:
                if len(seed) > MAX_SIZE:
                    continue
                total += 1
                target = args.out / hashlib.sha1(seed).hexdigest()
                if not target.exists():
                    target.write_bytes(seed)
                    added += 1
    print(f"{total} seeds, {added} new files in {args.out}", file=sys.stderr)


if __name__ == "__main__":
    main()
