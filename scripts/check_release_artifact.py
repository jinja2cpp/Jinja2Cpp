#!/usr/bin/env python3
"""Validate a Jinja2C++ release source artifact the way a downstream consumer meets it.

docs/tasks/0225. Starting from a source archive (a published one, or one made here with
`git archive`, which is what GitHub serves for a tag), the check

1. unpacks it into an empty directory, outside any Git checkout, and configures it with
   JINJA2CPP_BUILD_TESTS=OFF;
2. builds it and runs test/package/check_package.cmake from the archive, which installs to
   a scratch prefix and builds, links and runs a separate consumer through find_package and
   pkg-config (static or shared);
3. checks the install: the package version agrees with project(VERSION) (and with
   --expect-version), every public header of the archive is installed unchanged, nothing
   else is installed under include/jinja2cpp/ but the two generated headers, and LICENSE and
   THIRD-PARTY-NOTICES.txt are installed with the sections docs/licensing.md requires;
4. checks that the build relied on nothing but the archive and its declared dependencies:
   the unpacked tree is byte-identical after the build (no in-source generation step), and
   FetchContent populated only the dependencies the configuration declares;
5. writes a JSON report (archive hash, toolchain, dependency sources, results) for the RC
   signoff (docs/tasks/0221).

Network: the internal dependency mode downloads its pinned dependencies (expected-lite,
fmt, fast_float) unless FETCHCONTENT_SOURCE_DIR_<NAME> is passed through --cmake-arg or an
init cache (-C, via --cmake-arg). The external modes need those packages installed. The
report says which dependency came from where.

Usage:
  scripts/check_release_artifact.py --git-ref HEAD --work /tmp/rc [--config Release]
      [--shared] [--deps-mode internal] [--expect-version 2.0.0] [--cmake-arg=-DX=Y ...]
      [--report rc.json]
  scripts/check_release_artifact.py --archive jinja2cpp-2.0.0.tar.gz --work /tmp/rc ...
"""

import argparse
import datetime
import hashlib
import json
import os
import platform
import re
import shutil
import subprocess
import sys
import tarfile
import zipfile
from pathlib import Path

# Sections THIRD-PARTY-NOTICES.txt must have: the code and data compiled into every build or
# installed with it (docs/licensing.md, "Third-party code and data in the artifacts")
REQUIRED_NOTICES = [
    "unordered_dense",
    "lexertk",
    "value-types",
    "fast_float",
    "{fmt}",
    "expected-lite",
    "Unicode Character Database",
    "Python html.entities",
]
# Headers the build generates into include/jinja2cpp/ (CMakeLists.txt, configure_file)
GENERATED_HEADERS = {"jinja2cpp/version.h", "jinja2cpp/features.h"}
# FetchContent names each configuration may populate with tests off (thirdparty/*.cmake)
DECLARED_FETCHES = {
    "internal": {"expected-lite", "fmt", "fast_float"},
    "external-boost": {"expected-lite", "fmt", "fast_float"},
    "external": set(),
    "conan-build": set(),
}
BIGINT_BOOST_FETCHES = {"boost", "boost_multiprecision", "boost_config"}


class CheckFailed(Exception):
    pass


def log(msg):
    print(f"[check_release_artifact] {msg}", flush=True)


def run(cmd, cwd=None, env=None):
    log("> " + " ".join(str(c) for c in cmd))
    rc = subprocess.run([str(c) for c in cmd], cwd=cwd, env=env).returncode
    if rc != 0:
        raise CheckFailed(f"command failed ({rc}): {' '.join(str(c) for c in cmd)}")


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def tree_digest(root):
    """Map of relative path -> sha256 for every file under root."""
    out = {}
    for dirpath, _, files in os.walk(root):
        for name in files:
            p = Path(dirpath) / name
            out[p.relative_to(root).as_posix()] = sha256(p)
    return out


# --- archive ------------------------------------------------------------------------------

def make_git_archive(repo, ref, dest):
    """What GitHub serves for a tag: `git archive` of the commit, export-ignore honoured."""
    version = project_version_from_text(
        subprocess.run(["git", "-C", str(repo), "show", f"{ref}:CMakeLists.txt"],
                       check=True, capture_output=True, text=True).stdout)
    out = dest / f"jinja2cpp-{version}.tar.gz"
    run(["git", "-C", repo, "archive", "--format=tar.gz", f"--prefix=jinja2cpp-{version}/", "-o", out, ref])
    return out


