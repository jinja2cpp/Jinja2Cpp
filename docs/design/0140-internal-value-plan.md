# 0140 `InternalValue` size: plan

Master 50dec81, GCC 13 x86-64, Release. Plan only. No code before wave 2's P5c merges.

## 1. The finding: the copy protocol costs more than the bytes

`sizeof(InternalValue)` is 72 B: the `std::variant` is 48 B (its biggest alternative is
`TargetString` at 40 B, a variant nested in the variant), `m_parentData` is 16 B, and 2 flag
bytes are padded to 8.

The task assumed most of the cost was in the bytes: cache lines, and lists 2.25x denser.
The measurements point somewhere else. Every copy, move and destroy of a value goes through
`std::variant`'s jump table (or two of them for assignment) plus the `m_parentData`
`shared_ptr`, even when the value is just an `int64_t`.

An analogy: today, passing a number from one hand to the next is a trip through a customs
desk that asks what is in the parcel, stamps it and re-wraps it. The plan replaces that with
a claim ticket. The ticket holds the number or the view itself, and only real luggage (a
heap object) gets a refcount.

### 1.1 How many copies, moves and destroys a render makes

These come from a scratch build that counts `InternalValue` special members by alternative,
per render (`--count` mode, 5 iterations):

| case | copies | moves | dtors | allocs | copies by kind (top 4) |
|---|--:|--:|--:|--:|---|
| chat_llama | 210 | 826 | 1,428 | 348 | Map 121, TStr 44, Undef 18, bool 14 |
| chat_mistral | 555 | 1,510 | 2,804 | 474 | TStr 205, Map 195, int 56, bool 43 |
| chat_qwen | 134 | 983 | 1,521 | 337 | Map 46, bool 36, TStr 19, Undef 18 |
| config_file | 1,576 | 3,735 | 6,679 | 1,703 | TSV 336, Undef 285, bool 252, Map 233 |
| dict_ops | 411 | 771 | 1,239 | 315 | TSV 200, KVP 100, int 53, TStr 50 |
| expressions | 457 | 208 | 2,018 | 3 | int 403, bool 51, Undef 3 |
| filters | 37 | 158 | 279 | 46 | TSV 23, TStr 5, List 4, bool 3 |
| for_filter_if | 621 | 1,100 | 1,921 | 206 | Map 617 |
| for_loop_vars | 204 | 1,104 | 1,907 | 2 | TStr 100, TSV 100 |
| for_range | 106 | 108 | 118 | 3 | int 103 |
| html_autoescape | 780 | 2,627 | 4,982 | 1,011 | TStr 315, bool 185, int 120, Map 81 |
| inheritance | 105 | 207 | 414 | 76 | TStr 51, Map 50 |
| macros | 1,007 | 3,008 | 3,516 | 407 | TStr 502, int 201, TSV 200, Map 100 |
| many_tags | 300 | 2,402 | 4,204 | 2 | bool 300 |
| mitsuhiko_table | 15,031 | 14,048 | 16,072 | 1,025 | int 10,000, Undef 3,006, List 2,007 |
| strings | 969 | 2,478 | 4,438 | 975 | TStr 546, TSV 408, Undef 11 |
| large_static, plain_text, substitute | 0 | ≤2 | ≤4 | 1 | |

Key: TStr is `TargetString`, TSV is `TargetStringView`, KVP is `RecursiveWrapper<KeyValuePair>`.

What the counts say:

- **Moves outnumber copies about 2 to 1, and almost all traffic is cheap kinds**: int, bool,
  undefined, views and lists/maps (refcounted). `mitsuhiko_table` does about 1.4 copies,
  1.4 moves and 1.6 destroys per cell, which is already lean. The count is not the problem.
  The cost of each operation is.
- **`m_parentData` was copied 0 times across the whole suite.** We pay 16 B per value and a
  `shared_ptr` check on every operation for a feature no benchmark exercises.
