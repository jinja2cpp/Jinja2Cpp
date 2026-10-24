#!/usr/bin/env python3
"""Generates src/unicode_names.h: the Unicode character names Python's \\N{...} escape accepts.

Jinja2 decodes string literals with Python's unicode-escape codec, whose \\N{name} looks the
name up in CPython's name database. That database computes two families of names and stores
the others:

- "CJK UNIFIED IDEOGRAPH-XXXX" and "HANGUL SYLLABLE ..." are computed, and matched case
  sensitively (4 or 5 upper-case hex digits; the Hangul jamo names greedily). They are not
  in the table: unicode_names.cpp computes them from the ranges written here.
- Every other name and the name aliases (scripts/unicode_name_aliases.txt, Unicode's
  NameAliases.txt, which Python cannot list) are matched without regard to ASCII case.
  The "...-XXXX" names of the CJK compatibility ideographs, Nushu and Khitan characters are
  stored too, so they are written here as ranges and matched like stored names.

The other stored names are written as words: a word list in byte order, each word with a code
(its frequency rank, so that the frequent ones take one byte), and the names as their codes,
sorted, with an index of every 16th word and name for a binary search. The table follows the
Unicode database of the Python that runs this script, so it must be the Python the parity
oracle runs on (.github/workflows/parity-expectations.yml); the script refuses any other.

    python3.12 scripts/gen_unicode_names.py > src/unicode_names.h
    python3.12 scripts/gen_unicode_names.py --check src/unicode_names.h
"""
import codecs
import collections
import io
import pathlib
import sys
import unicodedata

ORACLE_PYTHON = (3, 12)
ALIASES = pathlib.Path(__file__).resolve().parent / "unicode_name_aliases.txt"

# Computed by CPython, matched case sensitively
COMPUTED = ("CJK UNIFIED IDEOGRAPH-", "HANGUL SYLLABLE ")
# Stored by CPython, but "<prefix><hex code point>": written as ranges
HEX_NAMED = ("CJK COMPATIBILITY IDEOGRAPH-", "NUSHU CHARACTER-", "KHITAN SMALL SCRIPT CHARACTER-")

# Word indices below ONE_BYTE take one byte, the others two: ONE_BYTE + high, low
ONE_BYTE = 192
END_OF_NAME = 255
# Every BLOCK-th word and name has its offset in an index, for a binary search
BLOCK = 16


def ranges(cps):
    result = []
    for cp in sorted(cps):
        if result and result[-1][1] == cp - 1:
            result[-1][1] = cp
        else:
            result.append([cp, cp])
    return result


def decode(name):
    try:
        return codecs.decode(("\\N{" + name + "}").encode("ascii"), "unicode-escape")
    except UnicodeDecodeError:
        return None


def collect():
    stored = []
    families = {prefix: [] for prefix in COMPUTED + HEX_NAMED}
    for cp in range(0x110000):
        name = unicodedata.name(chr(cp), None)
        if name is None:
            continue
        prefix = next((p for p in families if name.startswith(p)), None)
        if prefix is None:
            stored.append((cp, name))
            continue
        if prefix != "HANGUL SYLLABLE " and name != "%s%04X" % (prefix, cp):
            sys.exit("unexpected name %r of U+%04X" % (name, cp))
        families[prefix].append(cp)

    aliases = []
    for line in ALIASES.read_text(encoding="utf-8").splitlines():
        if not line or line.startswith("#"):
            continue
        cp, alias = line.split(";")
        cp = int(cp, 16)
        if decode(alias) != chr(cp) or decode(alias.lower()) != chr(cp):
            sys.exit("Python does not resolve the alias %r to U+%04X" % (alias, cp))
        aliases.append((alias, cp))
    return stored, families, aliases


def token_bytes(indices):
    data = bytearray()
    for i in indices:
        if i < ONE_BYTE:
            data.append(i)
        else:
            data.append(ONE_BYTE + (i >> 8))
            data.append(i & 0xFF)
    data.append(END_OF_NAME)
    return bytes(data)


def encode_names(stored):
    counts = collections.Counter(word for _, name in stored for word in name.split(" "))
    by_frequency = sorted(counts, key=lambda w: (-counts[w], w))
    if len(by_frequency) > (END_OF_NAME - ONE_BYTE) * 256:
        sys.exit("too many words for two-byte indices")
    code = {word: i for i, word in enumerate(by_frequency)}

    # Words in byte order, each as its length, its letters and its code (2 bytes, high first)
    words = sorted(by_frequency)
    word_data = bytearray()
    word_index = []
    for n, word in enumerate(words):
        if n % BLOCK == 0:
            word_index.append(len(word_data))
        if not 0 < len(word) < 256:
            sys.exit("bad word %r" % word)
        word_data.append(len(word))
        word_data.extend(word.encode("ascii"))
        word_data.extend(code[word].to_bytes(2, "big"))

    # Names in the byte order of their codes, each as its code point (3 bytes, high first)
    # and its codes
    keyed = sorted((token_bytes(code[w] for w in name.split(" ")), cp) for cp, name in stored)
    name_data = bytearray()
    name_index = []
    for n, (key, cp) in enumerate(keyed):
        if n % BLOCK == 0:
            name_index.append(len(name_data))
        name_data.extend(cp.to_bytes(3, "big"))
        name_data.extend(key)
    return words, word_data, word_index, name_data, name_index


