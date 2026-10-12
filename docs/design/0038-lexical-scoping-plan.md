# 0038: lexical scoping for macros (design plan)

> Approved by Ruslan 2026-10-12 in the "Macro name scoping plan (0038)" thread, after an advisor review.
> Sections 10 and 11 (advisor revisions and the ownership map) supersede sections 4 and 7 where they differ:
> no handle table, weak anchors, modules owned by the render, no sweep. Implementation tasks: 0165, 0166, 0167.


Master 689995b. Behaviour checked with Python Jinja2 3.1.6 and with a probe linked against
the Debug `libjinja2cpp.a` of the same commit; costs measured with callgrind on a Release
`jinja2cpp_bench` (GCC 13, `bench/count.py` conventions). Nothing in the repo was edited.

## 1. Problem and current mechanism

**The tension.** Jinja2 compiles a macro into a closure: body and defaults read the *live*
bindings of the frames that defined it, at call time. Jinja2C++ keeps every binding on the
render's dynamic stack of scopes, and 0117's slots assume that stack (a slot frame is a LIFO
workspace block valid only inside its unit call). "A macro sees the frames that defined it"
(semantics) pulls against "frames are stack-allocated, borrowed by views, freed LIFO"
(speed, and the rule of no long-lived pointers into `RenderContext`).

