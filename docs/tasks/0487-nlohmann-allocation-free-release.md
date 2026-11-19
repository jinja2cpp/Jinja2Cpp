---
status: in-progress
priority: low
area: robustness
depends: [0279]
touches: [include/jinja2cpp/json_adapter.h, include/jinja2cpp/reflected_value.h, bindings/nlohmann_json/include/jinja2cpp/binding/nlohmann_json.h, test/allocation_failure_test.cpp, test/binding/nlohmann_json_binding_test.cpp, docs/api-2.0.md]
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
   reserves nothing. No recursion (deep host documents), no heap, O(n * depth) in the worst
   case: measure it on the bench JSON documents against nlohmann's own destructor.
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

## Outcome

- **Hooks** (`json_adapter.h`, both optional): `JsonTraits::Release(Json&) noexcept` empties a
  document the view owns before its destructor; `JsonTraits::Copy(const Json&, Json&)` builds the
  copy of a non-const lvalue in place, so that a copy that runs out of memory leaves a partial
  document in the view's storage, which `Release` empties. Traits without them behave as before.
- **Owned storage.** `Create` has a `const Json&` (copy) and a `Json&&` (move) overload. With
  `Release`, the view allocates its storage (one `make_shared` holding an empty document whose
  destructor calls `Release`) before it moves or copies the document in, so a failed allocation
  leaves an rvalue as it was. `Reflect(std::shared_ptr<Json>)` wraps the pointer in a holder that
  calls `Release` when it holds the last reference (`use_count() == 1`); one more allocation per
  such `Reflect`, on the host's setup path. The docs say not to lock a `weak_ptr` to such a
  document while its last `Value` drops. A `std::shared_ptr<const Json>` is not emptied.
- **Found on the way:** `Reflector<std::shared_ptr<T>>::Create` took `const std::shared_ptr<T>&`
  and "moved" it, which copied: `Reflect(std::move(ptr))` left the caller's pointer set, so the
  view never held the last reference. It takes the pointer by value now.
- **nlohmann drain.** Post-order: pop the last child of the deepest container on the path while it
  is a scalar or an empty container; a ring of 256 ancestors on the stack, restart from the root
  when a deeper path runs out of it. The work is capped at 8 steps per removed value plus 4096, so
  a chain deeper than about 1500 levels is emptied only partly and nlohmann's destructor frees the
  rest (and allocates); an untrusted deep document cannot make the drain quadratic (verifier round
  1 measured 10.7 s for a 400 000-deep chain before the cap). Measured on the 22
  `bench/cases/*/data.json` documents (200 copies, 5 rounds, `-O2`): `Release` plus the destructor
  took 0.41x to 1.21x the time of nlohmann's own destructor, since that destructor moves every
  child into a heap vector; a 5000-deep chain takes 1.13x (the cap stops the drain early).
- **nlohmann copy.** Built in place, recursively, up to 512 levels, then nlohmann's own copy
  (whose frames are smaller) for what lies deeper, which is not protected. Members are appended
  without a lookup (`emplace_hint(end())` for `std::map`, `emplace_back` on `ordered_map`'s vector):
  `ordered_map::emplace` scans for duplicates, which made a wide `ordered_json` copy quadratic.
- **Item 3, other types.** The vendored `polymorphic` frees its control block in `reset()` and
  does not allocate. `boost::json::value` destroys its children recursively and frees through its
  `storage_ptr`, with no allocation (recursion depth is bounded by the parser's `max_depth`, 32
  by default). simdjson (0302) is a handle type, which the view never owns.
