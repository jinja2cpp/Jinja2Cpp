#!/usr/bin/env python3
"""Moves C++ code from the Jinja2C++ 1.x API to 2.0 (docs/api-2.0.md section 5.2, docs/tasks/0077).

Two passes, usable separately:

  --log FILE   Reads a build log (GCC, Clang or MSVC output; `-` is stdin) and rewrites every
               deprecated 1.x name the compiler reported. The 1.x names are marked
               JINJA2CPP_DEPRECATED("jinja2cpp-2: <NewName>"), so the compiler has already
               resolved which calls are on a jinja2 type; the script replaces the old
               identifier at the reported location with <NewName>.
  --rewrite P  Applies a small table of regexes to the files under P for the changes that
               are compile errors rather than warnings (renamed Settings fields, removed
               headers, ...). Run it first: code that does not compile has no warnings.

Typical use:

  python3 jinja2cpp_migrate.py --rewrite src/
  cmake --build build 2>&1 | tee build.log          # with -Wdeprecated-declarations (the default)
  python3 jinja2cpp_migrate.py --log build.log --root .

Use `--dry-run` to see a unified diff and write nothing. GCC prints display columns by
default; `-fdiagnostics-column-unit=byte` makes them exact, but the script also finds the
name when the column is off, as long as it is on the reported line.

Requires Python 3.8 or later and the standard library only.
Exit status: 0 on success (also when nothing is left to do), 2 on usage errors.
"""

import argparse
import difflib
import os
import re
import sys

MARKER = "jinja2cpp-2: "
SOURCE_SUFFIXES = (".h", ".hpp", ".hh", ".hxx", ".c", ".cc", ".cpp", ".cxx", ".ipp", ".inl")

# ----------------------------------------------------------------------------------------------
# Build log parsing
# ----------------------------------------------------------------------------------------------

# Clang and GCC: path:line:col: warning: message. A leading indent or "In file included" text
# is allowed; the path may start with a Windows drive letter.
_GNU_LINE = re.compile(
    r"^\s*(?P<path>(?:[A-Za-z]:)?[^:\n]+?):(?P<line>\d+):(?P<col>\d+):\s*(?:warning|error):\s*(?P<msg>.*)$")
# MSVC: path(line[,col]): warning C4996: message, possibly followed by " [project.vcxproj]"
_MSVC_LINE = re.compile(
    r"^\s*(?P<path>.+?)\((?P<line>\d+)(?:,(?P<col>\d+))?\)\s*:\s*(?:warning|error)\s+C\d+:\s*(?P<msg>.*)$")
_PROJECT_SUFFIX = re.compile(r"\s*\[[^\]]*\.(?:vcxproj|vcproj|proj)\]\s*$")
_QUOTE_OPEN = "'`‘"
_QUOTE_CLOSE = "'’"
_NEW_NAME = re.compile(r"[^\s'\"’\[]+")


class Diagnostic:
    def __init__(self, path, line, col, old, new, col_in_bytes):
        self.path = path
        self.line = line
        self.col = col  # 1-based, or None when the log has no column
        self.old = old
        self.new = new
        self.col_in_bytes = col_in_bytes

    def key(self):
        return (self.path, self.line, self.col, self.old)


def _strip_balanced(text, open_ch, close_ch):
    """Removes every balanced open_ch...close_ch group, nested ones included."""
    out = []
    depth = 0
    for ch in text:
        if ch == open_ch:
            depth += 1
        elif ch == close_ch and depth:
            depth -= 1
        elif not depth:
            out.append(ch)
    return "".join(out)


