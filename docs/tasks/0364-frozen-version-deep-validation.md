---
status: open
pr:
priority: medium
area: robustness
depends: [0338, 0363]
touches: [.github/workflows/deep-validation.yml, scripts/deep_validation.py, docs/deep-validation.md]
---
# Validate a frozen version deeply, from time to time

**Decision (Ruslan, 2026-10-30, 0338 step F thread).** Per-PR validation beyond today's CI is too
expensive at the moment. The heavy checks run as a routine on a frozen version of the library
(a release candidate or a tagged master commit), and the problems they find are fixed as tasks.
The TLA+ model stays a design tool: change the spec first, keep the mutants failing. Its proof
is deferred (0363).

**Why both halves.** The model checks orderings of steps across frames and threads; it does not
check the C++. Coverage and debug builds check the C++, but only the orderings a test happens
to run: every defect the model found (0337, 0335, 0340, step D's partial pin) was in code the
tests already covered line by line. Trace validation (0338 step B) joins them: each ordering a
test, a fuzz input or a stress run executes is also checked against the model.

**The routine**, one workflow run per frozen commit (`workflow_dispatch` with a ref), each part
a job that can run alone:
1. **MC/DC and function coverage** with clang's `-fcoverage-mcdc` (Clang 18 and later; the
   older versions cap the conditions per decision) on `jinja2cpp_tests`. Targets: every
   function of `src/borrowed_cells*`, `src/cycle_collector*` and `src/template_module*` run,
   and MC/DC of their decisions reported; the rest of `src/` reported, no target yet. A
   function or decision outcome no test reaches becomes a test or a task.
2. **Debug builds**, the full suite each: `JINJA2CPP_NODEREF_CHECKS=FULL`,
   `JINJA2CPP_DEBUG_CONTAINERS` (0344), ASan+UBSan, TSan with the thread stress tests repeated,
   the allocation-failure sweep (step D), and the latest-LLVM analysis (0345) on the same
   commit.
3. **Traces against the model.** The `JINJA2CPP_CELL_TRACE` build runs the unit tests, the
   fuzz corpus and the TSan stress tests, and `tla/check.py --trace` checks every trace.
4. **The full TLC suite** at its pass levels, plus the runs without a level bound of 0351 with
   a time budget, on a larger machine if one is available (about 40-60 min for the suite on
   32 cores, from the 4-core timings).

**Done when** the workflow exists, its first run on a frozen commit is triaged (each finding
fixed or filed), and `docs/deep-validation.md` says how to start it and what each part covers.
Task 0010 (a per-PR diff-coverage gate) stays separate.
