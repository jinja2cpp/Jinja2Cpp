---
status: in-progress
priority: high
area: perf
touches: [src/robin_hood.h, src/ankerl/unordered_dense.h, src/hash_map.h, src/internal_value.h, src/render_context.h, src/render_workspace.h, src/borrowed_cells.h, test/name_map_test.cpp, .clang-format-ignore, sbom/components.json]
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

**Decision (perf track, 2026-11-14).** Switched to `ankerl::unordered_dense` 5.3.3, vendored in
`src/ankerl/` and used only through `src/hash_map.h`. Measured against master 1f489e5 with
`bench/count.py` (instructions per iteration, GCC 13 Release):

- 4.11.0 (last 4.x) and 5.3.3, both as a value-stable map (values in segments that never move, which
  the lookup cache and the loop target slots need, 0158), cost +1% to +5% on renders as first ported:
  the same across versions, so 5.3.3, the maintained line, was taken and tuned.
- Its costs came from our side and were removed: scope maps are swapped rather than moved in and out
  of the scope stack and the spare scope (a move costs several times a swap); `InternalValue`'s move
  assignment and `RenderWorkspace::ReturnMaps` are forced inline (GCC outlined both); the probe past the
  home group stays inline for `std::string` keys (`key_compare_is_call`, our NameEqual compares inline),
  since out of line it took the key by reference and gave every name lookup a stack protector; the
  has-a-table checks read the values' capacity instead of `bucket_count()`.
- Segment size 512 bytes (8 values): 1024 added up to 3 KB per render on some rows, 256 added
  allocations on Loads.

Result: renders -4.6% (`for_loop_vars`) to +1.98% (`many_tags`), most rows faster; Loads -0.3% to
+0.2%; `plain_text` +0.38%. Left for 0506: +1 to +3 allocations on four render rows (a fresh map's block
list) and the cost of adding a new name (`many_tags`, `config_file` +0.75%). `boost::unordered_flat_map`
was not measured: it moves values on insert, so it is not a candidate for the scope maps, and Boost is
leaving the core (0230). The two robin_hood local additions are not needed: unordered_dense has a
transparent `try_emplace` and allocates through `std::allocator`; its inserts keep a table valid when an
allocation or a constructor throws (test/allocation_failure_test.cpp passes).

The name hash is keyed per process (`detail::HashName`, src/hash_map.h). unordered_dense hashes
unseeded and never rehashes a crowded group, where robin_hood re-rolled its multiplier: the security
review of #891 found names chosen offline to share one group made each set or lookup about 90x slower
per step under `UntrustedSettings`. The key is drawn on the first hash and xored in before one odd
multiply; collisions of the unkeyed hash itself are 0507, the memory budget's uncounted scope maps 0508.
