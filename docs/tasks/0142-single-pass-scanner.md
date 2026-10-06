---
status: open
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
