---
status: done
priority: low
area: parity
depends: [0001]
touches: [test/parity/parity_test.cpp]
---
# Run the corpus through the wide-string API

**Problem.** Jinja2C++ supports `std::wstring` templates (`TemplateW`), and
`MULTISTR_TEST` checks both forms in unit tests, but the parity corpus renders narrow
templates only, so wide-specific divergences stay invisible.

**Proposal.** Render every case a second time through `TemplateW` (UTF-8 → wide for the
template, context and loader files, wide → UTF-8 for the result) and compare with the
same expectation and allow-list. Divergences that occur only in the wide path get a
separate kind or a `wide.` id prefix.

**Done when.** `ctest -R parity` runs both paths, and wide-only divergences are listed
like narrow ones.

**Outcome.** `ParityWide/*` renders every case through `TemplateW` and holds it to the
narrow result; a case is listed as `wide.<id>` only when the two differ and the wide one
does not match Python. On the first run there were no wide-only divergences, once the
harness converted UTF-8 itself: `jinja2::ConvertString` depends on the C locale and
dropped every non-ASCII string (filed as 0035). The run also showed that
`operators.floordiv_by_zero` is undefined behaviour (SIGFPE when run alone), so it is
now listed as `crash` under 0015.
