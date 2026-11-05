#!/usr/bin/env python3
"""Unit tests of scripts/jinja2cpp_migrate.py: log parsing, location handling, the --rewrite table.

The GCC and Clang lines are real compiler output for test/migration/legacy_api_1x.cpp (GCC 13,
Clang 18; GCC in a UTF-8 locale prints curly quotes). The MSVC lines follow the documented
format `path(line[,col]): warning C4996: 'declaration': message`; MSBuild adds an indent and a
trailing ` [project.vcxproj]`.
"""

import contextlib
import importlib.util
import io
import os
import tempfile
import unittest

HERE = os.path.dirname(os.path.abspath(__file__))
_spec = importlib.util.spec_from_file_location(
    "jinja2cpp_migrate", os.path.join(HERE, "..", "..", "scripts", "jinja2cpp_migrate.py"))
migrate = importlib.util.module_from_spec(_spec)
_spec.loader.exec_module(migrate)

GCC = ("a.cpp:{line}:{col}: warning: 'bool jinja2::v2::Value::isString() const' is deprecated: "
       "jinja2cpp-2: IsString [-Wdeprecated-declarations]")
GCC_UNICODE = ("a.cpp:{line}:{col}: warning: \u2018bool jinja2::v2::Value::isString() const\u2019 is deprecated: "
               "jinja2cpp-2: IsString [-Wdeprecated-declarations]")
CLANG = "a.cpp:{line}:{col}: warning: 'isString' is deprecated: jinja2cpp-2: IsString [-Wdeprecated-declarations]"
MSVC = "a.cpp({line},{col}): warning C4996: 'jinja2::v2::Value::isString': jinja2cpp-2: IsString"
MSVC_NO_COLUMN = "a.cpp({line}): warning C4996: 'jinja2::v2::Value::isString': jinja2cpp-2: IsString"


def parse(text):
    return migrate.parse_diagnostic(text)


