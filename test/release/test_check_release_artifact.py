"""Unit tests of scripts/check_release_artifact.py on fabricated archives and installs (docs/tasks/0225).

The end-to-end run builds the library from an archive (CI, release validation); these
tests prove that each defect the task names makes a check fail, without a build:
a missing public header, a wrong package version, a missing notice, and a build that
writes into its source tree or fetches an undeclared dependency.

    python3 -m unittest discover -s test/release -v
"""

import importlib.util
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location("check_release_artifact", ROOT / "scripts/check_release_artifact.py")
cra = importlib.util.module_from_spec(spec)
sys.modules["check_release_artifact"] = cra
spec.loader.exec_module(cra)

VERSION = "2.0.0"


def write(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="utf-8")


def notices(sections):
    body = "Jinja2C++ third-party notices\n\n"
    for name in sections:
        body += "-" * 78 + f"\n{name}\nVersion: x\n\nLicense text\n\n"
    return body


class Fixture:
    """A minimal unpacked archive and the install a correct build makes from it."""

    def __init__(self, tmp):
        self.src = tmp / "src"
        self.prefix = tmp / "prefix"
        write(self.src / "CMakeLists.txt", f"project(Jinja2Cpp VERSION {VERSION})\n")
        write(self.src / "LICENSE", "Mozilla Public License Version 2.0\n...\n")
        write(self.src / "include/jinja2cpp/value.h", "// value\n")
        write(self.src / "include/jinja2cpp/polymorphic_value/indirect_cxx14.h", "// indirect\n")
        write(self.src / "bindings/nlohmann_json/include/jinja2cpp/binding/nlohmann_json.h", "// binding\n")
        for rel in ("jinja2cpp/value.h", "jinja2cpp/polymorphic_value/indirect_cxx14.h",
                    "jinja2cpp/binding/nlohmann_json.h"):
            write(self.prefix / "include" / rel, (self.header_source(rel)).read_text())
        write(self.prefix / "include/jinja2cpp/version.h", f'#define JINJA2CPP_VERSION_STRING "{VERSION}"\n')
        write(self.prefix / "include/jinja2cpp/features.h", "// features\n")
        write(self.prefix / "lib/cmake/jinja2cpp/jinja2cpp-config-version.cmake", f'set(PACKAGE_VERSION "{VERSION}")\n')
        write(self.prefix / "lib/pkgconfig/jinja2cpp.pc", f"Name: jinja2cpp\nVersion: {VERSION}\n")
        write(self.prefix / "share/doc/Jinja2Cpp/LICENSE", (self.src / "LICENSE").read_text())
        write(self.prefix / "share/doc/Jinja2Cpp/THIRD-PARTY-NOTICES.txt", notices(cra.REQUIRED_NOTICES))

    def header_source(self, rel):
        if rel.startswith("jinja2cpp/binding/"):
            return self.src / "bindings/nlohmann_json/include" / rel
        return self.src / "include" / rel

    def check(self, expect_version=None):
        return cra.check_install(self.src, self.prefix, expect_version)


