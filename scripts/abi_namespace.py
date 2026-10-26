#!/usr/bin/env python3
"""Keep every `namespace jinja2` in include/, bindings/ and src/ inside the inline ABI namespace (docs/tasks/0076).

    scripts/abi_namespace.py            # rewrite the files in place
    scripts/abi_namespace.py --check    # list what would change, exit 1 if anything would

Public and internal code live in `jinja2::v2`, an inline namespace: users still write
`jinja2::Value`, but the mangled names carry the major version, so a program built against
1.x headers fails to link with a 2.x library instead of misbehaving. The tag is spelled out
in every file rather than hidden behind a macro (house clang-format and clang-tidy handle the
literal form); this script writes it so that nobody has to:

    namespace jinja2            namespace jinja2
    {                           {
    ...                   ->    inline namespace v2
    }                           {
                                ...
                                } // namespace v2
                                }

A nested `namespace jinja2::detail` becomes `namespace jinja2 { inline namespace v2 {
namespace detail` (C++17 cannot nest an inline namespace in that form). An opening already
followed by `inline namespace vN` with another N gets the new tag, so the 3.0 retag is one
run with TAG changed. The vendored polymorphic_value headers are rewritten in their own
upstream brace style; robin_hood.h and lexertk.h do not use namespace jinja2.
"""

import argparse
import os
import re
import subprocess
import sys

TAG = "v2"

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
ROOTS = ("include", "bindings", "src")
OPENING = re.compile(r"^namespace jinja2(?P<nested>(?:::\w+)*)(?P<brace>\s*\{)?\s*$")
ANY_OPENING = re.compile(r"(?<!using )\bnamespace\s+jinja2\b")
INLINE = re.compile(r"^inline namespace (?P<tag>v\d+)(?P<brace>\s*\{)?\s*$")
CLOSE_COMMENT = re.compile(r"^\}(?P<space>\s*)//\s*namespace\s+(?P<name>[\w:]+)\s*$")


def code_mask(text):
    """Copy of text with comments, string and character literals blanked, newlines kept."""
    out = list(text)
    i, n = 0, len(text)

    def blank(a, b):
        for k in range(a, b):
            if out[k] != "\n":
                out[k] = " "

    while i < n:
        c = text[i]
        if text.startswith("//", i):
            j = text.find("\n", i)
            j = n if j < 0 else j
            blank(i, j)
            i = j
        elif text.startswith("/*", i):
            j = text.find("*/", i + 2)
            j = n if j < 0 else j + 2
            blank(i, j)
            i = j
        elif c == '"' or c == "'":
            # R"delim( ... )delim", with any encoding prefix
            prefix = re.search(r"(?:u8|[uUL])?R$", text[max(0, i - 3) : i])
            before = i - len(prefix.group(0)) - 1 if prefix else -1
            if c == '"' and prefix and (before < 0 or not (text[before].isalnum() or text[before] == "_")):
                m = re.match(r'"([^(\s]*)\(', text[i:])
                if m:
                    end = text.find(")" + m.group(1) + '"', i + len(m.group(0)))
                    end = n if end < 0 else end + len(m.group(1)) + 2
                    blank(i + 1, end - 1)
                    i = end
                    continue
            if c == "'" and i > 0 and text[i - 1].isalnum() and i + 1 < n and text[i + 1].isalnum():
                i += 1  # digit separator: 1'000
                continue
            j = i + 1
            while j < n and text[j] != c and text[j] != "\n":
                j += 2 if text[j] == "\\" else 1
            blank(i + 1, min(j, n))
            i = j + 1
        else:
            i += 1
    return "".join(out)


def matching_close(mask_lines, line, col):
    """Line index of the `}` closing the `{` at (line, col) of the masked lines."""
    depth = 0
    for li in range(line, len(mask_lines)):
        start = col if li == line else 0
        for ch in mask_lines[li][start:]:
            if ch == "{":
                depth += 1
            elif ch == "}":
                depth -= 1
                if depth == 0:
                    return li
    raise ValueError("unbalanced braces")


def brace_after(mask_lines, line):
    """(line, col) of the first `{` at or after the end of `line` (the opening's brace)."""
    for li in range(line, len(mask_lines)):
        col = mask_lines[li].find("{")
        if col >= 0:
            return li, col
        if li > line and mask_lines[li].strip():
            break
    raise ValueError("namespace opening without a brace")


