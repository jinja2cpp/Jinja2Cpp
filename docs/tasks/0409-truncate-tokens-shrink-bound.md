---
status: done
priority: low
area: robustness
depends: [0359]
touches: [src/token_budget.cpp#TruncateText]
shares: [test/filters_test.cpp]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/863
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

**Resolution.** The shrink loop lowers the budget by the excess for four passes, then also halves it, so a host
tokenizer whose counts do not follow the text gets at most about 4 + log2(n) passes (two host calls each); the
`count(result) <= n` guarantee holds since a zero budget leaves only `end`. Test:
`TokenBudget.NonMonotonicTokenizerMakesBoundedCalls`, narrow and wide.
