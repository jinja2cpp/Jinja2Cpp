---
status: open
priority: low
area: api
depends: [0359]
touches: [src/token_budget.cpp, include/jinja2cpp/prompt.h]
---
# Batch count callback for `pack`

Filed by the 0358/0359 plan (Ruslan 2026-11-05).

**Problem.** Hugging Face tokenizers count a batch of texts much faster than one text at a time;
`pack` calls the count callback once per item.

**Proposal.** An additive `Tokenizer` constructor overload taking a batch count function; `pack`
measures the items it may take in one call when it is set, keeping its "at most once per item"
rule.

**Done when.** A test with a fake batch tokenizer shows one batch call for a pack over many items
and the same result as the per-item path.
