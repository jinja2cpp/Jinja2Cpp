---
status: open
priority: low
area: parity
depends: [0087]
touches: [src/statements.cpp#ForStatement, src/template_parser.cpp#ParseFor]
shares: [src/statements.h, test/parity/divergences/statements.txt]
---
# `for` loop corners that still differ from Jinja2

**Problem.** Found by the verifier of 0087 while probing the rewritten loop; each one
predates that rewrite and is pinned by a corpus case listed in
`test/parity/divergences/statements.txt`:

- `statements.for_filter_uses_loop`: the `if` filter of a loop sees `loop` (the state of
  the previous item). Jinja2 has no `loop` there and fails. `loop.length`/`revindex` in
  the filter already fail (0087); the rest should too.
- `statements.for_filter_sees_body_namespace`: the filter runs one item ahead, because
  the loop fetches the next item before rendering the body (for `nextitem`/`last`).
  Jinja2 fetches lazily, so a `namespace` write in the body reaches the next filter call.
- `statements.for_over_none`: `for x in none` (or a number) renders the `else` body;
  Jinja2 raises TypeError.
- `statements.loop_recursive_no_args`: `loop()` in a recursive loop renders nothing;
  Jinja2 requires the iterable.
- `statements.loop_recursive_filtered`: `for x in l if c recursive` is a parse error in
  Jinja2C++; Jinja2 accepts it.

Also seen, not pinned (output that Jinja2 prints differently anyway): `{{ loop }}` prints
a dict where Jinja2 prints `<LoopContext 1/3>`, `loop is mapping` is true, and
`loop == loop` is false.

**Proposal.** Fetch the next item lazily (only when `nextitem`, `last`, `length` or
`revindex` is asked for), which fixes the filter lookahead and matches Jinja2's
`LoopContext._peek_next`; hide `loop` from the filter scope; fail on non-iterables and
on `loop()` without an argument; allow `recursive` after a filter in the parser.

**Done when.** The five lines are gone from the statements allow-list.
