---
status: open
priority: low
area: perf
touches: [bench/count.py, bench/frames.py]
shares: [src/statements.cpp, src/internal_value.h, .github/workflows/benchmark.yml, bench/README.md]
---
# The PR gate names hot functions that GCC stopped inlining

**Problem.** Between 2026-10-19 and 2026-10-26, the most common cause of a held perf
verdict was GCC moving a hot helper out of line after unrelated code grew. Each case cost
a round of callgrind diffing by the owner before it was found and forced inline:

| PR | What GCC did |
|---|---|
| #614 | `PeekBinaryOperator` out of line |
| #629 | `ParseCallParamsImpl` out of line |
| #643 | `PopStatement`, `OutputCheckOf`, and `emplace_back` after the `ToValueList` split |
| #663 | `loop.changed` setup inlined into the `GetLoopAttr` frame; an `InternalValue` temporary that added a stack-protector frame |
| #642, #641, #643 | the `std::string` move in `RenderAsString` became a PLT call (fixed in 6fd648b) |
| #673 | `BinaryMathOperation::operator()(long,long)` (fixed by force-inlining in #672) |
| 0289 | the expected-lite swap in `ParseBinary` |
| #663, #667, #672 | `InternalValue::operator=(const InternalValue&)` in `RenderLoopInSlots` went out of line and back in (an `.isra` clone): `Render/dict_ops` moved by -0.47%, +0.34% and -0.34%, exactly ±1,000 Ir. The 100 `k, v` unpackings per render cost 25 Ir each out of line, against 15 inline. |

A fixed list of symbols that must not exist out of line would not help:
- every function above except `InternalValue`'s copy assignment is already forced
  inline (`JINJA2CPP_HOT_INLINE` or `JINJA2CPP_ALWAYS_INLINE`), so it cannot regress;
- the next case will be a function nobody has listed yet.

The copy assignment flips back and forth today: `nm` lists four out-of-line `.isra`
copies on master 0620aca, and statements.cpp's copy is inlined there. Forcing it inline
at the loop-unpack call site is a small fix that can come first.

The scout of 2026-10-26 found no other header-inline function on a hot path that is
emitted out of line with more than 0.1% of a case:
- `InternalValue`'s copy constructor: 1,100 calls per `html_autoescape` and `many_tags`
  render, 6 Ir per call;
- `AsStringView<char>`: 1,500 calls, 7 Ir per call.

`OutStream::WriteValueTo<T>` (16,680 calls per `mitsuhiko_table` render) is out of line on
purpose, as an explicit instantiation in `out_stream.cpp`.

What would catch the next case is a by-name check of the base and head profiles, which
the gate already has.

**Proposal.**
0. Force-inline `InternalValue`'s copy assignment where `RenderLoopInSlots` unpacks
   loop variables, or the whole operator if no case grows. Check that `Render/dict_ops`
   stays at master's count when `statements.cpp` changes.
1. `count.py --keep-profiles DIR` keeps each case's callgrind output instead of
   deleting it.
2. A new `bench/frames.py base_dir head_dir` compares the two sets per case. It lists
   functions that appear in head with at least 200 calls per iteration and either did not
   exist in base or have twice base's calls (a callee that used to be inlined). For each
   function it prints the case, calls per iteration and self Ir. It also lists functions
   whose self Ir per call moved by more than 20% with unchanged calls, which catches a
   new stack-protector or spill frame. It matches names after normalising template
   arguments, so a renamed container type (`boost::container::small_vector` to
   `SmallVector` in #650) is not reported as new.
3. The `instructions` job runs it and appends the list to the step summary. It is a
   report, not a gate.

**Done when.** On a local replay of #643's first head against its base, `frames.py` names
the outlined `emplace_back` without a manual callgrind diff, and an unchanged-source
rebuild reports nothing.
