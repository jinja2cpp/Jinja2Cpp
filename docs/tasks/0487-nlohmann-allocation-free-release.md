---
status: open
priority: low
area: robustness
depends: [0279]
touches: [include/jinja2cpp/json_adapter.h, bindings/nlohmann_json/include/jinja2cpp/binding/nlohmann_json.h, test/allocation_failure_test.cpp, docs/api-2.0.md]
---
# Release owned nlohmann::json documents without allocating

**Problem.** 0279 documented the limit: `nlohmann::json`'s destructor is `noexcept` and its
`json_value::destroy` (v3.12.0) flattens a non-empty array or object into a heap
`std::vector<basic_json>` first, so running out of memory while a document is destroyed ends in
`std::terminate`. The library destroys a document in three places: the last `Value` of a
`Reflect(Json&&)` or `Reflect(std::shared_ptr<Json>)` view drops its owner; a non-const lvalue
is copied into an owned view (`Reflector<Json>::Create(Json doc)`); and a by-value `doc` unwinds
when `std::make_shared` fails. The vendored `polymorphic` (value_types, kept as a modified copy
behind `jinja2::types`) is the precedent: a third-party type whose behaviour we adjust on our
side instead of waiting for upstream.

**Proposal.**
1. An optional traits hook, `JsonTraits<Json>::Release(Json&) noexcept`, that the owned view's
   deleter (and the `Create` failure path) calls before the document's destructor. The nlohmann
   binding implements it as an allocation-free post-order drain: descend to a container whose
   last child is a scalar or an empty container, `pop_back()` / `erase()` that child (destroying a
   scalar or an empty container never allocates), repeat; the root ends empty and its destructor
   reserves nothing. No recursion (deep host documents), no heap; O(n * depth) only in the worst
   case, so try nlohmann's own path first only when it cannot fail is not possible: measure the
   drain on the bench JSON documents and keep it if the cost is noise next to the free calls.
2. `Create(Json doc)`: move into the shared storage only after it is allocated (allocate the
   control block with an empty `Json`, then swap), so a failed `make_shared` leaves the caller's
   document untouched instead of destroying a by-value copy during unwinding.
3. Check the vendored `polymorphic`/`indirect` and the other bindings (boost_json, simdjson when
   0302 lands) for the same pattern: a `noexcept` destructor or release path that allocates.
4. The upstream report is 0488.

**Tests.** test/allocation_failure_test.cpp: `Reflect(std::move(doc))`, `Reflect(doc)` of a
non-const lvalue and `Reflect(std::make_shared<nlohmann::json>(...))` rows, in both modes of
`jinja2cpp_alloc_tests` (0256), with the `Value` dropped inside the injection. They terminate
today.

**Done when.** Those rows pass in both modes, and the nlohmann limit is removed from
docs/api-2.0.md 3.9 and the binding header.
