#!/usr/bin/env python3
"""Checks that the Python module, built alone against an installed jinja2cpp, includes no header
of the checkout (docs/tasks/0357 S4): each header its objects depend on, as Ninja recorded them,
must come from the module's own sources (python/), the installed prefix, the build's Python and
nanobind, or the system (/usr). A header of the checkout's include/, src/ or bindings/ fails the
check, as does one from anywhere else.

    python3 python/check_includes.py <module build dir> <install prefix>
"""

import os
import subprocess
import sys
import sysconfig


def headers(build_dir):
    """The headers that the module's objects were compiled from (ninja -t deps)."""
    out = subprocess.run(["ninja", "-C", build_dir, "-t", "deps"], check=True, capture_output=True, text=True).stdout
    found = set()
    current = None
    for line in out.splitlines():
        if not line.startswith(" "):
            # "<object>: #deps N, deps mtime ..."; the module's objects only
            current = line.split(":", 1)[0] if "_jinja2cpp" in line else None
            continue
        if current and line.strip():
            path = line.strip()
            if not os.path.isabs(path):
                path = os.path.join(build_dir, path)
            found.add(os.path.realpath(path))
    return found


def under(path, root):
    root = os.path.realpath(root)
    return path == root or path.startswith(root + os.sep)


def main():
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    build_dir, prefix = sys.argv[1], sys.argv[2]
    module_dir = os.path.dirname(os.path.realpath(__file__))
    checkout = os.path.dirname(module_dir)
    allowed = [module_dir, prefix, "/usr"]
    allowed += [p for p in {sysconfig.get_path(k) for k in ("include", "platinclude", "purelib", "platlib")} if p]
    found = headers(build_dir)
    if not found:
        sys.exit("check_includes: no dependencies of _jinja2cpp recorded in " + build_dir + " (not a Ninja build?)")
    bad = []
    for path in sorted(found):
        if any(under(path, root) for root in allowed):
            continue
        if under(path, checkout):
            bad.append(path + "  (the checkout, not the installed package)")
        else:
            bad.append(path + "  (outside " + ", ".join(allowed) + ")")
    if bad:
        print("check_includes: the module includes headers it must not:", *bad, sep="\n  ")
        sys.exit(1)
    jinja = [p for p in found if under(p, prefix)]
    print(f"check_includes: {len(found)} headers, {len(jinja)} from {prefix}, none from the checkout")


if __name__ == "__main__":
    main()
