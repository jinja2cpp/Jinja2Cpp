---
status: open
priority: low
area: perf
touches: [src/markup.h#MarkupEscape]
shares: [src/internal_value.h]
---
# MarkupEscape calls InternalValue's copy assignment out of line

**Problem.** `MarkupEscape`'s escaping lambda default-constructs its result and then assigns
`val` to it when there is nothing to escape. GCC emits that assignment as an out-of-line
`InternalValue::operator=(const InternalValue&) [clone .isra.0]`. On `Render/html_autoescape`
this costs about 3,000 Ir a render (0.25%) on master after #735.

#735 (0330 item 0) tried building the result in place in each branch
(`InternalValue result(val)`, then `SetMarkup`). That recovered the 0.25% on
`html_autoescape`, but it shifted GCC's inlining in other translation units that include
markup.h. `Render/strings` went +0.39%, `Module/keep` +0.38% and `Render/filters` +0.25%
against the change without it, so #735 left it out.

**Proposal.**
- Build the result in place without changing what the including translation units inline,
  for example by moving `MarkupEscape` out of line into one .cpp.
- Or call `CopyFrom` only on the no-escape branch.
- Measure every row with `count.py`. The whole-TU inlining shifts are the risk here, not
  the escaping itself.

**Done when.**
- `Render/html_autoescape` recovers at least 0.2% against the master base.
- Callgrind shows no out-of-line copy assignment from a `MarkupEscape` frame.
- No row gets slower by more than 0.1%.
