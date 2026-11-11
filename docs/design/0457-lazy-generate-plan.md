# 0457 plan: `Template.generate()` renders on demand, on the caller's thread

Architect plan, 2026-11-11, on master c9fe6ed; decisions taken by Ruslan the same day (below).
The probes behind it ran in a session scratchpad and are not kept; their results are quoted.

## Decisions (Ruslan, 2026-11-11)

- **D1:** option (i), `RenderOptions::suspension` (one pointer) lands before the G1 freeze, like
  `writeEachPiece` and `generationSpans`. So 0457 is not Python-only: PR 1 adds a public field.
- **D2:** option (i). `next()` from a thread other than the one that started the render raises
  `RuntimeError` with a message naming `render()` and the async route; documented as a divergence.
  A lazy async iterator, which Starlette and FastAPI iterate on the event-loop thread, is task 0461.
- **D3:** option (ii), revised from the plan's recommendation: pin boostorg/context's `fcontext`
  assembly (`make_fcontext`, `jump_fcontext`, `ontop_fcontext`; BSL-1.0, SBOM and notice entries, no
  Boost C++ interface) behind a small C++17 class in the module; Windows uses the OS fiber API. Reason:
  that assembly has run on every major ABI for a decade (AArch64 callee-saved FP registers, Apple and
  Linux symbol names, Windows TIB fields), so the module reviews a known switch instead of writing one.
- **D4:** accept and document that a reference cycle through a suspended generator leaks until it is
  closed; the fix is task 0462.
- **D5:** add a Python 3.14 row to the Python CI module job (the stack-protection path).
- Not taken: C++20 coroutines (stackless: every frame between the render entry and the sink write would
  become a coroutine, a frame allocation per statement call on every render, and the C++17 floor).

## 1. The tension

Jinja2's `generate()` is a Python generator: the template pauses at each item, the consumer runs
arbitrary Python (including other renders and other generators), and the template continues later on
the thread that calls `next()`, with its callables on that thread. Closing or collecting the generator
ends the template there.

The engine is a recursive C++ renderer whose invariants contradict pausing mid-render:

1. **Renders on a thread nest LIFO.** Per-render state lives in thread-locals that each render saves and
   restores in stack order (`RenderWorkspace` frames and lent maps, `t_renderLimits`, `t_hostPoll`,
   `RenderDepthGuard`'s depth, `CycleCollector::s_current`, and in the module `t_render`, `t_lease`,
   `RecursionGuard::t_depth`). A paused render that another render overtakes breaks that order.
2. **A render runs on its thread's stack.** `StackNearlyExhaustedSlow` reads the thread's bounds once
   and skips the check for a frame outside them (src/recursion_guard.cpp:131, "runs on one of its own").
3. **A plain render pays nothing for features it does not use.** The release plan says 0457 is
   Python-only (docs/release-2.0.md G1 row), and the G1 freeze stops `RenderOptions` from growing.

Lazy, same-thread, interleavable, closeable generators (the Python requirement) cannot be had without
changing invariants 1 and 2. Doing it without adding anything to the plain render path or breaking
the freeze is the engineering constraint.

### Evidence (probes, this session)

