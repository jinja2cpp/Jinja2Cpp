---
status: open
priority: high
area: robustness
touches: [include/jinja2cpp/binding/nlohmann_json.h, test/binding/nlohmann_json_binding_test.cpp]
shares: [src/internal_value.cpp, src/generic_adapters.h]
---
# Segfault iterating arrays reflected from `nlohmann::json`

**Problem.** Passing an `nlohmann::json` array through `jinja2::Reflect` and iterating it
in a template (`{% for x in l %}`) crashes in
`GenericListAdapter<ByRef>::Enumerator::MoveNext` (`src/internal_value.cpp`, around
line 425). Found by the parity probe driver; not yet root-caused. A by-reference adapter
over a temporary is the first suspect.

**Proposal.** Reproduce under ASan in a unit test in `test/binding/nlohmann_json_binding_test.cpp`,
fix the lifetime issue, keep the test.

**Done when.** The new test passes under the `address+undefined` sanitizer job.
