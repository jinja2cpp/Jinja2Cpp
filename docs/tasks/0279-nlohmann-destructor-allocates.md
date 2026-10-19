---
status: open
priority: low
area: robustness
depends: [0253]
touches: [src/binding/nlohmann_json_parser.h, include/jinja2cpp/binding/nlohmann_json.h, test/allocation_failure_test.cpp]
---
# nlohmann::json destruction allocates, so OOM there terminates

**Problem.** Found by the allocation-failure sweep in 0253 (PR #597, CI job "C++20 gcc-14
Release shared=ON json=nlohmann"). `nlohmann::basic_json::json_value::destroy` (json.hpp,
v3.12.0) flattens a non-empty array or object into a heap `std::vector` before freeing it.
That runs inside `~basic_json`, which is `noexcept`, so an allocation failure there is
`std::terminate`, not the `std::bad_alloc` the error contract (docs/api-2.0.md 2.1) promises.
With `JINJA2CPP_WITH_JSON_BINDINGS=nlohmann` this is reached from `{% meta %}` parsing
(`GetMetadata`) and from host values reflected through `jinja2cpp/binding/nlohmann_json.h`.
`boost` and `rapid` bindings are not affected. 0253 skips the metadata row of
`jinja2cpp_alloc_tests` in nlohmann builds; the Load and Render rows pass.

**Options.** Report upstream (an allocation-free `destroy`, for example falling back to
recursion when the reserve fails); patching the fetched header is not worth carrying. Or
document it as a limitation of the nlohmann binding in the 2.0 memory contract (0252, 0276).
Recommended: document and report upstream; the default binding is `boost`.

**Done when.** The limitation is documented (or fixed) and the metadata row runs, or is
skipped with a pointer to the documentation, in nlohmann builds.
