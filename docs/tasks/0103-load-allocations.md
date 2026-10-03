---
status: open
priority: medium
area: perf
depends: [0011]
touches: [src/template_parser.h#InvokeParser, src/lexer.cpp, src/lexer.h, src/expression_parser.cpp]
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
