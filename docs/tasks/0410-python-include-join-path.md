---
status: open
priority: low
area: parity
depends: [0354]
touches: [python/src/loaders.cpp, python/src/environment.cpp#LoadTemplate, python/jinja2cpp/environment.py]
shares: [python/tests/test_environment_api.py, docs/python.md]
---
# `include`, `import` and `extends` do not call `env.join_path`

**Problem.** In Jinja2 a template's `{% include 'x' %}`, `{% import %}`, `{% from %}` and
`{% extends %}` compile to `environment.get_template('x', parent=<name of the template>)` (and
`select_template` for a list), which calls the environment's `join_path(template, parent)`. A
subclass that overrides `join_path` to make names relative to the including template (the
documented use) therefore changes what the tags load. The module's engine reads the loader
directly (`SourceHandler::OpenStream`, `LoadTemplate` in the engine): `get_template` and
`select_template` in Python call `join_path`, the tags do not (docs/python.md "Differences";
`test_include_does_not_go_through_join_path` pins the behaviour).

**Proposal.** The engine resolves a tag's name in `TemplateEnvImpl` before the filesystem
handlers see it. Give the Python module a hook there, without a public-header change: a
private callback on `TemplateEnv` (in `src/template_env_impl.h`, set by the module the way
`detail::t_hostPoll` is) that maps (name, parent template name) to the name to load, called only when
the environment's class overrides `join_path` (a `type(env).join_path is not Environment.join_path`
check at configuration time, so the default costs nothing). Return the joined name to
the engine's loader path, so that the cache key is the joined name as in Jinja2.

**Done when** the pytest case compares an overriding subclass's `include`, `import`, `from` and
`extends` with Jinja2's, and the docs/python.md difference is removed.
