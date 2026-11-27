---
status: open
priority: low
area: robustness
depends: [0336, 0572]
touches: [include/jinja2cpp/reflected_value.h, src/internal_value.cpp, test/borrowed_cells_test.cpp]
---
# A module keeps the cell of a reflected `shared_ptr` it wrote, one per call

**Problem.** Found while fixing 0572. `Reflector<T>::CreateFromPtr(const std::shared_ptr<T>&)`
(include/jinja2cpp/reflected_value.h) lends a struct over an aliasing pointer into a holder that
keeps the `shared_ptr` and a `ReflectedMapImpl` member. The accessor is an `OwnedAccessor` that
`make_shared` did not make, so `weak_from_this()` is empty and `AccessorIsLent` reads it as an
accessor the host may lend again (internal_value.cpp). A template module that writes such a value,
`Reflect(std::make_shared<S>(...))` passed fresh to each call or a `shared_ptr` field of a fresh
struct (`d.innerStruct.update(k=1)`), keeps one cell per call, and each cell keeps its struct,
until the module goes; `DropUnheld` walks all of them at every call's end. The list case of the same
shape drops since 0572 (`BorrowedCellsSingle.ModuleCallFreesTheCellOfANestedItemOfAGenericArgument`,
whose comment points here).

**Proposal.** Let the library recognise the holder, as `WithOwner`'s `LentItemHolder` is recognised
since 0572: build it as the deleter of a shared pointer that owns nothing (one allocation, found by
`std::get_deleter`), or make the holder's accessor share the holder's control block so that
`weak_from_this()` names it. Then the accessor's own owners decide, as for an accessor
`make_shared` made. The header is public: keep the inline code C++17 and the value's ABI as it is.

**Done when** a module call per request with a fresh `Reflect(std::make_shared<S>())` argument, and a
fresh struct whose `shared_ptr` field the macro writes, leaves a bounded number of the structs alive
(a token's `use_count()`), with a test row for each.
