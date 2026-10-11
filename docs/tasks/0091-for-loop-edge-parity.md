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
  the filter already fail (0087); the rest should too. Since 0154 reading `loop.last` or
  `loop.nextitem` in the filter fails too, and so does `if loop`, where Jinja2 sees an
  undefined (falsy); `loop is defined` is still true. A filter run by a peek from the body
  sees no `loop` of its own. Left: in Jinja2 an inner loop's filter sees
  the *outer* loop's `loop` (`{% for y in a %}{% for x in b if loop.last %}` uses the
  outer one); here the filter run in place (no peek from the body) reads the inner one, while a
  filter run by a peek sees the outer one, so the result depends on whether the body
  reads `loop.last`. Running the in-place filter in a child context too (as
  `ForStatement::FetchFiltered` does) would unify them, at a cost per item.
- ~~`statements.for_filter_sees_body_namespace`~~: fixed by 0154 (filtered loops over
  slots fetch lazily and peek for `last`/`nextitem`/`length`/`revindex`). Loops inside a
  recursive loop still filter one item ahead (`RenderLoopInScopes`).
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

**Done when.** The four remaining lines are gone from the statements allow-list.
