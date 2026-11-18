---
status: open
priority: low
area: robustness
depends: [0415, 0509]
touches: [fuzz/fuzz_common.h]
---
# Fuzz the oldstyle gettext parser path

**Problem.** Found by the 0415 verifier, left by 0509. The fuzz targets (fuzz/fuzz_common.h `ConfigureEnv`)
turn `Settings::Extensions::i18n` on but never turn `i18nNewstyle` off, so the oldstyle `{% trans %}` node
(`MarkSafeIfAutoescape(gettext(...)) % {vars}`, `%%` undoubling) and the oldstyle gettext globals have no fuzz
coverage, nor does an install that fixes the style (`TemplateEnv::InstallGettextCallables(GettextStyle, ...)`).

**Proposal.** Let a byte of the input that the targets already split off for settings choose
`i18nNewstyle` and a `GettextStyle` install (null translations), keeping the existing seeds' meaning (a byte value
that maps to today's newstyle default), and add a seed with an oldstyle `{% trans %}` block.

**Done when.** A fuzz run reaches the oldstyle trans parser (coverage report or a seed under fuzz/), and the
replay job passes.
