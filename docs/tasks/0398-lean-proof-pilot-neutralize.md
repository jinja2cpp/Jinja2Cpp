---
status: open
priority: low
area: robustness
depends: [0358]
touches: [proof/, docs/lean-pilot.md, test/parity/cases/prompt.py]
---
# Lean 4 proof pilot: the `neutralize` edge rule

Filed at Ruslan's request (2026-11-05), after the discussion of where machine-checked proofs
pay off next to the TLA+ work. **Scheduled after the G1 API/ABI freeze**; `neutralize` is
frozen by then, so a proof of it does not chase a moving spec.

**Problem.** Two tools pull in opposite directions. TLC and Apalache (0338, 0351) are cheap
to start and expensive to run: cost grows with the state space. A Lean proof costs nothing to
run and a lot to write, and breaks when the code it describes changes. The cell and borrowed
memory protocol is concurrency plus C++ memory semantics, so it stays on TLA+ (Ruslan,
2026-10-30: the model is a design tool, 0364 validates a frozen commit). Pure, deterministic
functions with a crisp specification and lasting value are where Lean pays off, and we have no
measurement of what one proof of that kind costs. The security claim of 0358 is the best first
target: today it rests on a proof sketch in `0358/plan.md` section 2 and on a fuzz target.

**Proposal.** One pilot, with a cost cap, whose product is the measurement.

1. **Target.** The guarantee of `neutralize` (0358, section 2 of its plan; PR #766): for the
   rules *Patterns*, *Inside*, *Edges* and *Validity*, any occurrence of a special token in the
   output either lies inside trusted text (template text, prompt-safe values) or begins and ends
   in trusted text. In other words, no special token appears in the output unless trusted
   template text wrote it. Model: strings as lists of code points (the narrow/wide split is an
   encoding detail and out of scope; the model treats a code point as the unit), a finite
   set of specials and tags, an output built as a concatenation of trusted and escaped pieces.
   Prove (a) the rules produce a string with no special inside an escaped piece, (b) the edge
   rule closes the occurrences that span a piece boundary, (c) idempotence, and (d) validity
   (separator non-empty and in no pattern) is exactly the side condition the proof needs.
2. **Executable spec.** The Lean definition of `Neutralize` is a program. Compile it to a
   small CLI (`proof/`) that reads patterns and inputs and prints outputs. A script generates
   cases from it (the edge shapes: values starting with a proper suffix of a special, ending
   with a proper prefix, overlaps, case variants, split specials across `{{ a }}{{ b }}`) and
   writes them as rows under `test/parity/cases/` for the prompt group, expectations checked in
   like the Python-oracle ones. The C++ is diffed against the Lean output in the corpus, the
   way trace validation links the TLA+ model (0338 b) to the code. **The gap to keep in mind:
   Lean proves the model, not our C++.** The generated cases are the only link, and the
   document says so.
3. **Cost cap.** Stop and write up at whichever comes first: 3 working days of agent or
   engineer time, or about USD 100 of model spend. Record in `docs/lean-pilot.md`: hours and
   spend per theorem, lines of Lean per line of C++ specified, the size of the toolchain in CI
   (Lean via `elan`, Mathlib or core only), how the proof reacted to one deliberate spec change
   (for example, a second separator or a new boundary character), and whether the generated
   cases found anything the fuzz target and the table did not.
4. **CI.** The proof builds in a separate weekly and on-demand job, like TLC (0350), never in
   the per-PR path. Pin the Lean toolchain and dependencies by version and hash (AGENTS.md
   rules on pins and `sbom/components.json` apply if a library is fetched).

**Candidate follow-ons, decided on the pilot's numbers and not part of this task:**
- float formatting and `repr` (0305, 0309, 0039): a deterministic function of a double, with
  Python as the oracle and shortest round-trip as the property;
- the lexer and tokenizer: a total function from text to tokens, where the property is that
  every input yields tokens or an error;
- dict key identity and equality (0212): `a == b` implies the same key, and the hash
  contract;
- the prompt matcher's trie against a naive scan (equivalence), which the fuzz target already
  approximates.

**Not in scope: the cell protocol.** Interleavings of the borrowed-cell protocol (0338,
0351) stay on TLA+ with trace validation and the deep validation of a frozen version (0364).
Proving them in Lean means modelling interleavings by hand, weeks per invariant. If the
inductive proof of 0363 is ever attempted, the choice between TLAPS and Lean can be taken
there, informed by this pilot's cost numbers; this task does not decide it.

**Done when.** `proof/` holds the Lean development; `lake build` succeeds and the theorems of
step 1 have no `sorry` (or `docs/lean-pilot.md` lists each unproved one and why); the
generated cases run in the parity corpus against the C++ with no allowance; and
`docs/lean-pilot.md` has the cost record of step 3 and a recommendation: continue to a
follow-on, narrow to the executable oracle only, or stop.

**Next.** A decision on the follow-ons above, and an input to the 0363 choice of prover.
