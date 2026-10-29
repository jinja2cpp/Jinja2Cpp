---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/695
priority: low
area: robustness
depends: [0331]
touches: [src/borrowed_cells.h, src/borrowed_cells.cpp, test/borrowed_cells_test.cpp]
---
# A module's live cell read through a container the host keeps

**Problem.** A template module's borrowed cells live across its calls (docs/tasks/0049 P2).
Since 0331 D8 a container a call hands to the host directly (`keep(l)` after `l.append(x)`)
is copied on a later call's write, so the host keeps that call's final state. A container
the call makes that holds the adapter (`keep([l])`) does not get that protection: the host
keeps the template's list, whose item is `l`'s adapter, attached to the module's live cell.
A later call's write shows through it (`[[0, 1, 2]]` where the first call ended with
`[[0, 1]]`), and reading it on another thread while a module call writes the cell is a data
race, which Ruslan's rule (no mutable state shared between renders, threads and the host)
excludes. The values are freed correctly; the cell freezes when the module goes.

Pinned by `BorrowedCellsKept.ModuleCellInsideAKeptList` (current behaviour, with the target in
its comment).

**Options.**
- Freeze the module's cells at the end of every call and give the next call that writes a
  fresh cell copied from the frozen one, re-linking the module's own adapters (option 3 of
  the D8 brainstorm). Costs an O(n) copy for every call that writes, unless the frozen cell is
  found unshared.
- Note, at the export of a template container during a module call, that the call handed out
  values; at the next call's first write to any cell, copy every cell whose links include one
  held from outside the module's context (needs a way to tell those links apart).

**Decision (Ruslan, 2026-10-29).** Call-end pinning (0335's design) plus owning links to module cells (`FixModuleLinksOwn`); 0340 takes its option (a), `FixPinOnAttach`. Next: the C++ step.

**Design (model-first, 0338 step A; decided 2026-10-29).** The pin of 0335 (`FixCallPin`) covers
`keep([l])` and nested items as well: the pin resolves each item of an exported template container,
and each nested item of an exported list, by identity, so the host's value keeps the state at the
call's end and later calls copy before they write. Two lifetime rules come with it, since the
host's value can hold the adapter object that the module's cell links:

- **A link to a module's cell owns the cell** (`FixModuleLinksOwn`), as a link to a frozen cell
  does: `Link` takes a reference, `Unlink` releases it. A host thread that drops the last
  reference to such an adapter then writes no `m_links` vector of a live cell. `IsUnheld` counts
  references instead of links, the module's `Finish` needs no walk of the links (except a global's,
  which the thread's snapshot holds), and the collector's traversal must report these links
  (`TraverseFrozen` extends to module cells; an obligation for the collector spec, 0338 step E).
  Mutant `module-links-unowned`: a later call's adapter kept by `keep([l])`, dropped on another
  thread, unlinks from the live cell (`NoCrossThreadWrite`).
- The host's reads go only through the pinned context, never through a link, so the module
  re-linking an adapter shared with a pinned storage is not a race.

The model now has item removal (`l.pop()` of a nested list, op `drop`), the path by which a pinned
storage becomes the only holder of a nested adapter. Runs: `module-0332` (keep([x]), nested writes,
removal) and `threads-module-kept` pass; `no-call-pin-kept-list` and `no-call-pin-nested` are this
task's counterexamples. 0340 (the same unlink in a render's window before `Finish`) is decided
separately: the model uses its option (a), `FixPinOnAttach`.

**Done when.** The test row expects `[[0, 1]]` and a TSan row (two threads calling the module,
one reading the kept list) is clean, with `Module/keep` unchanged in `bench/count.py`.

**Implementation** (PR #695). With 0335's `CallExports`: a list or dict the template owns, handed out in a module
call, is a view whose items that are the module's data read through the call's exports by identity
(`CallExportItem`), and its pin takes every cell of the module (simpler than a walk of template
containers, and only copies more). Every link but a global's owns its cell (`CellLink::OwnsBit`), for
a render's cells too: that is how the C++ closes 0340 instead of option (a), since a link in an
`ItemAdapters` block has no object of its own for the registry to hold (tla/README.md, "Designs";
`FixLinksOwn` in the model). The remaining case of a global's nested item is task 0346.
