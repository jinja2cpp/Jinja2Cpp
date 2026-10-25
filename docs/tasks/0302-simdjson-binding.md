---
status: open
priority: low
area: api
depends: [0230, 0311]
touches: [bindings/simdjson/, test/binding/simdjson_binding_test.cpp]
shares: [thirdparty/internal_deps.cmake, sbom/components.json, docs/dependencies.md, .github/workflows/bindings.yml]
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

**Proposal (rebased onto 0311's contract, 2026-10-25).** Start after 0311's S5 (bindings
as components). Add `bindings/simdjson/include/jinja2cpp/binding/simdjson.h` on
`jinja2cpp/json_adapter.h`:
- **dom:** `JsonTraits<simdjson::dom::element>` with `Node = dom::element` and
  `IsHandle = true`. `Reflect(element)` borrows (the `dom::parser` or document must outlive
  the render); `ReflectJson(std::shared_ptr<dom::parser>, element)` owns. Provide
  `Elements()`, because `dom::array::at` is linear; `Find` is linear too (document it). The
  dom view keeps repeated keys and `Find` returns the first entry where Python takes the
  last: a documented library divergence. `ToValue` follows Python.
- **on-demand:** `JsonTraits<simdjson::ondemand::document>` (and `value`), copy-only: the
  `ForEach*` functions iterate and return `JsonResult` on any `simdjson_result` error;
  `get_number_type() == big_integer` gives `JsonKind::BigInt` with `AsBigIntText` from
  `raw_json_token()`; `Location(Node)` from `current_location()` so errors say `(char N)`
  (0311 decision 9); `Root` takes `document&` and handles a scalar document
  (`SCALAR_DOCUMENT_AS_VALUE`).
- Integers as 0178. Fetched only when `JINJA2CPP_TEST_BINDINGS` lists `simdjson`; the SBOM
  records it as optional/test with a hash-pinned URL.

**Done when.** The 0311 conformance suite passes for `dom::element` (view and copy) and
`ondemand::document` (copy) in `bindings.yml` on Linux, MSVC /MT and macOS;
`find_package(jinja2cpp COMPONENTS simdjson)` and `pkg-config jinja2cpp-simdjson` work in the
package job; docs/dependencies.md lists simdjson as public-optional; a Google Benchmark case
(not the gate) compares on-demand `ToValue` with nlohmann parse plus `ToValue` on a bench
case's data.json.
