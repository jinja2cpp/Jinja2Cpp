---
status: open
priority: medium
area: ci
depends: [0600]
touches: [.github/workflows/clang-tidy.yml, .github/workflows/python.yml]
---
# clang-tidy never sees `python/src/`, `fuzz/` or `bench/`

**Problem.** The house style (modern memory-safe C++, no raw pointers unless needed) holds for every C++
file the project ships or runs on untrusted input, but clang-tidy covers only part of them:

- `.github/workflows/clang-tidy.yml` triggers on `src/`, `include/`, `bindings/`, `test/` only, the
  changed-line step diffs those four directories, and the whole-tree step matches `(src|test)/.*\.cpp`.
- `python/src/` (20 `.cpp`/`.h` files: the Python module's host boundary, which REVIEW.md treats as an
  untrusted-input surface, and the fiber code) needs `-DJINJA2CPP_BUILD_PYTHON=ON` to be in the compile
  database, which the clang-tidy job never configures. `scripts/preflight.sh` skips it too (0429).
- `fuzz/` (17 files) is in the default compile database (the replay drivers), but no step selects it.
- `bench/` (2 `.cpp`) needs `-DJINJA2CPP_BUILD_BENCHMARKS=ON`.

So a raw `new`, an unchecked optional or a use-after-move in the module's adapters fails no check.

**Proposal.** In `python.yml`, which already builds the module, add a clang-tidy step on changed lines of
`python/src/` (whole file once 0601 lands) with the root `.clang-tidy`, excluding the assembly and the
vendored fiber code if it has a provenance notice; add `fuzz/` to the clang-tidy workflow's paths, diff
and whole-tree regex (it needs no extra configuration); leave `bench/` to a later batch unless its first
whole-file run is small. Count each directory's hits first and clean them in the same PR, or file them.

**Done when.** A one-line change in `python/src/adapters.cpp` and in `fuzz/fuzz_targets.cpp` each runs
clang-tidy in CI, and the whole-tree run covers `fuzz/`.
