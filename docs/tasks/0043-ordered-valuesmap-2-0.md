---
status: open
priority: medium
area: release
depends: [0031]
touches: [include/jinja2cpp/value.h, include/jinja2cpp/reflected_value.h, src/value_visitors.h#ValueRenderer, CMakeLists.txt]
shares: [src/ordered_map.h, src/internal_value.cpp, include/jinja2cpp/binding/nlohmann_json.h, README.md, test/parity/divergences.txt]
---
# Insertion-ordered ValuesMap (2.0.0)

**Problem.** After 0031, dict literals and kwargs keep insertion order, but a dict passed
from C++ arrives as `jinja2::ValuesMap`, which derives from `std::unordered_map`: iterating
it gives hash order, which differs between standard libraries, and its repr is sorted to
stay stable (`literals.dict_order` stays a divergence for that reason). Making it ordered
changes a public type's layout, an ABI break that needs SOVERSION 2. Ruslan chose on
2026-10-01 to keep wave 2 non-breaking and land this in a planned 2.0 release together with
any other pending public-API breaks, so there is one SOVERSION bump.

**Proposal** (step 3 of the 0031 plan in `docs/tasks/0031-insertion-ordered-mappings.md`):
- move `src/ordered_map.h` to `include/jinja2cpp/utils/ordered_map.h` and rebase
  `struct ValuesMap : OrderedMap<std::string, Value>`; keeps every `unordered_map` member
  except the bucket API, `hash_function`/`key_eq`/`get_allocator`, node handles and the
  implicit conversion to `std::unordered_map<std::string, Value>&`; update the doc comment
  in value.h that says it is based on `std::unordered_map`;
- drop the key sort in `ValueRendererBase::operator()(const MapAdapter&)` so dicts print in
  their own order, and sort `GetKeys` of reflected structs (`reflected_value.h`, a static
  `unordered_map`) so their repr stays the same on every platform;
- `project(... VERSION 2.0.0)`, SOVERSION 2, README migration notes (dropped members; context
  dicts, repr and `xmlattr` follow insertion order; kwargs follow call order);
- add a `Reflector<nlohmann::ordered_json>` (only `nlohmann::json`, which is sorted, has one),
  so JSON input can keep document order;
- corpus cases on context dicts with unsorted keys: iteration, repr, `first`/`join`/`reverse`.
- Every new mapping producer (`items`, dict methods, `dict()`, `namespace()`, `urlencode` of
  a dict) must build an `InternalDict`, not an `InternalValueMap`, or it brings hash order back.

**Done when.** `statements.for_dict_keys` and `literals.dict_order` are gone from
`test/parity/divergences.txt`, no line uses kind `unordered` (then retire the kind and the
wide multiset comparison in parity_test.cpp), and the library reports SOVERSION 2.

**Next.** `ValuesMap`'s key type is frozen again until 3.0, so settle non-string keys (0036)
before or with this release if they need a public type.