def next_code_line(mask_lines, line):
    for li in range(line + 1, len(mask_lines)):
        if mask_lines[li].strip():
            return li
    return None


def rewrite(text):
    """Return text with every namespace jinja2 opening wrapped in `inline namespace TAG`."""
    lines = text.split("\n")
    # Each pass fixes the first opening that needs it; the next pass re-reads the result.
    for _ in range(10000):
        mask_lines = code_mask("\n".join(lines)).split("\n")
        edit = None
        for li, raw in enumerate(lines):
            m = OPENING.match(mask_lines[li].rstrip())
            if not m:
                continue
            same_line = bool(m.group("brace"))
            open_line, open_col = brace_after(mask_lines, li)
            close_line = matching_close(mask_lines, open_line, open_col)
            close_text = lines[close_line]
            cm = CLOSE_COMMENT.match(close_text.strip())
            space = cm.group("space") if cm else " "
            if m.group("nested"):
                inner = m.group("nested")[2:]
                head = ["namespace jinja2", "{", f"inline namespace {TAG}", "{", f"namespace {inner}"]
                if same_line:
                    head = ["namespace jinja2 {", f"inline namespace {TAG} {{", f"namespace {inner} {{"]
                closing = "}" + space + f"// namespace {inner}" if cm else close_text
                edit = (li, li + 1, head, close_line, [closing, "}" + space + f"// namespace {TAG}", "}" + space + "// namespace jinja2"])
                break
            nxt = next_code_line(mask_lines, open_line if not same_line else li)
            im = INLINE.match(mask_lines[nxt].strip()) if nxt is not None else None
            if im:
                if im.group("tag") == TAG:
                    continue
                inline_brace_line, inline_col = brace_after(mask_lines, nxt)
                inline_close = matching_close(mask_lines, inline_brace_line, inline_col)
                lines[nxt] = lines[nxt].replace(im.group("tag"), TAG, 1)
                icm = CLOSE_COMMENT.match(lines[inline_close].strip())
                if icm:
                    lines[inline_close] = "}" + icm.group("space") + f"// namespace {TAG}"
                edit = ()
                break
            insert = [f"inline namespace {TAG} {{"] if same_line else [f"inline namespace {TAG}", "{"]
            edit = (open_line + 1, open_line + 1, insert, close_line, ["}" + space + f"// namespace {TAG}", close_text])
            break
        if edit is None:
            # Any other spelling (indented, one-line `namespace jinja2 { ... }`) would stay
            # outside v2 unnoticed: a free function in it becomes a second overload
            for li, code in enumerate(mask_lines):
                if ANY_OPENING.search(code) and not OPENING.match(code.rstrip()):
                    raise ValueError(f"line {li + 1}: write `namespace jinja2` alone at the start of a line")
            return "\n".join(lines)
        if edit == ():
            continue
        a, b, new_open, close_line, new_close = edit
        lines[close_line:close_line + 1] = new_close
        lines[a:b] = new_open
    raise RuntimeError("too many namespace openings")


def files():
    out = subprocess.run(["git", "ls-files", "--", *ROOTS], cwd=REPO, capture_output=True, text=True, check=True).stdout
    for path in out.split():
        if path.endswith((".h", ".cpp")) and not path.endswith(("robin_hood.h", "lexertk.h")):
            yield path


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--check", action="store_true", help="change nothing; exit 1 if a file would change")
    args = parser.parse_args()

    changed = []
    for path in files():
        full = os.path.join(REPO, path)
        with open(full, encoding="utf-8", newline="") as f:
            text = f.read()
        try:
            new = rewrite(text)
        except ValueError as e:
            print(f"{path}: {e}", file=sys.stderr)
            return 2
        if new != text:
            changed.append(path)
            if not args.check:
                with open(full, "w", encoding="utf-8", newline="") as f:
                    f.write(new)

    if args.check:
        for path in changed:
            print(f"{path}: namespace jinja2 not wrapped in inline namespace {TAG}")
        if changed:
            print("Run scripts/abi_namespace.py and commit the result (docs/tasks/0076).")
            return 1
        return 0
    print(f"{len(changed)} files rewritten")
    return 0


if __name__ == "__main__":
    sys.exit(main())
