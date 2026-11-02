---
status: open
priority: low
area: perf
depends: [0360]
touches: [src/template_parser.cpp, src/load_settings.h, src/filter_factories.cpp]
shares: [src/template_env.cpp, src/template_impl.h, src/recursion_guard.h]
---
# Keep the dormant chat-template preset free on every Load

**Problem.** The chat-template preset (0360, #737) is meant to cost nothing unless
`ConfigureForChatTemplates` turns it on. The perf track still measured a fixed cost of about
350 to 430 instructions on every Load in a plain environment (train #741 at 0dfaa72, job
125378979067). The rows are `Load/strings` +0.60%, `Load/int_arith` +0.53% and `Module/keep`
+0.48%. Allocations did not change, and render rows are flat or down. The perf track accepted
these rows as named exceptions on condition that the cost gets a follow-up.

**Suspects** (not yet profiled):
- the `detail::Dialect` that `LoadSettings` carries and `ParseLimitsScope` copies into
  `t_parseLimits` on every Load;
- the dialect read in `ParseNonKeywordStatement` and in `FindBuiltinFilter(name, dialect)`
  for every filter name;
- any change in how `StatementsParser` or the lexer handle keywords, now that
  `generation`/`endgeneration` exist (they are parsed as identifiers, only in the
  Transformers dialect).

**Proposal.** Run `bench/count.py --filter 'Load/(strings|int_arith)'` with callgrind
annotations on master and on 2fc3fac (the base before #737) to find the functions whose
counts grew. Then move that work behind one flag check that a Jinja2-dialect Load reads once,
or out of the Load path altogether.

**Done when.** The three rows are back within ±0.1% of their counts before #737, with
`chat_template_preset_test` and test/chat_templates still passing, narrow and wide.