- **`Callable` and `KeyValuePair` allocate on every copy *and every move*.** The cause is
  `boost::recursive_wrapper`: its move constructor is `new T(std::move(...))`
  (recursive_wrapper.hpp:128). In `config_file` that is about 98 copies plus 98 moves of
  callables and 60+60 of pairs, so about 300 of its 1,703 allocations per render (~18%).
  This is a bug-grade finding with a cheap fix (P1).
- **`InternalValue`'s move is not `noexcept`** (checked with `static_assert`), for the same
  reason: `recursive_wrapper(recursive_wrapper&&)` calls `new`. So every
  `std::vector<InternalValue>` that grows copies its elements instead of moving them, and
  so do `std::optional`/`robin_hood` paths that test `is_nothrow_move_constructible`. Part
  of the copies counted above are those reallocation copies. P1 fixes this first, and the
  counts are re-taken on P1 before P2's estimate is final.

### 1.2 What those operations cost

I rebuilt master with the five special members out of line and `noinline`, then ran
callgrind. Inclusive Ir of the special members, minus the out-of-line overhead (the
difference between that build's total and master's total), gives an estimate of the
inlined share on master:

| case | master Ir/render | special-member share (est.) |
|---|--:|--:|
| mitsuhiko_table | 6,657,461 | 26% |
| strings | 1,061,867 | 30% |
| dict_ops | 401,589 | 33% |
| config_file | 1,917,397 | 28% |
| for_loop_vars | 275,065 | 28% |
| macros | 1,021,823 | 27% |
| for_filter_if | 447,263 | 22% |
| chat_mistral | 708,729 | 21% |
| html_autoescape | 1,614,243 | 17% |
| many_tags | 1,262,450 | 13% |
| expressions | 518,971 | 9% |

"Inclusive" includes the real work hanging off an operation: long-string copies
(malloc+memcpy), `recursive_wrapper` allocations, and freeing a list's items when the last
reference goes. So this is an upper bound on what the representation change can remove.

The per-operation microbenchmark (callgrind Ir, `noinline` wrappers, -O2) compares master
with a 32 B prototype: a 16 B trivially-copyable immediate plus one
`std::shared_ptr<const ValueObject>`:

| operation | master 72 B | prototype 32 B |
|---|--:|--:|
| int copy-assign | 56 | 17 |
| int move-assign | 55 | 19 |
| int copy-construct + destroy | 52 | 23 |
| list copy-assign | 113 | 17 |
| list move-assign | 83 | 31 |
| list copy-construct + destroy | 110 | 38 |
| 40-char string copy-construct + destroy | 281 | as list: refcount, no malloc |

That is 2.5-3.3x fewer instructions for scalars and lists, and about 7x for long strings.
On top of that, values get 2.25x denser in slots, vectors (lists, call arguments), which
callgrind's Ir does not show but the cache-sim column will. Maps gain less:
`pair<std::string, InternalValue>` at 64 B is still over robin_hood's 48 B flat threshold
(robin_hood.h:2532-2535), so scope maps stay node maps. `InternalDict` is a `std::list`
(ordered_map.h:44), so it does not get denser either.

### 1.3 Expected effect (revises the task's -3..-8%)

Removing about 60% of a 9-33% share gives **-5..-20% Render instructions**. List- and
loop-heavy cases sit at the top (mitsuhiko_table about -15%, dict_ops, strings,
config_file, macros about -12..-18%). expressions and many_tags are around -5%.
Confidence: medium for scalars and lists (prototype measured). Low for strings, because the
string creation path changes too (§3, risk R2).

## 2. Target layout (32 B, built from library types)

Revised on 2026-10-10 after Ruslan asked why not use standard, fmt or boost containers. The
first draft used a hand-written union of trivially copyable fields. A library-only layout
measures as fast or faster at the same 32 B, so it replaces the union:

