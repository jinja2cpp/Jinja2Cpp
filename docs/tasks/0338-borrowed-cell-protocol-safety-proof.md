---
status: open
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/688
priority: medium
area: robustness
depends: [0337]
touches: [tla/CellProtocol.tla, tla/check.py, tla/README.md]
---
# From a bounded model of the borrowed-cell protocol to a safety argument

**Problem.** `tla/CellProtocol.tla` (#688) is bounded evidence about the cell protocol of 0049 P2
and 0331. It found F1 (0337), and taking out any #673 fix gives a counterexample. It is not a
proof that the C++ implementation is safe. The searches are bounded, several implementation paths
are left out (README, "Not modelled"), sequentially consistent interleavings stand in for the
atomics, and the correspondence to the code is a table rather than an argument. Ruslan's review
of #688 (2026-10-28) set out the smallest credible target: a model of every ownership and
synchronisation transition, plus an explicit correspondence to the C++. It does not need a model
of every template operation.

**Proof obligations.**
- Every dereference targets a live object.
- No two accesses conflict without synchronisation. This is stronger than `NoCrossThreadWrite`:
  it includes host reads and reference releases.
- The host's data is never mutated.
- A completed call's snapshot is stable under the stated contract (0332 and 0335 included or
  explicitly excluded).
- Alias identity and generation are correct.
- An exception at any allocation boundary preserves all of these.
- Cleanup is safe, and it terminates with a stated retention bound.

**Coverage, in the order of the review.**
1. **Ownership and escape paths.** Keep separate lifetimes for host data, adapters, cells,
   storage accessors, item blocks, iterators and exports (accessor and items are separate since
   9b66287). Distinguish owning references from borrowed pointers. Model every escape path:
   direct exports, `keep([l])`, nested containers, module namespaces, callback results, retained
   iterators, and values passed into another render. Model copying, moving and dropping a
   reference. Give 0332 and 0335 a contract position: either fixed, or excluded from the claim
   with the reason.
2. **Failure and reentry.** Cover nested renders and module calls, normal return, exception
   unwinding, render finish and module destruction. Inject failure at each allocation boundary
   that leaves a distinct state. Today the model fails only `Link`, the copy in `Unshare`, the
   globals conversion and the sort restore. Add export construction, `Adopt` and the registry
   insert.
3. **Concurrent reads and destruction.** Model host reads and releases concurrent with
   mutation, freeze and destruction, and the publication of ownership through the module lock.
   Justify each atomic step, or split it. For the atomics (`CellCounters`, the epoch), either
   model the relevant weak-memory behaviour or write a separate C++ memory-order argument.
4. **Collection and identifier reuse.** Cover self and mutual cycles, cycles through exported
   wrappers, the collector's traversal (`Traverse`, `NoteFrozen`), DropUnheld and destruction
   order. Show that nothing reachable from outside is collected and that cleanup terminates.
   Cover address reuse, and the wraparound of the epoch and of call ids. State any permitted
   leak precisely.
5. **Containers and caches.** Lists and dicts may share transitions only where their ownership
   and invalidation behaviour is the same. Add dict mutation during iteration, lazy and generic
   accessors (0336), `CellStep`/`QuietEpoch` and cached aliases, since these can bypass lookup or
   cleanup. For each implementation path left out, name the transition that represents it, or
   exclude it from the claim.

**Correspondence.** Map each relevant C++ operation to a model transition. Say which transitions
are atomic and why: a lock held, or a single-threaded owner. Argue that the abstraction includes
every implementation behaviour relevant to the obligations. ASan/TSan, allocation-failure sweeps
and the mutant runs support the correspondence but do not replace it.

**Beyond bounds.** A general claim needs either inductive invariants (checked with TLAPS, or with
TLC on the inductive form), or a justified cutoff argument: why two threads, two calls and these
counts are enough. It also needs an explicit account of the host behaviour that is supported.
The claim stays the protocol's correctness within that contract, not the whole engine's.

**Proposed approach (the model thread's answer to the review, for Ruslan to decide).** The
obligations above stand. The route differs in five places, because the protocol is still
moving: 0332, 0335 and 0336 are open, and each changes the transitions. A proof of today's
protocol would be redone after each of them.
1. **Settle the contract in the model first.** Design 0332, 0335 and 0336 in the spec before the
   C++ (model-first), then freeze the transition set. The inductive work starts only after that,
   so it is done once.
2. **Make correspondence mechanical: trace validation.** A debug or test build of the C++ logs
   the protocol's events (Find, Link, Adopt, NoteExport, Unshare, BeginCall/EndCall, Finish,
   DropUnheld, a free). TLC checks each logged trace against the spec, so a code path the spec
   lacks fails the check. This is the method of the TLA+ trace-validation work, and it is
   cheaper and more trustworthy than a correspondence written by hand. The existing tests, the
   parity corpus and the fuzz corpus then feed it traces.
3. **Unbounded depth via inductive invariants checked by tools, then TLAPS only if needed.**
   Write the invariants in inductive form and check `IndInv /\ Next => IndInv'`. Apalache does
   this symbolically for fixed sizes; TLC can do it from IndInv as the initial predicate in small
   scopes. That removes the depth bound, the weaker half of "bounded". A cutoff argument
   (why 2 threads, 2 calls and 2 identities suffice) covers the sizes, and is tractable here
   because the protocol is symmetric in threads and identities.
4. **Failure-atomic transitions instead of a failure point per allocation.** Restructure each
   protocol step as allocate first, then commit with `noexcept` operations. The model then needs
   only "fails before commit" per step, which is one disjunct each. A C++ sweep that fails the
   Nth allocation, under ASan, checks that the code is shaped that way. Modelling every
   allocation separately grows the state space and still trusts the boundaries were all found.
5. **Memory order by design and argument, not in TLA+.** TLA+ is a poor tool for weak memory.
   As far as the model shows, the protocol shares only four kinds of state across threads:
   - the module's state, under its mutex;
   - immutable host data and the globals snapshot;
   - frozen cells and kept values, published by a call's return or a render's end;
   - the counters (`CellCounters`: epoch and live).

   The first step is to confirm that list against the code. Keep it that way. Write the memory-order argument
   for the counters as a short document, check it with TSan and, if they ever carry more than a
   counter, with a C11 model checker (GenMC or CDSChecker) on an extracted kernel.

The cycle collector is a separate subsystem with its own invariants. It gets its own spec with
an interface contract to this one, rather than growing this spec: two small models are checked
far more deeply than one large one. The order then becomes: contract (1), trace validation
(2), ownership and escape paths, failure atomicity (4), concurrent reads and destruction, the
collector spec, the inductive invariants (3), and memory order (5) alongside.

**Until then.** The README keeps its "Not modelled" list and the bounds of each pass run visible,
and it does not describe the model as a proof.

**Done when.**
- Each coverage item above is either modelled, with a run in `tla/check.py` and a mutant that
  shows the run sees the defect, or listed as excluded from the claim with the reason.
- `tla/README.md` maps each modelled transition to its C++ and states its atomicity.
- The memory-order argument exists, as a model or as a document.
- The bounded results come with either inductive invariants or a cutoff argument, or the README
  states that the result is bounded evidence only.

**Step B, trace validation (2026-10-28).** `-DJINJA2CPP_CELL_TRACE=ON` logs the protocol's
events, `tla/check.py --trace` checks each test's trace against `tla/CellTrace.tla`, which
applies the rules in `tla/CellRules.tla`; one Debug CI row runs it with its mutants. It sees F1
(0337) from the C++ traces before #689. See tla/README.md, "Trace validation".
