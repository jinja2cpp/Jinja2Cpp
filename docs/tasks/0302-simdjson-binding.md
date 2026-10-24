---
status: open
priority: low
area: api
depends: [0230]
touches: [include/jinja2cpp/binding/simdjson.h, test/binding/simdjson_binding_test.cpp]
shares: [CMakeLists.txt, thirdparty/internal_deps.cmake, thirdparty/thirdparty-conan-build.cmake, sbom/components.json, docs/dependencies.md]
---
# An optional simdjson binding

**Problem.** Hosts that feed large JSON documents into templates pay for the parse
twice: once in their JSON library, then again when the document is reflected into
`jinja2::Value`. The opt-in binding headers (`jinja2cpp/binding/boost_json.h`,
`nlohmann_json.h`, `rapid_json.h`) cover the common libraries but not simdjson, the
fastest one, whose on-demand API can be walked straight into reflected values.

simdjson is not a fit for the core. 0230's `{% meta %}` reader stays internal (Ruslan,
2026-10-24): meta blocks are small and parsed once, and the core must follow the memory
contract and `Settings::Limits`. simdjson's size, its CPU-dispatched kernels and its
padded input belong to a binding that users choose, not to every build.

**Proposal.** Add `include/jinja2cpp/binding/simdjson.h` in the style of the other
bindings: `Reflect` for `simdjson::dom::element`, `dom::object` and `dom::array`, and an
on-demand adapter that builds a `Value` without a DOM. Integers map as the other bindings
do (int64, or a big-integer backend where `uint64`/`bigint` values exceed it; 0178).
Fetch it only when tests or `JINJA2CPP_WITH_SIMDJSON_BINDING=ON` ask for it, and record it
in `sbom/components.json` as test/optional. Benchmark it in bench/ against the nlohmann
binding on a large context.

**Done when.** `test/binding/simdjson_binding_test.cpp` runs the same reflection rows as
the nlohmann binding test in one Linux and one MSVC CI job; the header is installed;
docs/dependencies.md lists simdjson as public-optional.