def unpack(archive, dest):
    dest.mkdir(parents=True)
    if archive.name.endswith(".zip"):
        with zipfile.ZipFile(archive) as z:
            z.extractall(dest)
    else:
        with tarfile.open(archive) as t:
            if hasattr(tarfile, "data_filter"):
                t.extractall(dest, filter="data")
            else:
                t.extractall(dest)  # noqa: S202 - our own or a published release archive
    entries = [p for p in dest.iterdir()]
    root = entries[0] if len(entries) == 1 and entries[0].is_dir() else dest
    if not (root / "CMakeLists.txt").is_file():
        raise CheckFailed(f"{archive}: no CMakeLists.txt at the archive root")
    return root


# --- facts read from the archive and the install ------------------------------------------

def project_version_from_text(text):
    m = re.search(r"project\(Jinja2Cpp VERSION ([0-9.]+)\)", text)
    if not m:
        raise CheckFailed("no project(Jinja2Cpp VERSION x.y.z) in CMakeLists.txt")
    return m.group(1)


def public_headers(src):
    """Header path (as installed, below include/) -> source file, from include/ and bindings/*/include/."""
    out = {}
    roots = [src / "include"] + sorted((src / "bindings").glob("*/include"))
    for root in roots:
        for p in root.rglob("*"):
            if p.is_file():
                out[p.relative_to(root).as_posix()] = p
    return out


def find_one(prefix, pattern):
    found = sorted(prefix.glob(pattern))
    return found[0] if found else None


def check_install(src, prefix, expect_version=None):
    """The checks on an installed prefix; returns a list of failure messages."""
    failures = []
    version = project_version_from_text((src / "CMakeLists.txt").read_text(encoding="utf-8"))
    if expect_version and expect_version.lstrip("v") != version:
        failures.append(f"project(VERSION) is {version}, the release is {expect_version}")

    verfile = find_one(prefix, "**/cmake/jinja2cpp/jinja2cpp-config-version.cmake")
    if not verfile:
        failures.append("jinja2cpp-config-version.cmake is not installed")
    else:
        m = re.search(r'set\(PACKAGE_VERSION "([^"]+)"\)', verfile.read_text(encoding="utf-8"))
        if not m or m.group(1) != version:
            failures.append(f"CMake package version {m.group(1) if m else '?'} != {version}")
    pc = find_one(prefix, "**/pkgconfig/jinja2cpp.pc")
    if not pc:
        failures.append("jinja2cpp.pc is not installed")
    else:
        m = re.search(r"^Version: *(\S+)", pc.read_text(encoding="utf-8"), re.M)
        if not m or m.group(1) != version:
            failures.append(f"pkg-config version {m.group(1) if m else '?'} != {version}")
    vh = prefix / "include/jinja2cpp/version.h"
    if not vh.is_file() or f'JINJA2CPP_VERSION_STRING "{version}"' not in vh.read_text(encoding="utf-8"):
        failures.append(f"include/jinja2cpp/version.h is missing or does not say {version}")

    expected = public_headers(src)
    for rel, srcfile in sorted(expected.items()):
        inst = prefix / "include" / rel
        if not inst.is_file():
            failures.append(f"public header not installed: include/{rel}")
        elif inst.read_bytes() != srcfile.read_bytes():
            failures.append(f"installed header differs from the archive: include/{rel}")
    inc = prefix / "include/jinja2cpp"
    if inc.is_dir():
        installed = {p.relative_to(prefix / "include").as_posix() for p in inc.rglob("*") if p.is_file()}
        for extra in sorted(installed - set(expected) - GENERATED_HEADERS):
            failures.append(f"installed header not in the archive: include/{extra}")

    lic = find_one(prefix, "share/doc/*/LICENSE")
    if not lic or not lic.read_text(encoding="utf-8").startswith("Mozilla Public License Version 2.0"):
        failures.append("share/doc/<project>/LICENSE (MPL-2.0) is not installed")
    elif lic.read_bytes() != (src / "LICENSE").read_bytes():
        failures.append("installed LICENSE differs from the archive's")
    notices = find_one(prefix, "share/doc/*/THIRD-PARTY-NOTICES.txt")
    if not notices:
        failures.append("share/doc/<project>/THIRD-PARTY-NOTICES.txt is not installed")
    else:
        text = notices.read_text(encoding="utf-8")
        sections = re.split(r"^-{20,}$", text, flags=re.M)[1:]
        names = [s.strip().splitlines()[0] if s.strip() else "" for s in sections]
        for want in REQUIRED_NOTICES:
            if not any(n.startswith(want) for n in names):
                failures.append(f"THIRD-PARTY-NOTICES.txt has no section for {want}")
    return failures