```
class InternalValue                                  // 32 B
{
    Immediate m_imm;                                 // std::variant, 24 B, trivially copyable
    boost::intrusive_ptr<const ValueObject> m_obj;   // 8 B, null for immediate kinds
};
using Immediate = std::variant<UndefinedTag, EmptyValue, bool, int64_t, double,
                               SmallString,   // char[14] + size + flags (16 B): up to 14 chars
                               StringView,    // {const char*, uint32_t len, uint8_t flags}
                               WStringView, ValueRef, UnboundTag,
                               HeapTag>;      // {Kind, flags}: which ValueObject m_obj holds
static_assert(std::is_trivially_copyable_v<Immediate>);   // C++17 (P0602) makes copy a memcpy
static_assert(sizeof(InternalValue) == 32 && std::is_nothrow_move_constructible_v<InternalValue>);
```

`ValueObject` derives from `boost::intrusive_ref_counter<ValueObject, boost::thread_safe_counter>`.
Objects are made by a `MakeRef<T>(args...)` helper, so no call site writes `new`.

Per operation (callgrind Ir, -O2, same harness as §1.2):

| layout | size | int copy= | int move= | int copy+dtor | heap copy= | heap move= | heap copy+dtor |
|---|--:|--:|--:|--:|--:|--:|--:|
| master `std::variant` | 72 | 56 | 55 | 52 | 113 | 83 | 110 |
| `boost::variant2`, same alternatives | 72 | 75 | 72 | 75 | 119 | 101 | 113 |
| trivial `std::variant` + `std::shared_ptr` | 40 | 19 | 20 | 25 | 19 | 32 | 40 |
| hand-written union + `std::shared_ptr` (first draft) | 32 | 17 | 18 | 23 | 17 | 30 | 38 |
| **trivial `std::variant` + `boost::intrusive_ptr` (chosen)** | **32** | **16** | **15** | **22** | **19** | **17** | **25** |
| hand-written union + intrusive (24 B) | 24 | 14 | 13 | 20 | 17 | 15 | 23 |

