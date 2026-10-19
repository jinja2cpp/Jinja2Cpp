---
status: open
priority: high
area: release
depends: [0076, 0221]
touches: [abi/, abi/suppressions.ini]
---
# Commit the ABI baselines at the 2.0 RC and turn the abidiff gate on

**Problem.** Since 0076 the three `abi` rows of linux-build.yml (one shared RelWithDebInfo
row per standard) dump the library's ABI with abidw and compare it with the last master
build. Until baselines are committed the job only reports: the public API is still
changing in the API wave, and failing on every change would block it. The SOVERSION
policy (README, "Compatibility and versioning") is enforced only once
`abi/libjinja2cpp.so.2.cxx{17,20,23}.abi` exist; then `scripts/abi_report.sh` fails an
incompatible change unless SOVERSION was bumped.

**Proposal.** On the RC commit (0221 G1 freeze), download the `abi-cxx17`, `abi-cxx20` and
`abi-cxx23` artifacts of that commit's linux-build run and commit them as `abi/`. Before
that, read one report and review `abi/suppressions.ini`: it suppresses only the vendored
value_types and robin_hood/lexertk names; `jinja2::v2::detail` stays checked because
inline header code calls exported detail functions. Add suppressions only for noise that
is shown not to reach users. Two things to settle first: a RelWithDebInfo dump is about 31 MB
(1.2 MB gzipped), so commit the baselines compressed or trimmed and teach
`scripts/abi_report.sh` to read them; and the dump includes weak std and Boost symbols the
library exports (`boost::system` among them), so a Boost pin bump may read as a break
unless those are suppressed or hidden. `scripts/**` is in linux-build's `paths-ignore`, so
a PR that touches only `abi_report.sh` runs no abi row: check such a change by hand.

**Done when.** The baselines are on master, a deliberate test change to an exported
layout makes the abi rows red, and docs/release-2.0.md lists the step in its RC procedure.
