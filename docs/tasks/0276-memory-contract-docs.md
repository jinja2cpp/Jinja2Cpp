---
status: open
priority: medium
area: docs
depends: [0256, 0257, 0275]
touches: [README.md, docs/api-2.0.md]
---
# User documentation for the memory contract

**Release impact (0221).** G6: the README is part of the advertised surface.

**Work.** After 0256, 0257 and 0275 land, adjusted to what landed:
- README "Memory and threads" subsection under 0076's "Compatibility and versioning", from
  section 5 of [design/0252-memory-contract-plan.md](../design/0252-memory-contract-plan.md):
  what is allocated and kept where, out of memory vs budgets, `RenderTo` and sinks,
  `ReleaseThreadCaches`, untrusted templates (link to 0262's limits);
  and the known limit that OOM inside nlohmann::json's destructor (the nlohmann binding)
  terminates the process (0279; drop the note if 0279 fixes it first);
- docs/api-2.0.md 3.9 brought to its final form; Doxygen notes on the new declarations;
- the notes for MIGRATION.md (0077: `Render` no longer reports `bad_alloc` as
  `UnexpectedException`) and docs/security.md (0224: link to the contract).

**Done when.** G6's snippet compile covers the README examples (`RenderTo`, a sink,
`ReleaseThreadCaches`).
