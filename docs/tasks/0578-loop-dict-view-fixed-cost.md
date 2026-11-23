---
status: open
priority: medium
area: perf
depends: []
touches: [src/internal_value.cpp, src/internal_value.h]
---
# Win back the for-loop start cost that the live dict views added

**Problem.** Found by the 2026-11-23 perf scout (`/mnt/project-files/perf-track/2026-11-23-scout.md`).
Two parity changes of the week added fixed cost to every `for` loop start:

1. **`DictViewEnumerator::Rebind` uses `dynamic_cast`** (src/internal_value.cpp, from 0523, train
   #940). A loop over a dict now walks a keys view (0538), and the loop's recycled enumerator calls
   `Rebind` with it. Callgrind on `Render/hf_gpt_oss`: `__dynamic_cast` and the `__do_dyncast`
   walk cost about 1,200 instructions per call, +1,320 a render (+0.8% of the row; CI: #940
   +0.80%). `DictView` is `final`, so `typeid(*list) == typeid(DictView)` (as the sibling
   `StepEnumerator::Rebind` already does for `ValuesListAdapter`) gives the same answer for a few
   instructions.
2. **`ConvertToLoopList` adds about 30 instructions per loop start on a list** (from 0538, train
   #956). It calls `AsMap` before falling back to `ConvertToList`, out of line. `Render/mitsuhiko_table`
   has 1,001 loop starts a render: ConvertToList's inclusive cost went from 43 k to 74 k
   instructions per render (CI: #956 +0.55% on `mitsuhiko_table*`, about 40% of 0518's gain).
   Test for a list value first (the common case) and only then look for a mutable dict.

**Done when.** `Render/hf_gpt_oss` and `Render/mitsuhiko_table*` give back at least -0.6% and
-0.5% in `bench/count.py` against master, no row above +0.1%, no new allocation. The 0538 and 0523
parity cases (live keys view in a loop, a dict changed during the loop) still pass.
