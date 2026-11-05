---
status: done
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/755
priority: low
area: parity
depends: []
touches: [src/string_converter_filter.cpp#ApplyUrlEncode]
---
# `loop|urlencode` iterates the rest of the loop

**Problem.** Jinja2's `do_urlencode` treats any `abc.Iterable` as a sequence of pairs,
and `LoopContext` is iterable: it yields the items the loop has not reached yet, and
advances the loop. So `{% for i in [1] %}[{{ loop|urlencode }}]{% endfor %}` renders
`[]` in Python. On the last item nothing is left, and on an earlier item the rest is
unpacked as pairs, which usually fails. Jinja2C++ quotes `str(loop)` and renders
`[%3CLoopContext%201/1%3E]`. The corpus pins this as `filters.urlencode_loop`, kind
`output`. Found by the 0362 verifier.

**Proposal.** Iterate the loop's remaining items in `ApplyUrlEncode`, as a `for` over
`loop` does (see the 0304 lines in statements.txt for how such a loop advances the outer
loop). Then unpack them as for any iterable.

**Done when.** `filters.urlencode_loop` matches, narrow and wide, and a case with items
left behaves as Python does.

**Done (parity batch 3, group B).** `ApplyUrlEncode` iterates any object whose accessor
iterates (`IMapAccessor::Iterates`: host objects with `__iter__`, `loop`), so `loop|urlencode`
takes the rest of the loop as `(item, loop)` pairs. Cases `urlencode_loop_rest` and
`urlencode_loop_rest_lists` pin a loop with items left.
