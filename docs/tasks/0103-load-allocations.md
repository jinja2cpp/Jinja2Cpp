---
status: done
priority: medium
area: perf
depends: [0011]
touches: [src/template_parser.h#InvokeParser, src/lexer.cpp, src/lexer.h, src/expression_parser.cpp, src/template_parser.cpp, src/value_methods.h]
---
# Loading a tag-heavy template allocates about 115 times per tag

**Problem.** Found with the allocation counts and gperftools heap profiles of the
benchmark suite (0011). `Load/many_tags` (300 lines, each with an `if`/`else`, four
expressions and a `set`) makes 34,249 allocations and 4.8 MB of requests per load,
against 184 for `Load/mitsuhiko_table`. In the heap profile
(`--heap-profile`, `google-pprof --alloc_objects`):

- Half the allocations are `std::vector` growth (`_M_realloc_insert`). Every tag gets a
  fresh `lexertk::generator` and `Lexer` in `TemplateParser::InvokeParser`, and their
  token vectors grow from empty: `Lexer::Preprocess` (`m_tokens.push_back`) is 35% of
  the reallocations, the generator's token list in `InvokeParser` another 44%.
- `make_shared` of expression nodes is 38%: every level of the precedence chain
  (`ParseLogicalOr` → `ParseLogicalAnd` → ... → `ParseMathPow`) wraps its result, even
  when it passes a single operand through.
- `GetKeyword` is 10% of the load's instructions at about 1500 instructions per call
  (noted in 0102), so it seems to run far more often than once per identifier.

**Ideas.** Keep one generator and one token vector in the parser and `clear()` them
per tag, or `reserve` from the tag length. Return operands unwrapped where a level has
no operator. Find out why `GetKeyword` runs so often.

**Done when.** `Load/many_tags` allocations and instructions are measured before and
after with `bench/count.py --baseline`, and the load is at least 2x cheaper.

**Done** in PR_LINK. `Load/many_tags` went from 44.15M to 21.99M instructions (-50.2%)
and from 34,249 to 16,559 allocations (-52%); every other `Load/` case got 31-50%
cheaper and no `Render/` case changed. What the profile turned out to say:

- `GetKeyword` ran at every level of the precedence chain for every token it looked at,
  about 22,000 times per load. The lexer now classifies each symbol once and stores the
  keyword in the `Token`; the parsers read `tok.keyword`. The lookup itself goes by first
  character instead of a binary search with a `memcmp` per step.
- The generator and token vector are kept across tags (`LexBuffers`), and tokens are
  built in place in the vector.
- The precedence levels did not wrap single operands, contrary to the hypothesis above;
  their cost was copies: a `Token` (with its `InternalValue`) copied at every level, the
  parse result moved out of each level instead of returned in place, and the postfix and
  filter parsers called for every operand. Each level now keeps one result variable, and
  `ParseUnaryPlusMinus` calls those parsers only when the next token can start them.
- `StatementsParser` copied the whole `Settings` (ten strings) for every statement tag.
- Smaller: `MarkMacroSpecialNames` returns at once outside macros, attribute subscripts
  no longer allocate a constant index node, `IsMethodName` binary-searches, the block end
  scan skips plain characters, `endif` moves the statement info instead of copying it.

What is left is in [0107](0107-load-costs-round-2.md).
