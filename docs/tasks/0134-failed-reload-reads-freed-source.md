---
status: open
priority: high
area: robustness
touches: [src/template_impl.h#Load, test/template_api_test.cpp]
---
# Reloading a template, when the new Load fails, leaves it reading freed memory

**Problem.** Found by the 0118 architect review (plan in the perf track folder, section
1.6) and confirmed with valgrind on master 448bb7c. `TemplateImpl::Load` move-assigns
the new source into `m_template` before parsing it, so the old buffer is freed when
`Load` returns. If the parse fails, `m_renderer` is still the old tree, whose
`RawTextRenderer`s point into that freed buffer. Load `200 x 'A' + "{{ x }}"`, then Load
`"{{ broken"`, then render: valgrind reports `Invalid read ... TemplateRenderer::RenderBody`
(block freed in `BasicTemplate::Load`), and the render "succeeds" with garbage. Any
embedder that hot-reloads into the same `Template` object is exposed, as are copies that
share the implementation, including templates from the environment cache.

**Proposal.** Parse into fresh state (source, renderer, macros/blocks tables) and commit it
to the template only when parsing succeeds, so a failed `Load` keeps the previous
template working (the architect's recommendation; the alternative is to leave the
template empty, which is also memory-safe). Add a test that loads, fails a reload and
renders, run under the sanitizer configuration, and have the fuzz harness load each input
a second time into the same template.

**Done when.** The sequence above renders the first template's output (or reports an
unloaded template, if Ruslan picks that) and is clean under ASan and valgrind.
