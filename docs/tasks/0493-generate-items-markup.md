---
status: in-progress
priority: low
area: parity
depends: [0456]
touches: [python/src/lazy_render.cpp, python/src/environment.cpp#CallbackSink]
shares: [python/tests/test_generate.py]
---
# generate() items are str where Jinja2 yields Markup

**Problem.** Found by 0456. Under autoescape Jinja2's `generate()` yields the escaped value of a
`{{ }}` it evaluates at run time as `Markup` (`escape(value)`), and a text run in volatile code
(`{% autoescape x %}a{% endautoescape %}`) as `Markup('a')`; folded runs and template text are `str`.
The module's sink makes every item a `str`:

```
Environment(autoescape=True).from_string("a{{ x }}b").generate(x="<")
Jinja2: ['a', Markup('&lt;'), 'b']     module: ['a', '&lt;', 'b']
```

The text is the same, so `"".join(...)`, `TemplateStream` and `dump` agree; only a consumer that tests
`isinstance(item, Markup)` (or escapes an item again) sees the difference.

**Proposal.** Tell the sink which pieces are markup (a flag per piece the stream knows from the value it
wrote, or from the output node's autoescape setting) and make those items `Markup` in the module. Or
record it as deliberate.

**Done when.** The example yields Jinja2's item types, with a case in python/tests/test_generate.py.


**Resolution (batch 9 D).** `RenderOptions::pieceIsMarkup` (docs/api-2.0.md): the render marks a piece before
its `Write` (renderer.h `MarkOutputPiece` for an output node, `{% filter %}` and `{% call %}` and a recursive
loop for their own piece), and the held piece publishes the mark. The module's sinks (`CallbackSink`, the lazy
render's) make those items `Markup`. Pinned by `OutputTest.SinkLearnsWhichPiecesAreMarkup` (both character
types) and `test_items_are_markup_where_jinja2s_are` (both generate modes). Left: a filter block whose filter
returns a non-string (`length`) yields its `str()` where Jinja2 yields the int (docs/python.md).
