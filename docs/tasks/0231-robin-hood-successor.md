---
status: open
priority: low
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