def cache_entries(build):
    out = {}
    for line in (build / "CMakeCache.txt").read_text(encoding="utf-8", errors="replace").splitlines():
        m = re.match(r"^([^#/][^:=]*)(?::[A-Z]+)?=(.*)$", line)
        if m:
            out[m.group(1)] = m.group(2)
    return out


def dependency_sources(build, src):
    """Where each dependency came from: FetchContent (downloaded or a provided source dir) or find_package."""
    cache = cache_entries(build)
    deps = {}
    deps_dir = build / "_deps"
    populated = sorted(p.name[:-len("-build")] for p in deps_dir.glob("*-build")) if deps_dir.is_dir() else []
    pins = {}
    manifest = src / "sbom/components.json"
    if manifest.is_file():
        for c in json.loads(manifest.read_text(encoding="utf-8")).get("components", []):
            pins[c["id"].replace("-", "_")] = c.get("version")
    for name in populated:
        provided = cache.get(f"FETCHCONTENT_SOURCE_DIR_{name.upper()}")
        deps[name] = {
            "how": "fetchcontent-provided" if provided else "fetchcontent-download",
            "source": provided or str(deps_dir / f"{name}-src"),
            "pinned": pins.get(name.replace("-", "_")),
        }
    for key, value in sorted(cache.items()):
        # <Pkg>_DIR names the directory find_package took a package config from
        if not key.endswith("_DIR") or key.startswith(("CMAKE_", "FETCHCONTENT_")):
            continue
        pkg = key[:-len("_DIR")]
        configs = [f"{pkg}Config.cmake", f"{pkg.lower()}-config.cmake"]
        if pkg not in deps and not value.startswith(str(build)) and any((Path(value) / c).is_file() for c in configs):
            deps[pkg] = {"how": "find_package", "source": value}
    return populated, deps


def check_fetches(populated, deps_mode, bigint):
    allowed = set(DECLARED_FETCHES.get(deps_mode, set()))
    if bigint == "boost" and deps_mode in ("internal", "external-boost"):
        allowed |= BIGINT_BOOST_FETCHES
    return [f"FetchContent populated {name}, which a tests-off {deps_mode} build does not declare"
            for name in populated if name not in allowed]


def check_tree_unchanged(before, after):
    failures = []
    for rel in sorted(set(before) | set(after)):
        if rel not in after:
            failures.append(f"the build deleted {rel} from the source tree")
        elif rel not in before:
            failures.append(f"the build wrote {rel} into the source tree")
        elif before[rel] != after[rel]:
            failures.append(f"the build modified {rel} in the source tree")
    return failures


def toolchain(build, cmake):
    cache = cache_entries(build)
    ver = subprocess.run([cmake, "--version"], capture_output=True, text=True).stdout.splitlines()
    info = {
        "platform": platform.platform(),
        "cmake": ver[0] if ver else "?",
        "generator": cache.get("CMAKE_GENERATOR"),
        "cxx_compiler": cache.get("CMAKE_CXX_COMPILER"),
    }
    for f in build.glob("CMakeFiles/*/CMakeCXXCompiler.cmake"):
        text = f.read_text(encoding="utf-8")
        for key in ("CMAKE_CXX_COMPILER_ID", "CMAKE_CXX_COMPILER_VERSION"):
            m = re.search(rf'set\({key} "([^"]*)"\)', text)
            if m:
                info[key.lower()] = m.group(1)
    return info


# --- driver --------------------------------------------------------------------------------

