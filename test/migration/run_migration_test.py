#!/usr/bin/env python3
"""Round trip of scripts/jinja2cpp_migrate.py on a 1.x style translation unit (docs/tasks/0077).

  1. copy legacy_api_1x.cpp to <build-dir>/migration/legacy_api.cpp (pristine on every run)
  2. migrate --rewrite: the hard breaks, which would not compile
  3. build the jinja2cpp_migration_fixture target; the log must have jinja2cpp-2 deprecations
  4. migrate --log with that log
  5. rebuild: no jinja2cpp-2 diagnostic and no warning for the file may be left,
     and the file must equal legacy_api_2x.cpp
"""

import argparse
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
SCRIPT = os.path.join(HERE, "..", "..", "scripts", "jinja2cpp_migrate.py")
TARGET = "jinja2cpp_migration_fixture"
MARKER = "jinja2cpp-2:"


def run(command, **kwargs):
    return subprocess.run(command, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                          universal_newlines=True, errors="replace", **kwargs)


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--build-dir", required=True)
    parser.add_argument("--config", default="", help="configuration of a multi-config generator")
    args = parser.parse_args()

    workdir = os.path.join(args.build_dir, "migration")
    fixture = os.path.join(workdir, "legacy_api.cpp")
    log_path = os.path.join(workdir, "build.log")
    os.makedirs(workdir, exist_ok=True)
    shutil.copyfile(os.path.join(HERE, "legacy_api_1x.cpp"), fixture)

    build = ["cmake", "--build", args.build_dir, "--target", TARGET]
    if args.config:
        build += ["--config", args.config]
    migrate = [sys.executable, SCRIPT]

    def touch():
        # A rewrite keeps the old mtime granularity on some file systems: force a recompile
        os.utime(fixture, None)

    logs = []

    def fail(message):
        print("FAIL: " + message)
        for title, text in logs:
            print("----- %s -----\n%s" % (title, text))
        return 1

    # The hard breaks first: they are errors, not warnings
    step = run(migrate + ["--rewrite", fixture])
    logs.append(("migrate --rewrite", step.stdout))
    if step.returncode:
        return fail("--rewrite exited with %d" % step.returncode)

    # MSVC reports a warning number once per source line (two deprecated calls on one line give
    # one C4996), so a line can need a second pass: repeat build and --log until the build is
    # clean, as MIGRATION.md tells users to. Three passes cover the fixture on every compiler.
    passes = 0
    while True:
        touch()
        build_step = run(build)
        logs.append(("build %d" % (passes + 1), build_step.stdout))
        if build_step.returncode:
            return fail("the fixture does not build (pass %d)" % (passes + 1))
        if passes == 0 and MARKER not in build_step.stdout:
            return fail("the first build reported no '%s' deprecation: the test would pass vacuously" % MARKER)
        left = [line for line in build_step.stdout.splitlines()
                if MARKER in line or ("legacy_api" in line and "warning" in line.lower())]
        if not left:
            break
        if passes == 3:
            return fail("still reported after %d --log passes:\n%s" % (passes, "\n".join(left)))
        with open(log_path, "w", encoding="utf-8") as f:
            f.write(build_step.stdout)
        step = run(migrate + ["--log", log_path, "--root", args.build_dir])
        logs.append(("migrate --log %d" % (passes + 1), step.stdout))
        if step.returncode:
            return fail("--log exited with %d" % step.returncode)
        passes += 1

    with open(fixture, "rb") as f:
        migrated = f.read()
    with open(os.path.join(HERE, "legacy_api_2x.cpp"), "rb") as f:
        expected = f.read()
    if migrated != expected:
        return fail("the migrated file differs from legacy_api_2x.cpp (diff %s %s)" %
                    (fixture, os.path.join(HERE, "legacy_api_2x.cpp")))
    print("OK: the 1.x fixture migrated to the golden 2.0 file and builds without deprecation warnings")
    return 0


if __name__ == "__main__":
    sys.exit(main())