The library alone does not fix the cost. `boost::variant2` is slower than `std::variant`
here, because a variant with non-trivial alternatives must dispatch on every copy whichever
library implements it. The win comes from the split: every alternative that can be trivially
copyable is, and all ownership sits in one smart pointer. Once that split is made, the
standard and boost types deliver it, and a hand-written union would buy only 2-3
instructions and 8 B. The 24 B union is therefore dropped from P3 (task 0150 becomes "decide
on numbers", not "build").

Other library pieces the plan now uses:
- `fmt::basic_memory_buffer` for 0151 (build a string once, then move it into one
  `StringObject`).
- `boost::intrusive_ref_counter` instead of a home-made refcount.

Kept out, with reasons:
- `boost::container::small_vector` for call arguments: a separate change, not this task.
- `boost::flyweight` or interned strings: lifetime across threads is the hard part, and
  0149 covers the case that matters (template constants).

Flags live inside the alternatives that need them: markup in the string kinds and
`HeapTag`, unbound as its own `UnboundTag`. `IsUndefined()` tests `UndefinedTag` plus
`HeapTag{Undefined-with-info}`.

Kinds:

- **Immediate** (no `m_obj`): Undefined (no info), None, Bool, Int, Double, SmallString,
  StringView, WStringView, ValueRef. Views keep today's borrowing rules.
  `len` is 32-bit. Views made from host strings (`ValueRef`/`GenericList`) are not bounded by
  `MaxSequenceSize`, so a view longer than 2^32-1 becomes a heap String instead of being truncated.
  Undefined-with-info is a heap kind, and visitors receive an `UndefinedRef` view, so dispatch
  takes no refcount. `IsUndefined()` tests both Undefined kinds.
- **Heap** (`m_obj` set; the kind says which `ValueObject` subclass it points to):
  String and WString (`StringObject{std::basic_string}`), List (the `IListAccessor`
  itself, which derives from `ValueObject`, so there is no extra indirection), Map
  (`IMapAccessor`), KeyValuePair, Callable, Renderer, Undefined-with-info
  (`UndefinedInfo` becomes a `ValueObject`), and **Anchored**, which replaces
  `m_parentData`. An Anchored value is `{InternalValue inner; InternalValue parent}` in one
  `make_shared`, the same single allocation `SetParentData` makes today. `parent` is stored
  stripped of its own anchor, which matches today: `SetParentData` keeps only the parent's
  data (internal_value.cpp:33-42, relied on by the a.b.c chain at expression_evaluator.cpp:265-269).
  Accessors forward to `inner`, so readers never see it. Markup stays on the outer flags.
  `SetParentData` on an already-anchored value replaces the parent. `IsEqual` keeps
  comparing parents as today (internal_value.cpp:209-220). Whether that is right is a
  parity question for later, not this task.
- The Renderer kind is a small holder that owns the `shared_ptr<IRendererBase>`.
  `IRendererBase` is an arena node (renderer.h:21) and cannot be a `ValueObject`. The holder
  costs one allocation on a cold path.

Copy is `m_imm = o.m_imm; m_obj = o.m_obj;`: a 24 B memcpy plus a null test, or one atomic
increment. The exception is a list with clonesOnCopy set (generators,
internal_value.cpp:1190, loop filters at statements.cpp:1036/1077). Its copy clones the
accessor first, then assigns, so a throwing clone leaves the target unchanged. That is one
flag test on the copy path. Move steals and sets the source to Undefined. It is guarded
against self-move and keeps the target's unbound bit semantics (a moved-from Slot stays
what it is today). The destructor is a null test. No
jump table remains on the copy path. Everything that owns memory is the one `boost::intrusive_ptr` member, and the variant holds
only trivially copyable alternatives. No manual lifetime management, no manual
refcounting and no hand-written union is introduced. Typed access to `m_obj` goes through
`HeapTag`-checked member functions, with a `dynamic_cast` check in Debug builds. This is the memory-safe shape Ruslan asked for:
the one non-owning pointer per view kind is exactly today's `string_view`/`ValueRef`.

**What the intrusive pointer costs in P2:** the 24 `make_shared<XAccessor>`-style sites
switch to `MakeRef<XAccessor>`. `LoopState`'s `enable_shared_from_this`
(statements.cpp:209/355) becomes `intrusive_ptr(this)`. There is no aliasing constructor,
but the Anchored kind covers that case. No `weak_ptr` is needed for values: TemplateExpired
is about template ownership, not values.

### What changes for code that reads values

There are about 380 access sites in 16 files (`GetIf<` 133, `std::get_if` 49, `std::get` 21,
`std::visit` 9, `Apply`/`ApplyUnwrapped` 81, visitor overloads taking
`const ListAdapter&`/`MapAdapter&`/`std::string&` 77). Most route through `GetIf<T>` and
`Apply<Visitor>`, which become the only seam:

- **Strings:** visitors already handle both `std::basic_string` and
  `std::basic_string_view`, because `ApplyUnwrapped` passes `TargetString`/`TargetStringView`
  today. A heap string arrives as `const std::basic_string&`; SmallString and views arrive as
  `std::basic_string_view`. `GetIf<std::string>` sites become `AsStringView()`, or
  `TakeString()` where they move the string out or edit it in place
  (`ParseAdjacentStrings`, expression_parser.cpp:49-62). `AsStringView() &&` is deleted,
  because a view into a temporary's SSO bytes would dangle.
- **Lists and maps:** `GetIf<ListAdapter>` cannot return a pointer into a value that no
  longer stores a `ListAdapter`. It returns a non-owning `ListRef` (accessor reference plus
  flags) that has `ListAdapter`'s read API through a shared CRTP base. `ToAdapter()` makes an
  owning copy where a site keeps the list. The same goes for `MapRef`. `ListRef` never
  clones, so handing a generator to a visitor cannot advance a copy of it.