| Probe | Result |
|---|---|
| Two `RenderTo(writeEachPiece)` renders, each on a ucontext fiber that suspends in `Write`, interleaved A-start, B-start, A-finish (`interleave.cpp`, Debug `build/libjinja2cpp.a`) | `RenderWorkspace::Release: Assertion m_frames.back().frame.handle == frame.handle failed`: `TemplateRenderer::RenderBody` and `BlockStatement::RenderBody` hold a `UnitCall` slot frame across their body's writes (statements.cpp:2675, 2793). In Release the wrong record is popped and later `Take`s hand out slots still in use (P0). |
| A template recursing through a macro, rendered on a fiber (`deep.cpp`) | thread stack: `Maximum recursion depth exceeded`; 1 MiB fiber: the same (depth count 256); **512 KiB and 256 KiB fibers: SIGSEGV**, because the stack check is skipped off the thread's stack (P1). |
| Embedded CPython calling Python on an mmap'd ucontext stack (`fiberpy.cpp`): plain calls, 50000-deep Python recursion, `repr` of a 200000-deep list | 3.12.3 / 3.13: calls fine; the deep `repr` gives RecursionError on an 8 MiB fiber but **SIGSEGV on 256 KiB and 1 MiB** (counter limits are sized for an 8 MiB stack; 3.13 needs 2-4 MiB). **3.14.0: every call aborts** (`Fatal Python error: _Py_CheckRecursiveCall: Unrecoverable stack overflow`). 3.14.1-3.14.6 and 3.15.0b4: calls fine, but the stack check is switched off outside the thread's bounds ("assume that we have the wrong stack limits, and do nothing"), so the deep `repr` gives **SIGSEGV even on 8 MiB**. |
| The same with `PyUnstable_ThreadState_SetStackProtection(tstate, low, size)` resolved by `dlsym` (as an abi3 module would), Reset after (`fiberpy2.cpp`, 3.14.6) | RecursionError `Stack overflow (used 96 kB)` on a 128 KiB fiber; the main stack's protection is back after Reset. The function exists in 3.14.1+ and 3.15, not 3.14.0. |
| gevent shape: a greenlet started on the thread's stack enters a fiber, Python on the fiber switches to the hub greenlet (`glprobe2.cpp`, greenlet 3.5.6) | **Process abort**: `g_switchstack: greenlet: Failed low-level slp_switch(). The stack is probably corrupt` (greenlet assumes one contiguous stack per thread). Greenlets created and switched entirely on the fiber work (`glprobe.cpp`). |
| greenlet copying a stack that holds ASan-instrumented C++ frames (`asanext.cpp`) | `detect_stack_use_after_return=0`: ASan `stack-buffer-underflow in memcpy`; `=1` (the CI setting): passes. |
| Switch cost, x86-64 (`switchcost.cpp`) | glibc `swapcontext` round trip 421 ns (two `rt_sigprocmask` syscalls); a 20-line assembly switch 26 ns; mmap 8 MiB + guard page + touch + munmap 5.1 us. |
| Jinja2 3.1.6 oracle | `generate()` runs nothing before the first `next()`; `next()` from another thread works; `close()` stops further calls; `next()` of the running generator from its own callable raises `ValueError: generator already executing`; a cycle through a suspended generator is collected. |

### Answers to the questions in the brief

1. **CPython thread state across switches.** At every switch point no Python frame pushed by the render
   is live, so `PyThreadState` needs no swapping. The switch happens only in the module's sink `Write`,
   which the engine calls only from `OutStream::FlushBuffer`/`WriteLong` on the render's root stream
   (out_stream.cpp:54-117). It calls `Write` after a body node (renderer.h:111-118), never in a `catch`
   handler, and never while a host call runs: host calls return values, macro and `caller()` output goes
   to a string stream of its own, a value is stringified before it is written (`WriteRenderedToSink`), and
   on error the buffer is dropped, not flushed. A Python iterator in `{% for %}` is a generator object
   whose frame is on the heap and is not running between `PyIter_Next` calls (adapters.cpp:150).
   Per-thread CPython counters (`py_recursion_remaining`, 3.12/3.13 `c_recursion_remaining`, `cframe`,
   `datastack`, 3.14 `c_stack_refs`, `exc_info`) are balanced at the switch. `tstate->context` is the
   resumer's, as for a Python generator. Guard in code, not only by argument: `Write` switches only when
   `PyEval_GetFrame()` is the frame recorded at resume, `!PyErr_Occurred()`, `!std::current_exception()`
   and `std::uncaught_exceptions() == 0`. Otherwise it queues the item and keeps rendering, which is
   correct but not lazy.
