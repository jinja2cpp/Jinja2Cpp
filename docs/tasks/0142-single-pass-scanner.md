---
status: done
priority: low
area: perf
touches: [src/template_parser.cpp, src/template_parser.h, src/lexer.cpp, src/lexer.h]
---
# Load scans the source twice

**Problem.** MiniJinja Loads 1.5-2.4x faster than us (PR #414). After 0118's arena removes
malloc (about 20% of Load), two passes over the source remain (the rough tag splitter and
the lexer, about 21% of `many_tags` Load).

**Proposal.** After 0118 P5: one scanner that finds tag boundaries and tokenises tag bodies
in the same pass, emitting nodes straight into the arena.

**Done when.** `many_tags` Load a further -10..-15% (low confidence), about 1.2x MiniJinja.

**Also owns phase 6's Load cost** (0118/0117 phase 6, PR #446, accepted by the perf track
2026-10-10): interning each name at parse costs about 100 instructions per distinct name, Load
+0.12..+1.33% (substitute +1.33%, macros and html_autoescape +1.01%). A `string_view` straight
out of the lexer for the `ValueRef` path removes the copy and part of the probe; measure it
against the counts on master 0aa521d.

**Done** (PR to be linked). Measured with `bench/count.py` against master with #452 (0152)
merged, Release build:

- `TagLexer` (src/lexer.h) lexes a tag from its start and stops at the first end delimiter
  (with an optional `-`/`+`) that starts a token outside brackets, as Jinja2 does; the
  splitter no longer scans the tag first, and lexertk and `Lexer::Preprocess` are gone.
  The fine parse takes each block as the splitter closes it, so the list of blocks is gone
  too. Tags the lexer cannot decide (end delimiters that may start inside a token, lexing
  errors, `;`, unbalanced brackets, unclosed tags, a number ending in its exponent) keep
  the old scan by characters and are lexed on their own, with the same results.
- `Token` no longer carries an `InternalValue`: a literal's value is read from the source
  when the parser asks (`LexScanner::GetValue`), so tokens are four plain fields to build,
  copy and drop. This was half of the gain.
- Phase 6's Load cost: names of `ValueRef`s go from the source to the arena's symbol table
  as a `string_view` (`LexScanner::GetAsView`), -0.5..-1.4% Load on its own.
- Result: Load -17.8% (filters) to -30.0% (mitsuhiko_table), `many_tags` -23.3%,
  `substitute` -23.6%, `plain_text` -8.3%.
- Two lexer leniencies kept for parity with the old output are filed as 0161.