- **Accessors become const objects.** `IMapAccessor::SetValue` (internal_value.h:367, used at
  statements.cpp:126) and `ImportedMacroRenderer::InvokeMacro` (statements.cpp:1679-1685)
  are called through non-const pointers today. They become `const` members that mutate
  `mutable` state, as `GetMutableItems` already does, so `m_obj` can stay
  `intrusive_ptr<const ValueObject>` with no `const_cast`. Tuple and fieldNames become
  `CreateAdapter` arguments. All 13 `MarkAsTuple` calls are on fresh adapters, the
  conditional ones included.
- **`GetData()`** (28 sites) disappears. Its callers move to the seam in P0.

## 3. Phases

Each phase is its own PR, gated by the perf thread's CI instruction-count verdict.

| phase | what | touches | effect |
|---|---|---|---|
| **P1** | Make copies and moves stop allocating, and make move `noexcept`: `RecursiveWrapper<Callable>`/`<KeyValuePair>` become `std::shared_ptr<const T>` (`MakeWrapped` keeps its name, so statements.cpp:1317 does not change), plus the nothrow `static_assert`s. Re-take §1.1's counts on P1. | internal_value.h, value_visitors.h | config_file about -18% allocations. Vector growth moves instead of copying. Ir gain measured here, not guessed. |
| **P0a** | Seam outside wave 2's files: `Kind()`, `ListRef`/`MapRef`, `UndefinedRef`, `AsStringView`/`TakeString`; direct `std::get_if`/`std::get`/`std::visit`/`GetData()` moved behind `GetIf`/`Apply`; const accessor mutators; tuple/fieldNames as `CreateAdapter` arguments. | everything except the wave 2 files | Ir flat (±0.5%), layout unchanged |
| **P0b** | The same in wave 2's files, once wave 2 releases them: expression_evaluator.cpp (28 sites, including `holds_alternative<TargetString>` at :421 and `MarkAsTuple` at :734), expression_evaluator.h (`InlineScalar::Holds`, placement-new over `GetData()`, :547-555), statements.cpp (11, including :126 and :1679), template_parser.* (3). Then split `TargetString`/`TargetStringView` into plain alternatives. | wave 2's files, internal_value.* | Ir flat. InternalValue 72 → 64 B. |
| **P2** | The representation in §2: trivial `std::variant` + `boost::intrusive_ptr<const ValueObject>`, Anchored replaces `m_parentData`, Slot's unbound bit becomes `UnboundTag`, `static_assert(sizeof(InternalValue) == 32)`. | internal_value.*, slot_frame.h, value_visitors.h, the 24 accessor `make_shared` sites (LoopState's in statements.cpp, so after wave 2) | -5..-20% Render, re-estimated on P1's counts (§1.3). Slots, vectors and call arguments 2.25x denser. |
| **P3** (optional) | (a) No refcount for template-owned constants (P5c). (b) 24 B only if P2's cache-sim shows density still matters; it would need a hand-written union for 2-3 Ir per op, so the default is no. | internal_value.*, after wave 2 | measured, kept or dropped on numbers (see risk R5) |

Order: P1 → P0a → P0b → P2 → P3. P0b, P2 and P3 need wave 2 finished; P1 and P0a could
start as soon as code is allowed (after P5c).

