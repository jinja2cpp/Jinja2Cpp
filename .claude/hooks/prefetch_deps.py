#!/usr/bin/env python3
"""Pre-fetch Jinja2Cpp's FetchContent dependencies with git.

Cloud sessions can clone public GitHub repositories, but their egress proxy
refuses GitHub *archive* downloads (https://github.com/<o>/<r>/archive/...),
which is how thirdparty/internal_deps.cmake pins most dependencies. This script
reads the pins from the CMake files, clones each one at the pinned tag/commit
and writes a CMake initial-cache script that points FetchContent at the clones
via FETCHCONTENT_SOURCE_DIR_<NAME>.

Usage: prefetch_deps.py <repo-root> <cache-dir>
Prints the path of the generated initial-cache script (pass it as `cmake -C`).
"""
import pathlib
import re
import subprocess
import sys

DECLARE_RE = re.compile(r"FetchContent_Declare\(\s*([\w-]+)(.*?)\)", re.S)
ARCHIVE_RE = re.compile(
    r"URL\s+https://github\.com/([\w.-]+)/([\w.-]+)/archive/(?:refs/tags/)?([\w.-]+?)\.tar\.gz")


def pins(repo_root):
    for cmake_file in ("thirdparty/internal_deps.cmake",):
        text = (repo_root / cmake_file).read_text()
        for name, body in DECLARE_RE.findall(text):
            m = ARCHIVE_RE.search(body)
            if not m or "PATCH_COMMAND" in body:
                # Release assets (Boost) download fine; patched deps (RapidJSON)
                # must go through FetchContent so the patch is applied.
                continue
            owner, repo, ref = m.groups()
            yield name, f"https://github.com/{owner}/{repo}.git", ref


def checkout(url, ref, dest):
    marker = dest / ".jinja2cpp-pinned-ref"
    if marker.exists() and marker.read_text() == ref:
        return
    dest.mkdir(parents=True, exist_ok=True)
    git = ["git", "-C", str(dest)]
    if not (dest / ".git").exists():
        subprocess.run(git + ["init", "-q"], check=True)
        subprocess.run(git + ["remote", "add", "origin", url], check=True)
    is_sha = re.fullmatch(r"[0-9a-f]{40}", ref) is not None
    refspec = ref if is_sha else f"refs/tags/{ref}"
    subprocess.run(git + ["fetch", "-q", "--depth", "1", "origin", refspec], check=True)
    subprocess.run(git + ["checkout", "-q", "--force", "FETCH_HEAD"], check=True)
    marker.write_text(ref)


def main():
    repo_root = pathlib.Path(sys.argv[1]).resolve()
    cache_dir = pathlib.Path(sys.argv[2]).resolve()
    lines = []
    for name, url, ref in pins(repo_root):
        dest = cache_dir / "src" / name
        checkout(url, ref, dest)
        lines.append(f'set(FETCHCONTENT_SOURCE_DIR_{name.upper()} "{dest}" CACHE PATH "" FORCE)')
    init_cache = cache_dir / "fetchcontent-sources.cmake"
    init_cache.write_text("\n".join(lines) + "\n")
    print(init_cache)


if __name__ == "__main__":
    main()
