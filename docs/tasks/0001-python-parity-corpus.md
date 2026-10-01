---
status: done
priority: high
area: parity
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/294
touches: [test/parity/, CMakeLists.txt, .github/workflows/parity-expectations.yml, docs/parity.md]
---
# Differential parity corpus against Python Jinja2

**Problem.** The project goal is maximum parity with Python Jinja2, but the test suite
encodes what Jinja2C++ *does*, not what Jinja2 does. Expected values were written by hand,
so a divergence and its test can agree with each other and both be wrong. A probe of
159 small templates against jinja2 3.1.6 (Oct 2026 audit) gave:

| outcome | count |
|---|---|
| identical output | 56 |
| different output | 62 |
| rejected by C++, accepted by Python | 32 |
| accepted by C++, rejected by Python | 3 |
| rejected by both | 6 |

Typical divergences: Python reprs of `None`/`True`/lists/dicts/tuples, floor division and
modulo of negatives (`-7//2`, `-7%3`), `x and 'a' or 'b'` returning a bool, `float`
filter output, `tojson` spacing and key order, string methods (`s.upper()`), `format`
with `%`-placeholders. Some empty C++ outputs may be artefacts of the probe driver
passing lists through reflection (see 0002); triage before fixing.

Probe sources: `parity_probe.py` and `parity_probe_driver.cpp` from the audit (to be
moved into `test/parity/`).

**Proposal.**
1. Check in a corpus (`test/parity/cases/*.j2` + context JSON) and a generator that renders
   each case with Python Jinja2 and stores the expected output next to it. Python is only
   needed to regenerate, not to run the tests.
2. A gtest suite renders every case and compares against the stored output. Known
   divergences are listed in one allow-list file with a reason each, so the list can only
   shrink deliberately.
3. CI job regenerates expectations with a pinned jinja2 and fails if the stored files drift.

**Done when.** `ctest -R parity` runs the corpus; the allow-list is the single place that
records divergences; CI fails on a new divergence.

**Next.** Once the corpus exists, the number of allow-listed cases becomes the parity
metric to drive down; deliberate divergences (e.g. wide strings) need documenting.

**Outcome.** `test/parity/` holds 616 cases in 15 areas, generated expectations, the
`jinja2cpp_parity` ctest (`ctest -R parity`), the allow-list `divergences.txt` with a kind
and an owning task per case, and the `parity-expectations` workflow that regenerates with
jinja2 3.1.6 and fails on drift. First measurement: 259 of 616 cases match. The probe's
empty outputs were mostly real (lists print empty, `join` drops numbers, string `length`
is empty), not reflection artefacts. Gaps are mapped in [docs/parity.md](../parity.md)
and split into tasks 0012-0033.
