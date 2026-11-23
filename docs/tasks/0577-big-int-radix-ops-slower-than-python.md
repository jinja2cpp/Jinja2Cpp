---
status: open
priority: medium
area: perf
depends: []
touches: [src/big_int.cpp, src/value_methods.cpp, test/big_int_test.cpp]
---
# Huge-integer bit_count and power-of-two formatting are slower than Python

**Problem.** Found by the 2026-11-23 perf scout (`/mnt/project-files/perf-track/2026-11-23-scout.md`).
`Render/big_int_radix` (added by 0543) is the only Render case where Jinja2C++ is slower than
Python Jinja2: 0.49x (25-29 ms against 12.5 ms, median of 3 runs on master 784eab1). The template
builds `('f' * 262144)|int(base=16)` (8,192 words) and then, 20 times, renders `bit_length()`,
`bit_count()`, `'{:x}'.format(x)|length` and `is odd`.

Callgrind on 784eab1, one render, 308.3 M instructions:

| Frame | Self | Per unit |
|---|---:|---|
| `WordsBitCount` (src/big_int.cpp, via `BitCount`) | 88.5 M (28.7%) | 540 Ir a word |
| `WordsDigits` (base 16) | 89.1 M (28.9%) | 68 Ir a hex digit |
| `StrOps::FormatValue` + `DecorateDigits` (src/value_methods.cpp) + `memcpy` | about 80 M (26%) | several whole-string passes and copies of the 65,536-digit string |
| `ParsePythonInt` + `IntegerFromDigits` (the one `int(base=16)`) | about 17 M | once a render |

- `WordsBitCount` clears one set bit per iteration (`rest &= rest - 1`), so an all-ones word takes
  32 iterations. Python's `int.bit_count` uses a hardware popcount per digit.
- `WordsDigits` computes each digit from a bit offset with a division, a modulo and a 64-bit window.
  For base 16 (and 2, 4, 8 with words that hold whole digits) it can emit the 8 digits of a word in a
  fixed loop.
- `FormatValue` moves and copies the digit string through `DecorateDigits` even when the spec has
  no width, sign, `#` or grouping.

**Proposal.**
1. Count bits with a portable SWAR popcount (C++17 public floor; `__builtin_popcount` on GCC/Clang
   and `__popcnt` on MSVC are optional, behind the existing compiler checks). Expected: 88 M to
   about 2 M.
2. Emit whole words in `WordsDigits` when `32 % bitsPerDigit == 0`; keep the window path for base 8
   and 32.
3. In `FormatValue`, return the digits unchanged when the spec has nothing to decorate, and reserve
   once otherwise.

**Overlap.** 0543 P4 (backend word entries) also touches src/big_int.cpp. Run this after P4, or
in the same thread.

**Done when.** `Render/big_int_radix` renders at least as fast as Python Jinja2 in `bench/run.py`,
instruction count at least -50% in `bench/count.py`, no other row above +0.3%. `bit_count` and
`{:x}`/`{:o}`/`{:b}`/`{:#_x}` results are unchanged in test/big_int_test.cpp for both character
types, including a word with the top bit set, a single-bit value and zero.
