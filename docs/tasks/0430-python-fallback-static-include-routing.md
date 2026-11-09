---
status: open
priority: low
area: api
depends: [0354]
touches: [python/jinja2cpp/environment.py, python/jinja2cpp/fallback.py, python/src/environment.cpp, docs/python.md]
---
# Route static include, import and extends names at load time (Python fallback)

**Problem.** The Python module falls back to Jinja2 for a template the engine cannot parse, but a
native template that includes one (`{% include 'inc' %}` where `inc` has a `{% print %}`) fails at
render time with the engine's `TemplateSyntaxError`. A render retry was dropped from 0354 (D7 item 3):
"no Python ran in this render" cannot be proven, since host-object adapters call into Python (`IsMarkupLike`,
`IsMapping`, attribute reads, iteration of one-shot iterators), so a retry can repeat effects or find an
iterator consumed.

**Proposal.** At load time, for a template the engine parsed, list its constant include, import and extends names
(`FindReferencedTemplates`, the engine's equivalent of `jinja2.meta.find_referenced_templates`), load each through
the environment, and when one needs Jinja2 route the whole including template to Jinja2 (a fallback template),
transitively. Dynamic names stay unsolved and keep the engine's error.

**Decisions needed.** (1) Loader calls move to load time: the includes' `get_source` is called when the including
template is loaded, not when it renders, which changes loader call counts and order, and raises
`TemplateNotFound` for a missing include at `get_template` instead of at `render` (or the lookup must treat
a missing name as "not routed" and leave the error to the render). (2) How cycles and the load cost of deep
include trees are bounded.

**Done when.** `get_template('main').render()` with a static include of an unparsable template renders as Jinja2
does, with the decision on loader call timing recorded in docs/python.md, and tests for include, import, extends,
a missing include and a cycle.
