---
status: done
priority: low
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/870
area: robustness
depends: [0336]
touches: [src/internal_value.cpp, src/borrowed_cells.cpp, test/borrowed_cells_test.cpp]
---
# A fresh generic container nested in a call's argument keeps its module cell

**Problem.** Since 0336 a template module drops, at a call's end, the cell of a generic list or map
that nothing but the call's adapter owns (`BorrowedCell::IsUnheld`, `MayBeLentAgain`). That covers a
generic container passed as an argument or returned by a host callable. It does not cover one nested
in the template's own copy of a `ValuesList`/`ValuesMap` argument: `consume([MakeGenericList(...)])`
with a new list each call, where the macro writes `xs[0]`. The item is lent by reference
(`LendOwnedItem` -> the convertor -> `ListAdapter::CreateAdapter(const GenericList&)`, a `ByRef`
holder), so its adapter reports no owner, `IsUnheld` returns false, and the module keeps one cell
and one accessor per call until it goes. Found by the verifier on the 0336 PR: after three calls the
generator's token had `use_count()` 4, back to 1 once the module was destroyed. Generic items of a
global (`LendHeldItem`) are the host's data and rightly keep their cells.

**Proposal.** Lend a generic item of owned data (`BySharedVal` parents) through a `BySharedVal`
adapter whose owner is the parent's owner plus the item's own copy, so that `AdaptersOwner` and
`MayBeLentAgain` see it; the item's cell then drops once the parent's cell has dropped and nothing
else owns the accessor (the nested pass of `DropUnheld` already loops).

**Done when.** A test like `BorrowedCellsSingle.ModuleCallFreesTheCellsOfFreshGenericData` with the
fresh list nested in a `ValuesList` argument frees the generator after each call, under ASan/LSan,
and `Module/keep` does not move in `bench/count.py`.

**Resolution.** The proposal's cause was not the one at work: the convertor already copies a
generic item by value into a `BySharedVal` adapter. What kept the cell was the anchor of the read:
`a[0]` carries its parent (`SetParentData`), the cell kept that value as its source, and the parent
`ValuesList` holds the `GenericList` that shares the item's accessor, so `MayBeLentAgain` stayed
true for as long as the cell lived. A generic adapter that owns its copy (`BySharedVal`) keeps
the accessor alive itself, so its cell now keeps the value without the anchor (`CellSource`,
`InternalValue::WithoutParentData`). `BorrowedCellsSingle.ModuleCallFreesTheCellOfAGenericItemOfAnArgument`
passes a fresh list in a `ValuesList` and a reflected struct in a `ValuesMap`: each call's cells go
at the next call's end, and all of them with the module.
