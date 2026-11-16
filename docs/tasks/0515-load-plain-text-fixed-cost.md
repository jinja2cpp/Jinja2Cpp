---
status: in-progress
pr:
priority: medium
area: perf
depends: []
touches: [src/template_impl.h, src/template_parser.h, test/template_api_test.cpp, test/metadata_test.cpp]
---
# The fixed cost of a Load: `Load/plain_text` against inja

**Problem.** In the engines baseline of 2026-11-15 (`perf-track/2026-11-15-engines.md`), inja loads
the 25-character `plain_text` case in 186 ns and Jinja2C++ in 291 ns (0.64x): the one Load row where
another engine is clearly ahead. The case has no tags, so the row is the fixed cost of a Load:
constructing the template, taking the environment's settings, copying the source, parsing one text
block, sealing the tree and destroying it all again. On master 3f66175 (GCC 13.3 Release, pinned
name-hash key) it is 3,760 instructions and 4 allocations per Load:

| Part | Ir | Notes |
|---|---:|---|
| `BasicTemplate` constructor | ~600 | `GetLoadSettings` 414: mutex ~60, `operator==(Settings)` ~294 |
| `TemplateImpl::Load` itself | ~330 | source copy into a `unique_ptr<string>` box, name, metadata |
| `TemplateParser::Parse` | ~600 | one text block |
| `NodeArena::SealWith` | ~560 | `MoveNodes` 225, about 40 per node relocated and checked |
| `~BasicTemplate` | ~570 | the sealed tree, the source, the box |
| malloc/free | ~500 | impl, source characters, source box, sealed buffer |

**Change (P1).**
- The first Load of a template parses its source in a string inside `TemplateImpl` rather than in a
  string on the heap that is moved into a `unique_ptr` once the parse succeeds. Only a template
  nothing else holds and that has no tree yet is loaded in place (`LoadsInPlace`), so the string has
  no tree pointing into it when the Load starts; a tree points into its source, and a short source
  lives inside the string object, so the string never moves after that. A failed Load empties it. A
  Load from host code the first Load calls on the same template (the autoescape selector) parses
  into a heap string of its own, as every Load did before, and the outer Load's result wins (the
  verifier's use-after-free on the first draft, which picked one of two slots by state the nested
  Load changed). One allocation (the box) fewer per Load; the template object is 8 bytes larger.
- The parser no longer copies `Settings::defaultMetadataType` into itself and then into the
  template's `MetadataInfo`: the template's settings have it, and `GetMetadataRaw` and the metadata
  parse read it from there. The metadata location is moved only when the new or the old template has
  a metadata block (a template without one leaves the location's file name empty).
- `NormalizeTemplateNewlines` looks for `\r` with `char_traits::find` (an inlined `memchr`) instead
  of `basic_string::find`, a call into the library first.

Result against 3f66175 (local count, pinned key): `Load/plain_text` 3,760 to 3,338 (-11.2%), 4 to 3
allocations, 8 bytes less retained; `Load/substitute` -5.6%, `Load/for_range` -3.2%, other Loads
-0.7..-2%, Render rows within +0.14%; the full table is in the PR.

Tried and dropped: copying a `string_view` source straight into its slot (`assign`) instead of
building a string and moving it in: +38 Ir, as `assign` allocates exactly and the move did not.
Comparing the settings' short strings by size with one or two word loads: -4 Ir only, as the
compare loop of a two-character delimiter is already cheap and the switch costs as much.

**What is left** is task 0516: about 0.72x of inja after this change.

**Acceptance.**
- `Load/plain_text` at least 10% fewer instructions and one allocation fewer, no Load or Render row
  up by more than the bar (0.5%) in the CI count job.
- Reloads of each source size (inside and outside the small-string buffer) and failed reloads keep
  rendering the loaded template (`TemplateApiTest.ReloadsKeepEachSourceSize`), and so does a Load
  from the selector of the first Load (`TemplateApiTest.LoadFromTheSelectorOfTheFirstLoad`); metadata follows
  reloads and failed reloads (`MetadataTest.Metadata_FollowsReloads`).
- Address/undefined sanitizer run of the template, metadata and reload tests.
