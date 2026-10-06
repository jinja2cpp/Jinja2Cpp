---
status: done
priority: high
area: perf
touches: [src/statements.cpp#RenderBody, src/render_context.h, src/template_impl.h, src/template_env.cpp]
---
# A tiny render is 98% setup and teardown

**Problem.** PR #414 puts Tera 2 about 2x ahead of us on tiny renders (`plain_text` 136 vs
258 ns, `substitute` 0.45x). Callgrind on master faea865: of 2,130 instructions in a
`plain_text` render, the template itself is 33. Building the `self` map at every template
entry, plus its teardown, is about 44%; converting the environment globals on every render
is about 15%; converting the render parameters is about 1,000 of `substitute`'s 4,025
instructions. `RenderContext` construction is not a lever (39 instructions).

**Proposal.** (docs/design/perf-design-overview.md section 1a)
- a) Build `self` only when a lookup asks for it. Check Python Jinja2's precedence for
  `self` (a context variable named `self`, `self` inside blocks and macros) and add parity
  rows first.
- b) Convert the environment globals once per change (a generation counter on the env)
  and keep the converted snapshot per thread, until 0117's `RenderWorkspace` owns it.
- c) Convert render parameters on first use. Deferred: 0117 P4 absorbs it if scheduled.

**Done when.** a) and b) merged with parity rows; `plain_text` ~215 → 110-140 ns and
`substitute` ~-30% expected (medium confidence), `inheritance` -10..-15%.

**Result (PR #421).** a) and b) done, c) left to 0117 P4. Instructions per render
(`count.py`, against master faea865): `plain_text` 2,333 → 995 (-57%), `substitute`
4,111 → 2,675 (-35%), `inheritance` 282,970 → 208,437 (-26%, 77 fewer allocations),
`config_file` -5%, `large_static` -12%. Wall clock, single thread: `plain_text` 214 → ~80 ns.
- a) `self` is a `SelfRefExpression`: names the template sets in its own scopes win, else
  the frame's `self`, made on first use. Imported macros run with their module's frame, so
  their `self` is the defining template; `self` passed to an imported macro still renders
  the blocks of the template it came from. 27 `self_*` corpus rows; leftovers in 0144.
- b) The env keeps its globals as a shared map, replaced (not changed) while a render holds
  it, with a process-wide generation number. Each thread keeps the last converted snapshot;
  a render takes no lock when the generation matches. A template changing a global in place
  drops the thread's snapshot, so the change stays in that render. This also fixed a race:
  a global changed during a render left it reading freed strings.
- Found on the way and fixed: a macro of a module imported with `import ... as` crashed
  when it called another macro of the module (the import moved the names out of it).
- Left: the snapshot keeps the globals (and what user callables capture) alive until the
  thread's next render of another state or the thread's exit, not until the env dies.
  Alternating two environments on one thread converts at every render (one slot).
