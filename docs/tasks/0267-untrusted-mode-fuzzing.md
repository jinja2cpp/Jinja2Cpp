---
status: in-progress
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

**Trust model.** When this lands, update its entry under Known gaps in [docs/security.md](../security.md) (0224).

## Result

Four libFuzzer targets in `fuzz/untrusted_targets.cpp` and `fuzz/alloc_failure_target.cpp`,
each in the PR replay and nightly matrix of `.github/workflows/fuzz.yml`, and replayed over
`fuzz/regressions/` by ctest (`jinja2cpp_fuzz_replay`, `jinja2cpp_fuzz_alloc_replay`):

- `untrusted`: `FuzzLimits()` and a settings byte (autoescape, trim/lstrip, line statements,
  four delimiter sets, strict undefined, wide). `UntrustedLimits()` alone does not bound memory
  (filed as [0459](0459-render-memory-budget.md)), so `FuzzLimits()` lowers steps to 20000 and
  the sequence cap to 2^16, adds a 2 s render time and caps template size and loads; the worst
  retaining loop measured peaks at about 1 GB under ASan, below the 2560 MB RSS limit.
- `host`: reflected structs, vectors and a generated list; callables with typed, `*args`,
  `**kwargs` and `*context` parameters, one returning a fresh host object, one throwing; host
  filters and tests. Host data is made afresh per input, so a render that keeps it past its end
  reads freed memory on a later input.
- `loader`: `RealFileSystem` over a plain root, or `ConfinedFileSystem` over a root with an
  absolute link, a `..` link and a directory link out of it; canary files outside get new
  random text per input, and that text in the output aborts.
- `alloc_failure`: fails the Nth allocation of Load and Render (0250 §9.5, 0252/0253): a call
  returns or throws `std::bad_alloc`; a fired failure that does not throw must leave the outcome
  as it was, and the template and environment render as before afterwards. Its own executables,
  as it replaces `operator new`.

For these targets a timeout or out-of-memory input fails the job: `replay_corpus.py --strict`
on pull requests, no `-ignore_timeouts`/`-ignore_ooms` nightly. The untrusted job checks the
plumbing on every run: with `JINJA2CPP_FUZZ_INJECT_SLOW=1` the target hangs on an injected slow
seed, and the replay must fail on its timeout. `make_corpus.py --prefix TARGET` writes the seeds
with the prefix bytes (and rewritten delimiters).

With 0263 merged the seeds replay with no out-of-memory or timeout on any of the four targets,
which is 0263's done-when.

Not done here: 0095's structure-aware mutator does not exist yet, so nothing feeds it; 0096
(continuous service) is open. Both stay their own tasks.