2. **CPython C-stack checks.** 3.12/3.13 count; the fiber must be as large as a thread stack (8 MiB
   reserve) or a deep C recursion in a callable crashes. 3.14.0 aborts on any call from a foreign stack.
   3.14.1+ and 3.15 need `PyUnstable_ThreadState_SetStackProtection` at each resume and
   `ResetStackProtection` (or a re-set to the outer fiber) at each suspend. Without it they run unchecked.
   That function is not in the limited API; the module is abi3 (`STABLE_ABI`, `wheel.py-api = "cp312"`),
   so it is looked up at run time (`dlsym(RTLD_DEFAULT)`; on Windows `GetProcAddress` on the DLL that
   holds `Py_Version`). Free-threaded builds are out of scope while the module is abi3 (0356 plans cp314t).
3. **Engine stack checks.** The render must install the fiber's bounds in `t_threadStack` while it runs
   and put the thread's back at each switch. That is engine-internal, so a public field supplies the bounds
   (section 3). The fiber gets an 8 MiB reserve with a PROT_NONE guard page below it, committed lazily;
   the engine keeps its 128 KiB (ASan 512 KiB) reserve inside it. `MaxSequenceSize` is untouched. A
   template cannot create generators; host code nesting them is capped per thread (section 3).
4. **Sanitizers.** ASan `__sanitizer_start_switch_fiber`/`__sanitizer_finish_switch_fiber` around each
   switch (null fake-stack pointer on the last switch out of a finished fiber); TSan
   `__tsan_create_fiber`/`__tsan_switch_to_fiber`/`__tsan_destroy_fiber`. CI-python runs the module under
   ASan+UBSan (3.12, `detect_stack_use_after_return=1`); linux-build has ASan and TSan jobs for the core
   tests, which run the engine facility's tests through the same switch primitive.
5. **Exceptions, Ctrl-C, close, exit, re-entrancy.** The fiber entry is `noexcept`, catches everything
   into an `exception_ptr` (with `RenderState::abort`), switches back and never returns; the resumer
   rethrows. Ctrl-C: `RenderOptions::poll` runs on the fiber on the caller's OS thread, so
   `PyErr_CheckSignals` works; KeyboardInterrupt ends the render and surfaces from `next()`, after the
   items already given. Close: set `cancelled`, resume; the suspended `Write` throws a module-private
   `RenderCancelled`. The engine ends the render with it as a sink exception (RAII runs on the fiber), the
   entry drops it, and the resumer frees the stack. Collection: tp_dealloc closes the same way on the
   owner thread. On another thread, or after the module's `atexit` hook ran, the fiber is leaked without
   running anything. That is safe: a suspended fiber executes nothing, so the daemon-thread abort of the
   worker design cannot recur. A daemon *consumer* blocked inside a callable at exit behaves exactly as
   `render()` does today. Re-entrancy: `next()` on a running render raises `ValueError` (Python's own
   generator wrapper already does); `generate()` inside a callable of another lazy render nests a fiber
   (cap 8 per thread, then eager); two generators interleave freely (each has its own state bundle).
   Resuming from another OS thread raises `RuntimeError` (D2).
6. **Thread-locals.** Full list in section 3.1, each classified: swapped per suspension, safe to share,
   or asserted idle at `Write`.
7. **Cost.** A plain render pays nothing: the new code sits behind `hasHooks` in template_impl.h and in a
   new cold translation unit, and `OutStream` is untouched (the swap is a sink adapter). Per item: two
   switches (about 26 ns for both with the assembly switch), two swaps of about 300 bytes of
   thread-locals, and on 3.14+ two stack-protection calls. That is small next to the Python `str` and the
   generator step paid today. Per `generate()`: one pooled 8 MiB stack (5 us when the pool is empty) and
   one heap state bundle with a spare `RenderWorkspace`, also pooled per thread.
