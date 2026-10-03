---
status: open
priority: low
area: ci
depends: [0003]
touches: [.clusterfuzzlite/, fuzz/CMakeLists.txt]
shares: [.github/workflows/fuzz.yml]
---
# Continuous fuzzing on ClusterFuzzLite or OSS-Fuzz

**Problem.** `.github/workflows/fuzz.yml` (0003) fuzzes 5 minutes per target on pull
requests and an hour nightly, keeps the corpus in the Actions cache and fails on a crash.
It does not deduplicate crashes across runs, minimise or bisect them, track when a crash
was fixed, or report coverage, and an hour a night is a small budget for a parser and
evaluator of this size. The corpus lives in a cache that GitHub evicts after a week
without use.

**Proposal.** Once the nightly job has run clean for a couple of weeks (0093 fixed):
- Add ClusterFuzzLite (`.clusterfuzzlite/Dockerfile`, `build.sh`, `project.yaml`) on
  GitHub Actions: code-change fuzzing on PRs, batch fuzzing on a schedule, corpus pruning
  and coverage reports, with the corpus in a dedicated storage branch or bucket instead
  of the Actions cache. It reuses the targets in `fuzz/` unchanged.
- Apply to OSS-Fuzz if the project qualifies (it is a widely embedded parser of
  untrusted-ish input): far more CPU, crash triage with issue filing and fix tracking.
  The same `build.sh` serves both.
- Retire the bespoke fuzz job when one of them runs, keeping the differential check
  (`fuzz/differential.py`) as a nightly job of its own.
- While at it: under a Visual Studio multi-config generator `jinja2cpp_fuzz_replay` lands
  in `${CMAKE_BINARY_DIR}`, outside the `Debug/`/`Release/` folder that holds a shared
  `jinja2cpp.dll`; use a per-config output directory.

**Done when.** Crashes found by scheduled fuzzing are deduplicated and reported by the
service, a coverage report for the fuzz targets is published, and the corpus survives
more than a week without a run.
