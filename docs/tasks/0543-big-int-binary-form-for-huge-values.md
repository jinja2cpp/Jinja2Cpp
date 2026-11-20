---
status: open
pr:
priority: low
area: robustness
depends: [0287]
touches: [src/internal_value.h, src/big_int.cpp, src/big_int.h]
---
# Keep a binary form for big integers past the decimal string limit

**Problem.** Found while doing 0287. The core stores a big integer only as its canonical decimal
text (0270), so every operation that needs bits converts: `bit_length()`, `bit_count()`,
`'{:x}'`, `'{:o}'`, `'{:b}'` and `%x`/`%o`. 0287 made the conversion subquadratic, but at the
2^20-bit cap one conversion still costs about 0.1 s in Release, and a 256 KB `0x` literal still
takes about 0.13 s to load because its decimal form must be built. A loop over `x.bit_length()`
multiplies that, so untrusted template text keeps a 10-100x amplification over ordinary text of
the same size. Python does all of these in linear time.

Values this large come only from power-of-two literals and `int(s, base)` (whose binary form is
known for free) and from backend arithmetic; decimal sources stop at 4300 digits, and `str()`,
`repr()` and `tojson` of anything past 4300 digits raise anyway, so the decimal text of a huge
value is needed only for comparison, hashing, `IsOdd`, rounding, negation, the backend's
`fromDecimal` and export to a host `Value`.

**Proposal.** Needs an architect plan (value model, `BigIntObject` layout, memory accounting,
thread safety of any lazily filled form):

- store binary words in `BigIntObject` for values past `MaxIntStrDigits` digits, built at
  creation from the source digits or the backend result, so the operations above are linear;
- optionally make the decimal text of such values lazy, so loading a cap-sized literal is
  linear too; compare and hash on the binary form when both sides have one;
- optionally an NTT multiplication for the conversions that remain (about 2x at the cap).

**Done when.** A cap-sized radix literal loads in under 50 ms in Release, and `bit_length`,
`{:x}` and `float` on it run in under 10 ms each, measured by a test or a bench case (the targets
0287 set).
