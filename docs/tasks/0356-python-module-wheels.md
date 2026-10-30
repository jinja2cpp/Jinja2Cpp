---
status: open
priority: medium
area: build
depends: [0353]
touches: [.github/workflows/python-wheels.yml]
shares: [pyproject.toml]
---
# Python module P4: wheels for Linux, macOS and Windows; free-threaded audit

**Problem.** 0248's done-when needs wheels built in CI. PyPI publication follows the tag and
is on the postponed list (docs/release-2.0.md, section 5).

**Proposal.** Build wheels with cibuildwheel:
- **Platforms:** manylinux_2_28 (x86_64, aarch64), musllinux x86_64, macOS (arm64,
  x86_64) and Windows x64.
- **Python:** one `cp312-abi3` wheel per platform.
- **Free-threaded:** add `cp314t` after a TSan run of the module's tests justifies
  `Py_MOD_GIL_NOT_USED`. The adapters use `PyDict_GetItemRef` and critical sections.
- **Symbols:** hidden, with fmt linked statically.
- **Smoke test:** each wheel runs the parity driver.
- **When it runs:** on tags, weekly, and on changes to `python/`. Pull requests keep only
  the Linux job from 0353.
- **Name:** check that the name `jinja2cpp` is free on PyPI.

**Done when.** Wheels build and pass the smoke test on every listed platform, and the
free-threaded decision is recorded here.
