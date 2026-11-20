---
status: done
pr: https://github.com/jinja2cpp/jinja2cpp/pull/951
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

**Result.** The untrusted and host targets use all eight bits of their settings byte, so the choice went to the
spare bits of the two targets that have them (fuzz_common.h `gettext_bits`, `ApplyGettextBits`): the loader
target's second byte above `confined` (oldstyle, then a `GettextStyle` install of null translations) and the
allocation-failure target's ignored wide bit (oldstyle with a `FollowLoad` install). Zero is today's newstyle
default, which every earlier seed has there. make_corpus.py `--prefix loader|alloc_failure` writes hash-picked
bits and an oldstyle copy of every seed that uses gettext; fuzz/regressions/loader-oldstyle-trans.j2 and
alloc-failure-oldstyle-trans.j2 reach `ParseEndTrans` with `i18nNewstyle` off (checked under gdb), and the
replay of fuzz/regressions passes.