def old_name_of(declaration):
    """The unqualified name a quoted declaration introduces.

    'bool jinja2::v2::Value::isString() const'                 -> isString
    'template<class T> auto jinja2::v2::Value::get()'          -> get
    'jinja2::v2::Value::get<std::string>'                      -> get
    'using jinja2::v2::UserCallable::UserCallableFunctionPtr = using ... Function = class std::function<...>'
                                                               -> UserCallableFunctionPtr
    """
    text = _strip_balanced(declaration, "<", ">")
    text = text.split("=", 1)[0].split("(", 1)[0]  # GCC spells an alias 'using A = ...'; '(' starts the parameters
    names = re.findall(r"[A-Za-z_]\w*", text)
    return names[-1] if names else None


def parse_diagnostic(text):
    """Returns a Diagnostic for a line that reports a jinja2cpp-2 deprecation, else None."""
    if MARKER not in text:
        return None
    text = text.rstrip("\r\n")
    match = _MSVC_LINE.match(text)
    col_in_bytes = False
    if match:
        msg = _PROJECT_SUFFIX.sub("", match.group("msg"))
        # 'jinja2::v2::Value::isString': jinja2cpp-2: IsString
        decl = re.match("[%s](?P<decl>.+?)[%s]\\s*:\\s*%s" % (_QUOTE_OPEN, _QUOTE_CLOSE, re.escape(MARKER)), msg)
    else:
        match = _GNU_LINE.match(text)
        if not match:
            return None
        msg = match.group("msg")
        col_in_bytes = True
        decl = re.match("[%s](?P<decl>.+?)[%s] is deprecated" % (_QUOTE_OPEN, _QUOTE_CLOSE), msg)
    if not decl or MARKER not in msg:
        return None
    old = old_name_of(decl.group("decl"))
    new_match = _NEW_NAME.match(msg[msg.index(MARKER) + len(MARKER):])
    if not old or not new_match:
        return None
    col = match.group("col")
    return Diagnostic(match.group("path").strip(), int(match.group("line")), int(col) if col else None,
                      old, new_match.group(0), col_in_bytes)


def parse_log(lines):
    """Diagnostics of the log, deduplicated by (file, line, column, old name), in log order."""
    seen = set()
    result = []
    for text in lines:
        diag = parse_diagnostic(text)
        if diag and diag.key() not in seen:
            seen.add(diag.key())
            result.append(diag)
    return result


# ----------------------------------------------------------------------------------------------
# Rewriting from diagnostics
# ----------------------------------------------------------------------------------------------

def _word_regex(name):
    return re.compile(r"(?<![\w])%s(?![\w])" % re.escape(name))


def _char_column(line, diag):
    """0-based index into the decoded line for the 1-based column of the log."""
    if diag.col_in_bytes:
        return len(line.encode("utf-8", "surrogateescape")[:diag.col - 1].decode("utf-8", "surrogateescape"))
    return diag.col - 1


def find_occurrences(line, diag):
    """Start offsets in `line` where the old name should be replaced.

    Clang's column is the identifier. GCC's can be the `(` of the call or elsewhere in the
    expression, so look back from the column first, then forward. MSVC may have no column:
    then every whole-word occurrence on the line is replaced.
    """
    word = _word_regex(diag.old)
    if diag.col is None:
        return [m.start() for m in word.finditer(line)]
    col = min(_char_column(line, diag), len(line))
    before = [m.start() for m in word.finditer(line) if m.start() <= col]
    if before:
        return [before[-1]]
    after = [m.start() for m in word.finditer(line) if m.start() > col]
    return after[:1]


def replacement_for(line, start, diag):
    """The new name at `start`; a qualified one drops the qualifier the source already spells."""
    new = diag.new
    if "::" in new:
        qualifier, last = new.rsplit("::", 1)
        if line[:start].endswith(qualifier + "::"):
            return last
    return new


def _is_library_header(path):
    parts = path.replace("\\", "/").split("/")
    return "jinja2cpp" in parts[:-1]


