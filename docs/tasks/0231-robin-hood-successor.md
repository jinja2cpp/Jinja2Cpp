---
status: open
priority: high
area: perf
touches: [src/robin_hood.h, src/internal_value.h, src/render_context.h, test/name_map_test.cpp, .clang-format-ignore, sbom/components.json]
---
# robin_hood's upstream is archived

**Problem.** `src/robin_hood.h` (3.11.5, MIT) is vendored and locally modified
(`try_emplace_transparent` for 0118, `::operator new` allocation for 0154). Its upstream
repository is archived and points to its successor, `ankerl::unordered_dense` (MIT, same
author), so fixes and compiler-compatibility updates no longer arrive and the local
changes are a fork the project maintains alone (0187 audit, docs/dependencies.md).

**Proposal.** Measure `ankerl::unordered_dense` (vendored, single header) and, while
Boost stays, `boost::unordered_flat_map` against the current map with bench/ and the
Cachegrind gate; port the two local additions (transparent try_emplace, allocation
policy) and switch if neither regresses. Otherwise keep robin_hood and record the
decision here.

**Done when.** The map choice is measured and either switched or recorded, and
sbom/components.json matches what is vendored.

**Release scope (Ruslan 2026-10-18, 0221 thread).** The switch ships before 2.0: it is a
release gate (docs/release-2.0.md G12). The performance track chooses the successor from
its measurements. With the Boost-free core (0230) wanted for 2.0, `boost::unordered_flat_map`
is only a measurement baseline, not a candidate to ship.
