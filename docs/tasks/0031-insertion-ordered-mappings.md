---
status: in-progress
priority: medium
area: parity
depends: [0001, 0013]
touches: [include/jinja2cpp/value.h, src/serialize_filters.cpp]
shares: [src/internal_value.cpp, src/internal_value.h, src/generic_adapters.h]
---
# Mappings keep insertion order

**Problem.** Python dicts keep insertion order, and templates rely on it when iterating
a mapping or serialising it (`tojson`, `pprint`). `jinja2::ValuesMap` derives from
`std::unordered_map`, so the order is a hash order that can change between standard
libraries and versions: output is not reproducible across platforms (3 cases, more
hidden behind 0012 and 0013).

**Proposal.** Public API change: an architect plan should weigh replacing the base of
`ValuesMap` with an insertion-ordered map (ABI and source break for users who use
`unordered_map` members) against keeping `ValuesMap` and ordering only dict literals and
internal maps. Python's `tojson` sorts keys, so `tojson` can be fixed independently by
sorting.

**Done when.** No line of `test/parity/divergences.txt` names task 0031, and `ctest -R parity` passes.

## Plan (architect, Oct 2026)

**Contradiction.** Parity wants every mapping to iterate, print and pass kwargs in
insertion order; most user data arrives as `jinja2::ValuesMap`, a public
`std::unordered_map` whose layout is compiled into user code, so ordering it is an ABI
break (SOVERSION 1→2). Ordering literals, kwargs and internal maps needs no API change.

**Options.** A: rebase `ValuesMap` on an ordered map (ABI break, small source break).
B: keep `ValuesMap`, order only literals and kwargs; context dicts never match Python.
C (chosen): A in phases, internal work first, the public swap in a major release.
Everything B needs is also needed for A.

**Container.** `jinja2::OrderedMap<K, V>` in `include/jinja2cpp/utils/ordered_map.h`:
a `std::list<std::pair<const K, V>>` plus an `std::unordered_map<KeyRef, list_iterator>`
index, `KeyRef{const K*}` hashed through the pointer (allocation-free lookup in C++14).
Keeps `value_type`, mutable `it->second`, reference and iterator stability, O(1) erase
that keeps the others' order (Python `del`). Assigning to an existing key keeps its
position; `insert`/`emplace` of an existing key is a no-op. Copy rebuilds the index;
`==` is order-insensitive (Python dict `==`). Rejected: vendored `robin_hood.h` (not
installed, unversioned namespace, archived), Boost.MultiIndex (public Boost dependency,
no `operator[]`), vector-backed maps (`it->second` not assignable, references
invalidated, O(n) erase).

**Source compatibility of the swap.** Kept: constructors (default, initializer list,
range, copy, move), `operator[]`, `at`, `find`, `count`, `contains`, `equal_range`,
`emplace`, `try_emplace`, `insert` (all forms, hint ignored), `insert_or_assign`,
`erase`, `clear`, `size`, `empty`, `swap`, iteration, `reserve`, `==`, member typedefs.
Dropped: bucket API, `load_factor`, `rehash`, `hash_function`, `key_eq`,
`get_allocator`, node handles, and the implicit conversion to
`std::unordered_map<std::string, Value>&`.

**Where order reaches output** (all through `MapAdapter::GetKeys()`):
- change: `ListConverter` (for loops and every `ConvertToList` consumer) follows the map;
  dict repr (`value_visitors.h`) sorts keys today and stops sorting with the swap;
  `pprint` and `tojson` (boost/rapid serializers) sort keys, as Python does;
  `dictsort` becomes `stable_sort` (Python `sorted` is stable); `DictCreator` (dict
  literals; a duplicate key keeps its first position and takes the last value); the kwargs
  chain (`CallParams`, `CallParamsInfo`, `ParsedArguments*`, macro `kwargs`, `**kwargs`
  for user callables); reflected-struct `GetKeys` gets sorted when the repr sort goes.
- leave unordered: render-context scopes (hot path), loop variable, import namespaces,
  macro attributes, `groupby` items, template params and globals, named `format` args.
- not implemented yet, must use the ordered map when they land: `items` (0018), dict
  methods (0020), `urlencode` of a dict (0019), `dict()` (0030), `namespace()` (0021).

**JSON bindings** adapt documents in place (GenericMap), never converting to
`ValuesMap`: boost::json and rapidjson keep document order, `nlohmann::json` is sorted,
`nlohmann::ordered_json` has no reflector yet (follow-up).

**Steps.**
1. Sort keys in `pprint` and the tojson serializers; the parity harness parses case files
   as `nlohmann::ordered_json` so context dicts reach C++ in Python's order.
2. Add `OrderedMap`; `InternalDict = OrderedMap<std::string, InternalValue>` for dict
   literals and the kwargs chain; `dictsort` stable. Container unit tests; corpus cases
   for dict-literal iteration, macro kwargs order, `list` of a dict, order-insensitive `==`.
3. The breaking swap: `ValuesMap : OrderedMap`, drop the repr sort, sort reflected-struct
   keys, SOVERSION 2 / version 2.0.0 with migration notes; corpus cases on context dicts.

**Decision (Ruslan, 2026-10-01): break later.** This task lands steps 1 and 2; step 3
is task 0043, for a 2.0 release that bundles the pending public-API breaks.
`statements.for_dict_keys` (context dict) and `literals.dict_order` (repr sort) stay listed
under 0031 until then.

**Risks.** Two allocations per entry (list node plus index node) on dict literals and
kwargs. Measured on a Release build, 200k renders of a 3-key dict literal took about 190-200 ms
against 127-157 ms on master; with the index built only above 8 entries (smaller maps are
searched linearly, one allocation per entry) it is within noise of master, and macro kwargs
are slightly faster.
C++14/MSVC (no heterogeneous lookup, `std::list` move not `noexcept` on MSVC). The index
holds pointers into list nodes: run the container tests under ASan/UBSan.
