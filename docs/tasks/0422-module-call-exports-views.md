---
status: open
priority: low
area: api
depends: [0357]
touches: [src/borrowed_cells.cpp, src/internal_value.cpp#ExportCallList]
shares: [test/public_host_object_test.cpp]
---
# Exports of a running module call keep tuples, keys and lent members

**Problem.** Found while implementing 0357 S1a. 0357 gave the public export of engine values three
properties: an exported tuple answers `GenericList::IsTuple()`, an exported dict gives its own keys
through `GenericMap::GetItems()` (`{1: 'a', '1': 'b'}` gives two items), and template callables
inside a container passed to a host call are lent for the call. The views made while a template
module call runs (`ExportCallList`, `ExportCallMap`, `src/borrowed_cells.cpp:107,123`,
`src/internal_value.cpp:1627,1633`) have none of the three: a C++ callable reading a module's
exported tuple gets a list, a dict with non-string keys collapses them, and a macro inside a
container comes out as `None`.

**Proposal.** Give the call-export views the same three answers as the ordinary export views
(`TupleExportView`, `DictExportView`, `LendingMapView`/`LendingListView`), reusing those classes where
the cell protocol (0338, docs/tla) allows. Rows in `test/public_host_object_test.cpp` through a
`MakeModule` call, both character types.