def byte_rows(data, out):
    for i in range(0, len(data), 24):
        out.write("    " + ",".join(str(b) for b in data[i:i + 24]) + ",\n")


def generate(out):
    stored, families, aliases = collect()
    words, word_data, word_index, name_data, name_index = encode_names(stored)

    out.write("// Generated by scripts/gen_unicode_names.py from Python %d.%d (Unicode %s).\n"
              % (sys.version_info[0], sys.version_info[1], unicodedata.unidata_version))
    out.write("// Do not edit by hand. Included by src/unicode_names.cpp only.\n")
    out.write("#ifndef JINJA2CPP_SRC_UNICODE_NAMES_H\n#define JINJA2CPP_SRC_UNICODE_NAMES_H\n\n")
    out.write("#include <cstdint>\n\n")
    out.write("namespace jinja2\n{\ninline namespace v2\n{\nnamespace unicode_names\n{\n\n")
    out.write("constexpr unsigned OneByteWords = %d;\nconstexpr std::uint8_t EndOfName = %d;\nconstexpr unsigned BlockSize = %d;\n"
              % (ONE_BYTE, END_OF_NAME, BLOCK))
    out.write("// The longest stored name or alias, in letters\nconstexpr unsigned MaxNameLength = %d;\n\n"
              % max(len(n) for n in [name for _, name in stored] + [alias for alias, _ in aliases]))
    out.write("struct Range\n{\n    std::uint32_t first;\n    std::uint32_t last;\n};\n\n")
    out.write("struct Alias\n{\n    const char* name;\n    std::uint32_t cp;\n};\n\n")
    out.write("// clang-format off\n")
    for name, prefix in (("CjkUnified", COMPUTED[0]), ("Hangul", COMPUTED[1]), ("CjkCompatibility", HEX_NAMED[0]),
                         ("Nushu", HEX_NAMED[1]), ("Khitan", HEX_NAMED[2])):
        rows = ranges(families[prefix])
        out.write("// The code points named \"%s...\"\nconstexpr Range %sRanges[] = {\n" % (prefix, name))
        for first, last in rows:
            out.write("    {0x%X, 0x%X},\n" % (first, last))
        out.write("};\n")
    out.write("\n// Name aliases, upper case\nconstexpr Alias Aliases[] = {\n")
    for alias, cp in aliases:
        out.write("    {\"%s\", 0x%X},\n" % (alias, cp))
    out.write("};\n\n")
    out.write("// %d words in byte order, each as its length, its letters and its code: the rank of its\n"
              "// frequency, high byte first\n" % len(words))
    out.write("constexpr std::uint8_t Words[] = {\n")
    byte_rows(word_data, out)
    out.write("};\n// The offset in Words of every BlockSize-th word\nconstexpr std::uint32_t WordIndex[] = {\n")
    for i in range(0, len(word_index), 12):
        out.write("    " + ",".join(str(v) for v in word_index[i:i + 12]) + ",\n")
    out.write("};\n\n")
    out.write("// %d names in the byte order of their codes, each as its code point (3 bytes, high first),\n"
              "// the codes of its words (below OneByteWords one byte, else OneByteWords + high byte, low\n"
              "// byte) and EndOfName\n" % len(stored))
    out.write("constexpr std::uint8_t Names[] = {\n")
    byte_rows(name_data, out)
    out.write("};\n// The offset in Names of every BlockSize-th name\nconstexpr std::uint32_t NameIndex[] = {\n")
    for i in range(0, len(name_index), 12):
        out.write("    " + ",".join(str(v) for v in name_index[i:i + 12]) + ",\n")
    out.write("};\n// clang-format on\n\n")
    out.write("} // namespace unicode_names\n} // namespace v2\n} // namespace jinja2\n\n#endif // JINJA2CPP_SRC_UNICODE_NAMES_H\n")


def main():
    if sys.version_info[:2] != ORACLE_PYTHON:
        sys.exit("run with Python %d.%d, the parity oracle's version (found %d.%d)"
                 % (ORACLE_PYTHON + sys.version_info[:2]))
    if len(sys.argv) == 3 and sys.argv[1] == "--check":
        expected = io.StringIO()
        generate(expected)
        with open(sys.argv[2], encoding="utf-8") as f:
            if f.read() != expected.getvalue():
                sys.exit("%s is stale: regenerate it with %s" % (sys.argv[2], sys.argv[0]))
        return
    generate(sys.stdout)


if __name__ == "__main__":
    main()
