---
status: open
pr:
priority: low
area: robustness
depends: [0363]
touches: [tla/CellInductive.tla, tla/CellApalache.tla, tla/apalache.py, tla/CellProtocol.tla, tla/README.md]
---
# Find the cell protocol's inductive invariant

**Problem.** Task 0363 closed the mechanized proof of `tla/CellProtocol.tla` with the exhaustive
runs and the cutoff argument as the answer (tla/README.md, "The inductive proof: why the
exhaustive runs are the answer"). For fixed constants that is the same result; what a proof
would add is speed for larger bounds (Apalache) or every size (TLAPS), against a cost 0363
measured: on the module scope 6/2/2/1 one query per action and conjunct of `IndInv`
(`apalache.py sweep`) takes 20 s to 6 minutes, reads through an adapter run past 10 minutes,
and a full sweep is about 260 queries, 4 to 8 hours on 4 cores, per strengthening round.

**Proposal**, in order:
1. Lemmas from the counterexamples to induction, one conjunct each so that `sweep` checks them
   apart. The first, from `Read => NoForeignCell'`: an attached adapter's cell is of its data
   (`idn`, and `inc` unless frozen or J) and in a registry a frame of its thread may use; a
   registry of a thread outside `RenderThreads` is empty.
2. Flatten the indirect reads: a derived variable per adapter (its cell's storage, its cell's
   registry), or functions keyed by the pair, so that `cl[ad[a].cell].stor` is no if-then-else
   over the domain. Keep TLC's runs and the mutants passing and failing as before.
3. `GC` and `DropUnheld`: a heap lemma ("in a quiet state the alive objects are exactly the
   reached ones") checked once, in a heap-only module, and assumed by the protocol steps.
4. `snap` as a pair with a flag; drop `wr` from one-thread scopes.
5. For every size: TLAPS for the thread reduction of "Sizes: the cutoff argument", the one part
   of the cutoff that stays on paper after 0363.

**Done when** `apalache.py sweep` reports every step holding on the `module` scope at 6/2/2/1
and `apalache.py safety` holds (IndInv implies Safety), or a TLAPS proof of the thread reduction
replaces its paper argument.