class ParseTest(unittest.TestCase):
    def check(self, text, old, new, line=1, col=1, bytes_=True):
        diag = parse(text)
        self.assertIsNotNone(diag, text)
        self.assertEqual((diag.path, diag.line, diag.col, diag.old, diag.new, diag.col_in_bytes),
                         ("a.cpp", line, col, old, new, bytes_))

    def test_gcc(self):
        self.check(GCC.format(line=52, col=26), "isString", "IsString", 52, 26)

    def test_gcc_unicode_quotes(self):
        self.check(GCC_UNICODE.format(line=52, col=26), "isString", "IsString", 52, 26)

    def test_gcc_member_template(self):
        self.check("a.cpp:111:53: warning: 'auto jinja2::v2::Value::get() [with T = std::__cxx11::basic_string<char>]'"
                   " is deprecated: jinja2cpp-2: As [-Wdeprecated-declarations]", "get", "As", 111, 53)

    def test_gcc_template_declaration(self):
        self.check("a.cpp:7:3: warning: 'template<class T> auto jinja2::v2::Value::get()' is deprecated: "
                   "jinja2cpp-2: As [-Wdeprecated-declarations]", "get", "As", 7, 3)

    def test_gcc_free_function_and_references(self):
        self.check("a.cpp:116:40: warning: 'std::string jinja2::v2::AsString(const std::string&)' is deprecated: "
                   "jinja2cpp-2: ToString [-Wdeprecated-declarations]", "AsString", "ToString", 116, 40)
        self.check("a.cpp:99:18: warning: 'auto& jinja2::v2::Value::asString()' is deprecated: "
                   "jinja2cpp-2: AsString [-Wdeprecated-declarations]", "asString", "AsString", 99, 18)

    def test_gcc_aliases(self):
        self.check("a.cpp:62:51: warning: 'using jinja2::v2::UserCallable::UserCallableFunctionPtr = using "
                   "jinja2::v2::UserCallable::Function = class std::function<jinja2::v2::Value(const "
                   "jinja2::v2::UserCallableParams&)>' is deprecated: jinja2cpp-2: UserCallable::Function "
                   "[-Wdeprecated-declarations]", "UserCallableFunctionPtr", "UserCallable::Function", 62, 51)
        self.check("a.cpp:137:24: warning: 'using jinja2::v2::ErrorInfoTpl = class jinja2::v2::BasicErrorInfo<char>'"
                   " is deprecated: jinja2cpp-2: BasicErrorInfo [-Wdeprecated-declarations]",
                   "ErrorInfoTpl", "BasicErrorInfo", 137, 24)

    def test_clang(self):
        self.check(CLANG.format(line=93, col=20), "isString", "IsString", 93, 20)

    def test_clang_template_instance(self):
        self.check("a.cpp:26:18: warning: 'get<std::basic_string<char>>' is deprecated: jinja2cpp-2: As "
                   "[-Wdeprecated-declarations]", "get", "As", 26, 18)

    def test_clang_error_severity_and_indent(self):
        self.check("   " + CLANG.format(line=3, col=4).replace("warning", "error"), "isString", "IsString", 3, 4)

    def test_msvc_with_column(self):
        self.check(MSVC.format(line=52, col=26), "isString", "IsString", 52, 26, bytes_=False)

    def test_msvc_without_column(self):
        diag = parse(MSVC_NO_COLUMN.format(line=52))
        self.assertEqual((diag.line, diag.col, diag.old, diag.new), (52, None, "isString", "IsString"))

    def test_msvc_msbuild_decoration(self):
        text = "  " + MSVC.format(line=5, col=9) + " [C:\\build\\fixture.vcxproj]"
        diag = parse(text)
        self.assertEqual((diag.path, diag.old, diag.new), ("a.cpp", "isString", "IsString"))
        diag = parse("C:\\src\\a.cpp(5,9): warning C4996: 'jinja2::v2::Value::get<std::string>': jinja2cpp-2: As"
                     " [C:\\build\\fixture.vcxproj]")
        self.assertEqual((diag.path, diag.old, diag.new), ("C:\\src\\a.cpp", "get", "As"))

    def test_drive_letter_path(self):
        diag = parse("C:/src/a.cpp:3:4: warning: 'isString' is deprecated: jinja2cpp-2: IsString")
        self.assertEqual((diag.path, diag.line, diag.col), ("C:/src/a.cpp", 3, 4))

    def test_other_lines_are_ignored(self):
        for text in [
            "a.cpp:1:1: warning: 'foo' is deprecated: use bar [-Wdeprecated-declarations]",
            "a.cpp:1:1: note: 'isString' has been explicitly marked deprecated here",
            "a.cpp(1): warning C4996: 'strcpy': This function or variable may be unsafe.",
            "   52 |     return value.isString() || jinja2cpp-2: x",
            "In file included from a.cpp:1:",
            "",
        ]:
            self.assertIsNone(parse(text), text)

    def test_log_is_deduplicated_by_location(self):
        lines = [GCC.format(line=1, col=2)] * 2 + [GCC.format(line=1, col=3), CLANG.format(line=1, col=2)]
        self.assertEqual(len(migrate.parse_log(lines)), 2)  # same (file, line, col, old) once

    def test_old_name_of(self):
        for declaration, name in [
            ("jinja2::v2::Value::isString", "isString"),
            ("bool jinja2::v2::Value::isString() const", "isString"),
            ("std::vector<int> jinja2::v2::f(std::map<int, int>)", "f"),
            ("jinja2::v2::Value::get<std::map<int, std::string>>", "get"),
            ("EmptyValue", "EmptyValue"),
        ]:
            self.assertEqual(migrate.old_name_of(declaration), name, declaration)


