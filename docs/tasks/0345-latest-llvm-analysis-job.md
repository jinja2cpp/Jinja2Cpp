---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/696
priority: low
area: ci
depends: [0343]
touches: [.github/workflows/clang-analysis.yml]
---
# A report-only job with the latest LLVM for trunk lifetime and cross-TU analyses

**Problem.** The analyses that matter most for our lifetime class are newer than any compiler we
pin. Lifetime Safety's invalidation checks, cross-function inference and the stable
`-Wlifetime-safety-*` names are trunk (23), while Clang 22.1 has only `-fexperimental-lifetime-safety`.
The cross-TU static analyzer is not run at all. Our builds use Clang 18 and 20, and clang-tidy uses
22.1.8. The annotations that 0343 adds are portable, so a newer analyzer validating them helps
every compiler we support.

**Proposal.** A scheduled (nightly on master) and on-demand job that pins a recent LLVM, from nix
(a pinned nixpkgs revision) or the apt.llvm.org snapshot with a recorded version, keeping
TLS/checksum checks. It runs:
- `-Wlifetime-safety-strict` plus the experimental invalidation and inference groups over `src/`
  and `test/`;
- `-Wthread-safety -Wthread-safety-beta`;
- the static analyzer with CTU (`clang-extdef-mapping` + `-analyzer-config experimental-enable-naive-ctu-analysis=true`, or `scan-build --ctu`).
It reports findings as a job summary and an artifact, and it never blocks a merge. A finding that
reproduces on a released compiler, or under a sanitizer, becomes a task.

**Done when** the job runs nightly with a pinned LLVM version, its report lists findings by check,
and the first report is triaged into tasks or into a suppression list with reasons.
