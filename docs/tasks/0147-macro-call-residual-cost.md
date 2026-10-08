---
status: open
priority: low
area: perf
depends: [0117]
touches: [src/statements.cpp, src/render_workspace.cpp, include/jinja2cpp/user_callable.h]
---
# What a macro call still costs after its arguments moved to slots

**Problem.** After 0117 P2 (#434) Render/macros fell 13.5%, short of the plan's
-20..-25%. A profile of `MacroStatement::InvokeMacroRenderer` on the `macros` case
(callgrind, `--dump-instr=yes`, self cost by source line) puts the rest of a call, besides
rendering the body, at:
- about 230 instructions copying each argument out of the `const CallParams&` into its
  slot (variant copies and shared_ptr reference counts); the call site owns the
  evaluated arguments and could move them, but the callable signature takes them by
  const reference;
- about 100 taking and giving back the frame (`RenderWorkspace::Take`/`Release`);
- about 140 checking the arguments (`CheckMacroCallArgs`, the `any_of` over defaults, the
  keyword pre-lookup), most of which depends only on the macro and could be computed at
  Load.

**Proposal.** Measure each separately: pass evaluated arguments by rvalue to macro
callables (an internal overload; user callables keep theirs), precompute the per-macro
flags at Load, and a frame fast path for a unit whose frame is the same size as the
previous call's. Filed at the perf track's request (2026-10-08).

**API note for Ruslan.** The rvalue path is meant to stay internal: macros and other
callables the engine makes get an extra overload that takes the evaluated arguments by
value or rvalue. The public `user_callable.h` signatures are unchanged. It is listed in
`touches` only because the internal overload may sit next to the public one. If it can
live entirely in src/, the public header drops out of `touches`.
