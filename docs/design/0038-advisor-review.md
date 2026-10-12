# 0038 plan: advisor review (2026-10-12)


Checked: plan.md; src/render_context.h, src/statements.cpp (:823, :1745-1860, :1934-2292,
:959-1092), src/render_workspace.h, src/slot_frame.h, template_impl.h:370-415;
0117-name-slots-plan.md sections 2, 7, 8; perf-design-overview.md rules. Python Jinja2 3.1.6
renders of 18 probe templates (scratch `probe.py`, `probe2.py`); no builds run.

## Verdict: approve with changes

The contradiction: **"a macro reads the live frames that defined it"** (Jinja2 semantics,
late-bound, S6/D1/S19) against **"frames are stack-allocated, borrowed, freed LIFO, with no
long-lived pointer into RenderContext"** (0117/0129 speed, the no-raw-pointer rule). Option C
sacrifices the first requirement only for closures that outlive their activation (S7-S9, L4,
E1), where Python itself is either buggy (`missing` for loops/with: verified `'missingmissing'`,
`'[missing]'`) or needs heap cells. That is the right thing to give up now: the remaining
cases are rare, the divergences are listed, and C is reversible into A inside `ClosureSite`.

## 1. Soundness of running the body on the home context: holds (checked, no counterexample)

Every scope push on any context happens inside a `HomeCall` nested on the C++ stack, so all
pushes and pops on one context are properly nested whatever context they land on. I tried to
break it with: recursion through `with` and through an include (Python `0123` both), a macro
called from a child that calls back into a live sibling-defined closure, the lazy loop filter
(statements.cpp:1224 makes a child seeing a prefix, fine with extra scopes above), `caller`
inside an imported macro, and module contexts on the heap. All stay LIFO. Checks that hold it:
- Children see a fixed prefix (`VisibleScopesCount`, render_context.h:1142); a push on the
  parent always lands at or above `m_parentDepth`.
- Scope maps never move (`ScopeStack` chunks, :209-214); robin_hood nodes are stable.
- 0133 enters the loop body scope **once per loop** (statements.cpp:987/1092), so a closure
  defined in pass 1 and called in pass 2 keeps its serial: S6 `12` works by rule 2.
- No mid-render `catch` swallows a body exception (filters.cpp:841 and testers.cpp:182 are
  local; template_impl.h:381-410 ends the render), so a stray scope on a home after a throw
  cannot be observed today. `HomeCall` must still be full RAII (scope pop, view truncate,
  frame, epoch, autoescape, loop control, template frame): PR 3's heap module context and
  any future catch site would otherwise see a leftover scope.

