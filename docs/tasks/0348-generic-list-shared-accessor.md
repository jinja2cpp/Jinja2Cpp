---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/699
priority: medium
area: api
depends: [0335]
touches: [include/jinja2cpp/generic_list.h, include/jinja2cpp/value.h, include/jinja2cpp/make_generic_list.h, include/jinja2cpp/reflected_value.h, include/jinja2cpp/json_adapter.h, src/internal_value.cpp, src/borrowed_cells.h, src/borrowed_cells.cpp, docs/api-2.0.md]
---
# GenericList and GenericMap hold their accessor by a shared pointer

**Problem.** `GenericList` and `GenericMap` hold `std::function<const I...ItemAccessor*()>`. Every
accessor the library makes is a lambda that captures the accessor object by value, so the
`std::function` allocates it, and each copy of a `GenericList` copies the lambda (and allocates
again). A module call's export (0335, PR #695) needs a context that every value of the call shares
and that outlives the call (`CallExports`). It is a second allocation next to the view's
`std::function`, which is most of `Module/keep`'s +7.2% in #695 (one allocation per call that
hands out data). Ruslan agreed on 2026-10-29 to change the public API for this.

**Proposal.**
- `GenericList` and `GenericMap` hold `std::shared_ptr<const IListItemAccessor>` (resp.
  `IMapItemAccessor`). A copy is a reference-count increment, and the object is 16 bytes instead
  of 32 (check `ValueStorage` and `InternalValue` stay within their `static_assert`s).
- New constructor from `std::shared_ptr<const I...ItemAccessor>`, so that the library can make the
  accessor with `std::make_shared` (one allocation) or point into a larger object with the aliasing
  constructor.
- The `std::function` constructor stays for compatibility (it wraps the function in one shared
  holder), or becomes a constructor template over the callable that stores it in that holder
  without a second allocation. Record the choice in docs/api-2.0.md.
- `CallExports` is made with `std::make_shared` and holds the view of the first cell it hands out;
  the `GenericList` of that view uses the aliasing constructor, so a call that hands out one value
  allocates once, as before #695. The embedded view must not hold a strong reference to its own
  `CallExports` (a cycle): it keeps a raw back pointer, and views outside it a `shared_ptr`.

**Estimate.** The allocation and `std::function`'s manager calls are about 150-200 of the 490 Ir
`Module/keep` gained in #695, so this alone brings it to about +4%. The rest is the pin's own work
(the registry lookups at the call's end and `AddCell`'s dedup), which a later trim can target.
Copies of any `GenericList` (reflected and JSON lists, `MakeGenericList`) should get cheaper.

**Done when.** `Module/keep` and `Module/keep_written` are within +3% of master before #695, no
other `bench/count.py` row regresses, the public headers stay C++17, `docs/api-2.0.md` records the
change, and the MSVC, AppleClang and shared-library CI rows are green.