def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    what = ap.add_mutually_exclusive_group(required=True)
    what.add_argument("--archive", type=Path, help="a source archive (.tar.gz, .tar.xz, .zip)")
    what.add_argument("--git-ref", help="make the archive with `git archive <ref>` from --repo")
    ap.add_argument("--repo", type=Path, default=Path(__file__).resolve().parent.parent)
    ap.add_argument("--work", type=Path, required=True, help="an empty or new scratch directory")
    ap.add_argument("--config", default="Release")
    ap.add_argument("--shared", action="store_true", help="JINJA2CPP_BUILD_SHARED=ON")
    ap.add_argument("--deps-mode", default="internal")
    ap.add_argument("--generator", default="Ninja")
    ap.add_argument("--expect-version", help="the release's version, e.g. the tag (2.0.0 or v2.0.0)")
    ap.add_argument("--cmake-arg", action="append", default=[], help="extra configure argument (repeatable)")
    ap.add_argument("--consumer-arg", action="append", default=[], help="extra argument for the consumer's configure")
    ap.add_argument("--parallel", default=str(os.cpu_count() or 2))
    ap.add_argument("--report", type=Path, help="JSON report path (default <work>/report.json)")
    args = ap.parse_args(argv)

    work = args.work.resolve()
    if work.exists() and any(work.iterdir()):
        ap.error(f"{work} is not empty")
    work.mkdir(parents=True, exist_ok=True)
    cmake = shutil.which("cmake") or "cmake"
    report = {
        "check": "docs/tasks/0225",
        "started": datetime.datetime.now(datetime.timezone.utc).isoformat(timespec="seconds"),
        "config": {"build_type": args.config, "shared": args.shared, "deps_mode": args.deps_mode,
                   "cmake_args": args.cmake_arg},
        "failures": [],
    }
    report_path = args.report or work / "report.json"
    try:
        archive = make_git_archive(args.repo, args.git_ref, work) if args.git_ref else args.archive.resolve()
        report["archive"] = {"name": archive.name, "sha256": sha256(archive), "bytes": archive.stat().st_size,
                             "git_ref": args.git_ref}
        src = unpack(archive, work / "src")
        if any(src.rglob(".git")):
            raise CheckFailed("the archive contains Git metadata (.git)")
        before = tree_digest(src)

        # Git must not find an enclosing checkout from inside the unpacked tree
        env = dict(os.environ, GIT_CEILING_DIRECTORIES=str(work.parent))
        build = work / "build"
        bigint = "none"
        for a in args.cmake_arg:
            m = re.match(r"-DJINJA2CPP_WITH_BIGINT(?::\w+)?=(\w+)", a)
            if m:
                bigint = m.group(1)
        run([cmake, "-S", src, "-B", build, "-G", args.generator, f"-DCMAKE_BUILD_TYPE={args.config}",
             "-DJINJA2CPP_BUILD_TESTS=OFF", f"-DJINJA2CPP_DEPS_MODE={args.deps_mode}",
             f"-DJINJA2CPP_BUILD_SHARED={'ON' if args.shared else 'OFF'}", *args.cmake_arg], env=env)
        run([cmake, "--build", build, "--config", args.config, "--parallel", args.parallel], env=env)

        prefix = work / "prefix"
        consumer = ["-DCONSUMER_ARGS=" + ";".join(args.consumer_arg)] if args.consumer_arg else []
        run([cmake, f"-DBUILD_DIR={build}", f"-DPREFIX={prefix}", f"-DCONFIG={args.config}",
             f"-DEXPECT_SHARED={'ON' if args.shared else 'OFF'}", *consumer,
             "-P", src / "test/package/check_package.cmake"], env=env)
        report["consumer"] = "test/package built and passed against the install (find_package, pkg-config where supported)"

        failures = check_install(src, prefix, args.expect_version)
        populated, deps = dependency_sources(build, src)
        report["dependencies"] = deps
        failures += check_fetches(populated, args.deps_mode, bigint)
        failures += check_tree_unchanged(before, tree_digest(src))
        report["toolchain"] = toolchain(build, cmake)
        report["failures"] = failures
    except CheckFailed as e:
        report["failures"].append(str(e))
    report["result"] = "fail" if report["failures"] else "pass"
    report_path.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    for f in report["failures"]:
        log("FAIL: " + f)
    log(f"{report['result'].upper()}; report: {report_path}")
    return 1 if report["failures"] else 0


if __name__ == "__main__":
    sys.exit(main())
