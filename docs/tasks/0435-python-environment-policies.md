---
status: open
priority: low
area: parity
depends: [0355]
touches: [python/jinja2cpp/environment.py, python/src/environment.cpp, src/filters.cpp, src/serialize_filters.cpp, src/string_converter_filter.cpp, src/render_context.h, src/template_impl.h, src/template_render_options.cpp, src/template_parser.cpp, src/template_env.cpp, include/jinja2cpp/template.h, include/jinja2cpp/template_env.h, docs/api-2.0.md, test/parity/generate.py, test/parity/parity_test.cpp, test/parity/cases/policies.py, test/template_env_test.cpp]
shares: [python/tests/upstream/expected-failures.txt]
---
# `Environment.policies`

**Problem.** Jinja2's `env.policies` (`compiler.ascii_str`, `urlize.rel`, `urlize.target`,
`urlize.extra_schemes`, `truncate.leeway`, `json.dumps_function`, `json.dumps_kwargs`,
`ext.i18n.trimmed`) change filters and the i18n extension. jinja2cpp refuses a changed policy
with UnsupportedError (`fallback="never"`) or falls back to Jinja2. Upstream tests:
`test_filters.py::TestFilter::test_json_dump`, `test_urlize_rel_policy`,
`test_ext.py::TestInternationalization::test_trimmed_policy`, `test_trimmed_policy_override`
(lines of `python/tests/upstream/expected-failures.txt`, 0355).

**Proposal.** Settings for the policies the engine can honour natively (urlize rel/target/extra
schemes, truncate leeway, i18n trimmed, tojson's dumps kwargs), read from `env.policies` at load
as Jinja2 does; `json.dumps_function` set to a Python callable makes `tojson` call it.

**Done when.** The listed upstream tests pass through the module.
