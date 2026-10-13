---
status: open
priority: low
area: api
depends: []
touches: [include/jinja2cpp/template_env.h#Settings, src/template_impl.h]
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
