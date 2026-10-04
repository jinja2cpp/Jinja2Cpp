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

**Outcome.** Done in PR #376. `TemplateEnvImpl::LoadTemplate` looks the cache up before
creating a template, and each render keeps what it loaded by name (in the per-render
`RendererCallback`), so `include`, `extends` and `import` go to the environment once per
name per render; `include` renders the resolved template in place, without allocating a
renderer or copying its `shared_ptr`. `Render/inheritance`: 747k → 581k instructions
(-22%), 622 → 371 allocations; `MT/Render/inheritance` 13.4k/s on one thread, 22.3k/s on
two and 41.9k/s on four (was 9.6k, 8.6k and 6.1k on the same container).

Lookup policy (Ruslan, 2026-10-04: let the user choose): `Settings::templateLookup`.
`TemplateLookup::OncePerRender` (default) is the above. `TemplateLookup::EveryUse` looks the
template up each time the statement runs, as Jinja2 does, so with `autoReload` a file changed
during a render is seen by its next `include` and with `cacheSize = 0` every `include` reads the
file. It still skips the throwaway `Template` and the renderer allocation. On master 098d156,
`MT/Render/inheritance` on 1/2/4 threads measured 15-16k/21k/15k per second on master,
21k/41k/70-81k with `OncePerRender` and 21k/32k/56-61k with `EveryUse`. Instructions were 670k,
503k (-25%) and 524k (-22%).
The copy of the caller's scopes that remains is
0108.
