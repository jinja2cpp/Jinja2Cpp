---
status: open
priority: medium
area: performance
depends: [0545]
touches: [src/value_methods.cpp#SetEntries]
shares: [test/limits_test.cpp]
---
# Set methods over a big range are thousands of times slower in MSVC Debug than in a GCC Debug build

**Problem.** In merge train #973, `Limits/LimitsTest.DefaultsRender/MemorySetSubsetOfRange`
(`{% set s = {1: 0, 2: 0}.keys() - [] %}{{ s.issubset(range(300000)) }}`, default limits) took 1,116,342 ms on
"MSVC C++23 Debug /MD shared=false", and `MemorySetUnionOfRange` ran past the 2400 s ctest timeout. The same tests take
442 ms and 1051 ms in a Linux GCC Debug build (libstdc++ debug mode on: about 4 minutes for the whole suite). With the
old sizes (range(3000000)) the two MSVC Debug jobs ran for 6 hours. A gap of about 2500x is more than Debug iterator
checks explain, so a step of `SetEntries` (building the `InternalDict` of hash tokens from `ConvertToList(range)`,
`MakeDictKey`, `try_emplace`, `ChargeRenderMemory`) is probably quadratic or close to it under MSVC's Debug CRT or
`_ITERATOR_DEBUG_LEVEL=2`. It may also hit MSVC Release with smaller constants, and it reaches any template that
calls a set method on a big iterable.

The 0545 tests were cut to range(20000) under a 256 KiB budget in #971 to unblock the train, so the corpus no longer
shows the problem; this task pins it.

**Proposal.** Time `s.issubset(range(N))` for N = 10^4, 10^5, 3*10^5 on an MSVC Debug and a Release build (a CI
job step, since the cloud has no MSVC) to see how it scales, find the step that grows faster than linear and fix it.
Then add a limits or bench case with a size that fails if the time grows again.

**Acceptance.** MSVC Debug runs `s.issubset(range(300000))` within 20x of a GCC Debug build, and the scaling is
linear.
