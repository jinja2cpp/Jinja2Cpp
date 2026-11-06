---
status: open
priority: low
area: robustness
depends: [0359]
touches: [src/token_budget.cpp#TruncateText]
shares: [test/filters_test.cpp]
---
# Bound the host calls of truncate_tokens' shrink loop

**Problem.** When the prefix plus `end` still counts more than `n` tokens (tokens merging across
the join), `truncate_tokens` lowers the prefix budget and asks the tokenizer again. Each pass
lowers it by at least one, so a host tokenizer that is not monotonic can make the loop call it
about `n` times, and `n` comes from the template (0359 verifier, non-blocking).

**Proposal.** Cap the retries (for example at a small constant, then fall back to an empty
prefix plus `end`, or an error if `end` alone exceeds `n`), or shrink geometrically. Keep the
`count(result) <= n` guarantee.

**Acceptance.** A test with a non-monotonic fake tokenizer and a large `n` makes a bounded number
of calls, narrow and wide.
