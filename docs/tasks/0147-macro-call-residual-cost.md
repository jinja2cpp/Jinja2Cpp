---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/448
priority: low
area: perf
depends: [0117]
touches: [src/statements.cpp, src/statements.h, src/render_workspace.cpp, src/render_workspace.h]
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

**Result** (count.py against master 2486c22, Release, 200 macro calls per render):
Render/macros 1,019,941 -> 911,229 (-10.66%, about 540 instructions per call),
config_file -1.41%, html_autoescape -1.03%, every other case within +-0.1%. Measured
cumulatively in three steps: -2.6% (the any_of flag, one keyword pass), -7.5% (forwarding
values), -10.7% (the rest).
- Load-time flags: whether any default names another argument (the `any_of` per call),
  and, when the callable is made, whether each stored default is mutable (`IsMutable` per
  call).
- One pass over the keywords finds each one's argument and counts the extra ones; the
  check is then two comparisons, and the throwing path keeps the old messages
  (`ThrowExtraMacroArgs`). Binding caller/kwargs/varargs is skipped for a macro that
  catches none.
- Binding forwards each value: an argument or a stored default is copy-assigned into its
  slot once, instead of copied into a by-value parameter and then moved, the largest item.
- `RenderWorkspace::Take`/`Release` are inline while the current chunk has room.
  Caching the thread's workspace in `UnitCall` was tried and dropped: it reads the
  thread_local on calls of units without slots too (Render/plain_text +0.6%).
- Not done: passing the evaluated arguments by rvalue. A prototype that moved them out of
  `CallParams` saved only a further 1.25% on Render/macros (about 64 instructions per call),
  and it needs `Callable::StatementCallable` (internal_value.h, reworked by 0140) and the call
  sites in expression_evaluator.cpp (wave 2). After 0140 a copy of a string value is a
  reference count, so the gain shrinks further; the public user_callable.h was never needed.
  Corpus cases added for the argument checks: `statements.macro_kwargs_too_many_args`,
  `macro_varargs_unknown_kwarg`, `macro_kwarg_and_kwargs`, `macro_kwarg_repeats_positional_kwargs`.
  One behaviour change, toward Jinja2: a default that names another argument (`b=a`) is
  bound as evaluated, no longer copied when it is a list or dict, so `b` is the same list
  as `a`, as in Python (`statements.macro_default_refers_arg_mutable`). Stored defaults
  are still copied per call.
