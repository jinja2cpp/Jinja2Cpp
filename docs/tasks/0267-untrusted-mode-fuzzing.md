---
status: open
priority: high
area: robustness
depends: [0262]
touches: [fuzz/]
shares: [.github/workflows/fuzz.yml, fuzz/CMakeLists.txt]
---
# Fuzz targets for templates written by untrusted users

**Problem.** The fuzz targets of 0003 run with default settings, ignore timeouts and
out-of-memory (`-ignore_timeouts=1 -ignore_ooms=1`), never set autoescape, delimiters or trim
options, never expose host callables or reflected objects, and never use a filesystem loader.
These are the paths a host rendering user templates exercises.

**Proposal.**
- A limits-on target running under `UntrustedLimits()` (0262) that fails on timeouts and
  out-of-memory: with limits set, both are findings.
- A settings prefix byte selecting autoescape, delimiters, trim/lstrip and line statements.
- A host-surface target with callables, reflected structs and lists, `*context` callables.
- A loader target over a temp root with canary files outside it; the canary text in the output
  is a finding.
- An allocation-failure target (0250 §9.5, 0252) failing the Nth allocation.
- Feed 0095's structure-aware mutator into all of them; 0096 runs them continuously.

**Release role.** Gate G8: the extended RC campaign includes these targets.

**Done when.** The targets run in the PR and nightly jobs and the limits-on target fails the job
on an injected slow seed.
