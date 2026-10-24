---
status: done
priority: low
area: filters
depends: []
touches: [src/string_converter_filter.cpp]
shares: []
---
# urlize escapes Markup input

**Problem.** Under autoescape `{{ '<b>'|safe|urlize }}` renders `&lt;b&gt;`; Jinja2 3.1.6
renders `<b>` (urlize escapes `str` input, but a Markup passes through `markupsafe.escape`
unchanged). Found by the 0180 verifier with a host `jinja2::Markup` value; the template-side
`|safe` shows the same on master, so it is independent of 0180.

**Proposal.** In the urlize filter, escape the input only when it is not Markup (the
`MarkupEscape` rule from src/markup.h), and keep the result Markup under autoescape.

**Done when.** A parity case in `test/parity/cases/filters.py` with autoescape on,
`{{ '<b>x</b> http://a.b'|safe|urlize }}`, matches Jinja2, and the escaped form for a plain
string still matches.

**Progress.** The urlizer skips its escape step for a Markup input, as `markupsafe.escape` does.
Cases `autoescape.env_on_urlize_markup` and `autoescape.env_off_urlize_markup` pin it, with the escaped
form of a plain string beside it.
