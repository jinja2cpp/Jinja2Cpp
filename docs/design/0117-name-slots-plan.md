# 0117: resolve variable names to slots at Load (architect plan)

## Revision 2026-10-06 (after Ruslan's review)

- **No new raw pointers.** A unit call's slots are a `SlotFrame`: a
  `boost::span<Slot>` (Boost 1.92 is already pinned; `boost/core/span.hpp`, no new dependency)
  with an `operator[](SlotIndex)` that is bounds-checked in Debug (`boost::span::operator[]`
  `BOOST_ASSERT`s). `Slot` is `std::optional<InternalValue>`. `SlotIndex` and `UnitId` are
  strong typedefs.
- **Only views inside the call, handles beyond it.** Views (spans) never outlive the statement
  that made them. Anything that does outlive it holds a checked `FrameHandle` (arena offset plus
  generation), never a span or a pointer. Section 4.2.
- **`std::optional` per slot replaces rev. 1's bound bitmask.** This removes the shared mutable
  mask, the `const uint64_t*` in frame views and the 64-slot cap (old decision 7).
- **P1a now introduces `LookupResult`.** This small result type replaces `const InternalValue*`
  as the return of `FindValue` and `EvaluateRef`, and `MutableLookupResult` replaces
  `FindValueSlot`'s pointer. Its one private pointer is justified in 4.2.
- **New section 4.3** lists every pointer the plan introduces, keeps or removes, with a
  replacement or a justification.
- **New section 4.4** says how each piece works before and after 0118's `NodeRef<T>`.
- **Recursive loops are now left entirely to the dynamic path in P1-P3.** An escaped `loop`
  callable can be called after its loop has ended. Both engines render
  `{% set ns = namespace() %}{% for x in [[1]] recursive %}{% set ns.l = loop %}{{ x }}{% endfor %}[{{ ns.l([5]) }}]`
  as `[1][5]`, so no slot or frame handle may be captured there. This adds a new corpus case.
- **Estimates are unchanged within ±0.5%.** A span is the same loads as a pointer plus size.
  The costs added are 8 bytes per slot, one generation compare per call of the loop filter,
  and one virtual call per unit entry to reach the arena.
