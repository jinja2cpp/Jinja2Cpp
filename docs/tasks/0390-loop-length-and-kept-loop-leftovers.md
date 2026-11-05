---
status: in-progress
priority: low
area: parity
depends: [0304]
touches: [src/statements.cpp#RenderLoopInScopes, src/statements.cpp#RenderLoopInSlots, src/statements.cpp#LoopState]
shares: [test/parity/divergences/statements.txt]
---
# Loop length and kept loops: leftovers of 0304

**Problem.** Found by the 0304 verifier (parity batch 3, group B). With 0304 a loop fetches its
next item lazily, as Jinja2's `LoopContext` does, but four smaller differences remain. They are
the same on master before 0304.

1. `loop.length` (and `revindex`, the repr) is the list's size when the loop starts. Jinja2
   takes `len(iterable)` when `length` is first read, so a body that clears or pops the list
   before reading it sees the new size: `statements.for_length_after_clear` (Python `10True`,
   C++ `15True`) and, through a loop over `loop`, `statements.loop_iter_inner_length_after_pop`
   (Python `44;`, C++ `55;`). Appends already match (`for_length_after_append`).
2. A loop over `loop` kept past a `break` (`set ns.k = loop` in the inner loop) does not go on
   through the outer loop's rest: `statements.loop_iter_inner_kept_length` (Python `51True`,
   C++ `31False`).
3. `select`/`map` over a host generator collect it eagerly, where Python's generator fetches
   one item per step (no corpus case: needs a host generator; seen with a counting
   `MakeGenericList` generator).
4. A loop kept past a `break` (`set ns.k = loop`) collects its rest at the loop's end (the
   `use_count() > 2` check), so a host generator that throws after the break fails the render;
   Python renders, as it never fetches those items unless the kept loop is read.

**Proposal.** 1: keep `listSize` unknown for a mutable list (`GetMutableItems`) and read it on
the first `length` read; measure `Render/for_loop_vars` (which reads `loop.length`) with
`bench/count.py`. 2: when the inner loop is kept, collect the outer one's rest and keep it
with the inner. 3 and 4 only if a host generator in the wild needs them; otherwise mark 4
deliberate next to the lifetime rule of 0188 (a kept loop cannot run its enumerator later).

**Done when.** No line of `test/parity/divergences/` names 0390, or the remaining ones are marked
deliberate with Ruslan's word.

**Progress (parity batch 4, group C).**

- 1 fixed: a loop over a list the template can change (`GetMutableItems`) keeps the list in
  `LoopState::lengthOf` and takes its len() on the first read of `length`, `revindex` or the
  repr, kept from then on (`SetLengthSource`, `TakeLengthOf`). `revindex` is signed: it can
  be negative once the list shrinks, as in Python. A collected rest keeps a length already
  taken. New cases: `for_revindex_after_clear`, `for_length_read_then_clear`,
  `for_length_after_appends`, `for_kept_length_read_then_clear`.
- 2 fixed: an unfiltered loop over `loop` kept past its statement no longer collects the
  outer loop's rest; it keeps its enumerator and the statement's adapter
  (`KeepLoopState`, `LoopState::keepsEnumerator`), so the outer loop goes on through its
  rest and the kept loop reads from it later, as Jinja2's does. The outer loop is kept by
  the enumerator and collects its own rest at its end. New cases:
  `loop_iter_inner_kept_outer_goes_on`, `loop_iter_inner_kept_read_in_outer`,
  `loop_iter_inner_kept_iterated_after`.
- 3 moved to 0392, which pins the template-visible part (a list changed while a
  `select`/`map` result is iterated) with three corpus cases.
- 4 and `statements.for_kept_length_after_append` (a loop kept past its statement takes its
  length from the list at the statement's end; Jinja2 reads len() later) are proposed as
  deliberate: a kept loop cannot hold the statement's enumerator or list past the statement
  (0188). Ruslan chose deliberate on 2026-11-05 ("Deliberate, as 0188 and 0366"); the
  corpus line is marked deliberate in `test/parity/divergences/statements.txt`.

