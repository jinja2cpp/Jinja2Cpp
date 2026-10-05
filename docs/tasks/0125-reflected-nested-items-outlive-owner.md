---
status: open
priority: high
area: robustness
depends: []
touches: [include/jinja2cpp/reflected_value.h, src/internal_value.cpp#GenericListAdapter, src/internal_value.cpp#GenericMapAdapter]
---
# Nested items of a reflected value a callable returns outlive their owner

**Problem.** The fix for callable-returned `ValuesList`/`ValuesMap` (0115's first PR, which
copies strings and lends nested containers through an aliasing `shared_ptr`) does not cover
reflected values. A callable returning `Reflect(std::vector<std::vector<std::string>>{...})`,
`Reflect(std::vector<Rec>{...})` or a reflected struct with a `std::vector<std::string>`
field gives a `GenericList`/`GenericMap` held by a `BySharedVal` adapter, and its nested
items come from `Reflect(*p)` (`PtrItemAccessor` in `reflected_value.h`): raw pointers into
the parent. Once an item escapes the container, the root is freed and the item dangles
(heap-use-after-free under ASan, same on master before 0115). The verifier's generated
sweep still crashes on 132 templates, all with these sources, for example:

- `{% set r = rnested() | sort %}{{ r }}`
- `{% set ns = namespace(v=none) %}{% for it in rnested() %}{% set ns.v = it %}{% endfor %}{{ ns.v }}`
- `{% set r = rrec().copy() %}{{ r|tojson }}`
- `{% set r = rrec().items()|list %}{{ r|tojson }}`
- `{% set r = rrecs()[1:] %}{{ r|tojson }}`

A flat `Reflect(std::vector<std::string>)` is fine.

**Proposal.** Give the generic adapters an owner-aware path like `LendOwnedItem` in
`internal_value.cpp`: when the holder owns the root (`BySharedVal`), a nested
`GenericList`/`GenericMap` item must keep the root alive, e.g. the reflector hands out
sub-accessors that share ownership of the value (`ContainerReflector::CreateFromValue`
already owns its copy; the pointer variants need an owner handle), or the adapter wraps the
item's accessor provider so it captures the root's `shared_ptr`.

**Done when.** The templates above render under the ASan configuration, pinned as rows of
`OwnedUserDataTest` in `test/user_callable_test.cpp` with reflected sources.