How a call runs today (src/statements.cpp):
- `MakeCallable` (:1934) captures `this`, the tree view and autoescape, and evaluates every
  default that names no argument **at definition** (:1939, 0022's snapshot).
- `InvokeMacroRenderer` (:2228) pushes the body scope **on the caller's context** (:2247),
  binds the arguments in slots of a fresh unit frame (0117 P2) and renders there: every
  other name walks the caller's scopes (dynamic scoping).
- `caller` is made per call block (:2290) with the same callable, so the call body runs on
  the *macro's* context and sees the macro's loop target.
- The import wrapper (`ImportedValue`, :1843) finds the module by the name
  `$$_imported_<file>` **in the caller's context** (:1773) and runs the macro in
  `context.Clone(withContext)` with the module map bound below its scopes (`BindScope`,
  render_context.h:749; P0's `m_boundDepth`): "with context" means the call site, and a
  macro whose alias is not visible there renders nothing.
- `loop(...)` (`MakeLoopRecursion`, :823) and `super()` (:1490) run on whatever context
  calls them.
- Lookups by name (`VisitScopes`, render_context.h:993; `FindValueWithViews`, :1089) walk
  scopes innermost first with the frame views (`ScopeView`, slot_frame.h:137) of each index.
  Child contexts (:414) see a prefix of a parent's scopes. The resolver
  (name_resolver.cpp:229-277) never resolves across units; defaults sit in a `Dynamic` frame.
- No macro reaches user code (a `Callable` converts to an empty `Value`,
  src/internal_value.cpp:1944): escapes happen only within a render.

**Is "imported macros lose their module's variables" still true?** No. After 0117 P0 (#419)
`import_module_var`, `from_import_module_var`, `import_default_module_var` and
`import_param_shadows_module` all match Python. What is still wrong with imports is the
*context*: call-site leaks, "with context" read live at the call site, and the module found
by name (section 2, rows L5-L10).

## 2. Python semantics (verified)

Commands: `python3 -c "import jinja2; print(jinja2.Environment(loader=jinja2.DictLoader(files)).get_template('main').render(**ctx))"`
for each row, in a scratch script; C++ through the probe. `ctx` is empty unless shown;
statements rows use the corpus context (`x=3`).

| # | Template (abridged) | Python | C++ today |
|---|---|---|---|
| S1 | `set g='top'`; `m(): {{g}}`; `o(g): {{m()}}`; `o('inner')` (`macro_body_lexical_scope`) | `top` | `inner` |
| S2 | `m(): {{x}}`; `for x in [1,2]: m()` (corpus `x=3`) | `33` | `12` |
| S3 | `m(): {{loop.index}}` called in a loop | UndefinedError | `12` |
| S4 | `m(): for x in 'ab': caller()`; `for x in [1,2]: call m(): {{x}}` | `1122` | `abab` |
| S5 | macro defined in a loop reads the target / `loop` | `12` / `12` | same |
| S6 | macro defined in iteration 1, kept in a namespace, called in iteration 2 | `12` (late binding) | `12` |
| S7 | macro defined in a loop, called after the loop through a namespace | `missingmissing` | (empty) |
| S8 | `caller` stored in a namespace, called after the loop | `missing` | (empty) |
| S9 | nested macro `i` reads outer param `a`, escapes `o(1)`, `o(2)` | `12` | (empty) |
| S10 | `m(): [{{z}}]`; `o(): set z=1; m()` | `[]` | `[1]` |
| S11 | call body reads `z` set inside the macro / set at the call site | `[]` / `[T]` | `[M]` / `[M]` |
| S12 | `m(): {{x}}`; `apply(f): for x in [9]: f()`; `set x='T'` | `T` | `9` |
| S13 | `with a=1: macro m: {{a}}; with a=2: m()` | `1` | `2` |
| S14 | recursion; `m2 = m` then `m` redefined: `m2(2)` | `3210`; `X2` | same |
| S15 | `m(): {{g}}`; `m()`; `set g=1`; `m()` with `g=7` | `1` (first call empty) | `71` |
| S16 | `{{g}}` read at root first, then as S15 | `771` | `771` |
| S17 | S15 with the `set` inside an `if` | `7` | `7` |
| S18 | `m(): [{{x}}]` in a loop body that sets `x` later (`x=3` in context) | `[][1]` | `[3][1]` |
| S19 | `set y=5` at root, loop body defines `m(): {{y}}` then sets `y` | `[5][10][5][20]` | same |
| D1 | `set y=1; m(a=y); set y=2; m()` (`macro_default_reassigned_global`) | `2` | `1` |
| D2 | `m(a=i)` at root, called in `for i` | `[][]` | `[][]` |
| D3 | `m(a=bump())` called 4 times (counter in a namespace) | `1203` | `1101` |
| D4 | `m(a, b=a*2)`; `m(a=b, b=1)` with `b=7` | `1229`; `[]1` | same |
| D5 | `m(a=x)`; `m()`; `set x=1`; `m()` with `x=3` | `[][1]` | `[3][3]` |
| L1 | macro in an include (with context) reads includer's `set` and loop target | `T1` | `T1` |
| L2 | root macro called from an include that sets `x` | `3` (context) | `I` |
| L3 | include without context: only env globals, not render params | `[]` | `[]` |
| L4 | macro defined in an include escapes through a namespace | `I` | (empty) |
| L5 | `call wrap()` (imported without context) in `for i`: body `{{i}}` | `<1><2>` | `<><>` |
| L6 | same, body reads `v` that both importer and module set | `<main>` | `<mod>` |
| L7 | root macro passed to an imported macro (`apply(m)`) | `<G>` | `<>` |
| L8 | imported macro passed to another imported macro | `<V>` | `<>` |
| L9 | import inside a macro, macro kept in a namespace, called later | `V` | (empty) |
| L10 | `from m import mx with context`, called in `for x in [1,2]` | `[3][3]` | `[1][2]` |
| L11 | `set g=1; import with context; set g=2; m()` | `[1]` (snapshot) | `[2]` |
| L12 | module macro calls a sibling; module-level `set r = m()` | `H`; `V` | same |
| L13 | extends: child macro in parent block; `m(super)` from a block | `C`; `[B]` | same |
| L14 | `self.b()` in a macro | `BB` | `BB` |
| R1 | recursive loop body calls `m(loop, x)`, reads an *outer loop* target | `OO` | `OO` |
| E1 | macro handed to a Python function, called after render | `GGmissing` | n/a (empty `Value`) |

What the rows say:
- Bodies read their definition's frames (S1-S4, S10-S13), live and late-bound (S6, D1), and
  the template's *context* (params, globals) for names no frame binds (S2, L2). Call bodies
  close over the call site (S4, S11, L5, L6). Defaults are per call (D1-D5).
- Loop and `with` frames end with their activation: Jinja2 resets the variables and an
  escaped closure prints its `missing` sentinel (S7, S8; a Jinja2 bug). Macro-call and
  include frames stay alive in cells (S9, L4, E1).
- **idtracking**: a name a frame assigns is bound at frame entry, to the enclosing *local*
  binding when there is one (S19), to the context only if the frame itself reads the name
  first or assigns it only in a branch (S16, S17), else to undefined (S15, S18, D5).
- Imports: the module sees the importer's names *at import time* "with context" (L10, L11),
  only env globals without (L3).

## 3. Options

All keep `include/jinja2cpp/` unchanged.

### A. Owned frames (closures share heap frames)
Capturable frames (known at Load) become refcounted heap blocks with their scope map and
slots; closures hold them strongly; a render-end sweep breaks frame-closure cycles.
- Exact Python semantics, escapes included (S9, L4).
- Cost: an allocation per capturable activation (every loop holding a call block that reads
  its target, every macro call defining a nested macro), refcounts per definition and call,
  and loop slots leave the workspace (0117 P1 forks). 0117 option C's 2.5-3.5k LOC and risk.

### B. Captured-name snapshot per definition
At Load list each body's free names bound in a non-root enclosing frame; copy their values
into the closure at definition; read the root live.
- Simple lifetimes, but wrong for late binding (S6) and for names set after the definition
  (S19, nested macros), and the root still needs a live handle.
- An allocation per definition with a non-empty snapshot: every call block that reads a
  call-site local, in every loop pass.

### C. Home context and a static link (hybrid; recommended)
A closure records *where* it was defined: a checked handle to its defining context, the
scope depth, and that scope's serial. A call runs the body **on that home context**, in a
scope pushed above a *static link* that hides the scopes between the definition point and
the top of the home context. Bindings are read live through the existing lookups and views,
so late binding is free. Module roots get an owner (the module object) instead of the
render stack. A frame dies with its activation: an escaped closure reads that frame's names
as undefined (Python prints `missing` for loops and `with`; keeps macro and include frames).
- No allocation per definition or call; about 5 instructions a call for a top-level macro,
  15-20 for others (section 6).
- Imported calls get cheaper: no lookup of the module by name, no context per call.
- Cost: S9 and L4 become deliberate divergences; `RenderContext` gains scope serials and
  links in its views; one table of context handles in the workspace.

**Recommendation: C.** It fixes every row except the escape and idtracking rows,
with no measured regression past the gate. P4 can still move captured frames to shared
ownership later (option A's goal) without changing the closure's API (section 5).

## 4. Recommended design

### 4.1 Data
```cpp
// src/slot_frame.h (internal)
struct ContextHandle { std::uint32_t index = 0; std::uint32_t generation = 0; };  // 0 = none

// Where a closure was defined: what its body's free names resolve through (0038)
struct ClosureSite
{
    ContextHandle home;            // the defining context (RenderWorkspace table)
    std::uint32_t depth = 0;       // scopes it saw there
    std::uint32_t serial = 0;      // serial of scope depth-1, which must still be there
    std::uint32_t rootDepth = 0;   // its template's root scopes (TemplateFrame::baseDepth)
    std::uint32_t rootSerial = 0;
    std::uint64_t epoch = 0;       // its own lookup-cache epoch, taken at definition
    bool isTopLevel = false;       // defined at the template's top level (set at Load)
};
```
- `RenderWorkspace` gains a table of live contexts (`{RenderContext*, generation}`, free
  list). `RenderContext::Anchor()` registers lazily (one entry per context that defines a
  closure); `~RenderContext` unregisters and bumps the generation; `Resolve` returns null
  for a stale handle. The plan's one stored pointer: per-thread, behind a checked handle,
  cleared by the destructor, as `TemplateSlots` and `FrameHandle` do (a `weak_ptr` would
  need heap contexts).
- `ScopeStack` gains a `uint32_t` serial per entry from a per-context counter, next to
  `buckets` (two instructions per push). `ScopeView` gains `linkBelow`: "below this scope,
  continue at scope index `linkBelow`". The macro's argument view (pushed today at :2263)
  carries it; a macro without slots pushes an empty view only when the link hides something.
- The callable captures `ClosureSite` (32 bytes) in its already heap-allocated
  `std::function`: no new allocation; PR 2 removes the defaults vector.

### 4.2 Lookup of a body name
At call (`InvokeMacroRenderer`, the caller of a call block, `loop()`, `super()`):
1. Find the home context: the caller's context if its anchor equals `home` (a pointer-sized
   compare, the common case), else `workspace.Resolve(home)`.
2. Live and `serial` matches (skipped when `isTopLevel`): push the body scope on the home
   context with `linkBelow = depth`. Lookups from the body then walk: body scope and its
   argument slots, then scopes `[0, depth)` of the home context and its parents, then the
   external, global and built-in maps. The scopes the caller pushed above `depth` are
   skipped.
3. Live, but the scope at `depth-1` is gone (an escaped closure whose loop or macro call
   ended): link to `rootDepth` if the template root is still there (always, for the main
   template), else to the home context's parent prefix. Names of the ended frame read as
   undefined (S7, S8, S9).
4. Home dead (the include, `with`, set/filter block or module render that defined it has
   ended): run on the caller's context with every scope hidden (`linkBelow = 0`), so only
   render params, globals and built-ins are seen. Until PR 3, a dead home called through
   the module wrapper runs on the wrapper's context without a link, which is today's
   behaviour for imported macros.

An RAII `HomeCall` runs the body on the home context: it switches the tree view
(`ArenaSwitch`), autoescape, the unit frame (`UnitCall`) and the lookup epoch, and restores
them and pops scope and view on exit (Debug asserts the counts). `set` writes the body
scope; `set ns.attr`, `FindForWrite`, `ContextMapper` (`*context`), `FindSelf` and the call
block's macro lookup all go through `VisitScopes`/`FindValue`, so they follow the link.

Pushing on a context that is not the caller's is safe: a live home is an ancestor of the
running code on the C++ stack, or a module on the heap; push and pop are LIFO with respect
to the call; a child context sees a fixed prefix of its parent (`VisibleScopesCount`, :1142),
so 0108's invariant (no scope removed under a live child) holds; maps never move.

**Lookup cache.** The body runs under the closure's own epoch. Every call of one closure
then shares cache entries, and no two closures of one statement do (the same statement
defined at two depths of one context would otherwise share entries wrongly). Entries stay
valid between calls through 0158's per-name forgetting: a name added to a scope, a scope
cleared or left, and a slot bound or unbound each drop that name's bucket. The Debug
cross-check of every hit (`FindValueCached`, render_context.h:592) guards it. A top-level
macro keeps the home context's epoch (all its activations link to the same depth).

### 4.3 Defaults (PR 2)
Every default is evaluated per call, in the body scope after the arguments are bound,
through the same lookups as the body: lexical and live (D1, D3). `defaultRefersToArgs`
stays as the flag that binds missing arguments as undefined first, so a default never
sees an outer variable named like a later argument (D4). `MacroDefault`,
`definedDefaults` and the copy of mutable defaults go; `[]` is a new list per evaluation.
The resolver keeps defaults in their `Dynamic` frame (moving them into the macro unit so
`b=a` reads a slot is a P4 nicety).

### 4.4 `caller`, call bodies, `loop()`, `super()`
- `caller` gets the call site's `ClosureSite`: the body reads the call site, not the
  macro's locals (S4, S11), also inside an imported macro (L5, L6).
- `MakeLoopRecursion` and `super` capture their site too, or R1 and
  `super_passed_to_root_macro` regress once bodies stop seeing their caller's scopes.
  `self.<block>()` resolves through the template frame, unchanged.

### 4.5 Imports and includes (PR 3)
- **Includes** need nothing new: with context an include is a child of the includer's
  context and its macros' home (L1); without, it sees env globals only (L3); a macro called
  after its include ended falls under rule 4 (L4, deliberate).
- **Modules own their root.** `ImportedMacroRenderer` becomes `ImportedModule`, made before
  the module renders, holding a heap `RenderContext` (external scope: a snapshot of the
  importer's visible names "with context", as Python copies `get_all()` plus locals, L10,
  L11; empty otherwise), the module's `TemplateFrame` and blocks. The module renders *into*
  that context, so module macros are ordinary closures whose home lives with the module.
- **Exports hold the module strongly** (`from` aliases, namespace entries). Deleted: the
  `$$_imported_*` lookup, `Clone` per call, `BindScope`, `m_boundScope`, `m_boundDepth`,
  `FindValueFull` (L7-L9).
- **Cycles** (a module namespace holding one of its own exports) are broken by a render-end
  sweep: the workspace lists the render's modules (stack discipline for nested renders) and
  clears the scopes of any still alive. No value outlives its render; Debug asserts none left.

### 4.6 Extends and blocks
The parent renders in the child's context (`RenderAsParent`). Child macros called from
parent code, and parent macros called from child blocks, have that root context or one of
its block children as home (L13). A macro defined in a block has the block's context as
home and that template's root as its fallback.

### 4.7 Escape after the render
Impossible today (empty `Value`). If 0073 adds a module API or callable conversion, homes are
dead after the render: such a call should raise (0118's `TemplateExpired`), not render with
undefined names; matching E1 needs owned frames (option A, via P4).

## 5. Fit with 0117 P4 and the unified design

Answers to section 2 of the 0117 plan, "What 0038 must settle first":
1. **Free names in macro and call bodies** resolve through the closure's defining frames
   (its home context up to the recorded depth), at call time and live. The caller's stack
   is never consulted. Names no frame binds come from the defining template's context.
2. **Defaults** are evaluated per call in the macro unit, after the arguments, with the
   0022 pre-binding rule. The definition-time snapshot goes.
3. **idtracking quirks**: not reproduced. An unbound binding keeps falling through outward
   at read time (S15, S18, D5 diverge, and are pinned under a new task). Section 9,
   decision 3.

What P4 builds on:
- **Closure over frames** is `ClosureSite`. P4 adds the `FrameHandle` of the defining unit
  frame (0117's generation-checked handle) for index reads of captured slots. If captured
  frames ever need to outlive their activation (E1, S9), the site's target becomes 0117 C's
  `shared_ptr<SlotBlock>`; readers do not change.
- **Root slots**: the main root frame lives for the render, so closures reach root slots
  through their home; a module's root slots belong to `ImportedModule`, not the LIFO
  workspace.
- **Free-name resolution**: a closure's chain is fixed, so P4 can resolve free names once
  per closure into its `span<LookupResult>`; the closure epoch is the interim version and
  goes with the `LookupCache`.
- **Unified design**: one owner per template, unchanged (0118's `TemplateNode`/`weak_ptr`
  stay the path for values leaving a render); handles beyond the call; symbol ids untouched.

## 6. Performance

Measured on master 689995b (Release, callgrind over `CountedRegion` as `count.py` runs it):
- `Render/macros` 637,441 instructions; `InvokeMacroRenderer` 1,412 a call inclusive (body
  606, argument binding 313, scope exit 39, the rest frame and scope setup).
- A child `RenderContext` per call would add ~140 a call (ctor 79 + dtor 62, measured on
  `inheritance`'s includes), about +5% on `macros`: why C pushes on the home context.
- The imported-macro wrapper (name lookup, `Clone`, `BindScope`'s epoch, `~RenderContext`)
  is 3.6% of `Render/html_autoescape` (1,879,710 against 1,433,100 inner, 10 renders) and
  6.0% of `Render/config_file`.

| Case | PR 1 | PR 2 | PR 3 | Why |
|---|---:|---:|---:|---|
| Render/macros (top-level macro, 200 calls) | +0.1..+0.3% | -0.3..+0.3% | 0 | anchor compare + link store, ~5 instr a call; defaults evaluated instead of copied |
| Render/html_autoescape | ±0.2% | 0 | **-2.5..-3.5%** | wrapper overhead gone; module context reused |
| Render/config_file | ±0.2% | 0 | **-4..-6%** | same; calls from includes in loops |
| Render/inheritance | +0..+0.2% | 0 | 0 | scope serial per push; link test per view on a cache miss |
| Render/many_tags, chat_* | ±0.1% | 0 | 0 | no macros: a serial per `EnterScope` only |
| Render/mitsuhiko_table, for_* | ±0.1% | 0 | 0 | one push per loop entry |
| Render/plain_text, substitute | +0..+0.3% | 0 | 0 | `RenderContext` +40 B (anchor, serials) |
| Load (all) | 0 | 0 | 0 | at most an `isTopLevel` flag per macro |

- Allocations: PR 1 none; PR 2 -1 per macro definition with parameters; PR 3 the
  module's context lives inside the module object (same count), "with context" imports
  copy the visible names (Python does too).
- How it is checked, on every PR: `python3 bench/count.py --bench build-rel/bench/jinja2cpp_bench --out base.json`
  on the base, then `--baseline base.json` on the head, same container. Gate: Render
  within ±0.5% and Load ≤ +0.5% on every case. `--cache-sim` with I1mr on Render/macros
  not more than +5% (perf overview row 14). `bench/run.py --filter 'Render/(macros|html|config)'`
  to confirm the PR 3 gain in time. The PR description carries the table.
- The common case is free as asked: a top-level macro reading globals pays the anchor
  compare and, when called below its definition depth, the link field; no serial check,
  no epoch swap, no allocation.

## 7. Phases (one PR each, in order; parity lines deleted by the PR that fixes them)

**PR 1: macro and call bodies run where they were defined** (the core).
- Files: render_context.h (serials, link-aware walks, `Anchor`), render_workspace.h/.cpp
  (handle table), slot_frame.h, statements.h/.cpp (`MakeCallable`, `InvokeMacroRenderer`,
  `HomeCall`, `MacroCallStatement::Render`, `MakeLoopRecursion`, `super`),
  template_parser.cpp (`isTopLevel`).
- Deletes: `macro_body_lexical_scope`, `macro_reads_caller_loop_target`,
  `macro_reads_caller_loop`, `caller_loop_in_call_body`,
  `loop_in_filter_frame_macro_reads_target`.
- New statements.py rows (Python output; ✓ = matches after the PR):
  `macro_called_from_macro_hides_its_locals` `[]`,
  `macro_sees_global_not_caller_set` `1`, `macro_arg_macro_reads_def_site` `T`,
  `macro_called_in_with_hides_target` `[]`, `macro_in_with_reads_def_site_target` `1`,
  `macro_root_global_vs_loop_set` `[7]`, `macro_reads_context_not_loop_target` `3`,
  `call_body_hides_macro_local` `[]`, `call_body_reads_def_site_set` `[T]`,
  `call_body_args_and_def_site_target` `a1a2` (all ✓); pins that match today and must keep
  matching: `macro_late_binding_in_loop` `12`, `call_in_macro_reads_param` `<1><2>`,
  `recursive_loop_via_macro_reads_outer_target` `OO`.
- New loader.py rows: `include_cannot_rebind_macro_free_name` `3`,
  `call_block_on_imported_macro_reads_target` `<1><2>`,
  `call_body_reads_importer_not_module` `<main>`, `call_body_of_imported_loop_macro`
  `1122`, `root_macro_passed_to_imported_macro` `<G>` (✓), `super_passed_to_root_macro`
  `[B]` (pin).
- New divergence lines:
  - `statements.nested_macro_escapes_its_call output 0038 deliberate: a macro that outlives the macro call defining it reads that call's names as undefined`
  - `statements.macro_escapes_its_loop` and `statements.caller_escapes_its_loop`:
    `output 0038 deliberate: a closure called after its loop reads the loop's names as undefined; Jinja2 prints its missing sentinel`
  - `loader.include_defined_macro_escapes output 0038 deliberate: a macro that outlives the include defining it loses the include's names`
  - Under the new idtracking task (section 8): `statements.macro_root_name_set_after_call`
    (S15 with the corpus `x`: Python `[][1]`, ours `[3][1]`) and `statements.macro_loop_name_set_after_call`.
- Unit tests: `slot_frame_test.cpp`: a stale `ContextHandle` resolves to null after the
  context dies and after reuse of its slot; handles freed out of order; scope serials grow
  per push and survive `Clear`. `helpers_tests.cpp`: a lookup through a linked view skips
  the hidden scopes, honours the link inside a parent seen by a child, and `FindForWrite`
  follows it. `macro_test.cpp`: rows for nested macros, a macro called from a filter, a
  `*context` user callable in a macro body that sees the definition site, and an exception
  thrown in a body that restores the home context (ASan).
- Verifier plus the sanitizer configuration (a render-path change).

**PR 2: defaults per call.**
- Files: src/statements.h/.cpp (`BindMacroDefaults`, `MakeCallable`; `MacroDefault`
  deleted).
- Deletes `macro_default_reassigned_global`.
- New rows: `default_evaluated_per_call` `1203`, `default_reads_def_site_not_caller_loop`
  `[][]`, `default_in_loop_reads_target` `12`, `default_reads_outer_macro_param` `12`
  (pins), `default_root_name_set_after_call` `[][2]`; idtracking line
  `statements.default_root_name_set_after_context` (D5).
- `macro_test.cpp`: a mutable default is fresh per call; a default that calls a macro runs
  once per call.

**PR 3: modules own their root; imported macros run in their module.**
- Files: src/statements.h/.cpp (`ImportedModule`, `ImportStatement`, `ImportNames`,
  `ImportedValue`), src/render_context.h (delete `BindScope`, `m_boundScope`,
  `m_boundDepth`, `FindValueFull`; add the visible-names snapshot),
  src/render_workspace.h/.cpp (module list and sweep), src/template_impl.h (sweep at render
  end).
- New loader.py rows: `import_with_context_ignores_call_site` `[3][3]`,
  `import_with_context_snapshot` `[1]`, `import_with_context_set_after_import` `[]`,
  `imported_macro_passed_to_imported_macro` `<V>`, `imported_macro_outlives_import_scope`
  `V` (all ✓); pins: `imported_macro_calls_sibling` `H`, module-level `set r = m()` `V`,
  `loader.from_import_param_shadows_module_name` (P0's case).
- `import_test.cpp`: a module that stores one of its own exports in its namespace renders
  twice with LSan clean and the module freed (`weak_ptr::expired()`); a nested render from
  a user callable keeps its modules apart from the outer render's sweep.

**PR 4 (docs):** 0038 status and links, the new task files, parity.md's macro row (🟡 with
the deliberate escape note), then the steward's `task_index.py`.

Order matters: per-call defaults on a dynamic body would break D2 (which matches today), and
PR 3 runs module macros through PR 1's home calls.

## 8. Risks, related tasks, and what this makes hard next

Risks:
- **State on the home context** (loop control, autoescape, frame, tree view) must be
  unchanged after a call: `HomeCall` restores it; the verifier throws from arguments,
  defaults and bodies.
- **Link-aware walks** touch tuned lookups (0129); links live only in views, so the
  no-view fast path (`FindEntry`) is unchanged.
- **Embedders** relying on dynamic scoping (a macro reading its caller's loop variable)
  see new output: changelog entry, no opt-out (decision 7).
- **0108's child-prefix invariant** becomes load-bearing for pushes on a parent with live
  children: Debug-assert it.

Related tasks:
- **0022** (done): snapshot dropped in PR 2, `defaultRefersToArgs` kept. **0117**: P0's
  `m_boundDepth` goes in PR 3, P1/P2 unchanged, P4 per section 5. **0118**: escape handles
  unaffected. **0158** (done): its per-name forgetting keeps closure epochs valid.
- **0143**: the bullet "the body of a `{% call %}` to an imported macro sees the module's
  `self`" is fixed by PR 1. Both touch statements.cpp and render_context.h whole, so they
  cannot share a wave.
- **0145**: `ClosureSite` (home, statement, epoch) is the identity token macros need for
  `m == m`.
- **0051** (`autoescape.env_on_caller_markup_call_site`): the closure already carries the
  call site's autoescape; 0051 can use it, and 0038 leaves the line alone.
- **0052, 0047**: none (`macro_reads_caller_loop` matches via 0026's policy, checked).
  **0144**: `#LoopAccessor` against PR 1's `#MakeLoopRecursion`, disjoint anchors.
- **0155** (`ImportNames`), **0156** (`MacroCallStatement::Render`) and **0164**
  (render_context.h, render_workspace.*) overlap PR 1 and PR 3. Land 0155 and 0156 first
  (both small), and sequence 0164 after PR 1.
- The task's own "Next" (`set` scoping in loops and blocks) is the idtracking task below.

Candidate task files (out of scope; next free numbers):
- **idtracking frame-entry binding** (parity, low): Jinja2 binds a name a frame assigns
  at frame entry, from the context only if the frame reads it first. Cases: S15/S18, the
  loop form, D5 and the non-macro `{% for i in [1,2] %}{% for j in [1] %}{{ x }}{% endfor %}{% set x = i %}{% endfor %}`
  (Python `''`, ours `33`). It needs a per-name "loaded before stored / stored in a branch"
  bit at Load and no runtime cost; P4's root slots should carry it.
- **`caller()` without a caller** renders nothing; Python raises "No caller defined". A
  call body that reads `caller` binds its own (Python), so `{% call inner() %}{{ caller() }}{% endcall %}`
  inside a macro raises in Python and renders `<>` here.

What this makes hard next:
- E1 and S9 (closures outliving their activation or render) need owned frames (P4's
  `SlotBlock` or refcounted captured scopes).
- From PR 1 on a body may run on a context other than its caller's: every new render-time
  field of `RenderContext` must be saved by `HomeCall`.
- Resolver frames and runtime scopes gain a third description (links); the slot and cache
  cross-checks are the guard.

## 9. Decisions for the owner

1. **Option C** (home context + static link, checked handles, owned module roots) over
   owned frames (A) and snapshots (B)? *Recommend yes.*
2. **Closures that outlive their loop, `with`, macro call or include** read that frame's
   names as undefined, as deliberate divergences (Python prints `missing` for loops and
   `with`, keeps macro and include frames)? *Recommend yes; revisit with P4's owned
   frames.*
3. **Jinja2's idtracking** (S15, S18, D5): keep the outward fall-through and file a task?
   *Recommend yes.*
4. **"Import with context" snapshots** the importer's names at import time, as Python
   does, instead of reading the call site? *Recommend yes.*
5. **Modules owned by their exports**, with a render-end sweep for cycles (L9 matches)?
   *Recommend yes.*
6. **Amend 0038's "Done when"** to "no line names 0038 except `deliberate:` ones", as 0015
   did? *Recommend yes.*
7. **No opt-out setting** for the old dynamic scoping, changelog entry only? *Recommend
   yes.*
8. **Named exception** for Render/macros up to +0.7% in PR 1, if measured above 0.5%,
   since PR 3 nets macro-heavy cases negative? *Recommend yes, only if measured.*

## 10. Revisions after the advisor review (advisor-memo.md)
Verdict: approve with changes. These replace the matching parts of sections 4 and 7:
1. **Anchor token instead of the handle table (4.1).** Each context that defines a closure
   lazily makes a `shared_ptr<ContextAnchor>`; the anchor holds the context's address, which
   `~RenderContext` clears. `ClosureSite` holds the token. No TLS table, generation or free
   list; one allocation per closure-defining context. Copy/move of a context never copies it.
2. **`ClosureSite` is an opaque class with `Resolve()`**, so P4 (or option A later) can swap
   its internals without touching callers.
3. **Global forgets (4.2 cache).** A per-context `forgetGeneration`, bumped in `NewEpoch()`,
   is recorded in `ClosureSite` and compared per call; a mismatch takes a fresh epoch.
4. **`HomeCall` is full RAII**: scope pop, views, unit frame, epoch, autoescape, loop
   control, template frame. The invariant comments at render_context.h:409-413 and
   :1200-1202 are rewritten, with a child-side Debug assert.
5. **Modules (PR 3) are owned by the render, not by their exports.** The per-render module
   list holds them strongly; exports are plain closures whose anchor names the module
   context. No wrapper, no cycles, no sweep. Cost: modules imported in a loop live to the
   end of the render. Decision 5 changes accordingly.
6. **New corpus rows:** recursion via `with`/include (`0123`); `caller` called after its loop
   ended (`[]`, deliberate); import-with-context snapshot is shallow: a namespace mutated
   after import is seen (`[2]`), a re-`set` is not (`[1]`).
7. Changelog for decision 7 shows the migration: pass the loop variable as an argument.

## 11. Ownership map (Ruslan 2026-10-12: weak by default, strong only where it must be)
Strong owners, and only these:
1. **A context owns its own anchor.** `RenderContext` holds the only `shared_ptr<ContextAnchor>`,
   made lazily when it first defines a closure. It is a `shared_ptr` only because `weak_ptr`
   needs a control block: there is exactly one owner, and `~RenderContext` asserts
   `use_count() == 1` (nobody holds a lock while the context dies).
2. **The render owns imported modules.** The per-render module list holds each module as
   `std::unique_ptr<ImportedModule>`; the module's context has an anchor like any other.
3. **Already in the unified design:** the template owns its tree (`shared_ptr<TemplateImpl>`,
   held by the `Template` and by the render while it runs).

Everything else is weak or a scoped borrow:
- macro, `caller`, `loop()`, `super()` -> defining context: `weak_ptr<ContextAnchor>` in
  `ClosureSite`;
- an export (`from x import m`, `x.m`) -> its module: the same weak anchor of the module context;
  the export does not keep the module alive;
- a closure -> its template's tree: `weak_ptr<TemplateImpl>`; after the render a call raises
  `TemplateExpired` (0118's rule);
- `HomeCall` locks the anchor for the duration of one call only (a stack-scoped
  `shared_ptr`, never stored). The lock proves the context is alive at entry; the context
  cannot end during the call because the call is nested inside its lifetime on the C++ stack.
  The Debug assert in point 1 checks exactly that.
No closure, export or value owns a context, so no ownership cycle can form and no sweep
is needed. This replaces point 1 and point 5 of section 10 where they differ.
