---
status: done
priority: low
area: parity
depends: [0219]
touches: [src/statements.cpp#RenderLoopInScopes, src/statements.cpp#RenderLoopInSlots]
shares: [test/parity/divergences/statements.txt]
---
# A for loop over `loop` advances the outer loop early

**Problem.** Since 0219, iterating `loop` takes the rest of the loop's items as `(item, loop)`
pairs, so the outer loop goes on after them, as Jinja2's `LoopContext.__next__` does. A
`{% for y in loop %}` fetches its next item before its body runs (unfiltered loops always
do), and fetching it advances the outer loop. Jinja2's inner LoopContext fetches lazily,
only when the body reads `last`, `length`, `revindex` or `nextitem`. So the outer loop's
`index` read inside the inner body is one ahead, and a `break` there leaves the outer loop
one item further on: `statements.loop_iter_inner_body_reads_index` prints `3b4c4aTrue;`
where Jinja2 prints `2b3c3aFalse;4dTrue;`. The same early fetch makes the inner loop's own
`length` and repr one short (`{% for a in loop %}{{ loop.length }}` gives 2 where Jinja2 gives
3), and an item's `loop` iterated again inside the inner body yields nothing where Jinja2
yields what is left (0219 verifier). `loop|list` and loops that do not read the outer state
are not affected.

The same early fetch applies to every unfiltered loop over a list the body changes: removing
the current item visits a different set of items (`{% set q = [1, 2, 3] %}{% for x in q %}{{ x }}{% do q.remove(x) %}{% endfor %}`
gives `12`, Python `13`; `statements.for_remove_current_item`, moved here from 0049's P3 list,
which 0049 closed with). `loop.length`, `loop.revindex` and `loop.last` after an append already
match (`statements.for_length_after_append`), as does `pop` in a loop over context data
(`statements.for_pop_context_list`).

Also left by 0219: `loop|urlencode` quotes the loop's repr; Jinja2 iterates it as a sequence
of pairs (`1=%3CLoopContext+2%2F3%3E&...`). Pin it with a case when this is taken up.

**Proposal.** Let a for loop over an iterator whose items are expensive to take early (the
loop's iteration) fetch lazily, as the filtered loop over slots already does
(LoopState::lazyFilter): fetch the next item after the body, or when the body peeks. Only
worth it with the loop code open for something else; no template in the wild iterates `loop`.

**Done when.** No line of `test/parity/divergences/` names 0304.

**Done (parity batch 3, group B).** Every loop now fetches its next item after the body, or
when the body peeks (`last`, `nextitem`, `length`, `revindex`), as the slotted filtered loop
already did: `FetchAfterBody` in `RenderLoopInScopes` and `RenderLoopInSlots`. A loop over
`loop` also takes its length from `len(loop)` (the iterated loop's length), as Jinja2's
`LoopContext.length` does. Cases: `for_remove_current_item_recursive`, `for_append_in_body`,
`for_filter_sees_body_write(_recursive)`, `loop_iter_inner_length`, `loop_iter_item_loop_again`,
`loop_iter_inner_break_recursive`.
