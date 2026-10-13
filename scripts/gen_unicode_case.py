#!/usr/bin/env python3
"""Generate src/unicode_case.h: the full Unicode case mappings of str.upper(), str.lower()
and str.title()'s per-character titlecase, plus the Cased and Case_Ignorable sets that
decide Python's Final_Sigma rule.

The data is read from the Python that runs this script (unicodedata.unidata_version,
written into the header), so it matches that Python's str methods. Regenerate with
    python3 scripts/gen_unicode_case.py && clang-format -i src/unicode_case.h
"""
import pathlib
import sys
import unicodedata

ROOT = pathlib.Path(__file__).resolve().parent.parent
MAX = 0x110000


def mappings(fn):
    """{cp: [mapped code points]} for every code point the mapping changes."""
    result = {}
    for cp in range(MAX):
        if 0xD800 <= cp <= 0xDFFF:
            continue
        mapped = fn(chr(cp))
        if mapped != chr(cp):
            result[cp] = [ord(ch) for ch in mapped]
    return result


def split(maps):
    """Single code point mappings as runs, the others as rows of a table."""
    single = {cp: m[0] for cp, m in maps.items() if len(m) == 1}
    multi = {cp: m for cp, m in maps.items() if len(m) > 1}
    runs = []
    cps = sorted(single)
    i = 0
    while i < len(cps):
        first = cps[i]
        delta = single[first] - first
        stride = 1
        if i + 1 < len(cps) and cps[i + 1] == first + 1 and single[first + 1] - (first + 1) == delta:
            stride = 1
        elif i + 1 < len(cps) and cps[i + 1] == first + 2 and first + 1 not in maps and single[first + 2] - (first + 2) == delta:
            stride = 2
        last = first
        j = i + 1
        while j < len(cps) and cps[j] == last + stride and single[cps[j]] - cps[j] == delta:
            if stride == 2 and last + 1 in maps:
                break
            last = cps[j]
            j += 1
        runs.append((first, last, stride if last != first else 1, delta))
        i = j
    return runs, sorted(multi.items())


def ranges(pred):
    result, start = [], None
    for cp in range(MAX):
        if pred(cp):
            if start is None:
                start = cp
        elif start is not None:
            result.append((start, cp - 1))
            start = None
    if start is not None:
        result.append((start, MAX - 1))
    return result


def cased(cp):
    # 'cΣ' lowers to a final sigma when c is cased and not case-ignorable
    return not 0xD800 <= cp <= 0xDFFF and (chr(cp) + "Σ").lower()[-1] == "ς"


def case_ignorable(cp):
    # A case-ignorable code point is skipped whether or not it is also cased, so only the
    # skipped set matters: 'AcΣ' ends in a final sigma when c is skipped or c is cased
    return not 0xD800 <= cp <= 0xDFFF and not cased(cp) and ("A" + chr(cp) + "Σ").lower()[-1] == "ς"


def run_table(name, comment, runs):
    body = "\n".join("    {{ 0x{:04X}, 0x{:04X}, {}, {} }},".format(f, l, s, d) for f, l, s, d in runs)
    return "// {}\nconstexpr CaseRun {}Runs[] = {{\n{}\n}};\n".format(comment, name, body)


def multi_table(name, comment, rows):
    def row(cp, m):
        cps = m + [0] * (3 - len(m))
        return "    {{ 0x{:04X}, {{ 0x{:04X}, 0x{:04X}, 0x{:04X} }} }},".format(cp, *cps)

    return "// {}\nconstexpr CaseMulti {}Multi[] = {{\n{}\n}};\n".format(comment, name, "\n".join(row(cp, m) for cp, m in rows))


def range_table(name, comment, rows):
    body = "\n".join("    {{ 0x{:04X}, 0x{:04X} }},".format(a, b) for a, b in rows)
    return "// {}\nconstexpr CodePointRange {}Ranges[] = {{\n{}\n}};\n".format(comment, name, body)