class Report:
    """What the run changed and what it left for a human."""

    def __init__(self):
        self.changes = []  # (path, line, old, new)
        self.human = []  # (where, text)

    def change(self, path, line, old, new):
        self.changes.append((path, line, old, new))

    def left(self, where, text):
        self.human.append((where, text))

    def print(self, out=None):
        out = out or sys.stdout
        print("Changes: %d" % len(self.changes), file=out)
        for path, line, old, new in sorted(self.changes):
            print("  %s:%d: %s -> %s" % (path, line, old, new), file=out)
        print("Left for a human: %d" % len(self.human), file=out)
        for where, text in self.human:
            print("  %s: %s" % (where, text), file=out)


def _read_text(path):
    with open(path, "rb") as f:
        return f.read().decode("utf-8", "surrogateescape")


def _write_or_diff(path, before, after, dry_run, out):
    if before == after:
        return
    if dry_run:
        out = out or sys.stdout
        diff = difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                                    path, path + " (migrated)")
        out.write("".join(diff))
        return
    with open(path, "wb") as f:
        f.write(after.encode("utf-8", "surrogateescape"))


def apply_diagnostics(diagnostics, root, report, dry_run=False, out=None):
    by_file = {}
    for diag in diagnostics:
        if _is_library_header(diag.path):
            report.left("%s:%d" % (diag.path, diag.line),
                        "use of %s inside the Jinja2C++ headers, not rewritten" % diag.old)
            continue
        path = diag.path if os.path.isabs(diag.path) else os.path.join(root, diag.path)
        by_file.setdefault(os.path.normpath(path), []).append(diag)

    for path, file_diags in sorted(by_file.items()):
        try:
            before = _read_text(path)
        except OSError as e:
            for diag in file_diags:
                report.left("%s:%d" % (diag.path, diag.line), "cannot read the file (%s)" % e.strerror)
            continue
        lines = before.split("\n")
        edits = {}  # (line index, offset) -> (old, new)
        for diag in file_diags:
            where = "%s:%d" % (diag.path, diag.line)
            if not 1 <= diag.line <= len(lines):
                report.left(where, "line %d is outside the file" % diag.line)
                continue
            starts = find_occurrences(lines[diag.line - 1], diag)
            if not starts:
                report.left(where, "%s is not on the reported line (macro expansion or generated code?),"
                            " wanted %s" % (diag.old, diag.new))
            for start in starts:
                edits[(diag.line - 1, start)] = (diag.old, replacement_for(lines[diag.line - 1], start, diag))
        # Right to left, so the offsets of the edits still to do stay valid
        for (index, start), (old, new) in sorted(edits.items(), reverse=True):
            line = lines[index]
            lines[index] = line[:start] + new + line[start + len(old):]
            report.change(path, index + 1, old, new)
        _write_or_diff(path, before, "\n".join(lines), dry_run, out)


# ----------------------------------------------------------------------------------------------
# The table for what does not compile: --rewrite
# ----------------------------------------------------------------------------------------------

_ERROR_INFO_ARG = re.compile(r"^(?:jinja2::(?:v2::)?)?(ErrorInfo|ErrorInfoW)$")
_EXPECTED_OPEN = re.compile(r"(?<![\w:])nonstd::expected\s*<")
_UNEXPECTED_RECEIVER = re.compile(
    r"(?<![\w.>:\)\]])(?P<recv>[A-Za-z_]\w*(?:(?:\.|->)[A-Za-z_]\w*)*)(?:\.|->)get_unexpected\s*\(\s*\)")
_BRACKETS = {"<": ">", "(": ")", "[": "]", "{": "}"}