Follow-up task numbers reserved: **0149** (P3a static constants), **0150** (P3b: is 24 B worth a hand-written union, decided on P2's numbers), **0151** (direct string building into `StringObject`, see R2). I'll file them in the
first code PR.

## 4. Contradictions and how each phase resolves them

1. **Generic, safe visiting (std::variant) vs. cheap copies.** `std::variant` gives
   exhaustive, type-safe visiting, but its non-trivial copy is the dispatch we measured. The
   split resolves it, keeping `std::variant` itself. The variant holds only trivially
   copyable alternatives, so its copy is a memcpy with no jump table, and one owning
   `boost::intrusive_ptr` keeps ownership RAII. Visiting stays exhaustive: `std::visit` on
   the immediate kinds, then a `switch` on `HeapTag`'s kind (with `-Wswitch` as an error)
   for heap kinds.
   *The next contradiction it creates:* the `HeapTag` kind and the `ValueObject` subclass must agree.
   One constructor per kind is the only place a kind is set, and Debug builds
   `dynamic_cast`-check every typed access.
2. **Short strings: today's std::string SSO holds 15 chars, `SmallString` holds 14.** Strings
   of exactly 15 chars will allocate where they did not. A long `std::string` moved into a value
   costs one allocation for the `StringObject` (the string's own buffer moves in, so 2 blocks
   instead of 1). Copies get cheap, creations get dearer. The counts show 3x more string
   moves than copies, so R2 is real. P2 must report allocations next to Ir. If creations
   dominate, 0151 builds strings directly into a single-block `StringObject` (concat,
   filter results, macro bodies) before P2 lands rather than after.
3. **Move that may throw vs. containers that need noexcept.** Today the containers resolve
   this against us, by copying. P1 resolves it at the root (no allocation in a move), and
   the `static_assert` keeps it resolved.
4. **Atomic refcount vs. templates shared across threads.** Constants and globals are shared
   by concurrent renders, so the refcount must stay atomic. Valgrind counts `lock xadd` as
   one instruction, but it costs about 20 cycles. The cache-sim and wall clock on CI are the
   check. P3a removes the atomic for the commonest shared values, the template's own
   constants.
5. **Density vs. the call sites wave 2 owns.** About 40 sites live in
   expression_evaluator.cpp/.h and statements.cpp. P1 and P0a avoid them, P0b waits for
   wave 2 to release them, and P2 then needs no call-site edits there.

## 5. Risks and checks

- **R1, lifetime of borrowed values** (views, ValueRef, Anchored). Behaviour must match
  today's `ShouldExtendLifetime` rules. The existing lifetime tests and the 0115
  use-after-free cases run under ASan and UBSan in every phase, plus the fuzz corpus (0003).
- **R2, string creation cost.** See contradiction 2. Gate: no case may gain more than +5%
  allocations without the perf thread accepting it.
- **R3, MSVC/Apple Clang layout.** `static_assert` on 32 B and on trivially copyable
  `Immediate`. MSVC's `std::variant` has been trivially copyable for trivial alternatives
  since VS 2019 16.x; the `static_assert` catches an older toolset.
- **R5, P3a constants escaping to the host.** `CreateGenericList` captures `adapter = *this`
  (internal_value.cpp:1048-1051) and reaches user callables, so the host can keep a value
  after the Template is gone. A no-refcount Static kind would then be a use-after-free, and
  TemplateExpired does not cover this path. P3a must convert Static to a refcounted copy
  wherever a value crosses into `GenericList`/`GenericMap`/user callables, or it is dropped.
  The P3b intrusive count must not be copied by `*this` copies or by `Clone()`.
- **R4, wide strings.** Wide strings get no SSO (3 wchar_t would hardly help). WString is
  always heap or a view, so `mitsuhiko_table_wide` gets protocol gains but not SSO gains.

## 6. Review

The architect role reviewed this plan against the code (one round). Its 11 findings are folded in:
32 B layout fix, noexcept move, generator clone, const accessors, Anchored spec, Undefined
view, phase order vs. wave 2 files, self-move and Slot bit, SSO rvalue, 32-bit length,
map density and P3a escape.

## 7. Done when

P2 is merged with `sizeof(InternalValue) == 32`, Render Ir no worse anywhere and down at
least 5% on mitsuhiko_table, dict_ops and strings, and allocations no worse than +5% per
case. P3 is decided with numbers.
