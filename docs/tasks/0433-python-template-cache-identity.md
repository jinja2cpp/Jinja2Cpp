---
status: in-progress
priority: medium
area: parity
depends: [0355]
touches: [python/jinja2cpp/environment.py, python/src/environment.cpp]
shares: [python/tests/upstream/expected-failures.txt]
---
# `get_template` returns a new Template object each time, and template globals of includes

**Problem.** In Jinja2 `env.get_template(name) is env.get_template(name)` while the cache holds
the template, and a template whose source changed (auto_reload, the loader's `uptodate`) is a new
object. jinja2cpp caches the compiled template but returns a new Python wrapper each time
(`test_loader.py::TestLoaders::test_caching`, `test_limited_size_cache`,
`TestFileSystemLoader::test_caches_template_based_on_mtime`). Related: globals passed to
`get_template("main", globals=...)` overlay the env globals for templates `main` includes or
extends, and a template keeps the globals it was first loaded with
(`test_regression.py::TestBug::test_cached_extends[extends]`, `[include]`). Both are lines of
`python/tests/upstream/expected-failures.txt`.

**Proposal.** Keep the Python Template object in the environment's cache entry (bounded as the
cache is: `cache_size`, LRU from 0264), so the same object comes back until it is evicted or
reloaded; pass the template globals of the loading call to the templates it loads, as Jinja2's
`Environment._load_template` does. Check with 0405 (globals replaced under the cache) and 0427.

**Done when.** The listed upstream tests pass through the module.