8. **Platforms.** Windows: Win32 fibers (`ConvertThreadToFiberEx` only if `!IsThreadAFiber()`, undone
   after the switch back; MSVC's CRT keeps exception state per fiber). Linux and macOS, x86-64 and
   AArch64: a minimal assembly switch. Other POSIX: ucontext (421 ns round trip). Everywhere else, and
   on 3.14.0, unverified future CPython versions, a failed mmap, an active Linux CET shadow stack
   (`arch_prctl(ARCH_SHSTK_STATUS)`), or `greenlet` in `sys.modules`: today's eager path. The module is
   built in CI only on Linux (0356 adds wheels), so the switch primitive also builds into
   `jinja2cpp_tests`, which runs on macOS-15 (arm64), Windows MSVC and the Linux sanitizer jobs.

## 2. Options

### (a) Fibers in the module, plus a small engine facility for suspendable renders (recommended)

The module runs `RenderTo` on a stack of its own and switches back in its sink's `Write`. The engine gets
one public, additive input, a render whose sink may suspend it on a given stack. With it, the engine keeps
the render's per-thread state in a bundle that is swapped in for the render and out for each `Write`, and
checks the stack against the given bounds.

Cost: about 150-250 lines in the core (one new cold TU, a `RenderWorkspace::SwapContents`, friend access
to three private thread-locals, wiring under `hasHooks`) and one public field or overload (D1). About
500 lines in the module (switch primitive, lazy render object, CPython stack protection, Python
wrapper). Platform assembly for two ISAs. Gevent users stay eager (abort otherwise, measured). Resuming
on another thread is refused (D2). Cycles through a suspended generator are not collected (D4).

### (b) An engine-owned resumable render

Either stackful in the core (a public `ResumableRender` class owning the fiber, with hooks so the module
can set CPython's stack protection), or stackless (each statement renderer becomes a resumable state
machine, or C++20 coroutines, which the C++17 floor rules out). The stackful variant is (a) with the
fiber moved into the core: C++ hosts get lazy rendering too, but the public API grows by a class and
hook interface after G1, and the core gains platform assembly and stack management. The stackless
variant rewrites statements.cpp (about 4000 lines), macros, includes, extends/blocks, call blocks,
recursive loops and filters with bodies. That means months of work, a second render path to keep in
step with the first, and a likely regression past the instruction-count gate on the hot path. Not
recommended now; the stackful variant can be layered on (a) later if C++ users ask (section 6).

### (c) greenlet as the switcher, plus the same engine facility

Render inside a greenlet whose sink `Write` calls `parent.switch(item)`. greenlet copies stack slices
so everything stays on the thread's stack. CPython's checks (all versions, 3.14.0 included) and the
engine's stack check need nothing. Gevent, eventlet and Playwright's sync API work and are lazy.

Cost: a runtime dependency (optional, so eager without it); a per-switch copy that grows with the render's
stack depth (about 1 us); greenlet's memcpy of instrumented frames trips ASan unless
`detect_stack_use_after_return=1` (measured), so the ASan job's cover is fragile. While a render is
suspended its stack memory belongs to other code: any pointer into it that escapes the thread-local swap
is silent memory corruption, where a separate fiber stack leaves stale but valid memory. The engine
facility is still needed (the LIFO break is independent of the switcher). Cross-thread resume is refused
the same way.

(A worker thread that marshals every Python touch back to the consumer was considered. It would keep
cross-thread resume, but conversions, attribute reads, iteration and every `Py_DECREF` would each cost a
cross-thread round trip of microseconds, and the module would need a rewrite. Rejected.)

## 3. Recommendation: (a)

The engine facility is needed by every option, so it lands first and on its own. The switcher is a
module-internal choice behind it: if gevent users ask for laziness, a greenlet backend (c) can be added
for processes that import greenlet without touching the engine again.

### 3.1 Engine facility (core)

Public (include/jinja2cpp/template.h, C++17, additive; D1 picks field or overload):

```cpp
// The stack a render to a sink runs on when its host switched it there, and whose sink may switch
// the thread away inside Write and back before Write returns (docs/tasks/0457)
struct RenderSuspension
{
    const void* stackLow = nullptr; // lowest usable address, above any guard page
    std::size_t stackSize = 0;      // 0: the render runs on its thread's own stack
};
// RenderOptions:
const RenderSuspension* suspension = nullptr;
```

Contract: only for `RenderTo` with a `BasicOutputSink`. Every `Write` returns on the thread the render
started on. While a `Write` runs, the thread behaves as if no render were running, so other renders, plain
or suspended, may run, interleave, start and finish. Null, the default, costs nothing.

Internal (new src/render_suspension.{h,cpp}, cold):
- `RenderThreadState`: the bundle swapped with the thread on entry, before and after each `Write`, and on exit:
  - `RenderWorkspace` contents: a member swap of the thread's object with a spare, plus `s_renders`.
    References the render took to the `thread_local` object stay valid; frames, maps and chunks are on
    the heap and do not move. The spare is pooled per thread.
  - `t_renderLimits` (its `output` points into the render's `OutStream`), `t_threadStack` (the fiber's
    bounds, through the same reserve and TSan budget logic as `StackNearlyExhaustedSlow`, factored out),
    `t_hostPoll`, `RenderDepthGuard`'s depth (a friend accessor; codegen unchanged),
    `CycleCollector::s_current`, `t_parseLimits` (cheap; idle at `Write`).
  - Asserted idle at `Write` (debug): `t_pendingDestroy` depth 0 and empty, `t_failure` null,
    `t_lendFrame` null, `g_bridgingAccessor` null, `t_keepUndefinedExport` false.
  - Shared safely: `LookupCache` (epochs never repeat and are per cache, so other renders can only evict;
    render_context.h:183-198); the HTML escape buffer (a sink stream never keeps a fragment in it,
    `OutStream::ReachesSink`); the `JsonNodeOf` type cache; the template_slots hash seed; cell_trace.
- `DetachingSink<CharT>`: an `OutputSink` adapter that swaps around the host sink's `Write`. A throwing
  `Write` swaps back during unwinding.
- template_impl.h: under `if constexpr (hasHooks)`, a scope outside the `try` swaps in a fresh bundle
  (depth 0, no collector, fiber bounds) before `CycleCollector`, poll and limits, and swaps back after the
  error conversion. The target sink is wrapped in `DetachingSink`. Plain and string renders do not
  instantiate it.
- Review rule (REVIEW.md, one line): a new per-render `thread_local` in src/ joins the bundle or carries a
  comment saying why it can be shared.

### 3.2 Module (python/src, python/jinja2cpp)

- `python/src/fiber/`: a small static library with no Python, linked by the module and by `jinja2cpp_tests`.
  `Fiber` owns the stack (pooled mmap with a guard page on POSIX, `CreateFiberEx(64 KiB, 8 MiB)` on
  Windows) and exposes `Resume()`/`Suspend()`. It holds the hand-written x86-64 SysV and AArch64 AAPCS64
  switch for ELF and Mach-O (D3), the ucontext fallback, the ASan/TSan annotations, and an `Available()`
  probe for CET and arch.
- `PyLazyRender` (environment.cpp), a nanobind object:
  - It holds the template, the `dict(*args, **kwargs)`, the state (NotStarted/Running/Suspended/Done), the
    owner thread id, the pending item, the `exception_ptr` and `cancelled`.
  - The fiber entry calls the existing `RenderWith` with a `SuspendingSink` and `options.suspension` set.
  - `_next()`: on NotStarted it records the owner thread; on any other thread it raises `RuntimeError`
    (D2). It saves and swaps the module's `t_render`, `t_lease` and `RecursionGuard::t_depth`, sets
    CPython's stack protection on 3.14+, switches in, and on return gives the item, `None` at the end, or
    rethrows.
  - `close()` cancels as in section 1, answer 5.
  - Nesting: a per-thread chain of running fibers, so a suspend re-sets the outer fiber's protection
    instead of resetting to the thread's.
- Availability (`_lazy_generate_available()`): fiber available; Python 3.12, 3.13, 3.14.1+ or 3.15
  (allow-list through `Py_Version`; 3.14.0 and unlisted versions are eager); the stack-protection
  symbols found on 3.14+; `greenlet` not in `sys.modules` at the first `next()`; lazy nesting below 8 on
  the thread.
- `environment.py`: `generate()` stays a generator function. It makes the lazy render, then
  `while (item := r._next()) is not None: yield item`, and calls `r.close()` in `finally`. When lazy is
  unavailable it runs today's eager body. `TemplateStream` follows.

## 4. Decisions for Ruslan

- **D1, public surface and G1.** The facility is public, so 0457 is not Python-only as release-2.0.md
  says. Either (i) `RenderOptions::suspension` lands before the G1 freeze (one pointer, the precedent of
  `generationSpans` and `writeEachPiece`; recommended while the freeze still waits on his word), or
  (ii) an additive `RenderTo(sink, params, options, const RenderSuspension&)` overload that fits after
  G1. A record goes in docs/api-2.0.md either way.
- **D2, resuming on another thread.** Jinja2 allows it. Here it is refused with `RuntimeError`, because
  engine frames hold references to the owner's thread-locals and compilers cache TLS addresses across
  calls. Starlette's `StreamingResponse` over a sync iterator (`iterate_in_threadpool`, one worker-thread
  call per item) would then fail intermittently under load, where today's eager generator works.
  Options: (i) refuse and document; Starlette users call `render()` or wrap `list(generate())`
  (recommended). (ii) Also add an opt-out (an `Environment` option or a module switch) that keeps the
  eager path; it is not Jinja2 API.
