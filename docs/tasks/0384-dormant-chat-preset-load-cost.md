---
status: done
priority: low
area: perf
depends: [0360]
pr: https://github.com/jinja2cpp/Jinja2Cpp/pull/744
touches: [src/template_env.cpp, src/template_env_impl.h, src/expression_evaluator.h, bench/jinja2cpp_bench.cpp]
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

**Outcome.** Profiling showed that little of this was parse code. Under callgrind with jemalloc
preloaded, `Load/strings` differed by +2 instructions between a35a0e3^1 and c9398f9. The rest was
glibc's malloc taking other paths. Allocation counts did not change, but the heap the counted
loop started from did, for two reasons:
- `TemplateEnvImpl` got an 8-byte `m_dialect` field. That moved its `make_shared` block from 984
  to 992 bytes, which is the next chunk size class (992 to 1008 with the header). The dialect
  now lives only in `m_loadSettings`, as the big-int backend does: `DialectLocked` reads it from
  there, and `SetDialect` makes the load settings when it changes the dialect.
- `jinja2cpp_bench --count` loaded every case directory before it counted one case, so the four
  `hf_*` cases changed the heap that every other row started from (CI's base, which had no
  "transformers" key, counted its own cases). In `--count` mode the driver now loads only the
  case it counts.

One render-path cost was real: `CallExpression::TryCallMethod` asked for the dialect through
the renderer callback's virtual `GetDialect` on each call of a mutating method (+50 Ir per
`Module/keep_written` iteration). The node now records it at Load, in the padding after
`m_isNamedCallee` (the node stays 48 bytes).

Like for like (both binaries with the new `--count` driver, Release, GCC 13), compared with
a35a0e3^1: `Load/strings` +0.08%, `Load/int_arith` +0.02%, `Module/keep` 0.00%,
`Module/keep_written` +0.16% before the call-node change (+4 Ir after it), and all other Load rows
within ±0.11%. The few instructions left with jemalloc are code generation: GCC no longer inlines
`NodeArena::MakeSpan<char>` into `AssignTargetParser::ParseTarget` (+18 net), the `tojson`
dialect test in `FindBuiltinFilter` (+2 per filter name), and about one instruction per tag in
`ParseRoughMatch`.
