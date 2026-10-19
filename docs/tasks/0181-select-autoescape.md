---
status: done
priority: low
area: api
depends: []
touches: [include/jinja2cpp/template_env.h#Settings, src/template_impl.h, src/template_parser.h, src/statements.h#TemplateRenderer, src/template_env.cpp, test/autoescape_test.cpp, test/parity/parity_test.cpp, test/parity/generate.py]
shares: [src/statements.cpp]
---
# Autoescape chosen by template name (`select_autoescape`)

**Problem.** `Settings::autoescape` is a `bool`. Jinja2 also takes a callable of the template
name, usually `select_autoescape(['html', 'xml'])`, so one environment escapes `.html`
templates and leaves `.txt` ones alone (found in 0025, left over from 0051).

**Proposal.** A `std::function<bool(const std::string&)>` in `Settings` breaks its
`operator==`, so either compare it by identity or keep it outside `Settings`. The value
then has to flow per template from `TemplateImpl` into the renderer, as the bool does for
include and import today (`TemplateAutoescape` in `src/statements.cpp`).

**Done when.** A loader-based unit test renders an `.html` and a `.txt` template from one
environment with `select_autoescape`-like settings and gets escaped and unescaped output.

**Done** (PR #586). `Settings::autoescapeSelector` (an `AutoescapeSelector`: a callable of the
template name, compared by identity) and `SelectAutoescape(...)`. The name is the one the
template was loaded with, empty for an unnamed string template (Jinja2's None). Each template
records its setting when it loads; its top level and blocks escape by it, included and
imported templates run with it, and a parent keeps its child's runtime setting, as Jinja2's
EvalContext belongs to the context. A selector that throws fails the Load with
UnexpectedException. Corpus: `"autoescape": [extensions]` maps to `select_autoescape`.
