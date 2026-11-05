---
status: open
priority: low
area: api
depends: [0358]
touches: [src/prompt_escape.cpp, src/prompt_escape.h]
---
# NFKC-folded special token matching in `neutralize`

Filed by the 0358/0359 plan (Ruslan 2026-11-05).

**Problem.** `neutralize` matches special tokens on the raw text (exactly and ASCII
case-insensitively). A Hugging Face added token with `normalized=True` is matched after the
tokenizer's normalizer, and NFKC maps fullwidth forms to ASCII (`＜｜` U+FF1C U+FF5C become
`<|`), so fullwidth input that `neutralize` leaves alone can become a special after
normalization.

**Proposal.** An option of `PromptEscapeOptions` (off by default, additive through the pimpl)
that also matches the patterns against the NFKC fold of the text, using the generated Unicode
tables, and breaks the matching span in the original text.

**Done when.** Rows in the `PromptFilterTest` table (narrow and wide) show fullwidth input that
folds into a configured special is broken with the option and unchanged without it.
