---
status: open
priority: low
area: tooling
depends: [0354]
touches: [scripts/preflight.sh]
---
# `scripts/preflight.sh` skips clang-tidy for `python/src/*.cpp` and needs `build/`

**Problem.** Two gaps found while pushing the Python module (0354):

1. The clang-tidy step looks at `src include bindings test` only. A change that touches just
   `python/src/*.cpp` or `python/src/*.h` is reported as `SKIP clang-tidy: no C++ changes`, although
   the module's code is checked by the same `.clang-tidy`. (The changed-line `clang-format` step has the same
   blind spot for `python/src`.)
2. With `--build <dir>` omitted, the script reads `build/`; when it is absent (a runner that
   configured another directory, such as `build-py`) the clang-tidy and `null_compare` steps fail on a
   missing compile database instead of saying the build directory is needed.

**Proposal.** Add `python/src` to the changed-path lists of the format and tidy steps (the module's
compile database is in the build directory that has `-DJINJA2CPP_BUILD_PYTHON=ON`), and fail with a
clear message when `$build/compile_commands.json` is missing.

**Done when.** A change to one line of `python/src/adapters.cpp` runs clang-tidy and clang-format on it, and
running the script without a build directory names the missing directory.
