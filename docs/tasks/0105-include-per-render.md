---
status: done
priority: medium
area: perf
depends: [0011]
touches: [src/template_env.cpp#LoadTemplate, src/statements.cpp#IncludeStatement, src/statements.cpp#ParentTemplateRenderer]
---
# `include` and `extends` go through the environment's locked cache on every render

**Problem.** Found with the allocation counts and the threaded benchmarks
(`--threads`) of the suite (0011). `{% include %}` and `{% extends %}` call
`TemplateEnv::LoadTemplate` each time they render, so `inheritance` (an `include` per
list item) does it once per item. Each call:

- creates a new `Template` before looking at the cache
  (`auto tpl = Functions::CreateTemplate(env)` comes first in
  `TemplateEnv::TemplateCache::LoadTemplate`, `src/template_env.cpp`), so a cache hit
  still allocates (`TemplateEnvAccess::MakeHandle`, a `shared_ptr`, a `std::function`);
- takes the environment's `shared_timed_mutex` in shared mode and hashes the file
  name. A shared lock is still an atomic write to one cache line, which every thread
  rendering an `include` bounces.

`Render/inheritance` makes 622 allocations per render. It is the one case that does not
scale with threads: on an idle 4-core container it renders 11.2-11.6k/s on one thread,
9.9-14.7k/s on two and 8.2-12.1k/s on four, while `for_loop_vars` goes 8.3k → 29.7k/s,
`macros` 3.4k → 10.1k/s and `mitsuhiko_table` 655 → 2,262/s on four threads.

**Ideas.** Look the cache up before creating the template. Cache the resolved template
per include node when the name is a constant and `autoReload` is off (with an
environment generation counter to invalidate it when templates are added or the cache
is cleared), so the lock is taken once per template per environment, not per render.

**Done when.** `MT/Render/inheritance` scales like the other cases and its allocations
per render drop, measured with `--threads` and `bench/count.py --baseline`.

**Outcome.** Done in PR #PRNUM. `TemplateEnvImpl::LoadTemplate` looks the cache up before
creating a template, and each render keeps what it loaded by name (in the per-render
`RendererCallback`), so `include`, `extends` and `import` go to the environment once per
name per render; `include` renders the resolved template in place, without allocating a
renderer or copying its `shared_ptr`. `Render/inheritance`: 747k → 581k instructions
(-22%), 622 → 371 allocations; `MT/Render/inheritance` 13.4k/s on one thread, 22.3k/s on
two and 41.9k/s on four (was 9.6k, 8.6k and 6.1k on the same container).

Deliberate divergence: with `autoReload`, Python checks a template for changes each time
an `include` runs; Jinja2C++ now checks once per render, so a file edited during a render
is picked up by the next one. With `cacheSize = 0` an included file is parsed once per
render instead of once per `include`. The copy of the caller's scopes that remains is
0108.