# Rows that replace text: (regex, replacement template). All of them keep the line count.
_SIMPLE_ROWS = [
    (re.compile(r"(\.|->)m_defaultMetadataType\b"), r"\1defaultMetadataType"),
    (re.compile(r"(\bextensions\s*(?:\.|->)\s*)Do\b"), r"\1doStatement"),
    (re.compile(r"\buseLineStatements(\s*)=(\s*)true\b"), r'lineStatementPrefix\1=\2"#"'),
    # Deprecated, but Clang does not warn about a deprecated alias template, so the log never names it
    (re.compile(r"\bErrorInfoTpl\b"), "BasicErrorInfo"),
]
# Rows that remove a whole line
_LINE_REMOVALS = [
    re.compile(r"^[ \t]*[\w.\->]*\buseLineStatements[ \t]*=[ \t]*false[ \t]*;[ \t]*(?://[^\n]*)?\n", re.MULTILINE),
    re.compile(r"^[ \t]*#[ \t]*include[ \t]*[<\"]jinja2cpp/error_handler\.h[>\"][^\n]*\n", re.MULTILINE),
]
# Rows that only report: (regex, what to tell the human)
_REPORT_ROWS = [
    (re.compile(r"\buseLineStatements\b"),
     'useLineStatements is gone; use lineStatementPrefix = "#" (an empty prefix disables line statements)'),
    (re.compile(r"\b[jJ]inja2CompatMode\b"),
     "jinja2CompatMode was removed (the library never read it): delete the line"),
    (re.compile(r"\bIErrorHandler\b"), "IErrorHandler was removed (an empty class, used nowhere): delete its uses"),
    (re.compile(r"#[ \t]*include[ \t]*[<\"]jinja2cpp/polymorphic_value\.h[>\"]"),
     "jinja2cpp/polymorphic_value.h was removed; see MIGRATION.md"),
    (re.compile(r"#[ \t]*include[ \t]*[<\"]jinja2cpp/binding/rapid_json\.h[>\"]"),
     'RapidJSON binding removed; see MIGRATION.md "RapidJSON binding removed"'),
    (re.compile(r"\bget_unexpected\s*\("),
     "get_unexpected() on this receiver: write nonstd::make_unexpected(r.error()) by hand"),
]


def _template_arguments(text, open_index):
    """Splits the template argument list that starts after text[open_index] == '<'.

    Returns (arguments, index after the closing '>'), or None when the brackets do not
    balance before a ';' (not a template argument list after all).
    """
    args = []
    stack = []  # the closing brackets still to come
    start = open_index + 1
    for i in range(start, len(text)):
        ch = text[i]
        if ch == "<":
            if not stack or stack[-1] == ">":  # inside parentheses it is a comparison
                stack.append(">")
        elif ch in "([{":
            stack.append(_BRACKETS[ch])
        elif ch in ")]}":
            if not stack or stack.pop() != ch:
                return None
        elif ch == ">":
            if not stack:
                args.append(text[start:i])
                return args, i + 1
            if stack[-1] == ">":
                stack.pop()
        elif ch == "," and not stack:
            args.append(text[start:i])
            start = i + 1
        elif ch == ";":
            return None
    return None


def _expected_to_result(text):
    """nonstd::expected<X, jinja2::ErrorInfo> -> jinja2::Result<X>, ErrorInfoW -> ResultW.

    Returns the rewritten text and the (start, end, new) spans of its outermost rewrites.
    """
    spans = []
    for match in _EXPECTED_OPEN.finditer(text):
        if spans and match.start() < spans[-1][1]:
            continue  # inside a rewritten span
        parsed = _template_arguments(text, match.end() - 1)
        if not parsed or len(parsed[0]) != 2:
            continue
        (value, error), end = parsed[0], parsed[1]
        error_match = _ERROR_INFO_ARG.match(error.strip())
        if error_match:
            alias = "jinja2::Result" if error_match.group(1) == "ErrorInfo" else "jinja2::ResultW"
            spans.append((match.start(), end, "%s<%s>" % (alias, _expected_to_result(value.strip())[0])))
    out = text
    for start, end, new in reversed(spans):
        out = out[:start] + new + out[end:]
    return out, spans