class ApplyTest(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)

    def write(self, text, name="a.cpp", newline=""):
        # newline="": the bytes as written here, so Windows does not turn \n into \r\n
        path = os.path.join(self.tmp.name, name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, "w", newline=newline, encoding="utf-8") as f:
            f.write(text)
        return path

    def read(self, name="a.cpp"):
        with open(os.path.join(self.tmp.name, name), "rb") as f:
            return f.read().decode("utf-8")

    def apply(self, log_lines, dry_run=False):
        report = migrate.Report()
        out = io.StringIO()
        migrate.apply_diagnostics(migrate.parse_log(log_lines), self.tmp.name, report, dry_run, out)
        return report, out.getvalue()

    def test_clang_column_is_the_identifier(self):
        self.write("bool f(Value v) { return v.isString(); }\n")
        report, _ = self.apply([CLANG.format(line=1, col=34)])
        self.assertEqual(self.read(), "bool f(Value v) { return v.IsString(); }\n")
        self.assertEqual(report.human, [])
        self.assertEqual([c[1:] for c in report.changes], [(1, "isString", "IsString")])

    def test_gcc_column_is_the_paren(self):
        self.write("bool f(Value v) { return v.isString(); }\n")
        self.apply([GCC.format(line=1, col=42)])
        self.assertEqual(self.read(), "bool f(Value v) { return v.IsString(); }\n")

    def test_gcc_column_past_the_call_uses_the_nearest_name_before_it(self):
        self.write("auto n = a.isString() + b.isString() + 1;\n")
        self.apply([GCC.format(line=1, col=21), GCC.format(line=1, col=38)])
        self.assertEqual(self.read(), "auto n = a.IsString() + b.IsString() + 1;\n")

    def test_two_calls_on_a_line_are_edited_right_to_left(self):
        self.write("return v.isString() || v.isWString();\n")
        wide = GCC.format(line=1, col=35).replace("isString", "isWString").replace("IsString", "IsWString")
        self.apply([GCC.format(line=1, col=18), wide])
        self.assertEqual(self.read(), "return v.IsString() || v.IsWString();\n")

    def test_gcc_column_with_a_display_width_off_by_tabs_still_finds_the_name(self):
        self.write("\tauto s = v.isString();\n")
        self.apply([GCC.format(line=1, col=24)])  # a tab counted as 8 columns
        self.assertEqual(self.read(), "\tauto s = v.IsString();\n")

    def test_name_after_the_column_is_found_forward(self):
        self.write("auto s = v.isString();\n")
        self.apply([CLANG.format(line=1, col=1)])
        self.assertEqual(self.read(), "auto s = v.IsString();\n")

    def test_whole_words_only(self):
        self.write("auto s = myisString + isStringX + v.isString();\n")
        self.apply([CLANG.format(line=1, col=1)])
        self.assertEqual(self.read(), "auto s = myisString + isStringX + v.IsString();\n")

    def test_call_split_over_lines(self):
        self.write("const auto& items = value\n                        .asList();\n")
        self.apply(["a.cpp:2:26: warning: 'asList' is deprecated: jinja2cpp-2: AsList [-Wdeprecated-declarations]"])
        self.assertEqual(self.read(), "const auto& items = value\n                        .AsList();\n")

    def test_qualified_name_is_not_repeated(self):
        alias = "a.cpp:{line}:{col}: warning: 'UserCallableFunctionPtr' is deprecated: jinja2cpp-2: UserCallable::Function"
        self.write("UserCallable::UserCallableFunctionPtr a;\n"
                   "jinja2::UserCallable::UserCallableFunctionPtr b;\n"
                   "struct S : UserCallable { UserCallableFunctionPtr c; };\n")
        self.apply([alias.format(line=1, col=15), alias.format(line=2, col=24), alias.format(line=3, col=28)])
        self.assertEqual(self.read(), "UserCallable::Function a;\n"
                                      "jinja2::UserCallable::Function b;\n"
                                      "struct S : UserCallable { UserCallable::Function c; };\n")

    def test_msvc_without_column_rewrites_every_occurrence_on_the_line_once(self):
        self.write("return a.isString() && b.isString();\nreturn c.isString();\n")
        report, _ = self.apply([MSVC_NO_COLUMN.format(line=1)] * 2)
        self.assertEqual(self.read(), "return a.IsString() && b.IsString();\nreturn c.isString();\n")
        self.assertEqual(len(report.changes), 2)

    def test_msvc_column_counts_characters(self):
        self.write("auto s = \"\u00e4\u00f6\" + v.isString();\n")  # two two-byte characters before the call
        self.apply([MSVC.format(line=1, col=19)])
        self.assertEqual(self.read(), "auto s = \"\u00e4\u00f6\" + v.IsString();\n")

    def test_gcc_column_counts_bytes(self):
        self.write("auto s = \"\u00e4\u00f6\" + v.isString();\n")
        self.apply([CLANG.format(line=1, col=21)])
        self.assertEqual(self.read(), "auto s = \"\u00e4\u00f6\" + v.IsString();\n")

    def test_line_endings_are_kept(self):
        self.write("a.isString();\r\nb.isString();\r\n", newline="")
        self.apply([CLANG.format(line=1, col=3), CLANG.format(line=2, col=3)])
        self.assertEqual(self.read(), "a.IsString();\r\nb.IsString();\r\n")

    def test_macro_expansion_is_left_for_a_human(self):
        self.write("CHECK_STRING(v);\n")
        report, _ = self.apply([CLANG.format(line=1, col=1)])
        self.assertEqual(self.read(), "CHECK_STRING(v);\n")
        self.assertEqual(len(report.human), 1)
        self.assertIn("a.cpp:1", report.human[0][0])

    def test_library_headers_and_missing_files_are_reported_not_rewritten(self):
        self.write("auto s = v.isString();\n", "include/jinja2cpp/own.h")
        lines = ["include/jinja2cpp/own.h:1:12: warning: 'isString' is deprecated: jinja2cpp-2: IsString",
                 "gone.cpp:1:12: warning: 'isString' is deprecated: jinja2cpp-2: IsString",
                 CLANG.format(line=9, col=1)]
        self.write("one line\n")
        report, _ = self.apply(lines)
        self.assertEqual(self.read("include/jinja2cpp/own.h"), "auto s = v.isString();\n")
        self.assertEqual(len(report.human), 3)

    def test_dry_run_prints_a_diff_and_writes_nothing(self):
        self.write("auto s = v.isString();\n")
        report, out = self.apply([CLANG.format(line=1, col=12)], dry_run=True)
        self.assertEqual(self.read(), "auto s = v.isString();\n")
        self.assertIn("-auto s = v.isString();", out)
        self.assertIn("+auto s = v.IsString();", out)
        self.assertEqual(len(report.changes), 1)


