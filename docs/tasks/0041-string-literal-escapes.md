---
status: in-progress
priority: low
area: parity
depends: [0001]
touches: [src/helpers.h#CompileEscapes]
shares: [test/parity/divergences/]
---
# String literal escape sequences

**Problem.** Jinja2 decodes string literals with Python's `unicode-escape`, so `'\x42'`,
`'\u0043'`, `'\U00000044'`, octal `'\105'` and named `'\N{BULLET}'` all produce one
character, and `\a`, `\b`, `\f`, `\v`, `\0` are control characters. Jinja2C++
(`CompileEscapes` in `src/helpers.h`) decodes only `\n`, `\r` and `\t`; for any other
escape it drops the backslash and keeps the rest literally: `'A\x42'` renders `Ax42`
and `'\v'` renders `v`. Templates that build HTML entities or non-ASCII text with
escapes print garbage. Found while writing the `indent` cases of 0018.

**Proposal.** Decode the single-letter escapes and `\xHH`, `\uHHHH`, `\UHHHHHHHH` and
1-3 digit octal in `CompileEscapes`, emitting UTF-8 (narrow) or the platform's wide encoding, and
reject malformed or out-of-range escapes as Python does (a `SyntaxError`-style parse
error). `\N{name}` needs the Unicode name table; either generate a compact table with
`scripts/gen_unicode_printable.py`'s approach or leave it as a documented divergence.

Cases: `literals.string_escape_control`, `literals.string_escape_hex_octal`, `literals.string_escape_named`.

**Done when.** No line of `test/parity/divergences/` names task 0041, and `ctest -R parity` passes.

**Progress.** The single-letter escapes, backslash-newline, octal, `\xHH`, `\uHHHH` and
`\UHHHHHHHH` are decoded by `CompileEscapes` (UTF-8, UTF-16 or UTF-32 by the character
type), an unknown escape such as `\q` keeps its backslash as in Python, and a malformed or
out-of-range escape is a parse error (`ErrorCode::UnexpectedToken`). Landed in the PR for
this task.

**Remaining.** `\N{name}` is rejected as malformed (case `literals.string_escape_named`,
kind `rejects`) until a compact Unicode name table exists; the task closes with it.
