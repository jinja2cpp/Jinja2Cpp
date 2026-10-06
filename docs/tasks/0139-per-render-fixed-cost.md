---
status: open
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