class RewriteTest(unittest.TestCase):
    def rewrite(self, text):
        report = migrate.Report()
        return migrate.rewrite_text(text, "x.cpp", report), report

    def test_settings_fields(self):
        text, report = self.rewrite("s.m_defaultMetadataType = a;\np->m_defaultMetadataType = b;\n"
                                    "s.extensions.Do = true;\nx.extensions . Do=false;\nauto Do = 1; my_extensions.Doe = 2;\n")
        self.assertEqual(text, "s.defaultMetadataType = a;\np->defaultMetadataType = b;\n"
                               "s.extensions.doStatement = true;\nx.extensions . doStatement=false;\n"
                               "auto Do = 1; my_extensions.Doe = 2;\n")
        self.assertEqual(report.human, [])

    def test_error_info_tpl_alias(self):
        text, _ = self.rewrite("ErrorInfoTpl<char> a; jinja2::ErrorInfoTpl<wchar_t> b; MyErrorInfoTpl c;\n")
        self.assertEqual(text, "BasicErrorInfo<char> a; jinja2::BasicErrorInfo<wchar_t> b; MyErrorInfoTpl c;\n")

    def test_use_line_statements(self):
        text, report = self.rewrite("s.useLineStatements = true;\n"
                                    "    s.useLineStatements = false; // the default\n"
                                    "s.useLineStatements = flag;\n"
                                    "if (x) s.useLineStatements = false;\n")
        self.assertEqual(text, 's.lineStatementPrefix = "#";\n'
                               "s.useLineStatements = flag;\n"
                               "if (x) s.useLineStatements = false;\n")
        self.assertEqual([h[0] for h in report.human], ["x.cpp:3", "x.cpp:4"])
        self.assertEqual([(c[1], c[3]) for c in report.changes],
                         [(1, 'lineStatementPrefix = "#"'), (2, "(removed)")])

    def test_compat_mode_and_removed_headers_are_reported(self):
        text, report = self.rewrite("s.jinja2CompatMode = Jinja2CompatMode::None;\n"
                                    "#include <jinja2cpp/polymorphic_value.h>\n"
                                    '#include "jinja2cpp/binding/rapid_json.h"\n'
                                    "class H : public IErrorHandler {};\n")
        self.assertEqual(text.count("\n"), 4)  # nothing rewritten
        reasons = [h[1] for h in report.human]
        self.assertEqual(len(reasons), 5)  # both spellings in line 1
        self.assertTrue(any('MIGRATION.md "RapidJSON binding removed"' in r for r in reasons))

    def test_error_handler_include_is_removed(self):
        text, report = self.rewrite('#include <a.h>\n#include <jinja2cpp/error_handler.h>\n#  include "jinja2cpp/error_handler.h"\n#include <b.h>\n')
        self.assertEqual(text, "#include <a.h>\n#include <b.h>\n")
        self.assertEqual(report.human, [])

    def test_expected_to_result(self):
        text, _ = self.rewrite("nonstd::expected<std::string, jinja2::ErrorInfo> a;\n"
                               "nonstd::expected<void, ErrorInfo> b;\n"
                               "nonstd::expected<std::wstring, jinja2::ErrorInfoW> c;\n"
                               "nonstd::expected<std::map<std::string, std::vector<int>>, jinja2::v2::ErrorInfo> d;\n"
                               "nonstd::expected<nonstd::expected<int, ErrorInfo>, ErrorInfo> e;\n"
                               "nonstd::expected<decltype(a < b), ErrorInfo> f;\n"
                               "nonstd::expected<int,\n    ErrorInfo>\n    g;\n")
        self.assertEqual(text, "jinja2::Result<std::string> a;\n"
                               "jinja2::Result<void> b;\n"
                               "jinja2::ResultW<std::wstring> c;\n"
                               "jinja2::Result<std::map<std::string, std::vector<int>>> d;\n"
                               "jinja2::Result<jinja2::Result<int>> e;\n"
                               "jinja2::Result<decltype(a < b)> f;\n"
                               "jinja2::Result<int>\n    g;\n")

    def test_other_expected_types_are_left_alone(self):
        source = ("nonstd::expected<int, std::string> a;\nnonstd::expected<int> b;\n"
                  "nonstd::expected<int, jinja2::ErrorInfo, int> c;\nmy::nonstd::expected<int, ErrorInfo> d;\n"
                  "nonstd::expected<int, ErrorInfo\n")
        text, report = self.rewrite(source)
        self.assertEqual(text, source)

    def test_get_unexpected(self):
        text, report = self.rewrite("return r.get_unexpected();\nreturn a.b->c.get_unexpected( );\n"
                                    "return f().get_unexpected();\nreturn std::move(r).get_unexpected();\n"
                                    "return ns::r.get_unexpected();\n")
        self.assertEqual(text, "return nonstd::make_unexpected(r.error());\n"
                               "return nonstd::make_unexpected(a.b->c.error());\n"
                               "return f().get_unexpected();\nreturn std::move(r).get_unexpected();\n"
                               "return ns::r.get_unexpected();\n")
        self.assertEqual([h[0] for h in report.human], ["x.cpp:3", "x.cpp:4", "x.cpp:5"])

    def test_migration_is_idempotent(self):
        once, _ = self.rewrite("s.m_defaultMetadataType = a;\nnonstd::expected<int, ErrorInfo> a;\n"
                               "return r.get_unexpected();\n#include <jinja2cpp/error_handler.h>\n")
        twice, report = self.rewrite(once)
        self.assertEqual(once, twice)
        self.assertEqual((report.changes, report.human), ([], []))


