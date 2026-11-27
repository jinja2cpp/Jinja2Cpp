---
status: in-progress
priority: low
area: robustness
depends: [0543]
touches: [src/value_visitors.h, src/big_int.cpp, src/big_int.h, src/cycle_collector.h, src/cycle_collector.cpp, src/internal_value.h, test/big_int_test.cpp, test/limits_test.cpp, test/template_module_test.cpp, test/suspendable_render_test.cpp, test/embedding_lifecycle_threads_test.cpp]
---
# Stop converting a huge host BigInt on every access

**Problem.** Found by the verifier on the 0543 P1b PR (plan `/mnt/project-files/0543/plan.md`,
section 8 "Host regression", accepted as Q8). A public `jinja2::BigInt` is decimal text, and since
P1b a big integer past `HugeBigIntBits` (14285 bits) keeps binary words, so turning a host
`BigInt` of more than 4300 digits into a template value converts its decimal form to words
(`MakeBigInt` in `src/big_int.cpp`, reached from `value_visitors.h` `MakeBigIntValue(val.ToString())`).
A top-level parameter converts once per render, but an item of a host container converts on
every access: `{% for i in range(20) %}{{ l[0] > 0 }}{% endfor %}` with
`l = [BigInt(2**262143 + 12345)]` (78914 digits) took 118 ms for one access and 2.5 s for 20
(ASan build); before P1b an access copied the text, about 30 µs. Template text chooses the
loop count, so a host that passes huge integers in lists or maps gives templates a CPU
multiplier. P2 does not remove it; neither does P4 (backend words), as the host side stays decimal.

**Proposal.** Pick one, with a recorded decision if it touches the public API:
- a binary form in public `BigInt` (an ABI change: `FromWords`/`Words`, or words kept alongside
  the text), so conversion in both directions is a copy;
- or keep the converted value with the host adapter's item (a per-adapter cache, which needs the
  thread-safety and memory-accounting analysis of plan section 4).

**Done when.** Twenty accesses to a 262144-bit host `BigInt` in a list take about the time of one
conversion or less (bench case or test), and the behaviour of host big integers is unchanged.

**Progress.** Decision: option A, no public API change (architect plan `/mnt/project-files/0566/plan.md`).
A render's collector (`CycleCollector::Notes::hugeInts`, class `HugeHostInts` in `src/cycle_collector.h`)
holds the last four huge host BigInts it converted, keyed by content and by the budget that paid for
them; `HostBigIntValue` (`src/big_int.cpp`, called from `value_visitors.h`) shares the object on a hit.
An entry that held the last reference to a charged object credits the budget that paid and clears the
object's charge (`BigIntObject::ForgetCharge`), so a nested render's cache ending after its limits
credits no outer budget. Not cached: no collector, under `UncountedMemoryScope` (only uncharged
entries hit), and an object that with the others would take more than a quarter of the budget.
Tests: `BigIntCases` rows (narrow and wide, `hl`/`hmap` params), `BigIntTemplate.HugeHostItemConvertsOncePerRender`
(budget, timing ratio), `BigIntTemplate.HostBigIntValueSharesWithinTheCollector` (identity, uncharged,
credit on eviction), module, limits, suspendable (two interleaved renders, same thread) and threads
tests. Option C (a words form in the public BigInt) is filed as 0599. Measurements are in the PR.
