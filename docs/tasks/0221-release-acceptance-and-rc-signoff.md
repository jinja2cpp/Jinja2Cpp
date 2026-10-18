---
status: in-progress
priority: high
area: release
touches: [docs/release-2.0.md]
---
# Define 2.0 release gates and release-candidate signoff

**Problem.** The registry tracks individual fixes well, but has no release-wide
definition of done. Parity work keeps finding smaller follow-ups, while packaging,
migration and embedding decisions remain open. A green parity suite accepts the
recorded divergences; it does not establish complete Jinja2 compatibility or safety.

**Proposal.** Write docs/release-2.0.md with a finite release scope, named owners,
blocking versus accepted findings, and links to reproducible evidence for each gate:

- Finish ABI/header isolation (0076), migration (0077, 0222), distribution (0006,
  0225), public documentation (0227) and license packaging (0226).
- Triage the known namespace leak (0110), unsigned overflow (0172) and
  context-callable binding defect (0183) explicitly as release correctness
  candidates. Confirm that #556 closed the autoescape-context difference (0179), and
  decide whether its follow-up for volatile `{% autoescape %}` (0216) blocks release.
- Settle API-shaping choices before the freeze: host Markup (0180), name-based
  autoescape (0181), resource/recursion limits (0097/0098), and the Python/Jinja2
  compatibility target (0186). An explicit deferral and supported usage contract
  can be a decision; this task does not require every proposed feature to ship.
- Publish source-compatibility, ABI and SOVERSION expectations from api-2.0.md,
  including its accepted SOVERSION bumps when the public Settings layout grows.
- Define the RC validation window, supported platforms, exception/waiver process,
  and final tag/package/docs signoff. Changes after the freeze rerun affected gates.

Classify remaining parity gaps by impact and document accepted differences. Do not
require 100% of an expanding corpus, the rest of 0117, or every performance follow-up
as a release condition. Keep 0186's oracle policy and 0187's SBOM implementation in
their existing tasks rather than implementing them here.

**Done when.** A reviewer can use one document to identify the supported 2.0 scope,
each blocking task and owner, accepted limitations, and the commands/results needed
to approve an RC. The document distinguishes configured CI from observed passing
runs and records the exact candidate commit for validation.

**Next.** Use the gates to choose the stabilization queue and RC signoff; close the
linked implementation tasks independently.

**Progress.** docs/release-2.0.md has the gates (G1-G12), owners, accepted limitations,
compatibility promises, RC procedure and the decision list D1-D18 for Ruslan, including
the 0178/0230 conflict (D3). #556 is confirmed to have closed 0179. The task stays
in progress until the decisions are recorded and an RC commit is signed off.
On 2026-10-18 the GitHub issues, external PRs, milestones 1.2/1.3 and the website were
scouted against the backlog; findings filed as 0233-0245 and folded into the gates.
Ruslan decided D3 (pluggable big-integer backend, recorded in 0178).
Later on 2026-10-18 Ruslan set the scope rule: 2.0 takes every open task by default, and
only a postponed list with reasons stays out (docs/release-2.0.md sections 2 and 5, gate G0).
