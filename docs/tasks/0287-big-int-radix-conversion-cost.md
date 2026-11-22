---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/979
priority: low
area: robustness
depends: [0270, 0271]
touches: [src/big_int.cpp, src/big_int.h]
---
# Quadratic radix conversion of big integers near the bit cap

**Problem.** 0270 keeps a big integer as decimal text, and converts between bases with a
schoolbook base-1e9 loop. That is quadratic in the length, which the 2^20-bit cap (0178 plan Q6)
bounds but does not make cheap. The pre-push verifier of #613 measured, in a GCC `-O2`
build:

- a template holding `0x` and 262144 `f`s (about 256 KB, just inside the cap) takes 2.3 s to
  load; the 349525-digit `0o` and 2^20-bit `0b` literals take about 2.4 s;
- each `bit_length()`, `'{:x}'.format(x)` or `|float` on that value takes about 0.46 s, so a loop
  over it multiplies that;
- a 349526-digit `0o` literal takes 2.2 s before the bit check rejects it, because the conversion
  runs first;
- Debug builds are about five times slower.

Decimal literals and strings past 4300 digits are rejected in microseconds; only the power-of-two
bases reach the cap, since Python sets no digit limit for them.

Template text may be untrusted (REVIEW.md), so a 256 KB template that costs seconds to load is a
cheap denial of service, even though it is bounded.

**Proposal.** Measure after 0271, where a backend can hold the computed form and skip decimal
text, then choose one or more of:

- an exact bit count for power-of-two bases before converting (the leading digit's bits plus
  `log2(base)` per further digit), so an over-cap literal fails at once;
- `bit_length`, `{:x}` and `float` that use the computed form, or a cached binary form, rather than
  converting from decimal each time;
- a divide-and-conquer conversion (split by powers of the base) to bring conversion close to
  `O(n log^2 n)`;
- a lower cap for literals in template text than for values computed during a render.

**Done when.** A cap-sized radix literal loads in under 50 ms in Release, and `bit_length`,
`{:x}` and `float` on it run in under 10 ms each, measured by a test or a bench case.

**Done in this PR (Release, GCC -O3, a 4-core cloud runner; timings vary by about 20%).**

- Conversions split the number in halves by a power of the source radix and multiply in the
  target radix (Karatsuba past 48 limbs, a schoolbook base case whose 64-bit columns carry once
  every 17 rows in base 10^9), so a conversion is about O(n^1.6) rather than O(n^2). The same code
  serves base 10^9 (decimal form), 2^32 (binary form, `bit_length`, `{:x}`, `{:o}`, `{:b}`,
  `bit_count`) and b^k (`BigInt::ToDigits` / `FromDigits` in other bases).
- A power-of-two literal or `int(s, base)` gets its exact width from its digits before
  converting, so one over the cap fails in about a millisecond (it took 2.2 s), and a value made
  by a backend is not converted back to count its bits.
- Measured: the 262144-`f` hex literal loads in 0.12-0.21 s (was 2.3 s); `bit_length()` and
  `'{:x}'.format(x)` on it take about 0.1 s each (were 0.46 s); `|float` already failed at once.

**Not met.** The 50 ms load and 10 ms per operation targets need the binary form to be stored
rather than recomputed, which is a value-model change: moved to task 0543.