class CommandLineTest(unittest.TestCase):
    def run_main(self, *argv):
        out = io.StringIO()
        with contextlib.redirect_stdout(out):
            code = migrate.main(list(argv))
        return code, out.getvalue()

    def test_log_without_diagnostics_is_not_an_error(self):
        with tempfile.TemporaryDirectory() as tmp:
            log = os.path.join(tmp, "build.log")
            with open(log, "w") as f:
                f.write("a.cpp:1:1: warning: 'foo' is deprecated: nope\n")
            code, out = self.run_main("--log", log, "--root", tmp)
        self.assertEqual(code, 0)
        self.assertIn("no 'jinja2cpp-2:'", out)

    def test_usage_errors_exit_with_2(self):
        for argv in [[], ["--log", "/nonexistent/build.log"], ["--bogus"]]:
            with contextlib.redirect_stderr(io.StringIO()):
                with self.assertRaises(SystemExit) as raised:
                    migrate.main(argv)
            self.assertEqual(raised.exception.code, 2, argv)

    def test_rewrite_walks_directories(self):
        with tempfile.TemporaryDirectory() as tmp:
            os.makedirs(os.path.join(tmp, "sub"))
            for name in ["a.cpp", os.path.join("sub", "b.hpp"), "c.txt"]:
                with open(os.path.join(tmp, name), "w") as f:
                    f.write("s.m_defaultMetadataType = 1;\n")
            code, _ = self.run_main("--rewrite", tmp)
            texts = {}
            for name in ["a.cpp", os.path.join("sub", "b.hpp"), "c.txt"]:
                with open(os.path.join(tmp, name)) as f:
                    texts[name] = f.read()
        self.assertEqual(code, 0)
        self.assertEqual(texts["a.cpp"], "s.defaultMetadataType = 1;\n")
        self.assertEqual(texts[os.path.join("sub", "b.hpp")], "s.defaultMetadataType = 1;\n")
        self.assertEqual(texts["c.txt"], "s.m_defaultMetadataType = 1;\n")


if __name__ == "__main__":
    unittest.main()
