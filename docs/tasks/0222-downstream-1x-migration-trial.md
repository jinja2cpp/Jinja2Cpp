---
status: open
priority: high
area: release
depends: [0077]
touches: [test/migration/, scripts/check_migration.py]
shares: [.github/workflows/linux-build.yml, MIGRATION.md]
---
# Prove a downstream upgrade from 1.3.2 to 2.0

**Problem.** 0077 proposes testing the migration tool against our own old-style
tests. That does not show how an external application upgrades custom accessors,
reflection, filesystem handlers, callables and templates whose behavior changed.
Successful compilation alone can hide changed rendered output.

**Proposal.** Keep migration-tool implementation and MIGRATION.md in 0077. Add a
separate downstream trial: select a maintained consumer with its owner's agreement,
or commit a small self-contained consumer fixture modeled on these public extension
points. Record its origin and license; do not copy private application code.

Build the original fixture against 1.3.2, then migrate a disposable copy against the
candidate's installed package. Exercise renamed Settings fields and Value methods,
a custom list/map accessor, reflection, a context-aware callable, a loader, and
representative rendering. Pin expected template changes such as None/undefined,
whitespace, value printing, format syntax and macro scoping rather than requiring
the old and new outputs to be identical. Separate automatic edits from manual ones.

Run the repeatable fixture in CI. Cover the compiler diagnostic formats promised by
0077 there; do not duplicate the migration tool's parser tests in this task. Feed
findings and the tested upgrade transcript back to the migration documentation.

**Done when.** The original consumer builds/runs with 1.3.2, the migrated consumer
builds/runs against the installed RC, expected behavior changes are asserted, and
the documented remaining manual edits match the trial. Unsupported cases fail with
actionable guidance rather than being silently skipped.
