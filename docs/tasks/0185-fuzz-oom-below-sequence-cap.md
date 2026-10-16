---
status: done
priority: medium
area: robustness
depends: [0097]
touches: [src/internal_value.h#MaxSequenceSize, .github/workflows/fuzz.yml, fuzz/replay_corpus.py]
---
# The fuzz job fails on lists below the sequence cap

**Problem.** Found by the `fuzz render_wide` job on PR #505 (unrelated to its diff):
`{{ range(775807, -92233720368547753372036854775803, -9223758807)|list }}` reports about
10^9 items, under `MaxSequenceSize` (2^31, task 0097's hard cap), so `ListRef::ToValueList`
reserves 32 GB. ASan reports that single allocation as out-of-memory against the job's
`-rss_limit_mb=2560`, which `-ignore_ooms` does not cover, and the job goes red. Any
size-reporting range between roughly 2^26 and 2^31 items does the same, so the job can fail
on any PR, depending on what the fuzzer reaches in its time box.

**Approach.** Either lower the hard cap to what fits the fuzz RSS limit (2^26 items is 2 GB
of `InternalValue`; Python raises MemoryError long before 2^31 anyway), or wait for 0097's
opt-in `Settings` limits and set a low one in the fuzz targets. Add the input above to
`fuzz/regressions/`.

**Done when.** The input above fails the render with the "too long" error instead of
allocating, and it replays in `jinja2cpp_fuzz_regressions`.

**Resolution.** `MaxSequenceSize` is now 2^26, so the input above fails with "too long" and
replays from `fuzz/regressions/range-list-below-old-cap.j2`. Ruslan also split the fuzz
workflow in two (2026-10-16): pull requests replay the kept corpus once per input with no
new generation (`fuzz/replay_corpus.py`), so a PR goes red only when its change crashes a
known input; generation runs nightly and on demand, where a new finding fails that run
instead of an unrelated PR. The opt-in limits in 0097 are still open.
