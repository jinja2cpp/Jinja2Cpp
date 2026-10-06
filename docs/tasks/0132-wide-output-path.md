---
status: done
priority: low
area: perf
touches: [src/out_stream.h#OutStream, src/out_stream.cpp#WriteInt, src/template_impl.h#Render]
---
# Rendering to a wide string pays more per fragment than to a narrow one

**Problem.** The perf scout of 2026-10-06 (master 448bb7c) finds `Render/mitsuhiko_table_wide`
the case closest to Python: 1.44x, against 2.04x for the same template rendered narrow.
Python's time is the same for both (1.53 vs 1.58 ms); ours goes from 751 µs to 1.10 ms
(+46%) for 13% more instructions (8.89M vs 7.88M) and 4x the peak memory (1.38 MB vs
346 KB). Callgrind puts `OutStream::WriteValueSlow` at 11.5%, `WriteBufferSlow` at 5.7% and
`std::wstring::_M_append` plus `wmemcpy` at 6.4%.

0113's buffer is 512 *bytes*, so with a 4-byte `wchar_t` it holds 128 characters and
flushes four times as often; every value write takes the out-of-line `WriteValueSlow`
branch before reaching `WriteValueTo<wchar_t>`; and the output-size hint that reserves
the narrow target may not reach the wide one. Part of the gap is inherent (four bytes per
character to move), the rest is the narrow-only fast path.

**Proposal.** Measure each with `bench/count.py --baseline` and `run.py`:
1. Size the buffer in characters, not bytes (or give the wide stream its own larger buffer).
2. Dispatch `WriteValue` on the target type inline, so a wide string target gets the same
   direct `WriteValueTo` call as a narrow one.
3. Check that the output-size hint reserves the wide target as it does the narrow one.

**Done when.** `Render/mitsuhiko_table_wide` instructions drop by at least 8% and its peak
memory falls toward 4x the narrow output, with no narrow case slower than 1%.

**Outcome** (this PR, measured against master 8efc156 with `bench/count.py --baseline`):

| Change | `Render/mitsuhiko_table_wide` instructions |
|---|---:|
| Write wide numbers straight into the buffer (they went through a stack copy and a per-character `WriteAscii`), dispatch `WriteValue` to `WriteValueTo<wchar_t>` inline | -6.1% |
| Buffer of 512 characters rather than 512 bytes (2 KB for a wide stream; a narrow one still uses 512 bytes of it) | -2.5% more |
| Both | **-8.6%** (8.89M to 8.12M) |

`run.py`: the wide table 768 to 664 µs (-13.6%, 1.57x to 1.80x Python); no narrow case got
more expensive than +0.33% (`large_static`), `mitsuhiko_table` -1.3%, `for_range` -3.0%.
The output-size hint already reserved the wide string (item 3): its peak, 1.38 MB, is 3.98x
the narrow 346 KB, so memory had no gap to close.

Tried and dropped: copying 17-128-byte fragments inline in 16-byte blocks instead of
calling `memcpy` (wide fragments are four times longer) cost more instructions than it
saved (-8.2% instead of -8.6%) and made `large_static` 1.1% dearer.
