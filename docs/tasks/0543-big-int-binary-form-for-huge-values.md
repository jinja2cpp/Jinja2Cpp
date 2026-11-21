---
status: in-progress
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/974
priority: low
area: robustness
depends: [0287]
touches: [src/internal_value.h, src/big_int.cpp, src/big_int.h, src/value_visitors.h, src/internal_value.cpp, src/template_dict.cpp, src/filters.cpp, src/python_format.cpp, src/value_methods.cpp, test/big_int_test.cpp, test/parity/cases/methods.py, test/parity/expected/methods.json, bench/cases/big_int_radix]
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

**Progress.** Plan approved 2026-11-21 (`/mnt/project-files/0543/plan.md`, section 11: words
inline for values past 14285 bits, decimal inline only for decimal sources, no lazy decimal
cache; phases P1a, P1b, P2, P4; the parity rejection of surviving constants is 0564).

- P1a (refactor, no behaviour change): `BigIntView` is read through accessors, full-decimal
  readers go through `WithDecimal`, a view of text is capped at 4300 digits, and unary `+`,
  dict lookup and `abs` share the value's object. Load time measured in Release (GCC 13 -O2,
  4-core container, median of 7): `{{ (0x<262144 f>) is odd }}` loads in 135 ms, of which the
  lexer and parser take at most 1.5 ms (a 262144-digit token that fails right after lexing),
  so the 50 ms target is not blocked by the lexer; the rest is building the decimal form, which
  P1b/P2 remove. `(s|int(base=16)) is odd` from data renders in 131 ms and `x.bit_length()` on
  the literal in 101 ms.
- P1b (binary words): a `BigIntObject` holds the words of a huge integer (more than
  `HugeBigIntBits` = 14285 bits) after its header, then its decimal form, which every integer
  still keeps until P2. `MakeBigInt` converts a decimal source past 4300 digits once and keeps
  those words only when the integer is huge (the 4301-digit band is checked exactly); a
  power-of-two `int(s, base)` or literal packs its digits into words without arithmetic and
  converts only to decimal. On a huge integer `bit_length` is O(1), `bit_count`, `{:x}`/`%x`/
  `{:o}`/`{:b}` read the words, `is odd` reads the low word, `str`/`tojson`/`float` fail at once,
  negation and `abs` copy the words, comparison reads them from the top, and a dict key is the
  new kind `'B'` (sign, word count, words). Bench case `big_int_radix` (Release, GCC 13, median
  of 3): building `('f' * 262144)|int(base=16)` takes about 150 ms (the decimal form, which P2
  drops); after it, `bit_length`, `bit_count` and `'{:x}'.format(x)|length` together take about
  1 ms per iteration, where each was one 100 ms conversion before.
  A host `BigInt` past 4300 digits now converts to words each time it becomes a template value
  (plan Q8): once per render for a parameter, but on every access for an item of a host
  container (118 ms per access at 262144 bits, ASan build). Neither P2 nor P4 removes that:
  task 0566.