def collect_edits(text):
    """(start, end, new) edits of the table for one file's text, on the original offsets.

    Also returns the (start, reason) pairs that need a human.
    """
    edits = []
    for pattern, template in _SIMPLE_ROWS:
        edits.extend((m.start(), m.end(), m.expand(template)) for m in pattern.finditer(text))
    edits.extend((m.start(), m.end(), "") for pattern in _LINE_REMOVALS for m in pattern.finditer(text))
    edits.extend((m.start(), m.end(), "nonstd::make_unexpected(%s.error())" % m.group("recv"))
                 for m in _UNEXPECTED_RECEIVER.finditer(text))
    edits.extend(_expected_to_result(text)[1])
    edits.sort()

    # Two rows never match the same text in practice; if they do, the first one wins
    kept = []
    for edit in edits:
        if not kept or edit[0] >= kept[-1][1]:
            kept.append(edit)

    # What the table changes is not reported again as "use of a removed name"
    def covered(offset):
        return any(start <= offset < end for start, end, _ in kept)

    human = [(m.start(), reason) for pattern, reason in _REPORT_ROWS for m in pattern.finditer(text)
             if not covered(m.start())]
    return kept, sorted(human)


def rewrite_text(text, path, report):
    """Applies the table to one file's text and returns the new text."""
    edits, human = collect_edits(text)

    def line_of(offset):
        return text.count("\n", 0, offset) + 1

    for start, end, new in edits:
        report.change(path, line_of(start), " ".join(text[start:end].split()) or "(line)", " ".join(new.split()) or "(removed)")
    for start, reason in human:
        report.left("%s:%d" % (path, line_of(start)), reason)
    for start, end, new in reversed(edits):
        text = text[:start] + new + text[end:]
    return text


def rewrite_paths(paths, report, dry_run=False, out=None):
    files = []
    for path in paths:
        if os.path.isdir(path):
            for dirpath, dirnames, filenames in os.walk(path):
                dirnames[:] = sorted(d for d in dirnames if not d.startswith("."))
                files.extend(os.path.join(dirpath, n) for n in sorted(filenames) if n.endswith(SOURCE_SUFFIXES))
        elif os.path.isfile(path):
            files.append(path)
        else:
            report.left(path, "no such file or directory")
    for path in files:
        if _is_library_header(path):
            continue
        before = _read_text(path)
        _write_or_diff(path, before, rewrite_text(before, path, report), dry_run, out)


# ----------------------------------------------------------------------------------------------
# Command line
# ----------------------------------------------------------------------------------------------

def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Migrates code from the Jinja2C++ 1.x API to 2.0 (see the module docstring).")
    parser.add_argument("--log", action="append", default=[], metavar="FILE",
                        help="build log with deprecation warnings (GCC, Clang, MSVC); '-' is stdin; repeatable")
    parser.add_argument("--rewrite", nargs="+", default=[], metavar="PATH",
                        help="files or directories to apply the table of hard breaks to")
    parser.add_argument("--root", default=os.getcwd(), metavar="DIR",
                        help="directory relative paths of the log are resolved against (default: cwd)")
    parser.add_argument("--dry-run", action="store_true", help="print a unified diff, write nothing")
    args = parser.parse_args(argv)
    if not args.log and not args.rewrite:
        parser.error("give --log, --rewrite or both")

    report = Report()
    if args.rewrite:
        rewrite_paths(args.rewrite, report, args.dry_run)
    if args.log:
        lines = []
        for name in args.log:
            if name == "-":
                lines.extend(sys.stdin.read().splitlines())
                continue
            try:
                with open(name, "r", encoding="utf-8", errors="replace") as f:
                    lines.extend(f.read().splitlines())
            except OSError as e:
                parser.error("cannot read %s: %s" % (name, e.strerror))
        diagnostics = parse_log(lines)
        if not diagnostics:
            print("Note: the log has no '%s' deprecation diagnostics." % MARKER.strip())
        apply_diagnostics(diagnostics, args.root, report, args.dry_run)
    report.print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