- **D3, the POSIX switch.** (i) Hand-written assembly, about 25 lines for x86-64 and 35 for AArch64, as
  probed (26 ns); recommended, no new dependency. (ii) Fetch and pin boostorg/context's fcontext
  assembly (BSL-1.0; sbom and notices entries). (iii) ucontext only (421 ns per round trip, deprecated on
  macOS, needs `_XOPEN_SOURCE` and a deprecation pragma under `-Werror`).
- **D4, cycles through a suspended generator.** Jinja2 collects them; here they leak until closed
  (8 MiB reserved, KiBs resident, plus the objects). Either accept and document now with a follow-up
  task (params as borrowed `PyHost` from the render's private dict, which `tp_traverse` visits), or make
  it part of 0457.
- **D5, CI.** Add a Python 3.14 row to CI-python's module job (the stack-protection path), or rely on
  3.12 plus local probes until 0356's wheel matrix.

## 5. Steps (PR-sized)

**PR 1: engine facility (include/, src/, test/, docs/api-2.0.md).** Architect plan done; implementer;
verifier; Codex; security review (evaluator, untrusted input). Touches: include/jinja2cpp/template.h,
src/template_impl.h, src/render_suspension.{h,cpp} (new), src/render_workspace.{h,cpp},
src/recursion_guard.{h,cpp}, src/cycle_collector.h, CMakeLists.txt, test/output_test.cpp and a test-only
fiber helper.
Tests: a parameterised table `SuspendableRenderTest` (templates × patterns), checked against an eager
render of the same template:
- Templates: loop with `set`, include, extends with blocks, macro, call block, set and filter blocks,
  recursive loop.
- Patterns:
  - sequential;
  - interleaved A/B (the `interleave.cpp` assert today);
  - resumed from a callable of an eager render;
  - a suspendable render started inside a callable of another;
  - a plain render and `ReleaseThreadCaches()` while one is suspended (returns true; the suspended render
    still completes).
- Rows for limits: deep recursion on a 256 KiB fiber gives `RecursionLimitError`, not SIGSEGV (`deep.cpp`);
  `maxRenderSteps` and `maxOutputSize` count per render across suspensions; a poll that throws
  mid-suspension.
- Cancel at the k-th `Write`: destructors ran, and the thread state is clean (a following render matches
  and caches release).
These run on Linux (plain, ASan, TSan), macOS-15 arm64 and Windows MSVC. `preflight.sh --perf`: no
change past noise on the plain render gate.

**PR 2: switch primitive and lazy `generate()` (python/src, python/jinja2cpp, python/CMakeLists.txt,
python/tests/test_generate.py).** Implementer; verifier; Codex; security review (host boundary). Tests,
where possible as rows of a table `test_on_demand_matches_jinja2(source)`. Each row logs callable calls,
Python-generator production and consumer items, and compares the sequence with Jinja2's. Oracle
sequences recorded this session:
- `a{{ f(1) }}b{{ f(2) }}`: `item a, call 1, item 1, item b, call 2, item 2`;
- a macro: both calls before its one item;
- `{% for x in gen() %}`: `produce 0` before its items, `produce 1` after `]`;
- include, set and filter blocks.
Single tests:
- side effect not run after the first `next()` (the done-when);
- callables on the caller's thread;
- `threading.local`, an sqlite connection and an `RLock` held by the caller, used in callables;
- `contextvars` through `ctx.run(next, g)`;
- `close()` and `throw()` stop later calls; a weakref callback fires when an abandoned generator is
  collected;
- two interleaved generators, sandboxed and plain (`SecurityError` stays in the sandboxed one);
- nested generate in a callable; `next()` from its own callable gives `ValueError`;
- another thread gives `RuntimeError` (D2);
- deep template recursion and a callable's deep `repr` give `RecursionError`, no crash;
- KeyboardInterrupt (the existing rows);
- a subprocess exits 0 with a suspended generator alive, including in a daemon thread;
- the existing tests run in both modes (forced eager through a private switch).
The ASan CI job covers it; the upstream suite stays at 628 (expected-failures.txt unchanged).
Bench: python/bench `generate` of 10k items, lazy against eager against Jinja2.

**PR 3: docs and CI (docs/python.md, .github/workflows/python.yml if D5, task file).** Remove the
"runs the whole template at the first item" divergence. Add the D2 thread rule, the gevent and greenlet
eager fallback, 3.14.0 eager, and D4 if accepted. Mark 0457 done; update the G1 row text (the merge
steward owns the index). File the follow-ups: D4 if deferred, a greenlet backend if wanted.

## 6. What this makes hard next

- Every new per-render `thread_local` in the engine must join the bundle or be shown shareable. A
  missed one is a P0 interleaving bug that only a suspended render exposes. The REVIEW.md line and the
  interleaving table are the guards.
- Moving a render between threads is now structurally excluded: engine frames hold `thread_local`
  references. Supporting Starlette-style thread-hopping later would need a render that holds no
  thread-local references across a `Write`, which is close to option (b)'s stackless rewrite.
- Free-threaded wheels (0356, cp314t) must re-verify: atomic state, owner-thread checks without the GIL,
  and critical sections never held across a switch.
- Stack switching plus greenlet is a permanent incompatibility: either eager fallback, or a second
  greenlet backend.
- The module depends on an unstable CPython function on 3.14+. Each new CPython minor needs a check
  before the allow-list admits it.
- Native profilers and debuggers (py-spy `--native`, perf, gdb) see split stacks during lazy renders.
- 0456's item folding is independent, but any engine change that writes to the sink from a catch handler
  or a host callback breaks the switch invariant. The module then degrades to queuing (lazy no longer),
  so the on-demand table catches it.
- A public C++ resumable render (b, stackful) can later reuse PR 1's facility and the fiber library
  unchanged.
