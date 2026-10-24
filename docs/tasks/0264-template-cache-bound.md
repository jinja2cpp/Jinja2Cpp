---
status: open
priority: medium
area: robustness
touches: [src/template_env.cpp, src/template_env_impl.h, src/filesystem_handler.cpp, test/template_env_test.cpp, test/filesystem_handler_test.cpp]
---
# The template cache ignores its size, and `MemoryFileSystem` writes in a const path

**Problem.**
- `TemplateEnv` treats `cacheSize` only as on/off (src/template_env.cpp, `!= 0`); the cache is an
  `unordered_map` keyed by the raw name (src/template_env_impl.h). Names that differ only in `./`
  pieces are distinct keys for one file, so a long-running host rendering user templates grows
  without bound, quadratically in the name length.
- `MemoryFileSystem::OpenStream`/`OpenWStream` are const but fill the other character width into
  `mutable m_filesMap` without a lock (inferred from code: src/filesystem_handler.cpp), so two
  threads loading a file stored in the other width race.

**Proposal.** An LRU that honours `cacheSize` (Python's `Environment.cache_size`, default 400),
keyed by the normalised name. `MemoryFileSystem` converts per open and never caches in the const
path (a mutex member would change the class layout).

**Tests.** An include loop over `'./' * i ~ 'a.j2'` with `cacheSize` 4 keeps at most 4 entries
(counting handler); a TSan row with two threads loading a wide-stored file through the narrow API.

**Done when.** Both rows pass, the TSan job included.

**Trust model.** When this lands, update its entry under Known gaps in [docs/security.md](../security.md) (0224).
