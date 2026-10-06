# Cache locality brainstorm (2026-10-06, before 0117/0118 implementation)

Ruslan asked for one more pass on cache locality before the revised plans start. Facts checked on
master 8efc156+ in the cloud container (x86-64, GCC, Release). Analogy used throughout: the CPU is
a cook with a small worktop (L1 32 KB, about 512 lines of 64 B), a shelf (L2 ~1 MB) and a pantry
(RAM). Every trip to the pantry costs ~100 ns, about 300 instructions of work. Locality means
putting what the next step needs on the worktop already, in the order it will be used.

## What we measured / know

| Item | Size now | Note |
|---|---:|---|
| `InternalValue` | 72 B | more than one cache line per value once neighbours are counted |
| `Value` (public) | 40 B | |
| many_tags parse tree | ~1.06 MB | larger than L2; 0118 target ~0.4 MB |
| Output buffer (0113) | 512 B narrow | fits L1 |
| `TemplateImpl::m_outputSizeHint` | atomic, written every render | template_impl.h:617, store at :277 |

## Findings, ordered by expected payoff

### 1. Tree layout: allocate in execution order (0118 P4)
Today each node is its own `make_shared`, scattered across the heap in parse order with control
blocks between them. With one monotonic arena filled in pre-order, a render walks memory mostly
forward, and the hardware prefetcher (which spots forward strides) fetches the next lines before
we ask. Rule for the node factory: a parent's child span is allocated right after the parent's
header, children in the order they execute. Cost: nothing extra, it is how a bump arena works if
the parser emits nodes in the right order. Check: the parser builds some nodes bottom-up
(expressions), so expression subtrees come out post-order. Post-order is also sequential for an
evaluator that visits operands first, so this is fine; statements are the ones to keep pre-order.

### 2. Hot/cold split of node fields (fold into 0118 P3)
Every node carries source position / debug info that is read only on errors. Move it to a side
table indexed by `NodeRef` (same arena, separate region), so the hot part of a node is the
vtable pointer, kind and child span. Target: a typical expression node at 16-32 B, two to four per
line instead of one. This is the biggest single footprint win after the arena itself.

### 3. Shared-cache-line writes between threads (new small task, cheap)
`m_outputSizeHint` is a `mutable std::atomic` inside `TemplateImpl`, stored on every render. When
several threads render one template, every store invalidates that line in the other cores, and the
line also holds read-only hot fields (the root node handle, settings). That is false sharing: two
cooks who keep snatching the same cutting board. Fix options, cheapest first:
- store only when the value changes by more than, say, 25% (almost never after warm-up);
- `alignas(64)` the atomic onto its own line;
- keep the hint per thread (thread_local small map) or per render context.
The second item is `shared_ptr<TemplateImpl>` refcount traffic per render (atomic increment and
decrement on a shared line); the 0118 pin-set design keeps one strong ref per render, so that
leaves one pair per render, acceptable. Measure with the MT inheritance bench (0105).

### 4. Value size (separate value-model task, large)
72 B per `InternalValue` means a list of 10 values spans 12 lines, and every temporary on the
evaluator's path moves a lot of bytes. Options: a smaller variant (string_view/shared refs for big
alternatives, small ints and bools inline) to reach 24-32 B; NaN-boxing to 16 B is too invasive
for the variant-based API. This touches 2.0 value model (0043/0072), so it needs an architect plan
and Ruslan's call; not part of 0117/0118. Worth a task now so the scout tracks it.

### 5. Scope maps as SoA (0117 P5)
The linear chunked scope map already won the small-map benchmark. Laying it out as
structure-of-arrays (a chunk of 16 x 4-byte `SymbolId` keys = one line, values in a parallel
array) makes the search touch one line per 16 names, and SIMD compare is possible later. Values
stay 72 B each, touched only on a hit.

### 6. Slot frames contiguous (0117 P1/P2)
Slot frame = one `boost::span<std::optional<InternalValue>>` in a per-render stack arena, so a
macro call's locals sit in consecutive lines, and the loop frame pool (0133) sits next to them.

### 7. Lookup cache footprint
The thread_local lookup cache (0100) is keyed by expression. Check its entry size and count: if it
grows past a few KB it evicts the tree from L1. With slots most lookups bypass it, so it can shrink
after 0117.

### 8. Dispatch cost (later, not now)
Each node visit is a virtual call: vtable line plus an indirect branch per node. Large templates
have many node types, which loads the branch target buffer and the instruction cache. A tag plus
`switch` on hot node kinds, or flat bytecode as minijinja and Tera 2 do, would cut this. That is a
rewrite of the evaluator; record it as a direction after 0118, not in its scope.

### 9. User data
User `ValuesMap` is `std::unordered_map` (node-based, one allocation per entry, poor locality).
Changing it is a 2.0 ABI question (0043); the 0115 adapters already avoid copying it.

## Measurement

`perf` is not available in the container, so use cachegrind: `valgrind --tool=cachegrind
--cache-sim=yes` gives D1 and LL read/write misses per function. Proposal: add D1mr/DLmr columns to
`bench/count.py` (opt-in flag) and record them in the trend for the cases where 0118 claims a gain
(many_tags, mitsuhiko_table, expressions). Cachegrind models an idealised cache, so read relative
changes, not absolute numbers. Wall clock on the MT bench covers false sharing, which cachegrind
cannot see (it is single-threaded).

## What changes in the plans

- 0118 P3: hot/cold field split and pre-order child placement become explicit requirements.
- 0118: NodeRef checks are on in every configuration by default, with a compile-time switch to
  turn them off (Ruslan, 07:51). Budget +1-2% Release on expression-heavy cases.
- 0117 P5: SoA chunk layout for the scope map.
- New small task: `m_outputSizeHint` false sharing (item 3).
- New tasks to file, not scheduled: value size (item 4), dispatch (item 8), cachegrind columns.