- **Master moved during the review.** 0133 (#412, loop-frame reuse) landed: `mitsuhiko_table`
  is now 7.26M instructions per render. Measured again on 62dda01, `EvaluateRef` is 6.7%, the
  loop-frame release (`LoopFramePool::Give`) 4.3%, `ExitScope` 2.2%; `macros` is unchanged
  (`EvaluateRef` 16.1%, `ScopeRef::operator[]` 12.1%). P1's `mitsuhiko_table` estimate is
  now **-7..-10%**; P2's is unchanged. File:line references are against 448bb7c; 0133 moved
  statements.cpp (for example `static_assert(!is_flat)` is now :228) and render_context.h
  (the spare-scope test is now :393).
- **Scope containers (new section 4.5).**
  - A microbenchmark of exactly the scope shape (1-16 names; build, lookups, clear) and a
    robin_hood -> `boost::unordered_node_map` swap measured in the engine both argue
    against a Boost drop-in.
  - The node map is the only node-stable Boost option. It allocates per insert:
    `macros` +8.8%, `many_tags` +4.5%, `mitsuhiko_table` -1.4%.
  - Recommendation: a chunked linear `ScopeMap` for the scopes that remain (new **P5**), and
    `boost::unordered_flat_map` for the read-only external, global and built-in maps.
  - Then robin_hood can go once dicts move too.
- **Interned names** mean 0118's per-template `SymbolId` (Flyweight was dropped). Ids help
  only same-template scans; the cache stays keyed by expression site; runtime names (user
  keys, globals) stay strings. They do not change the container choice.
- **Decisions:** old 7 is dropped; 10 (spans) is settled on `boost::span`; 11 (existing
  pointers in `RenderContext`), 12 (`Slot` representation), 13 (scope containers) and 14
  (`SymbolId`) are new; 1 is re-targeted to post-0133 master.

Measured on master 448bb7c (HEAD b7e7b2b only changes docs). Release build `build-rel`
(GCC 13, x86-64, 4-core cloud container), callgrind over `CountedRegion` only, the same way
`bench/count.py` runs it. Behaviour was checked with Python Jinja2 3.1.6 and with a small probe
linked against `build/libjinja2cpp.a`. Nothing in the repo was edited.

## Revision 3 2026-10-06 (the integrated cache review)

Read `perf-design-overview.md` (same directory) first: it supersedes this plan's order
where they differ.
- **`Slot` = `InternalValue` with an unbound bit** in its flag word: 72 B instead of
  `std::optional`'s 80. A later value shrink (new task N3) makes slots 32 B with no
  change here.
- **`SlotArena` becomes a thread_local `RenderWorkspace`** with mark/release stack
  discipline, so nested renders on one thread stay re-entrant. It holds the slot
  buffer, 0133's loop-frame pool, the lookup cache and the env-globals snapshot (N5b).
  `FrameHandle` is unchanged.
- **The lookup cache is rekeyed by `{TemplateSlot, SymbolId}`, 32 entries**, instead of
  section 4.5's "keyed by expression site". After slots it serves only dynamic names,
  and a site-keyed entry never hits twice in a single-pass template. The change is
  gated on measurement (`many_tags`, `macros`, chat_*).
- **P1 also resolves `loop.<constant attribute>` to a `LoopAttr` enum** at bind time.
  String dispatch is about 17% of `for_loop_vars`; recursive and escaped `loop` stay
  dynamic.
- **P5's SoA `ScopeMap` has byte budgets:** 16 four-byte keys per 64 B line, values in a
  parallel array.
- **P4 absorbs N5c** (eager parameter conversion per render) if it lands first.
- **Order:** P0/P1a in wave 1; P1, P2, P5, P3 interleaved with 0118 P3-P5 in wave 2;
  P4 in wave 3 (overview section 5).

## 0. Summary

- **Most of the gain is in scope upkeep and misses, not in the cached hit.** A cache hit for
  `{{ cell }}` costs about 35 instructions today. A slot read would cost about 8. The bigger
  costs are elsewhere: building and clearing a robin_hood scope every time a loop or macro is
  entered (`ScopeRef::operator[]` plus `ExitScope`, 8% of `mitsuhiko_table` on 448bb7c and 6.5% after 0133, 18% of `macros`),
  and the lookups that miss because each such entry starts a new epoch (`macros`: 104
  instructions per lookup).
- **The contradiction can be split.** Jinja2's dynamic features (contexts passed to includes,
  imports, `globals`, `*context` callables, macros under today's dynamic scoping) only affect
  names that cross a *unit*. A unit is a template body, a macro body, a call body or a block
  body. Take a name read inside the unit that binds it, with no store of that name in between.
  It resolves to the same binding under both dynamic and lexical scoping. So a slot version
  limited to those names **does not depend on 0038**. Root (template-level) slots and closure
  capture do depend on it.
- **Recommendation: option B ("binding slots, dynamic rest"), in 4 PRs before 0038 and one
  after.** Each unit call gets one slot frame, a `boost::span<std::optional<InternalValue>>`
  from a per-render arena, addressed by `SlotIndex`. The binders that have static names are
  for-targets, `loop`, macro and call parameters, and `with` targets. Every other name keeps
  today's cached lookup. That lookup also scans the live frames by name, so includes,
  `*context` callables and dynamically scoped macros see exactly what they see today. In
  Debug, every slot read is checked against that by-name lookup.
- **Expected:** `mitsuhiko_table` -7..-10% against post-0133 master (P1), `macros` -20..-25% and `config_file` -8%
  (P2). **`expressions` only about -4%.** The task's "-10% on `expressions`" cannot be reached
  by name resolution, because that case is bound by the operator tree walk (0100 idea 8, 0118).
  `many_tags` needs root slots (P4, after 0038).
- **A parity bug blocks the Debug check and must go first (P0).** Inside an imported macro, a
  module-level name hides the macro's own parameter: the bound scope is searched before
  everything else (render_context.h:389). For `{% from "m" import f %}{{ f(5) }}`, with `m`
  setting `g = 1` and defining `f(g)`, Jinja2C++ renders `1` and Jinja2 renders `5`.
- **Containers (4.5): no Boost drop-in for scopes.** `boost::unordered_node_map` measured
  +8.8% on `macros` and +4.5% on `many_tags` (an allocation per insert). After P2, a chunked
  linear `ScopeMap` backs the scopes that remain (P5; -34% per scope cycle in the
  microbenchmark), and `boost::unordered_flat_map` backs the read-only external, global and
  built-in maps. 0118's `SymbolId` speeds up same-template scans only.

## 1. Current state

### 1.1 Where names live at render time

| What | Where | Notes |
|---|---|---|
| Scopes of a context | `ScopeStack` of `InternalValueMap` (robin_hood node map), chunks of 8, the first inside the context (src/render_context.h:158-255, 0129) | Node-stable: `EvaluateRef` and the lookup cache keep pointers into nodes (0088 S5) |
| Nested contexts | `RenderContext(RenderContext& other, size_t depth)` (:305) keeps `m_parent` plus how many of its scopes it sees (0108). Used by `Clone(true)` (:551) for include and import with context, `with`, set-block, filter-block, and by blocks and `super` | One own scope; lookups walk the parent chain |
| External / global / built-in | `m_externalScope` (the user's `ValuesMap`, converted per render), `m_globalScope` (env globals), `m_builtinScope` (template_impl.h:260) | Searched last |
| Imported module | `m_boundScope`, set by `ImportedMacroRenderer::InvokeMacro` (statements.cpp:1126-1131) | **Searched first**, before the macro's own parameters (render_context.h:389); bug, see P0 |
| Writes | Only through `ScopeRef` (render_context.h:132-152). `operator[]` starts a new epoch when a name is added; `Clear` and `ExitScope` (:356) do the same when the scope was not empty. Every write is in statements.cpp, and every written name is static: set targets, for targets, macro names, import aliases, `self`, `super`, `$transN`, `$$_imported_*` | This property is what makes static resolution possible |

### 1.2 The lookup

`ValueRefExpression::EvaluateRef/Evaluate` (expression_evaluator.cpp:104-117) calls
`RenderContext::FindValueCached` (render_context.h:451). That is a thread-local 128-entry
cache keyed by the node's address and the context epoch (`LookupCache`, :86-125). On a miss it
calls `FindValue` (:385): bound scope, then this context's scopes backward, then the parents'
visible scopes, then external, global, built-in. It is force-inlined (0129). Only names that
are found get cached.

Other by-name lookups, which must keep working whatever the design:
- `AssignTo` for `set ns.attr` (statements.cpp:60).
- `CallLoopCycle` finds `loop` (expression_evaluator.cpp:907).
- `MacroCallStatement::Render` (statements.cpp:1516).
- `ImportedMacroRenderer` finds `$$_imported_*` (:1136).
- `applymacro` (filters.cpp:631), user filters and tests (filters.cpp:2221, testers.cpp:252, :580).
- `FindValueSlot` for in-place mutation (render_context.h:484, used by
  expression_evaluator.cpp:238).
- `ContextMapper`, the `*context` argument of user callables (internal_value.cpp:1732, :1818).
  It sees every local.
- `_` builds a `ValueRefExpression("gettext")` during the render (global_functions.cpp:444).
  So nodes made at render time must keep working unresolved.

### 1.3 How each construct uses scopes

| Construct | Runtime scopes | Code |
|---|---|---|
| Template root | ctor scope, plus `self` written in `TemplateRenderer::RenderBody` | statements.cpp:841-883 |
| `extends` | The parent renders **in the child's context and root scope** (`RenderAsParent`) | :828-839 |
| `for` | Loop scope (`loop`, target; target slot pointer cached in `LoopTargetSlots`) plus a body scope cleared each pass; the filter pushes a temp scope with the target | :457-556, :558-592, :202-228 |
| `macro` / `call` body | One scope pushed **on the caller's context** (dynamic scoping): parameters, specials, body sets | :1483-1511 |
| imported macro | `Clone(withContext)` + `BindScope(module map)` + the macro scope | :1126-1131 |
| `with` | `Clone(true)` + `EnterScope`; targets evaluated in the outer context | :1544-1558 |
| `block` | `RenderContext(values, baseDepth or all)` + `EnterScope` (`super`) | :754-811 |
| `include` | `Clone(withContext)` + `EnterScope` | :991-1012 |
| `import` | `Clone(withContext)` + `EnterScope`, `TakeCurrentScope` | :1176-1204 |
| set-block, `filter` | `Clone(true)`; body sets are dropped | :644-650, :1578-1593 |
| `autoescape`, `trans` | `EnterScope` | :1595-1601, :1560-1576 |
| `if` | none: sets go to the enclosing scope | |

The parser already does some name analysis, by token scan rather than on the tree:
`defaultRefersToArgs` (template_parser.cpp:809) and the `caller`/`varargs`/`kwargs` flags
(template_parser.h:1590-1740). There is a `StatementVisitor` (src/ast_visitor.h), but no
visitor over expressions. `ValueRefExpression` is made at expression_parser.cpp:559 and
template_parser.cpp:1491, 1610, 1663, 1678.

### 1.4 Measured share (instructions; percentages are of the render; per-call figures are instructions divided by calls)

| Case (instr/render) | `EvaluateRef` | `Evaluate` (copying) | `ScopeRef::operator[]` | `ExitScope` | Notes |
|---|---:|---:|---:|---:|---|
| mitsuhiko_table (7.88M) | 6.2% (35/call, 10k calls/render, 9 of 10 hit) | 2.7% (`row`, nav) | 3.9% (~104/insert, 2 per row) | 4.3% (122/call, 2 per row) | Lookup plus scope upkeep about 17% |
| macros (1.18M) | 16.0% (104/call: nearly all miss, each call adds 4 names) | | 12.0% | 5.7% | `InvokeMacroRenderer` 59% inclusive |
| expressions (555k) | 5.4% (~20/call, hits) | 2.5% | ~0 | ~0 | `BinaryExpression` 60% inclusive |
| for_loop_vars (620k) | 3.2% | 1.3% | | | `loop.*` property lookups dominate |
| many_tags (1.29M) | 16.2% | 5.9% | 8.3% (in `AssignTo` 11.2%) | | Every top-level `set` adds a name, so a new epoch, so `flag`/`obj` miss |
| config_file (2.22M) | 6.8% | 1.9% | 4.6% | 2.8% | macro-heavy |
| html_autoescape (1.83M) | 3.2% | 1.1% | 2.6% | 1.4% | macros |
| inheritance (286k) | 12.2% | | | | `item` read inside an include: crosses a unit; dynamic in every option short of C |
| chat_llama (431k) | 3.3% | | | (`AssignTo` 1.2%) | |

Raw data: `cg.<case>` in this session's scratchpad (not kept). Counts per render are from
`count.py` on the same binary (scout run 2026-10-06).

## 2. The tension

**Requirement 1 (speed):** a name should be an index into a frame, resolved once at Load,
as Python's compiler does. Then no hashing, no epoch, no scope map per loop or macro entry.

**Requirement 2 (Jinja2 semantics):** a name's binding is partly dynamic:
- An `include` with context sees the caller's locals, loop variables included.
- `extends` runs the parent in the child's context. Blocks see template-level names, and
  scoped blocks see locals.
- Imported macros see their module, plus the importer's context "with context".
- `*context` callables, `applymacro` and user filters and tests look names up by string at
  render time.
- `set` creates a binding only once it runs. A read before it resolves outward:
  `{% for i in [1,2] %}{{ x }}{% set x = i %}{{ x }}{% endfor %}` gives `5152` with `x=5`.
- Undefined names go through the undefined policy.
- Jinja2C++ macros are still dynamically scoped (0038).

**What reconciles them:** split names by whether they cross a unit.
- *Intra-unit, no intervening store:* a read of N in unit U whose nearest enclosing binder
  of N is in U, with no frame on the path writing N through a map (`set`, `macro` definition,
  `import`). Its target is fixed at Load under dynamic and lexical scoping alike.
  -> **slot**.
- *Everything else:* free names, root names, names read in a nested macro, names written by
  `set` on the path. -> **dynamic** (today's cached by-name path).
- The dynamic path must still *see* slot frames, by name and in stack order: includes,
  `*context`, dynamic macros. So each scope entry carries a view of its frame's slots. The
  static and dynamic answers can then be compared on every read in Debug. That is the
  correctness tool: the whole unit suite plus the parity corpus become a resolver test.

**What 0038 must settle first** (only for the later phases):
1. What a free name in a macro or call body refers to: the defining unit's frames (closure)
   or the caller's stack. Root slots and closure capture (P4) need this answer. P1-P3 do not,
   because they never resolve across a unit.
2. Defaults: per call in the macro unit (0038), or the definition-time snapshot (0022). The
   resolver must put each default in the unit where it is evaluated. Until 0038 that follows
   `defaultRefersToArgs`.
3. Whether to reproduce Jinja2's idtracking quirks. Today Jinja2C++ resolves an unbound
   name outward on each read. Jinja2 decides per frame, at frame entry:
   - `{% for i in [1,2] %}{% for j in [1] %}{{ x }}{% endfor %}{% set x = i %}{% endfor %}`
     with `x=3`: Jinja2 gives `''` (the inner read binds to the outer body's not-yet-set
     `x`), Jinja2C++ gives `33`.
   - `{% macro m() %}{{ g }}{% endmacro %}{{ m() }}{% set g = 1 %}{{ m() }}` with `g=7`:
     Jinja2 gives `1`, Jinja2C++ gives `71`.

   A slot design can mirror either. The perf phases keep today's behaviour.

**Partial version before 0038: yes.** P1 slots for-targets and `loop`. P2 slots macro and
call parameters, specials and `with` targets. Both are intra-unit by construction. Recursive
loops keep map frames.

## 3. Options

All three keep `include/jinja2cpp/` unchanged: `RenderContext`, nodes and statements are
internal, and the ABI is untouched.

### A. No slots: sharpen the dynamic path
Per-name invalidation. Adding a name bumps one of 64 hash-bucket epochs instead of the global
one; clear and exit still bump the global epoch. Plus 0133's one scope per loop entry.
- **Gain:** `many_tags` about -12% (`flag`/`obj` stop missing after each `set`); `macros`
  -3%; `mitsuhiko_table` -2..-4% (with 0133).
- **Risk:** low. **Size:** ~150 LOC in render_context.h.
- **Cost:** it does not remove maps, epochs or misses at macro entry, and it does not move
  towards 0117's goal. It composes with B.

### B. Binding slots, dynamic rest (recommended)
At Load, the parser records each name node with its lexical frame, plus each frame's binders
and map stores. At the end of the parse a resolver gives a node a `SlotIndex` in its unit only
under the intra-unit rule; otherwise the node stays dynamic.

Runtime pieces:
- A per-render `SlotArena`, owned by the render's `RendererCallback` (one per render
  already, reachable from every context through `GetRendererCallback()`). LIFO, in chunks
  that never move, the first chunk inline. It hands out `SlotFrame`s and `FrameHandle`s.
- An RAII `UnitCall` in each function that renders a unit: `TemplateRenderer::RenderBody`,
  `InvokeMacroRenderer`, `BlockStatement::RenderBody` and the no-frame path at
  statements.cpp:759. It takes a frame from the arena, installs it in the context **by value**,
  and gives it back on exit.
- Frames inside a unit (for, with) only engage and reset ranges of the unit's frame.
- Each `ScopeStack` entry gains an optional frame view: two spans, values and names. So
  by-name lookups see slots.
- The `for` filter adapter can be run late (`GetLength` from inside a macro). It holds a
  `FrameHandle` and resolves it per call.

Estimates:
- **Gain:** P1 `mitsuhiko_table` -7..-10% (post-0133), `for_*`/`expressions` -3..-5%, chat -1..-3%. P2
  `macros` -20..-25%, `config_file` -8%, `html_autoescape` -4..-6%. `many_tags` and
  `inheritance` about 0 (±1%: by-name scans now also compare a few slot names).
- **Load:** +1..2% on small templates (one pass over refs using pre-hashed names); gate +2%.
- **Risk:** medium. A wrong resolution is caught by the Debug check, and an unresolved node is
  always correct.
- **Size:** P0 ~40, P1a ~150, P1 ~950, P2 ~300, P3 ~300 LOC.

### C. Full Python-style resolution (after 0038)
B plus:
- Root-frame slots, published by name through a static name index per unit (built at Load).
  Includes, blocks, `extends` parents and imports read them through that index.
- Macro and call bodies close over the defining frame. Frames that are captured leave the
  arena for shared ownership (`std::shared_ptr<SlotBlock>`); handles stay the access path.
- Free names are resolved once per unit call into a `boost::span<LookupResult>`, like
  Jinja2's `resolve` at frame entry.
- Then the `LookupCache` goes and most scope maps go.

Estimates:
- **Gain:** B plus `many_tags` -15..-25%, maybe `inheritance` -5%.
- **Risk:** high. It reworks `extends`/blocks/`self` (the parent shares the child's root
  scope today) and import export.
- **Size:** 2.5-3.5k LOC.
- **Parity:** it is where Jinja2's quirks would be reproduced exactly (decision 6).

**Recommendation: B now (P0-P3), C's pieces as P4 once 0038 has landed and the numbers ask
for it.** B's runtime (arena, frames, handles, frame views, resolver) is the base C builds on,
so none of it is thrown away. A's bucket epochs remain an option if `many_tags` matters
before P4.

## 4. Design of B

### 4.1 Load
- `ExpressionParser` gets a name sink. Every name node it makes is appended
  (expression_parser.cpp:559). The direct makers in template_parser.cpp (trans, the `_`
  call) register too, or stay dynamic.
  - Before 0118, the sink is `std::vector<std::shared_ptr<ValueRefExpression>>`: Load only,
    one refcount increment per name.
  - After 0118, it is `std::vector<NodeRef<ValueRefExpression>>` (4.4).
- `StatementsParser`/`TemplateParser` keep a frame stack next to `StatementInfoList`. Each
  frame records:
  - its unit,
  - its binder names (for target, `loop`, parameters, specials, `with` targets),
  - its map-store names (`set`, macro name, import aliases).

  `if` and `else` add no frame. Each statement assigns its sub-expressions to frames:
  - `for`: the iterable to the outer frame, the filter to a filter frame.
  - `macro`: defaults to the macro unit when `defaultRefersToArgs`, else to the outer frame.
  - `call`: the arguments to the outer frame, the body to its own unit.
  - `with`: the values to the outer frame.

  A `recursive` for loop, and everything inside it, adds nothing: all of it stays dynamic.
- At the end of the parse, for each ref (name N, frame F): walk F towards the unit root; take
  the first frame B that binds N.
  - Resolve to `Slot(B, N)` iff B is in F's unit, no frame from F to B map-stores N, and N is
    not map-stored in B's own runtime scope. That last test covers macro parameters
    reassigned with `set`, which share the macro's scope.
  - Otherwise the ref stays dynamic.
  - A binder that is map-stored in its own scope is not given a slot at all.
- `UnitLayout` (immutable after Load): one `SlotName` (name, hash) per slot, the frame ranges
  as `SlotRange{SlotIndex first; uint16_t count}`, and the size. The unit's statement owns it.
  Before 0118 it holds a `std::vector<SlotName>`; after 0118 an `ArenaSpan<SlotName>`.
  Statements refer to a layout by `UnitId`.
- The name node gains `SlotIndex m_slot = SlotIndex::Dynamic` and, in Debug, a `UnitKey`
  (4.4). Both are plain integers, so trivially destructible, as 0118's arena requires.

### 4.2 Types and render path

```cpp
// src/slot_frame.h (internal). boost::span from boost/core/span.hpp (Boost >= 1.78; 1.92 pinned)
enum class SlotIndex : uint16_t { Dynamic = 0xFFFF }; // made only by the resolver
enum class UnitId : uint16_t {};                      // a unit's layout in its template
using Slot = std::optional<InternalValue>;            // disengaged = not bound (yet)

// The slots of one unit call. A view: valid only inside the call that took it from the
// arena (held by value in RenderContext and in scope entries, which nest inside the call)
class SlotFrame
{
public:
    SlotFrame() = default;
    Slot& operator[](SlotIndex idx) const { return m_slots[static_cast<size_t>(idx)]; } // BOOST_ASSERT bounds
    boost::span<Slot> Range(SlotRange r) const { return m_slots.subspan(static_cast<size_t>(r.first), r.count); }
#ifndef NDEBUG
    UnitKey Unit() const { return m_unit; }
#endif
private:
    friend class SlotArena;
    boost::span<Slot> m_slots;
#ifndef NDEBUG
    UnitKey m_unit{};
#endif
};

// A frame named from outside the call that owns it (the loop filter adapter; P4 closures).
// Resolving it checks the generation, so a handle to a returned frame fails loudly.
class FrameHandle
{
public:
    SlotFrame Resolve(const SlotArena& arena) const; // assert + throw on a stale generation
private:
    uint32_t m_offset = 0;
    uint16_t m_size = 0;
    uint32_t m_generation = 0;
};

// Where a lookup found a value. A nullable reference that cannot be stored as a pointer,
// offset or deleted; valid until the scopes next change (the 0088 S5 contract)
class [[nodiscard]] LookupResult
{
public:
    LookupResult() = default; // not found
    explicit operator bool() const noexcept { return m_value != nullptr; }
    const InternalValue& operator*() const { assert(m_value); return *m_value; }
    const InternalValue* operator->() const { assert(m_value); return m_value; }
private:
    friend class RenderContext;      // the only places that make one
    friend class SlotFrameLookup;
    explicit LookupResult(const InternalValue& v) : m_value(&v) {}
    const InternalValue* m_value = nullptr;
};
// MutableLookupResult: the same over InternalValue&, returned by FindForWrite (ex FindValueSlot)

LookupResult ValueRefExpression::EvaluateRef(RenderContext& values)
{
    if (m_slot != SlotIndex::Dynamic)
    {
        const SlotFrame& frame = values.Frame();
        assert(frame.Unit() == m_unit);           // the unit's own frame is installed
        if (const Slot& slot = frame[m_slot])     // bounds-checked in Debug
        {
            // Debug: assert(&*slot == &*values.FindValue(GetHashedName()))
            return values.Found(*slot);
        }
    }
    return values.FindValueCached(Key(), GetHashedName()); // unchanged dynamic path
}
```

Notes on the types:
- **Why `LookupResult` keeps one private pointer.** A nullable reference is a pointer at the
  machine level, so the alternatives are its spellings.
  `std::optional<std::reference_wrapper<const InternalValue>>` is 16 bytes and adds a flag
  test. 0129 measured 4-8% swings on `many_tags`/`macros` from codegen changes in the inlined
  lookup.
  `LookupResult` is 8 bytes, trivially copyable and returned in a register, so it costs the
  same as today's pointer. Callers cannot do pointer arithmetic on it, store a raw pointer or
  delete it.
  A Debug build may also carry the epoch and assert at dereference that the scopes did not
  change; that is optional and measured separately.
- **Binding and epochs.** Engaging a slot (`emplace`) starts a new epoch, since it may
  shadow a cached dynamic lookup, exactly as `ScopeRef::operator[]` does now. Assigning to an
  engaged slot (each loop pass) does not. Resetting a frame range with engaged slots starts a
  new epoch.
- **`FindValue` (P1a)** returns `LookupResult`. For a scope entry with a frame view it
  compares the view's few pre-hashed names and skips disengaged slots, before the map. Keep
  the 0129 loop shape and check `many_tags`/`macros`.
- **Writes through the dynamic path.** `FindForWrite`, `set ns.attr` and `ContextMapper` reach
  slots through the same view. So in-place mutation (0020) and namespaces keep working.
- **The `for` statement** writes `loop` and its targets through `frame[SlotIndex]`.
  `LoopTargetSlots` (a vector of `InternalValue*` into map nodes) and its
  `static_assert(!is_flat)` (statements.cpp:204) are deleted. The `loop` object keeps one
  owner in its slot, so the `state.use_count() > 2` test (:546) is unchanged.
- **The loop filter adapter** holds the `FrameHandle` of its unit call. Each call does
  `ReinstallFrame guard(values, handle.Resolve(arena))`: the frame goes in by value and is
  restored on exit.

**Allocations and copies on the render path:**
- Nothing new per render (the inline first arena chunk) or per loop or macro entry.
- Removed: the robin_hood key `std::string` per insert, and the node table work for `loop`,
  targets and parameters.
- Values are constructed in slots (`emplace`) exactly where they were copied into map nodes,
  so the number of copies is unchanged.

**Cost of the safe types against rev. 1** (estimated; P1 measures it):
- `Slot` is 96 bytes instead of 88. That is memory only; the engaged test replaces the mask
  test.
- `RenderContext` holds a `SlotFrame` (16 bytes) instead of a pointer. Nested contexts copy
  it: two words.
- Scope entries gain two spans, 32 bytes, in raw chunk storage, so constructing a context
  touches none of it.
- Reaching the arena through the callback costs one virtual call per unit entry, about 5
  instructions per macro call (<0.1% of `macros`).
- `FrameHandle::Resolve` compares one generation per filter call (`for_filter_if` about
  +0.2%).
- Bounds checks and unit checks exist in Debug only.
- Net: the estimates in section 3 stand, with ±0.5% set aside for these types.

### 4.3 Pointer inventory

| Pointer | Origin | Decision |
|---|---|---|
| `Activation::slots` (`InternalValue*`) | rev. 1 | **Replaced** by `SlotFrame` (span), by value |
| Bound mask plus `const uint64_t*` in frame views | rev. 1 | **Replaced** by `std::optional` per slot |
| `RenderContext::m_activation` | rev. 1 | **Replaced** by `SlotFrame m_frame` by value |
| `RenderContext::m_slotArena` | rev. 1 | **Dropped**: the arena belongs to the per-render `RendererCallback` (`GetSlotArena()`) |
| `ValueRefExpression::m_unit` (`const UnitLayout*`) | rev. 1 | **Replaced** by a Debug-only `UnitKey` value (4.4) |
| Frame view in a scope entry | new | Two `boost::span`s, values and names. Each entry is popped before its unit call returns (strict nesting); Debug asserts at pop that the view lies in the installed frame |
| Name sink | new, Load only | `shared_ptr` before 0118, `NodeRef` after; no raw node pointers |
| Loop filter adapter's frame | new | `FrameHandle`, generation-checked. The adapter's existing captures, `this` and `&values`, are unchanged: the adapter dies with `RenderLoop`'s locals, because a kept `loop` collects the rest first (:546). 0118 replaces `this` for objects that escape |
| `loop(...)` recursion callable | existing (`ForStatement*`) | Gets **no** frame: recursive loops are dynamic (an escaped `loop` is callable after its loop, see the revision note). `ForStatement*` becomes a NodeRef plus `shared_ptr<const TemplateImpl>` under 0118 |
| `LoopTargetSlots` (`InternalValue*` into map nodes) | existing | **Deleted** in P1 |
| `FindValue` returns `const value_type*` | existing | **Replaced** by `LookupResult` in P1a |
| `EvaluateRef` returns `const InternalValue*` | existing (0088 S5) | **Replaced** by `LookupResult` in P1a (about 25 callers) |
| `FindValueSlot` returns `InternalValue*` | existing | **Replaced** by `MutableLookupResult FindForWrite` in P1a |
| `LookupCache` key: node address as `const void*` | existing (0100) | **Kept until 0118**: an opaque hash key, never dereferenced, with `Forget` for reused addresses. **After 0118**: a `uint64_t` NodeKey packing (TemplateKey, NodeRef index); `Forget` only for render-time nodes (4.4) |
| `LookupCache::Entry::slot` (`const InternalValue*`) | existing | **Becomes `LookupResult`** in P1a. It is the cache's payload, valid by epoch, and every hit is re-checked in Debug. **Removed in P4** with the cache |
| `RenderContext::m_parent`, `m_externalScope`, `m_globalScope`, `m_builtinScope`, `m_boundScope`, `m_currentScope`, `m_rendererCallback`, `m_templateFrame`, `m_lookupCache` | existing | **Not widened by 0117.** P0 adds only an integer (`m_boundDepth`). A separate cleanup can make the non-nullable ones references and the nullable ones `optional<reference_wrapper>` (decision 11) |
| P4 closures | future | Captured frames move to `std::shared_ptr<SlotBlock>`; access stays through handles or spans scoped to a call |

### 4.4 Nodes before and after 0118 (`NodeRef<T>`)

| Piece | Before 0118 (`shared_ptr` nodes) | After 0118 (`NodeRef<T>`, 32-bit index into the template arena) |
|---|---|---|
| Resolver input (name sink) | `std::vector<std::shared_ptr<ValueRefExpression>>` | `std::vector<NodeRef<ValueRefExpression>>`; the resolver writes through `arena[ref]` |
| Name node fields | `SlotIndex m_slot`; Debug `UnitKey m_unit` | Same; trivially destructible, suits 0118 P5 |
| Unit layouts | `std::vector<SlotName>` in the unit's statement; `UnitId` = index in the template's layout table | `ArenaSpan<SlotName>`; `UnitId` unchanged |
| `UnitKey` (Debug: the installed frame belongs to this node's unit) | (Load generation from a global atomic counter, `UnitId`) | (0118's TemplateKey, `UnitId`) |
| Statement slot fields (for targets, parameters) | `SlotIndex` in a `small_vector` | `ArenaSpan<SlotIndex>` |
| `LookupCache` key | node address, `Forget` in `~ValueRefExpression` | Packed (TemplateKey, NodeRef index); render-time nodes (the `_` alias) are a separate type and keep `Forget` (0118's own rule) |
| Macro, `super`, `self`, `loop()` callables that escape | capture `this` (existing) | `shared_ptr<const TemplateImpl>` plus NodeRef (0118); 0117 adds no node capture of its own |

If P1 lands before 0118 P4/P5, moving to NodeRef costs 0117 two mechanical edits: the sink's
element type and the cache key.

### 4.5 What should back the scope maps (robin_hood, Boost.Unordered, flat maps, interned names)

**Context.** (Boost.Flyweight was considered and dropped; see the symbol table below.)
- `src/robin_hood.h` is vendored, and its upstream is archived.
- `InternalValueMap` (src/internal_value.h:790) is a robin_hood node map. It is used for scopes
  and also for dicts, kwargs and attributes.
- The robin_hood-specific uses are few: `mask()` in `ExitScope` (render_context.h:393),
  `static_assert(!is_flat)` (statements.cpp:228), and `hash_bytes` for `HashedName`
  (internal_value.h:737).
- Scope storage must be node-stable while an expression runs. `EvaluateRef` (0088 S5) and the
  lookup cache keep references into nodes.
- martinus/map_benchmark measures large maps. Scope maps are small (1-16 names) and cycle:
  build, a few lookups, clear, once per loop or macro entry, into a map reused since 0100 B2.

**Microbenchmark of that shape.** It is in this session's scratchpad (`mapbench/scopebench.cpp`),
outside the checkout. GCC 13 -O3, C++20, callgrind.
- One cycle: insert N names into a reused, cleared map; look each name up twice (hits); make
  two lookups of an absent name (the walk-through of a cache miss); clear.
- Values are 88 bytes with a refcounted member, like `InternalValue`.
- String keys are looked up by a pre-hashed name (today's `HashedName`/`NameEqual`; Boost is
  told the hash avalanches). Interned keys are 32-bit ids.
- Instructions per cycle; allocations per cycle in brackets when not 0:

| Container | N=1 | N=2 | N=4 | N=8 | N=16 | Node-stable |
|---|---:|---:|---:|---:|---:|---|
| robin_hood node map (today) | 660 | 1,012 | 1,750 | 3,291 | 6,348 | yes |
| boost::unordered_flat_map | 566 | 940 | 1,691 | 3,211 | 6,248 | no |
| boost::unordered_node_map | 724 (1) | 1,250 (2) | 2,305 (4) | 4,532 (8) | 9,191 (16) | yes |
| std::unordered_map (heterogeneous find needs C++20) | 602 (1) | 1,257 (2) | 2,583 (4) | 5,045 (8) | 11,975 (16) | yes |
| boost::container::flat_map (small_vector<8>) | 610 | 1,376 | 2,833 | 7,286 | 18,351 | no |
| linear small_vector<8> of (hash, std::string, value) | **345** | **602** | **1,161** | **2,482** | 5,811 | only if never reallocated |
| linear small_vector<8> of (interned id, value) | **219** | **377** | **738** | 1,640 | 4,164 | only if never reallocated |
| boost::unordered_flat_map<id, V> | 409 | 618 | 1,038 | 1,878 | **3,558** | no |
| boost::unordered_node_map<id, V> | 551 (1) | 910 (2) | 1,628 (4) | 3,164 (8) | 6,443 (16) | yes |

Findings:
- **`boost::unordered_node_map` is the only node-stable Boost drop-in, and it loses.** It is
  +10..45% per cycle and allocates one node per insert per cycle. robin_hood recycles nodes
  through its pool, which 0100 B2 and 0133 rely on. Its finds alone are faster: hit 55 vs 66
  instructions, miss 32 vs 60, in a 2-name map (first run).
- **`boost::unordered_flat_map` only ties robin_hood here (-1..-14%), and is not node-stable.**
  map_benchmark's lead for flat maps comes from large maps.
- **`std::unordered_map` and `boost::container::flat_map` are rejected.** The flat map does
  ordered string compares.
- **A linear scan with a hash pre-check wins at every scope size** (-48% at N=1, -34% at N=4,
  -8% at N=16), and allocates nothing within its inline capacity. Scopes only add names or
  clear, never erase one, so a grow-only store in fixed chunks never moves an entry, which
  makes it node-stable. This is the same pattern as `ScopeStack` (0129).
- Engine-level swap (robin_hood -> `boost::unordered_node_map` for `InternalValueMap`, the
  rest unchanged, master 62dda01, `count.py` per render): see the table below.

| Render case (master 62dda01) | robin_hood | boost node map | Change | Allocations |
|---|---:|---:|---:|---|
| macros | 1,180,492 | 1,284,593 | **+8.8%** | 408 -> 1,214 |
| substitute | 4,111 | 4,426 | +7.7% | 2 -> 7 |
| plain_text | 2,333 | 2,441 | +4.6% | 2 -> 4 |
| many_tags | 1,293,874 | 1,351,444 | +4.5% | 3 -> 312 |
| for_filter_if | 518,203 | 531,291 | +2.5% | 207 -> 351 |
| dict_ops | 413,668 | 422,556 | +2.2% | |
| config_file / html_autoescape / inheritance | | | +1.4 / +1.3 / +1.3% | +513 / +278 / +117 |
| mitsuhiko_table | 7,263,513 | 7,164,706 | **-1.4%** | +10 (0133 keeps names, so only finds remain) |
| expressions / strings / for_range | | | -0.1 / 0.0 / -0.6% | |

The prototype swapped every `InternalValueMap`, dicts included. It was built in a scratch
copy of master 62dda01 outside the checkout: `is_avalanching` hash, `bucket_count()` for the
spare-scope test, `static_assert(!is_flat)` removed. It confirms the microbenchmark: Boost's
faster finds do not pay for one node allocation per insert.

**Recommendation for the slots-plus-dynamic-rest design.** After P1 and P2 the hot scopes
(loop variables, parameters) are slot frames, and a by-name scan of a frame is already this
linear shape over `boost::span<const SlotName>`. What is left:
1. **Residual local scopes:** the template root, `set` in bodies until P3, and `with`,
   set-block and filter-block scopes. Use a new internal `ScopeMap`:
   - grow-only, 8 entries inline, further chunks of 8 kept until the scope ends;
   - entries `(hash, name, InternalValue)` scanned with the hash pre-check;
   - a small index (`boost::unordered_flat_map<size_t hash, uint32_t entry>`, or open
     addressing) built when a scope passes 16 names. Root scopes such as `many_tags` reach
     30 or more.

   About 200 LOC plus unit tests, as **P5** (after P2, measured on its own). This removes
   robin_hood from the scope path without a dependency.
2. **External, global and built-in maps** (built before the render, read-only during it,
   tens to hundreds of keys): `boost::unordered_flat_map`. Its storage does not move while
   nothing is inserted; Debug asserts the size is unchanged during a render. This is where
   map_benchmark's large-map results apply.
3. **Dicts, kwargs, attributes:** a separate type and a separate decision. Insertion order is
   0031's question. 0117 only splits `InternalValueMap` into `ScopeMap` and the value-map type
   so that the two can diverge.
4. **`HashedName`'s hash:** use `boost::hash<std::string_view>` (Boost ≥ 1.81 string hash,
   avalanching) or a 20-line copy of `hash_bytes`, measured on Load. Then robin_hood.h can be
   deleted once dicts move too.

**Interned names: 0118's per-template `SymbolId` (Flyweight is dropped).** 0118's arena will
hold a symbol table per template: names are `string_view`s into the source, each mapped to a
32-bit `SymbolId`, immutable after Load. What that does to each piece:
- **Slots:** nothing on the hot path. Slots are positional (`SlotIndex`), so no name is
  compared. `SlotName` becomes `{SymbolId, hash, string_view}`. A by-name scan of a frame
  view compares ids when the reader belongs to the same template, which is the
  `LinearId` row, about 35% cheaper than `LinearStr`.
- **Residual `ScopeMap`:** entries are keyed `{SymbolId, hash, string_view}` and tagged with
  their template's key. A lookup from the same template compares ids. A lookup from
  another template compares the hash, then the string (`LinearStr`), because ids are
  per-template.
  - Cross-template readers: an `include` with context reading the caller's scopes, an
    `extends` parent reading the child's root, an imported macro "with context", the
    module names `import` copies into the importer.
  - Names written at run time without a symbol (`$$_imported_*`, names a module exports into
    the importer) carry no id and only ever take the string path.
- **Lookup cache:** stays keyed by expression site, (TemplateKey, NodeRef) after 0118, not by
  `SymbolId`. One symbol read at two sites can resolve to different bindings, and the
  `(site, epoch)` key is what keeps a hit safe. With P4, free names are resolved once per
  unit call per `SymbolId` into a `boost::span<LookupResult>` indexed by a per-unit dense
  number, and the cache goes.
- **Names that arrive at run time, with no `SymbolId`:**
  - user `ValuesMap` keys (the external scope), env globals, built-ins;
  - dict keys and kwargs;
  - strings looked up by `*context` callables, `applymacro`, and user filters and tests by
    name.

  All stay `std::string`-keyed with pre-hashed lookup, as today. A template's free-name node
  carries `hash` + `string_view`, so reading the external, global and built-in maps needs no
  symbol. Interning them per render would cost a table insert per user key per render and
  buys nothing: they are found by string once per site and epoch (today) or once per unit
  call (P4).
- **Does it change the container answer?** No. The chunked linear `ScopeMap` wins with
  either key: -34% vs robin_hood at N=4 with strings, -58% with ids. Ids widen the margin
  only for same-template lookups. `boost::unordered_flat_map<id>` wins at N=16 but is not
  node-stable.

**Build cost of the Boost pieces:**

| Piece | internal (FetchContent, 1.92) | Conan (boost/1.91.0) | external mode |
|---|---|---|---|
| Boost.Unordered (`unordered_flat_map`) | already configured transitively (the `boost_unordered` target exists, thirdparty-internal.cmake:42; internal_value.h already includes boost/unordered_map.hpp); list `unordered` in `BOOST_INCLUDE_LIBRARIES` explicitly | header-only, in the package | needs Boost ≥ 1.81 (flat) / 1.82 (node); add `find_package(boost_unordered)` / `Boost::headers`; raise the floor |
| `boost::span` (core) | transitive already | in the package | Boost ≥ 1.78 |
| Boost.Container (only if `small_vector` is used inside `ScopeMap`) | already built and linked (`libboost_container.a`) | in the package (compiled) | add the component |

## 5. Steps (one PR each, measured with `bench/count.py --baseline`, Render and Load)

| PR | Content | Files | Expected | Tests |
|---|---|---|---|---|
| **P0** (parity, 0038 area) | Search the bound module scope below the imported macro's own scopes: record `m_boundDepth` at `BindScope`, check the bound map once the walk passes it, also on the write path | render_context.h | neutral | new corpus `loader.from_import_param_shadows_module_name`; existing `loader.import_*` |
| **P1a** (mechanical) | `LookupResult` / `MutableLookupResult`: `FindValue`, `EvaluateRef`, `FindValueSlot` (renamed `FindForWrite`) and the cache payload move to them | render_context.h, expression_evaluator.h/.cpp, statements.cpp, filters.cpp, testers.cpp, internal_value.cpp | ±0.5% (same codegen) | existing suite; `Helpers.LookupCache*`; a `Helpers` test that a default `LookupResult` is false |
| **P1** | Name sink, frame stack, resolver (for frames only), `UnitLayout`, `SlotIndex`/`UnitId`, `SlotArena`/`SlotFrame`/`FrameHandle`, `UnitCall`, frame views, Debug checks; `for` target, `loop` and filter frame in slots; recursive loops untouched | template_parser.h/.cpp, expression_parser.h/.cpp, expression_evaluator.h/.cpp, render_context.h, new src/slot_frame.h, statements.h/.cpp, template_impl.h | mitsuhiko -7..-10% (post-0133), for_range/for_loop_vars/expressions/for_filter_if -3..-5%; Load ≤ +2% | rows in the `ForLoopTest` tables (forloop_test.cpp); `Helpers` tests: by-name sees an engaged slot and skips a disengaged one, epoch on engage and reset, a stale `FrameHandle` fails, out-of-range `SlotIndex` asserts (death test, Debug only); corpus cases below |
| **P2** | Macro and call parameters plus `caller`/`varargs`/`kwargs`, `with` targets, block `super` (and `$transN`) in slots; `InvokeMacroRenderer` emplaces into its frame | statements.cpp, template_parser.cpp, render_context.h | macros -20..-25%, config_file -8%, html_autoescape -4..-6% | rows in `MacroTest` (macro_test.cpp: parameter reassigned by `set`, nested macro reading an outer parameter as in `ClosureMacro`); corpus below |
| **P3** (measure first; may be rejected) | `set` targets in non-root frames as slots: the for body resets its range each pass, a read of a disengaged slot falls back outward through a static chain of `SlotIndex`es, today's semantics | template_parser.cpp, statements.cpp, render_context.h | chat_* / for_filter_if -1..-4% | `SetTest` rows (statements_tets.cpp); corpus `for_target_shadowed_by_body_set` etc. |
| **P5** (after P2; measure) | Split `InternalValueMap` into `ScopeMap` and the value-map type; `ScopeMap` = chunked linear store, `{SymbolId, hash, string_view}` entries once 0118's symbols exist (hash plus `std::string` before), an index past 16 names; external, global and built-in maps -> `boost::unordered_flat_map` (size frozen during a render, Debug-asserted); `HashedName` hash -> `boost::hash<std::string_view>` or a local copy | internal_value.h, render_context.h, new src/scope_map.h, template_impl.h, statements.cpp; thirdparty-internal.cmake (`unordered`), external_boost_deps.cmake (floor 1.81) | microbenchmark: -34% per scope cycle at 4 names; engine: residual scopes after P2 (root, `set` bodies); many_tags and macros first to check | `Helpers` unit tests for `ScopeMap` (stability across growth past a chunk, index past 16, cross-template string path); the full suite and corpus |
| **P4** (after 0038) | Option C pieces: root slots plus name index, closures over frames, free-name resolution per call, remove `LookupCache` | wide | many_tags -15..-25% | 0038's corpus lines; P4 gets its own plan |

Coordination:
- **0133 has landed** (#412). P1 replaces its pooled loop-scope maps with slot frames and
  keeps its frame and enumerator pooling. Measure against 62dda01 or later.
- **0118** (parse-tree arena) wants one thread to own expression_evaluator.h, statements.h
  and template_parser.* from its P3 to P5.
  - P0 touches only render_context.h, so it can go now.
  - P1a touches expression_evaluator.h, so it is either one small PR merged before 0118 P3
    starts, or it is done by the thread that owns those files.
  - P1 should follow 0118 P3; decision 4.

## 6. Parity risks and proof

**Main proof:**
- The Debug check (slot reference == by-name result, frame unit == node unit, `SlotIndex` in
  bounds) runs on every slot read.
- Run it over `ctest` and the parity corpus, narrow and wide (`ParityWide`), in the Debug CI
  jobs.
- During the rollout, also run it in the fuzz job and the sanitizer job. Both build
  RelWithDebInfo, so they need an internal `JINJA2CPP_CHECK_SLOTS` define that also enables
  `BOOST_ASSERT` there (decision 8).

**New corpus cases.** All checked against both engines on 2026-10-06.

Cases that match today and pin what the resolver must keep, in `statements.py` unless noted:
- `for_target_shadowed_by_body_set`:
  `{% for x in [1,2] %}{{ x }}{% set x = x * 10 %}{{ x }}{% endfor %}` gives `110220`.
- `for_nested_same_target`, which gives `a1a2`.
- `for_filter_reads_outer_target`:
  `{% for o in [1,2] %}{% for x in [1,2,3] if x != o %}{{ o }}{{ x }}{% endfor %}{% endfor %}`
- `loop_length_filtered_from_macro`:
  `{% macro m(l) %}{{ l.length }}{% endmacro %}{% for x in [1,2,3] if x > 1 %}{{ m(loop) }}{% endfor %}`
  The filter runs inside the macro, through its `FrameHandle`.
- `recursive_loop_else_reads_outer` gives `<O<OEO>>`.
- `recursive_loop_via_macro_reads_outer` gives `OO`.
- `recursive_loop_called_after_loop` gives `[1][5]`.
- `with_target_set_in_body`, which gives `12`.
- `with_targets_see_outer`: `{% set a = 5 %}{% with a = a + 1, b = a %}` gives `65`.
- `macro_param_set_in_body`, which gives `12`.
- `call_param_shadows_loop_target`, which gives `1`.
- `call_body_reads_loop_target`, which gives `12`.
- `set_block_in_loop_reads_target`, which gives `12`.
- `filter_block_in_loop`, which gives `A`.
- `tuple_target_partly_shadowed`, which gives `15`.
- `loop_target_namespace_attr`, which gives `2`.
- `macro_param_vs_caller_loop_var`, which gives `31`.
- `macro_param_inside_with`, which gives `2`.
- `set_in_if_inside_loop`, which gives `[][2]`.

New divergence lines, which pin 0038's semantics so that P4 has to change them on purpose:
- `macro_reads_caller_loop_target`: Jinja2C++ gives `12`, Python `''`. Owner 0038.
- `macro_reads_caller_loop`: Jinja2C++ gives `12`, Python raises UndefinedError. Owner 0038.
- `loader.include_sees_unreferenced_loop`: Jinja2C++ gives `[11][22]`; Python raises
  UndefinedError, because `loop` exists only when the body references it. Owner 0133, or a
  0117 follow-up (decision 5).
- The idtracking quirks of section 2, under 0038.

**Specific risks:**
- *A unit rendered without its frame installed.* The paths are the filter adapter, the no-frame
  block path (statements.cpp:759) and the `super`/`self` callables. The unit-key assert
  catches all of them.
- *A view that outlives its call.* Views are held only by value in `RenderContext`, in scope
  entries and in locals. Anything stored beyond the call holds a `FrameHandle`; this rule goes
  in a header comment and is checked in review. ASan's `detect_stack_use_after_return` and a
  generation bump on every frame return catch slips.
- *Stale cached results after a reset or a returned frame.* Prevented by the epoch on reset
  or return; the Debug cross-check catches it.
- *Exceptions.* `UnitCall` and range resets are RAII. ASan with `detect_leaks` covers macro
  errors thrown mid-binding (`CheckMacroCallArgs`) and filter errors in the adapter.
- *Recursion.* Macro recursion to `MaxRenderDepth` (256) takes slots from the arena, not the
  C++ stack.
- *Threads.* Layouts and node fields are written only during `Load`, before a template is
  published (env cache, `LoadTemplate` per render, 0105). The arena is per render. `MT/Render`
  scaling should be unchanged.
- *Malformed templates.* Resolution runs only after a successful parse. Fuzz corpora go
  through the Debug check.
- *External Boost mode.* `boost/core/span.hpp` needs Boost ≥ 1.78; the internal pin is 1.92
  and Conan uses 1.91. Add a version floor to the external `find_package` (one line in P1).

## 7. What this makes hard next

- Two descriptions of scoping have to stay in sync: the parser's frames and the statements'
  runtime scopes. Every new binding construct must register in both. The Debug check catches
  drift only where tests exercise it.
- Lexical closures (0038/P4) need frames that outlive the LIFO arena. With handles as the only
  long-lived access path, a captured frame can move to shared ownership without changing its
  readers. The rule "views only inside the call" must hold from P1 on.
- The `ScopeStack` entry gets wider (two spans), and the inlined `FindValue` gains a branch.
  0129's tuning has to be redone once.
- `LookupResult` becomes the vocabulary type for "a value found in place". Later code (0118's
  node accessors, P4's per-call free-name table) should use it rather than reintroducing
  pointers.
- With A or P4 the lookup cache could become per call. Until then, both mechanisms exist.

## 8. Decisions for the owner

1. **Re-target "Done when"** to `mitsuhiko_table` ≥ -7% against post-0133 master (P1), `macros` ≥ -15%
   (P2), no case worse than +1%, Load ≤ +2%, dropping "-10% on `expressions`"?
   *Recommend yes; `expressions` is the operator walk (0100 idea 8, 0118).*
2. **Start before 0038?** *Recommend yes for P0-P3, which are intra-unit and so invariant
   under 0038; P4 waits for 0038.*
3. **P0 (imported macro parameter shadowed by a module name):** a standalone parity PR
   under 0038, landing first? *Recommend yes; P1's Debug check needs it.*
4. **Order against 0118:** P0 now; P1a before 0118 P3 or by its owning thread; P1 after 0118
   P3. *Recommend this order. Moving P1 to NodeRef later is two mechanical edits (4.4).*
5. **Elide the `loop` object when no part of the body's unit references it?** This is Python
   parity (includes and macros then stop seeing `loop`) and saves an allocation per loop
   entry. *Recommend yes, as a follow-up PR after P1, coordinated with 0133.*
6. **Reproduce Jinja2's idtracking quirks** (section 2, point 3), or keep the current outward
   fallback? *Recommend keep in the perf phases and file them as divergences under 0038.*
7. *(Dropped: there is no slot cap now that `std::optional` replaces the bitmask; `SlotIndex`
   is 16-bit.)*
8. **Debug checks also in the fuzz and sanitizer jobs** through `JINJA2CPP_CHECK_SLOTS`
   (which also turns on `BOOST_ASSERT`) during P1-P3? *Recommend yes; drop it one release
   after P3.*
9. **`*context` callables keep seeing loop and macro locals** (Python's `pass_context` does
   not)? *Recommend keep for now and file a parity task; a perf PR changes no behaviour.*
10. **Spans:** use `boost::span` (already pinned, no new dependency) in src/ only, never in
    `include/`? *Recommend yes; the same choice as the revised 0118.*
11. **The existing raw pointers in `RenderContext`** (parent, scopes, callback, cache): clean
    them up in a separate task (references, or `optional<reference_wrapper>` where nullable)
    rather than inside 0117? *Recommend yes, filed as its own task after P2, so that perf PRs
    stay behaviour- and codegen-neutral apart from their own change.*
12. **`LookupResult` keeps one private pointer** (8 bytes, register-returned) rather than
    `optional<reference_wrapper>` (16 bytes)? *Recommend yes; it measured as codegen-sensitive
    in 0129. P1a can measure both if you prefer.*
13. **Scope containers:** keep robin_hood until P5, then a chunked linear `ScopeMap` for
    scopes, `boost::unordered_flat_map` for external, global and built-in maps, and no
    `boost::unordered_node_map` drop-in (+8.8% `macros`, +4.5% `many_tags`, measured)?
    *Recommend yes. robin_hood leaves the scope path in P5 and is deleted when dicts get
    their own type.*
14. **`SymbolId` in slots and scopes:** use 0118's per-template ids as the fast equality test
    for same-template lookups, keep the hash and string path for cross-template readers and
    for runtime names (user keys, globals, `*context`), and keep the cache keyed by
    expression site? *Recommend yes. Runtime names are never interned per render.*