Two written invariants become false in letter and must be rewritten plus Debug-asserted:
render_context.h:409-413 ("`other` must not add or remove scopes while [the child] is in
use") and :1200-1202 ("the parent pushes and pops no view while this context is alive").
In effect they hold because pushes land above the child's prefix; assert that on the child
side (record the parent's count at creation; at each lookup assert parent count >= it).

## 2. The handle table is not needed: use a per-context token (recommended change)

The table (`RenderWorkspace` list of `{RenderContext*, generation}`, free list, lazy
`Anchor`, TLS access on every miss) is a hand-rolled weak pointer. Simpler and the same
safety: `RenderContext` lazily makes `std::shared_ptr<ContextAnchor>` (`struct
ContextAnchor { RenderContext* context; }`), nulled in `~RenderContext`; `ClosureSite`
holds that `shared_ptr` (the token owns nothing, so no cycle). Resolve = one load and a
null test, no TLS, no generation, no index reuse, no rule-1 pointer-compare shortcut.
Cost: one allocation per context that *defines* a closure (root: once per render, about
0.05% of `Render/macros`; `caller` sites share the site's token). The copy/move
constructor (:376, :400) must not copy the token (Debug-assert copying an anchored context).
A `weak_ptr` to a heap `RenderContext` is not an option (an allocation per include/with/block).

Rules 3 and 4 (ended frame -> undefined, dead home -> params/globals only) are right
in-render: Python does not raise there either. Reserve `TemplateExpired` for calls after the
render (4.7), where the token is null.

## 3. Lookup cache with per-closure epochs: one gap to close (required, cheap)

Per-name forgetting (0158) covers every scope-level change I traced (add, clear, exit,
bind/unbind). The gap: `NewEpoch()` is the "forget everything" signal, and a closure's
private epoch ignores it. Today's `NewEpoch` sites (copy, child ctor, `SetLookupCache`,
`EnterScope(map&&)` non-empty, `BindScope`, `TakeCurrentScope`) are benign for a linked
closure because what they shadow lies above the link or dies with per-name forgets, but
that is an accident of the current call sites. Fix: a per-context `forgetGeneration`
bumped in `NewEpoch`; `ClosureSite` records it; at call, if it differs, the closure takes a
fresh epoch (one compare per call). Keep the Debug cross-check (:592) as the guard.
Recursion is safe: an activation's `set` names forget on add and on exit, so activation 1
re-finds its own after activation 2 returns.

## 4. PR 3: prefer render ownership over strong exports plus a sweep (recommended change)

Cycles are real and user-made (verified: `ns.mod = mod` with context import renders `True`
in Python). The sweep is sound (per-render weak list, stack discipline, nothing uses a
module after the render while `Callable` converts to an empty `Value`, internal_value.cpp:1944)
but it is a GC step with its own Debug assert and LSan test. Cycle-free alternative that
matches "one strong owner, weak escapes": the render owns modules (the per-render list the
plan already needs, holding `shared_ptr<ImportedModule>`), exports are the module's plain
closures whose `ClosureSite` token names the module context, no wrapper, no sweep. L9
(`V`) still works since the module lives to the end of the render. Cost: a module imported
inside a loop lives until the render ends instead of per pass (Python frees per pass);
bounded by the render, worth a sentence in the docs. If per-pass freeing matters more to
you, the plan's design is acceptable with: the list holds `weak_ptr`, the sweep is the
only clearer, and the "none left" assert runs after it.
Note the with-context snapshot is shallow: a namespace mutated after import is seen
(`[2]`, verified) while a re-`set` is not (`[1]`, L11). Add both rows.

## 5. P4 fit: clean, provided `ClosureSite` is opaque

C answers 0117 section 2's three questions and P4 only *adds* to the site (defining unit's
`FrameHandle`, later `shared_ptr<SlotBlock>`); the link-aware walk is the interim form of
P4's per-call free-name resolution. Keep `ClosureSite` a class with a `Resolve`, not a
struct that `statements.cpp` pokes, so the representation can change. A is not better now:
0133's `LoopFrame` is already a refcounted heap object, but loop values are released at
loop exit by design (and Python prints `missing` there), so A's extra correctness is only
S9/L4/E1 at an allocation per macro activation that defines a closure. Module root slots
(P4) need storage outside the LIFO workspace in either design.

## 6. Decisions (section 9)

1. Option C: **agree**, with the token (sec. 2) and opaque `ClosureSite`.
2. Escaped closures read undefined: **agree**; `missing` is a Jinja2 bug, do not imitate;
   raise only post-render.
3. idtracking as a separate task: **agree**; S15/S18/D5 need Load-time bits, not scoping.
4. With-context import snapshot: **agree** (L11 verified `[1]`; shallow, see sec. 4).
5. Exports own modules + sweep: **disagree**; render owns, exports weak (sec. 4).
6. Amend "Done when" to allow `deliberate:` lines: **agree**.
7. No opt-out: **agree**; the changelog entry should show the fix (pass the loop variable
   as an argument) since "macro reads the caller's loop var" is a common embedder pattern.
8. +0.7% named exception on `Render/macros`: **agree only if measured**; with the token
   and no rule-1 compare it should not be needed.

## 7. What would prove me wrong

- A template where a push on a live home lands below a child's prefix (would break sec. 1):
  I could not build one; `0108`'s prefix rule forbids it.
- `Render/macros` measured above +0.5% with the token (then the table's TLS-free path is
  not the cause and the serial per push is; measure `EnterScope` alone).
- A real workload importing in a loop with memory pressure (then take the sweep variant).

## 8. Next PR-sized step

PR 1 as planned, amended: token instead of the table; `forgetGeneration` compare;
`HomeCall` full RAII with the Debug count asserts; rewrite the two invariant comments and
add the child-side assert; `ClosureSite` opaque. Add rows: recursion via `with` and via
include (`0123`), `caller` stored and called after its loop (`[]`, deliberate; Python
`[missing]`), namespace-vs-set snapshot rows for PR 3. Verifier plus sanitizers, then
`bench/count.py` against 689995b on every case.