HELPERS = """// Maps cp with the single-code-point runs and the multi-code-point rows; returns how many
// code points were written to out (1 when cp does not change)
template<size_t R, size_t M>
size_t MapCase(const CaseRun (&runs)[R], const CaseMulti (&multi)[M], uint32_t cp, uint32_t (&out)[3])
{
    auto multiIt = std::lower_bound(std::begin(multi), std::end(multi), cp, [](const CaseMulti& row, uint32_t value) { return row.cp < value; });
    if (multiIt != std::end(multi) && multiIt->cp == cp)
    {
        size_t n = 0;
        for (; n != 3 && multiIt->to[n] != 0; ++n)
        {
            out[n] = multiIt->to[n];
        }
        return n;
    }
    out[0] = cp;
    auto it = std::upper_bound(std::begin(runs), std::end(runs), cp, [](uint32_t value, const CaseRun& r) { return value < r.first; });
    if (it != std::begin(runs))
    {
        const auto& run = *std::prev(it);
        if (cp <= run.last && (cp - run.first) % run.stride == 0)
        {
            out[0] = static_cast<uint32_t>(static_cast<int64_t>(cp) + run.delta);
        }
    }
    return 1;
}

// str.upper(), str.lower() and the titlecase of str.capitalize() for one code point
inline size_t ToUpper(uint32_t cp, uint32_t (&out)[3]) { return MapCase(UpperRuns, UpperMulti, cp, out); }
inline size_t ToLower(uint32_t cp, uint32_t (&out)[3]) { return MapCase(LowerRuns, LowerMulti, cp, out); }
inline size_t ToTitle(uint32_t cp, uint32_t (&out)[3]) { return MapCase(TitleRuns, TitleMulti, cp, out); }

// The Cased (and not Case_Ignorable) and Case_Ignorable sets of CPython's Final_Sigma rule
inline bool IsCased(uint32_t cp) { return InRanges(CasedRanges, cp); }
inline bool IsCaseIgnorable(uint32_t cp) { return InRanges(CaseIgnorableRanges, cp); }
"""


def main():
    parts = [
        "// Generated by scripts/gen_unicode_case.py from Unicode {}; do not edit.\n".format(unicodedata.unidata_version),
        "#ifndef JINJA2CPP_SRC_UNICODE_CASE_H\n#define JINJA2CPP_SRC_UNICODE_CASE_H\n",
        '#include "unicode_tables.h"\n\n#include <algorithm>\n#include <cstddef>\n#include <cstdint>\n#include <iterator>\n',
        "namespace jinja2::unicode\n{\n",
        "// cp + delta for every stride-th code point from first to last\n"
        "struct CaseRun\n{\n    uint32_t first;\n    uint32_t last;\n    uint32_t stride;\n    int32_t delta;\n};\n",
        "// A mapping to two or three code points; unused entries are 0\n"
        "struct CaseMulti\n{\n    uint32_t cp;\n    uint32_t to[3];\n};\n",
    ]
    for name, fn in (("Upper", str.upper), ("Lower", str.lower), ("Title", str.title)):
        runs, multi = split(mappings(fn))
        parts.append(run_table(name, "str.{}() of one code point, single code point results".format(name.lower()), runs))
        parts.append(multi_table(name, "str.{}() of one code point, longer results".format(name.lower()), multi))
    parts.append(range_table("Cased", "Cased and not Case_Ignorable", ranges(cased)))
    parts.append(range_table("CaseIgnorable", "Case_Ignorable (cased or not)", ranges(case_ignorable)))
    parts.append(HELPERS)
    parts.append("} // namespace jinja2::unicode\n\n#endif // JINJA2CPP_SRC_UNICODE_CASE_H\n")
    target = ROOT / "src" / "unicode_case.h"
    text = "\n".join(parts)
    # --check: CI (parity-expectations.yml) fails when the committed header is stale
    if "--check" in sys.argv[1:]:
        if target.read_text(encoding="utf-8") != text:
            sys.exit("%s is stale: regenerate it with %s" % (target.relative_to(ROOT), sys.argv[0]))
        return
    target.write_text(text, encoding="utf-8")


if __name__ == "__main__":
    main()
