---
status: open
priority: low
area: perf
depends: [0521]
touches: [src/template_module.cpp, src/params_table.h, src/template_impl.h]
---
# Copy the params of a nested render when a value of them is stored, not on every render

**Problem.** Split from 0521. A render nested in another one that shares its globals converts its
params as owned copies (`OwnedArgument`, 0481) on every render: O(size of the params) a render, so a
host that renders a template with a large params map inside a render of its own pays it each time,
whether or not the nested render stores anything of them in a shared global. 0521 gave the trees back
by ownership; the params are the part of 0481's cost that remains. 0521 plan step 3 makes the
conversion a single pass; this removes the copy for the renders that store nothing.

**Proposal.** Convert a param on its read, where `ParamsTable` resolves the name, under a keeper
only (a per-read mode of the table that copies the value it hands out and caches the copy for the
render), so that a param the template never reads, or reads without storing, costs nothing. The
open question is the one 0521 left out: deciding "stored where it can outlive the render" is a
reachability argument over the cell protocol again (0481 verifier round 2 and the advisor's lent
macro found two holes in it), so only a per-read copy that holds for every use, not a store check,
is acceptable.

**Done when.** `Nested/shared_globals` with a large params map costs what a render without a keeper
does; the 0481 and 0521 tests in `test/borrowed_cells_test.cpp` stay ASan clean; the bench rows stay
within the perf bar.
