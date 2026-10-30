---
status: in-progress
pr:
priority: medium
area: ci
depends: [0338]
touches: [.github/workflows/tla-model.yml, tla/check.py, tla/README.md]
---
# Run the long TLC checks of the cell model in GitHub Actions, and halve the two-thread runs

**Problem.** The bounded runs of `tla/CellProtocol.tla` take about 4.8 hours on four cores
(`render` alone 130 min and 21 GB of TLC state). An agent's session caps a command at two hours
and its container restarts kill background jobs, so on #695 the `mixed` and `threads` runs never
finished after two restarts and the owning-link design went in unconfirmed by them.

Measured on the session container (4 cores, 15 GB), `render` at lower levels:

| Level | Distinct states | Time |
|---|---|---|
| 8 | 0.54M | 75 s |
| 9 | 3.2M | 4 min 10 s |
| 11 (the suite) | 80.6M | 130 min |

The space grows about six times per level: the interleavings of independent template steps.
Constant-factor changes therefore buy less than one level each:
- a view that maps dead adapters, cells, storages and exports to one canonical record: -16%;
- fusing the GC step into the step before it: GC states are about 3% of the space;
- `-Xmx10g` instead of the default heap: no change at level 9;
- **thread symmetry** for the two-thread scopes: exactly half (`threads` at level 7: 608,444 to
  304,223 states, 54 s to 29 s). Sound for safety: no action or invariant names a thread, and a
  scope is reduced only when its render and module threads are each all threads or none.

**Done.**
- `tla/check.py` passes the threads as model values and adds `SYMMETRY ThreadPerms` for the
  symmetric scopes (`--no-symmetry` turns it off), takes `--except RUN...`, `--java-opts` /
  `TLC_JAVA_OPTS`, and writes a row per run to the GitHub job summary.
- `.github/workflows/tla-model.yml` runs the suite in four parallel jobs (`render`, `mixed`,
  `threads`, the rest), each with a 340-minute limit and the TLC state on the runner's larger
  disk: on pull requests that change the model or check.py, nightly on master when `tla/`
  changed in the last day, and on demand (`workflow_dispatch`, with an optional list of runs).

**Done when** the workflow runs green on its PR, which also confirms `mixed` and `threads` on
the owning-link code of #695, and the README's running section points at it.

Deeper evidence than the bounded levels (unbounded-depth runs of small scopes, simulation past
the bound, the inductive invariants of 0338 step F) is task 0351.