class CheckInstallTest(unittest.TestCase):
    def setUp(self):
        self._tmp = tempfile.TemporaryDirectory()
        self.f = Fixture(Path(self._tmp.name))

    def tearDown(self):
        self._tmp.cleanup()

    def assertFails(self, needle, failures):
        self.assertTrue(any(needle in m for m in failures), f"no failure mentions {needle!r}: {failures}")

    def test_correct_install_passes(self):
        self.assertEqual(self.f.check(), [])
        self.assertEqual(self.f.check("v2.0.0"), [])

    def test_missing_public_header(self):
        (self.f.prefix / "include/jinja2cpp/polymorphic_value/indirect_cxx14.h").unlink()
        self.assertFails("public header not installed: include/jinja2cpp/polymorphic_value/indirect_cxx14.h", self.f.check())

    def test_missing_component_header(self):
        (self.f.prefix / "include/jinja2cpp/binding/nlohmann_json.h").unlink()
        self.assertFails("include/jinja2cpp/binding/nlohmann_json.h", self.f.check())

    def test_stale_header(self):
        write(self.f.prefix / "include/jinja2cpp/value.h", "// older\n")
        self.assertFails("differs from the archive: include/jinja2cpp/value.h", self.f.check())

    def test_header_not_in_archive(self):
        write(self.f.prefix / "include/jinja2cpp/error_handler.h", "// 1.x leftover\n")
        self.assertFails("not in the archive: include/jinja2cpp/error_handler.h", self.f.check())

    def test_wrong_release_version(self):
        self.assertFails("the release is 2.0.1", self.f.check("2.0.1"))

    def test_wrong_package_versions(self):
        write(self.f.prefix / "lib/cmake/jinja2cpp/jinja2cpp-config-version.cmake", 'set(PACKAGE_VERSION "1.3.2")\n')
        write(self.f.prefix / "lib/pkgconfig/jinja2cpp.pc", "Version: 1.3.2\n")
        write(self.f.prefix / "include/jinja2cpp/version.h", '#define JINJA2CPP_VERSION_STRING "1.3.2"\n')
        failures = self.f.check()
        self.assertFails("CMake package version 1.3.2", failures)
        self.assertFails("pkg-config version 1.3.2", failures)
        self.assertFails("version.h", failures)

    def test_missing_license(self):
        (self.f.prefix / "share/doc/Jinja2Cpp/LICENSE").unlink()
        self.assertFails("LICENSE (MPL-2.0) is not installed", self.f.check())

    def test_missing_notices_file(self):
        (self.f.prefix / "share/doc/Jinja2Cpp/THIRD-PARTY-NOTICES.txt").unlink()
        self.assertFails("THIRD-PARTY-NOTICES.txt is not installed", self.f.check())

    def test_missing_notice_section(self):
        write(self.f.prefix / "share/doc/Jinja2Cpp/THIRD-PARTY-NOTICES.txt",
              notices([n for n in cra.REQUIRED_NOTICES if n != "Unicode Character Database"]))
        self.assertFails("no section for Unicode Character Database", self.f.check())


class CheckBuildIsolationTest(unittest.TestCase):
    def test_tree_written_by_build(self):
        before = {"CMakeLists.txt": "a", "src/x.cpp": "b"}
        after = {"CMakeLists.txt": "a", "src/x.cpp": "c", "src/unicode_tables.h": "d"}
        failures = cra.check_tree_unchanged(before, after)
        self.assertIn("the build modified src/x.cpp in the source tree", failures)
        self.assertIn("the build wrote src/unicode_tables.h into the source tree", failures)
        self.assertEqual(cra.check_tree_unchanged(before, dict(before)), [])

    def test_undeclared_fetch(self):
        self.assertEqual(cra.check_fetches(["expected-lite", "fmt", "fast_float"], "internal", "none"), [])
        self.assertEqual(len(cra.check_fetches(["fmt", "googletest", "nlohmann_json"], "internal", "none")), 2)
        self.assertEqual(cra.check_fetches(["fmt"], "internal", "none"), [])
        self.assertEqual(len(cra.check_fetches(["fmt"], "external", "none")), 1)
        self.assertEqual(cra.check_fetches(["boost_multiprecision", "boost_config"], "internal", "boost"), [])
        self.assertEqual(len(cra.check_fetches(["boost_multiprecision"], "internal", "none")), 1)

    def test_version_from_cmakelists(self):
        self.assertEqual(cra.project_version_from_text("cmake_minimum_required(VERSION 3.23)\nproject(Jinja2Cpp VERSION 2.1.3)\n"), "2.1.3")
        with self.assertRaises(cra.CheckFailed):
            cra.project_version_from_text("project(Other VERSION 1.0)")

    def test_repository_notices_have_every_section(self):
        text = (ROOT / "THIRD-PARTY-NOTICES.txt").read_text(encoding="utf-8")
        with tempfile.TemporaryDirectory() as tmp:
            f = Fixture(Path(tmp))
            write(f.prefix / "share/doc/Jinja2Cpp/THIRD-PARTY-NOTICES.txt", text)
            self.assertEqual(f.check(), [])


if __name__ == "__main__":
    unittest.main()
