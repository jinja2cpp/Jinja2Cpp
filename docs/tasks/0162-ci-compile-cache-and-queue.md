---
status: in-progress
priority: high
area: ci
touches: [.github/actions/setup-ccache/action.yml, .github/workflows/linux-build.yml, .github/workflows/macos-build.yml, .github/workflows/windows-build.yml, .github/workflows/conan-build.yml, .github/workflows/fuzz.yml]
---
# CI got slow: no compile cache, serial MSVC builds, macOS queue

**Finding (2026-10-11, from the Actions API).** `CI-macos-build` on master took 15-26 min
on 2026-10-03 and 35-150 min on 2026-10-10/11 (57 min on 89aa3b7). Two causes, split by job:
- *Build time.* The macOS build step summed to 47 job-minutes on 0374491 (2026-10-03) and 166
  on 89aa3b7 (2026-10-11): `macos-15 C++23 Debug` 16 -> 46 min, `macos-14 C++20 Debug`
  4 -> 26 min. The library and its tests grew; every job compiled everything from scratch
  (no ccache anywhere; only Conan packages and the fuzz corpus were cached).
- *Queue.* Six macOS jobs share the free plan's 5 macOS runners with every other run, so on
  89aa3b7 the jobs started 1, 8, 14, 23, 29 min after creation. Linux (up to 20 concurrent)
  queued its last jobs 14-18 min on the same push.
- Windows built with `NMake Makefiles`, which compiles one file at a time (15-20 min per job).

**Fix in this task.** `.github/actions/setup-ccache` (ccache through `hendrikmuhs/ccache-action`,
launcher through `CMAKE_<LANG>_COMPILER_LAUNCHER`, caches per job, saved by master pushes only;
PRs restore master's cache) on Linux, sanitizer, macOS, Windows (now Ninja with `/Z7`), Conan
and fuzz jobs.

**Not done here (decide with the numbers from the first master runs).**
- CodeQL (~30 min) cannot use ccache: a cache hit skips the compiler its extractor traces.
- `benchmark.yml` builds three times in Release; its instruction-count gate is path-sensitive
  (`-ffile-prefix-map`), so it was left alone.
- Coverage job: `.gcno` files with ccache need care.
- macOS rows: fewer jobs would cut the queue (declined once, see project memory
  jinja2cpp-ci-load-trim); revisit if the queue stays long with warm caches.
